#include <stdio.h>

#include "contiki.h"
#include "sys/log.h"
#include "etimer.h"

#include "coap-engine.h"
#include "coap-blocking-api.h"
#include "coap-observe-client.h"

//#include "../Utility/Timestamp/Timestamp.h"
#include "../Utility/RandomNumberGenerator/RandomNumberGenerator.h"
#include "../Utility/JSONSenML/JSONSenML.h"
#include "../Utility/Timestamp/Timestamp.h"

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

//[+] OBSERVING PARAMETERS
static coap_endpoint_t energy_manager_ep;
static coap_observee_t* solar_energy_res;
//[+] TIME PARAMETERS
#define SAMPLING_PERIOD 1 // in hours
int h_sampling_period = SAMPLING_PERIOD;
int sampling_period = 10; //3600 * h_sampling_period; in seconds

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

//----------------------------------RESOURCES----------------------------------//
extern coap_resource_t res_temperature_HVAC;

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
        // Extract the timestamp from the response
        char str_timestamp[TIMESTAMP_STRING_LEN];
        const uint8_t *buffer = NULL;
        coap_get_payload(response, &buffer);
        LOG_INFO("[Climate-manager] Initiliaze timestamp: %s\n", (char *)buffer);
        strncpy(str_timestamp, (char *)buffer, response->payload_len);
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
// Define a handler to handle the discovery response from the server
void discovery_chunk_handler(coap_message_t *response)
{
  const uint8_t *buffer = NULL;
    if(response == NULL) {
        LOG_ERR("\t Request timed out \n");
    } else if (response->code !=69)
    {
        LOG_ERR("\t Error: %d\n",response->code);
    }else{
        coap_get_payload(response, &buffer);
        strncpy(ip_energy_manager, (char *)buffer, response->payload_len);
        LOG_INFO("\t Successful node discovery: %s \n",ip_energy_manager);
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

static void solar_energy_callback(coap_observee_t *obs, void *notification, coap_notification_flag_t flag)
{
    LOG_INFO("[Climate-manager] Notification received:");
    
    static json_senml payload;

    static MeasurementData data[2];
    payload.measurement_data = data;
    payload.num_measurements = 2;

    static char base_name[MAX_STR_LEN];
    static char base_unit[] = "W";
    static char name[2][MAX_STR_LEN];
    static char unit[2][MAX_STR_LEN];
    static char time[2][TIMESTAMP_STRING_LEN];
    
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
        LOG_INFO("\t Start parsing the payload\n");
        parse_str((char*)buffer, &payload);
        //print_json_senml(&payload);
        // Update the predicted energy
        predicted_energy = payload.measurement_data[0].v.v;
        // Update the sampled energy
        sampled_energy = payload.measurement_data[1].v.v;
        LOG_INFO("\t Sampled energy: %.2f\n", sampled_energy);

        break;
    case OBSERVE_OK:
        LOG_INFO("\t Observe OK\n");
        
        break;
    case OBSERVE_NOT_SUPPORTED:
        LOG_INFO("\t Observe not supported\n");
        break;
    case ERROR_RESPONSE_CODE:
        LOG_INFO("\t Error response code\n");
        break;
    case NO_REPLY_FROM_SERVER:
        LOG_INFO("\t No reply from server\n");
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
  // Activate the temperature resource
  coap_activate_resource(&res_temperature_HVAC,"temperature_HVAC");
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
  //------------------------[2]-Discovery-Solar-Node---------------------------------//
    LOG_INFO("[Climate-manager] Discovery process started\n");
    max_attempts = MAX_DISCOVERY_ATTEMPTS;
    while (max_attempts != 0) {
        // Populate the coap endpoint structure
        coap_endpoint_parse(SERVER_EP, strlen(SERVER_EP), &server_ep);
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
  
  etimer_set(&sleep_timer, CLOCK_SECOND * sampling_period);
  do
  {
    //Process going to sleep
    PROCESS_YIELD();
    if(etimer_expired(&sleep_timer))
    {
      // Sense the temperature and manage the HVAC
      res_temperature_HVAC.trigger();
      // Update the timestamp
      advance_time(&timestamp, h_sampling_period);
      // Wait for the next sensing interval
      etimer_reset(&sleep_timer);
    }   
  } while (1);

  PROCESS_END();
}
/*---------------------------------------------------------------------------*/
