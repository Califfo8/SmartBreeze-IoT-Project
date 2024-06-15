#include "contiki.h"
#include "coap-engine.h"
#include "coap-blocking-api.h"
#include "sys/log.h"
#include "etimer.h"
#include "../fake_sensor/fake_sensor.h"
#include <stdio.h>

// emlearn generated model
#include "eml_net.h"
#include "../AI/solar_prediction.h"
// Log configuration
#define LOG_MODULE "App"
#define LOG_LEVEL LOG_LEVEL_INFO

// Resource registration path
#define NODE_INFO_JSON "{\"node\":\"energy-manager\",\"resources\":[\"solar_energy\"]}"
#define MAX_REGISTER_ATTEMPTS 3
#define SERVER_EP "coap://[fd00::1]:5683"
#define CREATED_CODE 65
int max_register_attempts = MAX_REGISTER_ATTEMPTS;
static const char *service_registration_url = "/registration"; // URL di registrazione

// solar panel energy resource
extern coap_resource_t res_solar_energy;
extern float solar_energy;
float sampled_energy = 0;
#define MAX_ENERGY 7500
#define MIN_ENERGY 0
#define SAMPLING_PERIOD 10 //3600 // Ogni ora

// Prediction parameters
float datetime[] = {7, 15, 0}; // month, day, hour
float future_hours = 1; // prediction horizon
float max_error = 500; // maximum prediction error
float correction_factor = 0;

void passing_time(float* dt, float hours)
{
  dt[2] = dt[2] + hours;
  if (dt[2] == 24)
  {
    dt[2] = 0;
    dt[1]++;
    if (dt[0] == 2 && dt[1] == 28) // February
    {
      dt[1] = 1;
      dt[0]++;
  
    }
    else if ((dt[0] == 4 || dt[0] == 6 || dt[0] == 9 || dt[0] == 11) && dt[1] == 31) // April, June, September, November
    {
      dt[1] = 1;
      dt[0]++;
    }
    else if (dt[1] == 32) // January, March, May, July, August, October, December
    {
      dt[1] = 1;
      dt[0]++;
      if (dt[0] == 13)
      {
        dt[0] = 1;
      }
    }
  }
}

float predict_solar_energy()
{
    float features[] = {datetime[0], datetime[1], datetime[2]};
    float prediction = 0;
    printf("%p\n", eml_net_activation_function_strs); // This is needed to avoid compiler error (warnings == errors)
    // Compute the prediction of the sampled solar energy
    prediction = solar_prediction_predict(features, 3);
    // Compute the correction factor;
    correction_factor = (sampled_energy - prediction)*(float)0.95;
    // Compute the prediction of the future solar energy
    passing_time(features, future_hours);
    prediction = solar_prediction_predict(features, 3);
    LOG_INFO("Predicted Solar energy: %f\n", prediction);
    prediction  += correction_factor;
    if (prediction < 0)
      prediction = 0;
    
    return prediction;
}

// Define a handler to handle the response from the server
void registration_chunk_handler(coap_message_t *response)
{
  printf("client_chunk_handler\n");
    if(response == NULL) {
        LOG_ERR("Request timed out or there was some problem\n");
    } else {
        LOG_INFO("successful node registration\n");
        max_register_attempts = 0; // Stop the registration attempts
        return;
    }

    // retry registration
    max_register_attempts--;
    // if max attempts are reached, signal to wait
    if(max_register_attempts == 0) {
        max_register_attempts = -1;
    }
}
float fake_solar_sensing(float old_value)
{
  // If is night, the solar energy is near to 0
  if (datetime[2] >= 20 || datetime[2] <= 6)
    return read_sensor_value(old_value, 200, 0,0.2, 1);

  // After sunrise, the solar energy increases
  if (datetime[2] >= 6 && datetime[2] <= 13)
    return read_sensor_value(old_value, MAX_ENERGY, MIN_ENERGY,0.5, 1);
  
  // After noon, the solar energy decrease
  if (datetime[2] >= 13 && datetime[2] <= 20)
    return read_sensor_value(old_value, MAX_ENERGY, MIN_ENERGY,0.5, 0);
  
  LOG_ERR("Error in the fake_solar_sensing function\n");
  return 0;
}

/*---------------------------------------------------------------------------*/
PROCESS(energy_manager_process, "energy-manager process");
AUTOSTART_PROCESSES(&energy_manager_process);
/*---------------------------------------------------------------------------*/
PROCESS_THREAD(energy_manager_process, ev, data)
{
  
  PROCESS_BEGIN();
  // [1] Registration to the coap server
   static coap_endpoint_t server_ep;
    static coap_message_t request[1]; // This way the packet can be treated as pointer as usual
    static struct etimer sleep_timer;

    while (max_register_attempts != 0) {
        // Populate the coap endpoint structure
        coap_endpoint_parse(SERVER_EP, strlen(SERVER_EP), &server_ep);
        // Prepare the request
        coap_init_message(request, COAP_TYPE_CON, COAP_POST, 0);
        coap_set_header_uri_path(request, service_registration_url);
        // Set the payload
        coap_set_payload(request, (uint8_t *)NODE_INFO_JSON, strlen(NODE_INFO_JSON));
        // Send the request and wait for the response
        COAP_BLOCKING_REQUEST(&server_ep, request, registration_chunk_handler);
        // Something goes wrong with the registration, the node sleeps and tries again
        if (max_register_attempts == -1) {
            etimer_set(&sleep_timer, CLOCK_SECOND * 30);
            PROCESS_WAIT_EVENT_UNTIL(etimer_expired(&sleep_timer));
            max_register_attempts = MAX_REGISTER_ATTEMPTS;
        }
      
    }
  // [2] Energy management
  LOG_INFO("Energy manager started\n");
  // Activate the solar energy resource
  coap_activate_resource(&res_solar_energy,"solar_energy");
  // Sensing interval
  do
  {
    sampled_energy = fake_solar_sensing(sampled_energy);
    LOG_INFO("Sampled Solar energy: %f", sampled_energy);
    // Predict the solar energy
    solar_energy = predict_solar_energy();
    passing_time(datetime, 1);
    etimer_set(&sleep_timer, CLOCK_SECOND * SAMPLING_PERIOD);
    PROCESS_WAIT_EVENT_UNTIL(etimer_expired(&sleep_timer));
  } while (1);

  PROCESS_END();
}
/*---------------------------------------------------------------------------*/
