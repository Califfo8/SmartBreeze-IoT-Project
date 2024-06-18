#include "contiki.h"
#include "coap-engine.h"
#include "coap-blocking-api.h"
#include "sys/log.h"
#include "etimer.h"
#include <stdio.h>
//#include "../Utility/Timestamp/Timestamp.h"
#include "../Utility/RandomNumberGenerator/RandomNumberGenerator.h"


//----------------------------------PARAMETERS----------------------------------//
//[+] LOG CONFIGURATION
#define LOG_MODULE "App"
#define LOG_LEVEL LOG_LEVEL_INFO


//[+] REGISRATION PARAMETERS
#define NODE_INFO_JSON "{\"node\":\"climate-manager\",\"resources\":[\"temperature\"]}"
#define MAX_REGISTER_ATTEMPTS 3
#define SERVER_EP "coap://[fd00::1]:5683"
#define CREATED_CODE 65

int max_register_attempts = MAX_REGISTER_ATTEMPTS;
static const char *service_registration_url = "/registration"; // URL di registrazione

//[+] TIME PARAMETERS
#define SAMPLING_PERIOD 10 // in seconds
int sampling_period = 10; //3600 * h_sampling_period; in seconds

//[+] TIMERS
#define SLEEP_INTERVAL 30 // in seconds
static struct etimer sleep_timer;

//----------------------------------RESOURCES----------------------------------//
extern coap_resource_t res_temperature_HVAC;

//----------------------------FUNCTIONS----------------------------------------//

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

//----------------------------MAIN PROCESS----------------------------------------//
PROCESS(climate_manager_process, "climate-manager process");
AUTOSTART_PROCESSES(&climate_manager_process);
//--------------------------------------------------------------------------------//
PROCESS_THREAD(climate_manager_process, ev, data)
{
  
  PROCESS_BEGIN();
  //------------------[1]-CoAP-Server-Registration-------------------------------//
   static coap_endpoint_t server_ep;
    static coap_message_t request[1]; // This way the packet can be treated as pointer as usual

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
            etimer_set(&sleep_timer, CLOCK_SECOND * SLEEP_INTERVAL);
            PROCESS_WAIT_EVENT_UNTIL(etimer_expired(&sleep_timer));
            max_register_attempts = MAX_REGISTER_ATTEMPTS;
        }
      
    }
  //------------------------[2]-Climate-Management----------------------------------//
  LOG_INFO("Climate manager started\n");
  // Activate the temperature resource
  coap_activate_resource(&res_temperature_HVAC,"temperature");
  
  do
  {
    // Sense the temperature and manage the HVAC
    res_temperature_HVAC.trigger();
    // Wait for the next sensing interval
    etimer_set(&sleep_timer, CLOCK_SECOND * sampling_period);
    PROCESS_WAIT_EVENT_UNTIL(etimer_expired(&sleep_timer));
  } while (1);

  PROCESS_END();
}
/*---------------------------------------------------------------------------*/
