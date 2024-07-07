#include <stdio.h>

#include "contiki.h"
#include "sys/log.h"
#include "etimer.h"
#include "os/dev/leds.h"
#include "os/dev/button-hal.h"

#include "coap-engine.h"
#include "coap-blocking-api.h"
#include "coap-observe-client.h"

#include "../Utility/RandomNumberGenerator/RandomNumberGenerator.h"
#include "../Utility/JSONSenML/JSONSenML.h"
#include "../Utility/Timestamp/Timestamp.h"
#include "../Utility/Leds/Leds.h"
//----------------------------------PARAMETERS----------------------------------//
//[+] LOG CONFIGURATION
#define LOG_MODULE "App"
#define LOG_LEVEL LOG_LEVEL_INFO


//[+] REGISRATION PARAMETERS
#define NODE_INFO_JSON "{\"node\":\"climate-manager\",\"resource\":\"temperature_HVAC\"}"
#define MAX_REGISTER_ATTEMPTS 3
#define SERVER_EP "coap://[fd00::1]:5683"
#define CREATED_CODE 65

int max_attempts = MAX_REGISTER_ATTEMPTS;
static const char *service_registration_url = "/registration"; // URL di registrazione
//[+] DISCOVERY PARAMETERS
#define RES_SOLAR_ENERGY "solar_energy"
#define MAX_DISCOVERY_ATTEMPTS 3
static char ip_energy_manager[40];
static const char *service_discovery_url = "/discovery";
//[+] CLOCK PARAMETERS
#define MAX_CLOCK_REQ_ATTEMPTS 3
static const char *service_clock_url = "/clock";
bool clock_sync = false;

//[+] OBSERVING PARAMETERS
static coap_endpoint_t energy_manager_ep;
static coap_observee_t* solar_energy_res;
//[+] TIME PARAMETERS
#define SAMPLING_PERIOD 1 // in hours
int h_sampling_period = SAMPLING_PERIOD;
int sampling_period = 3600 * SAMPLING_PERIOD;

Timestamp timestamp = {
  .year = 2024,
  .month = 7,
  .day = 15,
  .hour = 5,
  .minute = 0
};// DOPO DEVE ESSERE RICHIESTO ALL'APPLICAZIONE CLOUD

//[+] OBSERVED RESOURCE
float sampled_energy = -1;
float predicted_energy = -1;
//[+] TIMERS
#define SLEEP_INTERVAL 30 // in seconds
static struct etimer sleep_timer;

//[+] BUTTON PARAMETERS
bool button_pressed = false;
//----------------------------------RESOURCES----------------------------------//
extern coap_resource_t res_temperature_HVAC; 
// require sampled_energy, predicted_energy, timestamp and button_pressed definitions

//----------------------------FUNCTIONS----------------------------------------//

// Define a handler to handle the response from the server
void registration_chunk_handler(coap_message_t *response)
{
    if(response == NULL) {
        LOG_ERR("[Climate-manager] Request timed out\n");
    }  else if (response->code !=65)
    {
        LOG_ERR("[Climate-manager] Error: %d\n",response->code);
    }else{
        LOG_INFO("[Climate-manager] Successful node registration\n");
        max_attempts = 0; // Stop the registration attempts
        return;
    }

    // retry registration
    max_attempts--;
    // if max attempts are reached, signal to wait
    if(max_attempts == 0) {
        max_attempts = -1;
    }
}
// Define a handler to handle the discovery response from the server
void discovery_chunk_handler(coap_message_t *response)
{
  const uint8_t *buffer = NULL;
    if(response == NULL) {
        LOG_ERR("[Climate-manager]Request timed out \n");
    } else if (response->code !=69)
    {
        LOG_ERR("[Climate-manager] Error IN DISCOVERY: %d\n",response->code);
    }else{
        coap_get_payload(response, &buffer);
        strncpy(ip_energy_manager, (char *)buffer, response->payload_len);
        LOG_INFO("[Climate-manager] Successful node discovery: %s \n",ip_energy_manager);
        max_attempts = 0; // Stop the discovery attempts
        return;
    }
    // retry registration
    max_attempts--;
    // if max attempts are reached, signal to wait
    if(max_attempts == 0) {
        max_attempts = -1;
    }
}

