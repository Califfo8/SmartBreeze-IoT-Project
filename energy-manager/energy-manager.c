#include "contiki.h"
#include "coap-engine.h"
#include "coap-blocking-api.h"
#include "sys/log.h"
#include "etimer.h"
#include <stdio.h>
#include "../Utility/Timestamp/Timestamp.h"
#include "../Utility/Leds/Leds.h"
//----------------------------------PARAMETERS----------------------------------//
//[+] LOG CONFIGURATION
#define LOG_MODULE "App"
#define LOG_LEVEL LOG_LEVEL_INFO

//[+] REGISRATION PARAMETERS
#define NODE_INFO_JSON "{\"node\":\"energy-manager\",\"resource\":\"solar_energy\"}"
#define MAX_REGISTER_ATTEMPTS 3
#define SERVER_EP "coap://[fd00::1]:5683"
#define CREATED_CODE 65

int max_attempts = MAX_REGISTER_ATTEMPTS;
static const char *service_registration_url = "/registration"; // URL di registrazione

//[+] CLOCK PARAMETERS
#define MAX_CLOCK_REQ_ATTEMPTS 3
static const char *service_clock_url = "/clock";

//[+] TIME PARAMETERS
#define SAMPLING_PERIOD 1 // in hours
int h_sampling_period = SAMPLING_PERIOD; // in hours
int sampling_period = 3600 * SAMPLING_PERIOD; //3600 * h_sampling_period; in seconds

Timestamp timestamp = {
  .year = 2024,
  .month = 7,
  .day = 15,
  .hour = 5,
  .minute = 0
};// DOPO DEVE ESSERE RICHIESTO ALL'APPLICAZIONE CLOUD

//[+] TIMERS
#define SLEEP_INTERVAL 30 // in seconds
static struct etimer sleep_timer;

//----------------------------------RESOURCES----------------------------------//
extern coap_resource_t res_solar_energy;

//----------------------------FUNCTIONS----------------------------------------//

// Define a handler to handle the response from the server
void registration_chunk_handler(coap_message_t *response)
{
    if(response == NULL) {
        LOG_ERR("Request timed out\n");
    }  else if (response->code !=65)
    {
        LOG_ERR("[Energy-manager] Error in registration: %d\n",response->code);
    }else{
        LOG_INFO("[Energy-manager] Successful node registration\n");        
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

void clock_chunk_handler(coap_message_t *response)
{
    if(response == NULL) {
        LOG_ERR("Request timed out\n");
    }  else if (response->code !=69)
    {
        LOG_ERR("[Energy-manager] Error in registration: %d\n",response->code);
    }else{
        LOG_INFO("[Energy-manager] Received clock from server: %s\n", response->payload);
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


//----------------------------MAIN PROCESS----------------------------------------//
PROCESS(energy_manager_process, "energy-manager process");
AUTOSTART_PROCESSES(&energy_manager_process);
//--------------------------------------------------------------------------------//
PROCESS_THREAD(energy_manager_process, ev, data)
{
  PROCESS_BEGIN();
  // Activate the yellow LED to signal the start of the process
    leds_single_on(LEDS_YELLOW);
  // Activate the solar energy resource
  coap_activate_resource(&res_solar_energy,"solar_energy");
  //------------------[1]-CoAP-Server-Registration-------------------------------//
   static coap_endpoint_t server_ep;
    static coap_message_t request[1]; // This way the packet can be treated as pointer as usual

    while (max_attempts != 0) {
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
        if (max_attempts == -1) {
            etimer_set(&sleep_timer, CLOCK_SECOND * SLEEP_INTERVAL);
            PROCESS_WAIT_EVENT_UNTIL(etimer_expired(&sleep_timer));
            max_attempts = MAX_REGISTER_ATTEMPTS;
        }
      
    }
  //------------------------[2]-Clock-Request----------------------------------//
    LOG_INFO("[Energy-manager] Time request process started\n");
    max_attempts = MAX_CLOCK_REQ_ATTEMPTS;
    while (max_attempts != 0) {
        // Populate the coap endpoint structure
        coap_endpoint_parse(SERVER_EP, strlen(SERVER_EP), &server_ep);
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
  //------------------------[3]-Energy-Sensing----------------------------------//

  LOG_INFO("[Energy-manager] Started\n");
  leds_single_off(LEDS_YELLOW);
  ctrl_leds(LEDS_RED);
  // Sensing interval
  etimer_set(&sleep_timer, CLOCK_SECOND * sampling_period);
  do
  {
    //Process going to sleep
    PROCESS_YIELD();
    if(etimer_expired(&sleep_timer))
    {
      ctrl_leds(LEDS_GREEN);
      // Update the timestamp
      advance_time(&timestamp, h_sampling_period);
      // Sense the solar energy
      res_solar_energy.trigger();
      // Wait for the next sensing interval
      ctrl_leds(LEDS_RED);
      etimer_reset(&sleep_timer);
    }
    
  } while (1);

  PROCESS_END();
}
/*---------------------------------------------------------------------------*/
