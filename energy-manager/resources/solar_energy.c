#include "contiki.h"
#include "coap-engine.h"
#include "sys/log.h"
#include <stdio.h>
// emlearn generated model
#include "eml_net.h"
#include "../AI/solar_prediction.h"
//Utility functions
#include "../Utility/Timestamp/Timestamp.h"
#include "../Utility/RandomNumberGenerator/RandomNumberGenerator.h"
#include "../Utility/JSONSenML/JSONSenML.h"
//----------------------------------PARAMETERS----------------------------------//

//[+] LOG CONFIGURATION
#define LOG_MODULE "App"
#define LOG_LEVEL LOG_LEVEL_INFO

//[+] ENERGY PARAMETERS
#define MAX_ENERGY 7500
#define MIN_ENERGY 0
#define UNIT "W"
// Simulation parameters
#define MAX_NIGHT_ENERGY 200
#define DAY_STEP 0.2 // Energy increase simulation step during the day
#define NIGHT_STEP 0.1

//[+] PREDICTION PARAMETERS
//Actual date and time in [Year,Month, Day, Hour] format
extern Timestamp timestamp;
//Prediction horizon in hours
static int future_hours = 1;
//Correction factor for the prediction
static float correction_factor = 0;
//Scaler parameters for standardizing the input features
static const float SCALER_MEAN[] = {6.51393662, 16.15158457, 13.93967163}; 
static const float SCALER_SCALE[] = {2.65563593, 8.99027562, 3.8395894};

//----------------------------------RESOURCES----------------------------------//
// Energy produced by the solar panel in [future_hours] in the future
static float predicted_energy = -1;
// Energy produced by the solar panel in the last sampling period
static float sampled_energy = -1;

//----------------------------FUNCTIONS DEFINITIONS----------------------------------//

static float fake_solar_sensing(float old_value);
static void scaler(float* features, float* scaled_features);
static float predict_solar_energy();
static void res_get_handler(coap_message_t *request, coap_message_t *response,
                          uint8_t *buffer, uint16_t preferred_size, int32_t *offset);
static void res_event_handler(void);

// Resource definition
EVENT_RESOURCE(
        res_solar_energy, 
        "title=\"SolarEnergy\";rt=\"json+senml\";if=\"sensor\";obs", 
        res_get_handler, 
        NULL,
        NULL,
        NULL,
        res_event_handler);

//----------------------------FUNCTIONS DECLARATIONS----------------------------------//
static float fake_solar_sensing(float old_value)
{
  if (old_value == -1)
    old_value = 500;

  // If is night, the solar energy is near to 0
  if (timestamp.hour >= 20 || timestamp.hour <= 6)
    return generate_random_float(old_value, 200, 0, NIGHT_STEP, 1);

  // After sunrise, the solar energy increases
  if (timestamp.hour>= 6 && timestamp.hour<= 13)
    return generate_random_float(old_value, MAX_ENERGY, MIN_ENERGY, DAY_STEP, 1);
  
  // After noon, the solar energy decrease
  if (timestamp.hour >= 13 && timestamp.hour<= 20)
    return generate_random_float(old_value, MAX_ENERGY, MIN_ENERGY, DAY_STEP, 0);
  
  LOG_ERR("[Energy-manager] Error in the fake_solar_sensing function\n");
  return 0;
}

// Is needed for standardize the features in input to the model
static void scaler(float* features, float* scaled_features)
{
  for (int i = 0; i < 3; i++)
  {
    scaled_features[i] = (features[i] - SCALER_MEAN[i]) / SCALER_SCALE[i];
  }
}

static float predict(float* features)
{
  float scaled_features[]= {0, 0, 0};
  scaler(features, scaled_features);
  return solar_prediction_predict(scaled_features, 3);
}

