#include "contiki.h"
#include "coap-engine.h"
#include "sys/log.h"
#include <stdio.h>
//Utility functions
#include "../Utility/Timestamp/Timestamp.h"
#include "../Utility/RandomNumberGenerator/RandomNumberGenerator.h"
#include "../Utility/JSONSenML/JSONSenML.h"
//----------------------------------PARAMETERS----------------------------------//

//[+] LOG CONFIGURATION
#define LOG_MODULE "App"
#define LOG_LEVEL LOG_LEVEL_INFO

//[+] TIME PARAMETERS
#define H_SAMPLING_PERIOD 0.0025 // in hours
int sampling_period = 3600 * H_SAMPLING_PERIOD; //in seconds
//[+] USER PARAMETERS
#define MAX_TEMP_USER 25
#define MIN_TEMP_USER 10

//----------------------------------RESOURCES----------------------------------//
static float h_sampling_period = 1; // in hours
float max_temp_user = MAX_TEMP_USER;
float min_temp_user = MIN_TEMP_USER;
//----------------------------FUNCTIONS DEFINITIONS----------------------------------//

static void res_get_handler(coap_message_t *request, coap_message_t *response,
                          uint8_t *buffer, uint16_t preferred_size, int32_t *offset);
static void res_post_handler(coap_message_t *request, coap_message_t *response,
                          uint8_t *buffer, uint16_t preferred_size, int32_t *offset);

// Resource definition
RESOURCE(
        res_settings, 
        "title=\"res_settings\";rt=\"json\";if=\"sensor\"", 
        res_get_handler, 
        res_post_handler,
        NULL,
        NULL);

//----------------------------FUNCTIONS DECLARATIONS----------------------------------//
// Define the resource handler function
static void res_get_handler(coap_message_t *request, coap_message_t *response,
                          uint8_t *buffer, uint16_t preferred_size, int32_t *offset){
    
    LOG_INFO("[Climate-manager]//--------------NEW-GET-SETTING-REQUEST---------------------\n");
   
    // Prepare the json
    char json_str[MAX_PAYLOAD_LEN];
    int payload_len = -1;
    int values[] = {0, 0, 0};
    values[0] = (int)(h_sampling_period * DECIMAL_ACCURANCY);
    values[1] = (int)(max_temp_user * DECIMAL_ACCURANCY);
    values[2] = (int)(min_temp_user * DECIMAL_ACCURANCY);
    snprintf(json_str, MAX_PAYLOAD_LEN, "{\"Temp sampling period(h)\":%d,\"Max T\":%d,\"Min T\":%d}", values[0], values[1], values[2]);
    payload_len = strlen(json_str);
    // Convert the JSON SenML to a payload
    if (payload_len < 0)
    {
      LOG_ERR("\t Error in the json creation\n");
      coap_set_status_code(response, INTERNAL_SERVER_ERROR_5_00);
      coap_set_payload(response, buffer, 0);
      return;
    }else if (payload_len > preferred_size)
    {
      LOG_ERR("\t Buffer overflow\n");
      coap_set_status_code(response, INTERNAL_SERVER_ERROR_5_00);
      coap_set_payload(response, buffer, 0);
      return;
    }

    // Prepare the response
    coap_set_header_content_format(response, APPLICATION_JSON);
    coap_set_header_etag(response, (uint8_t *)&payload_len, 1);
    coap_set_payload(response, buffer, payload_len);

    // Print sended data for debug
    LOG_INFO("[Climate-manager] Sending settings: %s with size: %d\n", buffer, payload_len);
}

static void res_post_handler(coap_message_t *request, coap_message_t *response,
                          uint8_t *buffer, uint16_t preferred_size, int32_t *offset)
{
    LOG_INFO("[Climate-manager]//--------------NEW-POST-SETTING-REQUEST---------------------\n");
    // get the new settings
    const uint8_t *payload = NULL;
    int payload_len = coap_get_payload(request, &payload);

    if (payload_len < 0)
    {
      LOG_ERR("\t Error in the payload\n");
      coap_set_status_code(response, BAD_REQUEST_4_00);
      coap_set_payload(response, buffer, 0);
      return;
    }   
    LOG_INFO("[Climate-manager] New settings: %s\n", payload);
    int values[] = {0, 0, 0};
    sscanf((char*)payload, "{\"Temp sampling period(h)\":%d,\"Max T\":%d,\"Min T\":%d}", &values[0], &values[1], &values[2]);
    h_sampling_period = (float)(values[0])/DECIMAL_ACCURANCY;
    max_temp_user = (float)(values[1])/DECIMAL_ACCURANCY;
    min_temp_user = (float)(values[2])/DECIMAL_ACCURANCY;
    
    // Update the sampling period
    sampling_period = 3600 * h_sampling_period;
    LOG_INFO("[Climate-manager] New settings: Temp sampling period(h):%f | Max T:%f | Min T:%f\n", h_sampling_period, max_temp_user, min_temp_user);
    coap_set_status_code(response, CHANGED_2_04);
    coap_set_payload(response, buffer, 0);
    
}