void clock_chunk_handler(coap_message_t *response)
{
    if(response == NULL) {
        LOG_ERR("Request timed out\n");
    }  else if (response->code !=69)
    {
        LOG_ERR("[Climate-manager] Error in registration: %d\n",response->code);
    }else{
        LOG_INFO("[Climate-manager] Received clock from server: %s\n", response->payload);
        // Extract the timestamp from the response
        char str_timestamp[TIMESTAMP_STRING_LEN];
        strncpy(str_timestamp, (char*)response->payload, response->payload_len);
        string_to_timestamp(str_timestamp, &timestamp);
        
        max_attempts = 0; // Stop the registration attempts
        return;
    }

    // retry registration
    max_attempts--;
    // if max attempts are reached, signal to wait
    if(max_attempts == 0) {
        max_attempts = -1;
    }
}

static void solar_energy_callback(coap_observee_t *obs, void *notification, coap_notification_flag_t flag)
{
    LOG_INFO("[Climate-manager] Notification received:");
    
    json_senml payload;

    MeasurementData data[2];
    payload.measurement_data = data;
    payload.num_measurements = 2;

    char base_name[MAX_STR_LEN];
    char base_unit[] = "W";
    char name[2][MAX_STR_LEN];
    char unit[2][MAX_STR_LEN];
    char time[2][TIMESTAMP_STRING_LEN];
    
    payload.base_name = base_name;
    payload.base_unit = base_unit;
    
    payload.measurement_data[0].name = name[0];
    payload.measurement_data[0].unit = unit[0];
    payload.measurement_data[0].time = time[0];

    payload.measurement_data[1].name = name[1];
    payload.measurement_data[1].unit = unit[1];
    payload.measurement_data[1].time = time[1];

    const uint8_t *buffer = NULL;
    if(notification){
        coap_get_payload(notification, &buffer);
    }

    switch (flag) {
    case NOTIFICATION_OK:
        
        parse_str((char*)buffer, &payload);
        //print_json_senml(&payload);
        // Update the predicted energy
        predicted_energy = payload.measurement_data[0].v.v;
        // Update the sampled energy
        sampled_energy = payload.measurement_data[1].v.v;
        LOG_INFO("[Climate-manager][%s]Sampled energy received: %.2f\n", payload.measurement_data[1].time, sampled_energy);
        if (clock_sync == false) {
            
            Timestamp ts ={
                .year = -1,
                .month = -1,
                .day = -1,
                .hour = -1,
                .minute = -1
            };
            string_to_timestamp(payload.measurement_data[1].time, &ts);
            copy_timestamp(&ts, &timestamp);
            clock_sync = true;
            LOG_INFO("[Climate-manager]Clock syncronized\n");
            }
        break;
    case OBSERVE_OK:
        LOG_INFO("[Climate-manager] Observe OK\n");
        
        break;
    case OBSERVE_NOT_SUPPORTED:
        LOG_ERR("[Climate-manager] Observe not supported\n");
        break;
    case ERROR_RESPONSE_CODE:
        LOG_ERR("[Climate-manager] Error response code\n");
        break;
    case NO_REPLY_FROM_SERVER:
        LOG_ERR("[Climate-manager] No reply from server\n");
        break;
    }
    
}