static float predict_solar_energy()
{
    Timestamp ts_copy={
      .year = timestamp.year,
      .month = timestamp.month,
      .day = timestamp.day,
      .hour = timestamp.hour,
      .minute = timestamp.minute
    };
    float features[] = {0, 0, 0};
    float prediction = 0;
    // Copy the timestamp
    printf("%p\n", eml_net_activation_function_strs); // This is needed to avoid compiler error (warnings == errors)
    // Initialize the features
    convert_to_feature(&ts_copy, features);
    // Compute the prediction of the last sampled solar energy
    prediction = predict(features);
    // Calibrate the correction factor;
    correction_factor = (sampled_energy - prediction)*0.12 + correction_factor*0.88;
    // Compute the prediction of the future solar energy
    advance_time(&ts_copy, future_hours);
    convert_to_feature(&ts_copy, features);
    prediction = predict(features);
    // Apply the correction factor
    prediction  += correction_factor;
    // Check if the prediction is negative
    if (prediction < 0)
      prediction = 0;
    //log information
    LOG_INFO("[Energy-manager] \t-Correction Factor: %f\n", correction_factor);
    LOG_INFO("[Energy-manager] \t-Predicted Solar energy: %f\n", prediction);   
    return prediction;
}


// Define the resource handler function
static void res_get_handler(coap_message_t *request, coap_message_t *response,
                          uint8_t *buffer, uint16_t preferred_size, int32_t *offset){

    MeasurementData data[2];
    json_senml js_senml;
    char timestamp_str[TIMESTAMP_STRING_LEN];
    char base_name[BASE_NAME_LEN];
    int payload_len = 0;

    //Creo il timestamp
    timestamp_to_string(&timestamp, timestamp_str);
    // Create the base name
    get_base_name(base_name);

    // Inizializzo i valori delle risorse
    data[0].name = "predicted";
    data[0].unit = UNIT;
    strcpy(data[0].time, timestamp_str);
    data[0].v.vd = predicted_energy;

    data[1].name = "sampled";
    data[1].unit = UNIT;
    strcpy(data[1].time, timestamp_str);
    data[1].v.vd = sampled_energy;

    // Creo il JSON SenML
    js_senml.base_name = base_name;
    js_senml.base_unit = UNIT;
    js_senml.measurement_data = data;
    js_senml.num_measurements = 2;

    // Convert the JSON SenML to a payload

    payload_len = json_to_payload(&js_senml, (char*)buffer);

    if (payload_len < 0)
    {
      LOG_ERR("[Energy-manager] Error in the json_to_payload function\n");
      coap_set_status_code(response, INTERNAL_SERVER_ERROR_5_00);
      coap_set_payload(response, buffer, 0);
      return;
    }else if (payload_len > preferred_size)
    {
      LOG_ERR("[Energy-manager] Buffer overflow\n");
      coap_set_status_code(response, INTERNAL_SERVER_ERROR_5_00);
      coap_set_payload(response, buffer, 0);
      return;
    }

    // Prepare the response
    // Copy the response in the transmission buffer
    //memcpy(buffer, message, payload_len);
    coap_set_header_content_format(response, APPLICATION_JSON);
    coap_set_header_etag(response, (uint8_t *)&payload_len, 1);
    coap_set_payload(response, buffer, payload_len);

    // Print sended data for debug
    LOG_INFO("[Energy-manager] Sending data: %s\n", buffer);
}

static void res_event_handler(void) {
    char timestamp_str[TIMESTAMP_STRING_LEN];
    timestamp_to_string(&timestamp, timestamp_str);
    LOG_INFO("[Energy-manager] New sample at time %s\n", timestamp_str);
    // Sample the solar energy
    sampled_energy = fake_solar_sensing(sampled_energy);
    LOG_INFO("[Energy-manager] \t-Sampled Solar energy: %f", sampled_energy);
    // Predict the solar energy
    predicted_energy = predict_solar_energy();
    // Notify the observers
    coap_notify_observers(&res_solar_energy);
}
