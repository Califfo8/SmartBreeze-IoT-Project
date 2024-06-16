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
//----------------------------------PARAMETERS----------------------------------//

//[+] LOG CONFIGURATION
#define LOG_MODULE "App"
#define LOG_LEVEL LOG_LEVEL_INFO

//[+] ENERGY PARAMETERS
#define MAX_ENERGY 7500
#define MIN_ENERGY 0
// Simulation parameters
#define MAX_NIGHT_ENERGY 200
#define DAY_STEP 0.2 // Energy increase simulation step during the day
#define NIGHT_STEP 0.1

//[+] PREDICTION PARAMETERS
//Actual date and time in [Month, Day, Hour] format
extern int timestamp[];
//Prediction horizon in hours
static float future_hours = 1;
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
  if (timestamp[2] >= 20 || timestamp[2] <= 6)
    return generate_random_float(old_value, 200, 0, NIGHT_STEP, 1);

  // After sunrise, the solar energy increases
  if (timestamp[2] >= 6 && timestamp[2] <= 13)
    return generate_random_float(old_value, MAX_ENERGY, MIN_ENERGY, DAY_STEP, 1);
  
  // After noon, the solar energy decrease
  if (timestamp[2] >= 13 && timestamp[2] <= 20)
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
    int ts_copy[] = {timestamp[0], timestamp[1], timestamp[2]};
    float features[] = {0, 0, 0};
    float prediction = 0;
    printf("%p\n", eml_net_activation_function_strs); // This is needed to avoid compiler error (warnings == errors)
    // Initialize the features
    convert_to_float(ts_copy, features);
    // Compute the prediction of the last sampled solar energy
    prediction = predict(features);
    // Calibrate the correction factor;
    correction_factor = (sampled_energy - prediction)*0.12 + correction_factor*0.88;
    // Compute the prediction of the future solar energy
    advance_time(ts_copy, future_hours);
    convert_to_float(ts_copy, features);
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
                          uint8_t *buffer, uint16_t preferred_size, int32_t *offset) {
char message[12];
int length = 12;

snprintf(message, length, "%f", sampled_energy);

// Prepare the response
// Copy the response in the transmission buffer
memcpy(buffer, message, length);
coap_set_header_content_format(response, TEXT_PLAIN);
coap_set_header_etag(response, (uint8_t *)&length, 1);
coap_set_payload(response, buffer, length);
}

static void res_event_handler(void) {
    LOG_INFO("[Energy-manager] New sample at time [M:%d, D:%d, H:%d]\n", timestamp[0], timestamp[1], timestamp[2]);
    // Sample the solar energy
    sampled_energy = fake_solar_sensing(sampled_energy);
    LOG_INFO("[Energy-manager] \t-Sampled Solar energy: %f", sampled_energy);
    // Predict the solar energy
    predicted_energy = predict_solar_energy();
    // Notify the observers
    coap_notify_observers(&res_solar_energy);
}