//----------------------------MAIN PROCESS----------------------------------------//
PROCESS(climate_manager_process, "climate-manager process");
AUTOSTART_PROCESSES(&climate_manager_process);
//--------------------------------------------------------------------------------//
PROCESS_THREAD(climate_manager_process, ev, data)
{

  PROCESS_BEGIN();
  // Activate the yellow LED to signal the start of the process
    leds_single_on(LEDS_YELLOW);
  // Activate the temperature resource
  coap_activate_resource(&res_temperature_HVAC,"temperature_HVAC");
  //------------------[1]-CoAP-Server-Registration-------------------------------//
    static coap_endpoint_t server_ep;
    static coap_message_t request[1]; // This way the packet can be treated as pointer as usual

    // Populate the coap endpoint structure
    coap_endpoint_parse(SERVER_EP, strlen(SERVER_EP), &server_ep);
    while (max_attempts != 0) {
        // Prepare the request
        coap_init_message(request, COAP_TYPE_CON, COAP_POST, 0);
        coap_set_header_uri_path(request, service_registration_url);
        // Set the payload
        coap_set_payload(request, (uint8_t *)NODE_INFO_JSON, strlen(NODE_INFO_JSON));
        // Send the request and wait for the response
        COAP_BLOCKING_REQUEST(&server_ep, request, registration_chunk_handler);
        // Something goes wrong with the registration, the node sleeps and tries again
        if (max_attempts == -1) {
            etimer_set(&sleep_timer, CLOCK_SECOND * SLEEP_INTERVAL);
            PROCESS_WAIT_EVENT_UNTIL(etimer_expired(&sleep_timer));
            max_attempts = MAX_REGISTER_ATTEMPTS;
        }
      
    }
  //------------------------[2]-Clock-Request----------------------------------//
    LOG_INFO("[Climate-manager] Time request process started\n");
    max_attempts = MAX_CLOCK_REQ_ATTEMPTS;
    while (max_attempts != 0) {
        // Prepare the request
        coap_init_message(request, COAP_TYPE_CON, COAP_GET, 0);
        coap_set_header_uri_path(request, service_clock_url);
        // Set the payload
        coap_set_payload(request, (uint8_t *)"", strlen(""));
        // Send the request and wait for the response
        COAP_BLOCKING_REQUEST(&server_ep, request, clock_chunk_handler);
        // Something goes wrong with the registration, the node sleeps and tries again
        if (max_attempts == -1) {
            etimer_set(&sleep_timer, CLOCK_SECOND * SLEEP_INTERVAL);
            PROCESS_WAIT_EVENT_UNTIL(etimer_expired(&sleep_timer));
            max_attempts = MAX_CLOCK_REQ_ATTEMPTS;
        }
    }
  //------------------------[3]-Discovery-Solar-Node---------------------------------//
    LOG_INFO("[Climate-manager] Discovery process started\n");
    max_attempts = MAX_DISCOVERY_ATTEMPTS;
    while (max_attempts != 0) {
        // Prepare the request
        coap_init_message(request, COAP_TYPE_CON, COAP_GET, 0);
        coap_set_header_uri_path(request, service_discovery_url);
        // Set the payload
        coap_set_payload(request, (uint8_t *)RES_SOLAR_ENERGY, strlen(RES_SOLAR_ENERGY));
        // Send the request and wait for the response
        COAP_BLOCKING_REQUEST(&server_ep, request, discovery_chunk_handler);
        // Something goes wrong with the registration, the node sleeps and tries again
        if (max_attempts == -1) {
            etimer_set(&sleep_timer, CLOCK_SECOND * SLEEP_INTERVAL);
            PROCESS_WAIT_EVENT_UNTIL(etimer_expired(&sleep_timer));
            max_attempts = MAX_DISCOVERY_ATTEMPTS;
        }
    }

    //Register to the solar energy resource
    char solar_energy_ep[100];
    snprintf(solar_energy_ep, 100, "coap://[%s]:5683", ip_energy_manager);
    coap_endpoint_parse(solar_energy_ep, strlen(solar_energy_ep), &energy_manager_ep);

    solar_energy_res = coap_obs_request_registration(&energy_manager_ep, RES_SOLAR_ENERGY, solar_energy_callback, NULL);

  //------------------------[3]-Climate-Management----------------------------------//
  LOG_INFO("[Climate-manager] Climate manager started\n");
  leds_single_off(LEDS_YELLOW);
  ctrl_leds(LEDS_RED);
  etimer_set(&sleep_timer, CLOCK_SECOND * sampling_period);
  do
  {
    //Process going to sleep
    PROCESS_YIELD();
    if(etimer_expired(&sleep_timer))
    {
      // Update the timestamp
      advance_time(&timestamp, h_sampling_period);
      // Sense the temperature and manage the HVAC
      res_temperature_HVAC.trigger();
      // Wait for the next sensing interval
      etimer_reset(&sleep_timer);
    }else if(ev == button_hal_press_event)
    {
        button_pressed = true;
        ctrl_leds(LEDS_BLUE);
        // Update the timestamp
        clock_time_t remaining_time = etimer_expiration_time(&sleep_timer) - clock_time();
        int minutes_passed =  ( sampling_period - (remaining_time / CLOCK_SECOND)) / 60;
        
        Timestamp old_timestamp={
            .year = timestamp.year,
            .month = timestamp.month,
            .day = timestamp.day,
            .hour = timestamp.hour,
            .minute = timestamp.minute
        };
        advance_time_m(&timestamp, minutes_passed);
        res_temperature_HVAC.trigger();
        LOG_INFO("[Climate-manager] Minutes passed: %d minutes\n", minutes_passed);
        //restore the timestamp for the correct advance at the next sensing interval
        copy_timestamp(&timestamp, &old_timestamp);
        button_pressed = false;
    } 
  } while (1);

  PROCESS_END();
}
/*---------------------------------------------------------------------------*/
