#include "contiki.h"
#include "coap-engine.h"
#include "sys/log.h"
#include "os/dev/leds.h"
#include <stdio.h>
#include <stdlib.h>
#include "../Utility/RandomNumberGenerator/RandomNumberGenerator.h"
#include "../Utility/JSONSenML/JSONSenML.h"
#include "../Utility/Timestamp/Timestamp.h"
#include "../Utility/Leds/Leds.h"
//----------------------------------PARAMETERS----------------------------------//
//[+] LOG CONFIGURATION
#define LOG_MODULE "App"
#define LOG_LEVEL LOG_LEVEL_INFO

//[+] TEMPERATURE PARAMETERS
#define UNIT "C"
#define OFF 0
#define HVAC1 1
#define HVAC2 2
// Simulation parameters
#define MAX_TEMP 42
#define MIN_TEMP 5
#define HIGTH_RATE 0.1
#define LOW_RATE 0.05
#define P_HIGTH 0.3 // Probability of generating a high rate temperature

#define HVAC1_RATE 0.06
#define HVAC2_RATE 0.12

// User parameters
#define HVAC1_POWER_CONS 500
#define HVAC2_POWER_CONS 2000
float hvac1_power_cons = HVAC1_POWER_CONS;
float hvac2_power_cons = HVAC2_POWER_CONS;

//[+] USER PARAMETERS
extern float max_temp_user;
extern float min_temp_user;

//[+] OBSERVED PARAMETERS
extern float sampled_energy;
extern float predicted_energy;
extern Timestamp timestamp;
//[+] EXTERNAL PARAMETERS
extern bool button_pressed;
//[+] DECISION PARAMETERS
static float old_temperature = 20.0;
//----------------------------------RESOURCES----------------------------------//
// Sampled temperature
static float temperature = 20.0;

// HVAC
static int active_hvac = OFF;
//----------------------------FUNCTIONS DEFINITIONS----------------------------------//
static float fake_temp_sensing(float temperature);
static void manage_hvac();
void res_get_handler(coap_message_t *request, coap_message_t *response,
                          uint8_t *buffer, uint16_t preferred_size, int32_t *offset);
static void res_event_handler(void);

// Resource definition
EVENT_RESOURCE(
        res_temperature_HVAC, 
        "title=\"Temperature\";rt=\"json+senml\";if=\"sensor_actuator\";obs", 
        res_get_handler, 
        NULL,
        NULL,
        NULL,
        res_event_handler);

//----------------------------FUNCTIONS DECLARATIONS----------------------------------//
static float fake_temp_sensing(float temperature)
{
    if(active_hvac == OFF)
    {
        bool coin = flip_coin(P_HIGTH);
        if(coin)
            return generate_random_float(temperature, MAX_TEMP, MIN_TEMP, HIGTH_RATE, 1);
        else
            return generate_random_float(temperature, MAX_TEMP, MIN_TEMP, LOW_RATE, 1);
    }else if(active_hvac == HVAC1)
    {
        return generate_random_float(temperature, MAX_TEMP, MIN_TEMP, HVAC1_RATE, 0);
    }else if(active_hvac == HVAC2)
    {
        return generate_random_float(temperature, MAX_TEMP, MIN_TEMP, HVAC2_RATE, 0);
    }

    LOG_ERR("[Climate-manager] Error in the fake_temp_sensing function\n");
  return 0;
        
}

static void manage_hvac()
{
    if(sampled_energy == -1 || predicted_energy == -1)
    {
        LOG_ERR("[Climate-manager] Energy parameters not arrived\n");
        return;
    }else if (button_pressed)
    {
        LOG_INFO("[Climate-manager] BUTTON PRESSED: FORCED HVAC2 OPERATION\n");
        active_hvac = HVAC2;
        return;
    }
    // Compute the variation of the temperature
    float variation = (1 - temperature/old_temperature);
    float off_threshold = min_temp_user + (max_temp_user - min_temp_user)/4;
    float hvac1_threshold  = min_temp_user + 2*(max_temp_user - min_temp_user)/4;
    float hvac2_threshold  = max_temp_user - (max_temp_user - min_temp_user)/4;
    LOG_DBG("[Climate-manager] HVAC data:\n");
    LOG_DBG("[Climate-manager] -variation: %f\n", variation);
    LOG_DBG("[Climate-manager] -off_threshold: %f\n", off_threshold);
    LOG_DBG("[Climate-manager] -hvac1_threshold: %f\n", hvac1_threshold);
    LOG_DBG("[Climate-manager] -hvac2_threshold: %f\n", hvac2_threshold);

    // if the temperature is increasing
    if(variation < 0)
    {
        // Control if the HVAC is active
        switch (active_hvac)
        {
        case OFF:
            if(predicted_energy >= hvac1_power_cons || temperature >= hvac1_threshold)
            {
                active_hvac = HVAC1;
            }
            else if(predicted_energy >= hvac2_power_cons || temperature >= hvac2_threshold)
            {
                active_hvac = HVAC2; 
            }          
            break;
        case HVAC1:
            if(predicted_energy < hvac1_power_cons || temperature <= off_threshold)
            {
                active_hvac = OFF;
            }
            else if(predicted_energy >= hvac2_power_cons || temperature >= hvac2_threshold)
            {
                active_hvac = HVAC2;
            }        
            break;
        case HVAC2:
            if(predicted_energy < hvac2_power_cons || temperature <= off_threshold)
            {
                active_hvac = OFF;
            }                
            else if((predicted_energy >= hvac1_power_cons && predicted_energy < hvac2_power_cons) || temperature < hvac2_threshold)
            {
                active_hvac = HVAC1;
            }         
            break;
        default:
            LOG_ERR("[Climate-manager] Error in the manage_hvac function\n");
            break;
        }
        
    }else
    {
        switch (active_hvac)
        {
        case OFF:        
            break;
        case HVAC1:
            if(predicted_energy < hvac1_power_cons || temperature <= off_threshold)
            {
                active_hvac = OFF;
            }
            else if(predicted_energy >= hvac2_power_cons)
            {
                active_hvac = HVAC2;
            }        
            break;
        case HVAC2:
            if(predicted_energy < hvac2_power_cons || temperature <= off_threshold)
            {
                active_hvac = OFF;
            }                
            else if((predicted_energy >= hvac1_power_cons && predicted_energy < hvac2_power_cons))
            {
                active_hvac = HVAC1;
            }         
            break;
        default:
            LOG_ERR("[Climate-manager] Error in the manage_hvac function\n");
            break;
        }
    }

    // Control the leds
    switch (active_hvac)
    {
    case OFF:
        ctrl_leds(LEDS_RED);
        break;
    case HVAC1:
        ctrl_leds(LEDS_YELLOW);
        break;
    case HVAC2:
        ctrl_leds(LEDS_GREEN);
        break;
    default:
        LOG_ERR("[Climate-manager] Error in the manage_hvac function in leds control\n");
        break;
    }
    

}

/* Define the resource handler function */
void res_get_handler(coap_message_t *request, coap_message_t *response,
                          uint8_t *buffer, uint16_t preferred_size, int32_t *offset) {

    MeasurementData data[2];
    json_senml js_senml;
    char names[2][MAX_STR_LEN] = {"temperature", "HVAC"};
    char timestamp_str[2][TIMESTAMP_STRING_LEN] = {"", ""};
    char base_name[BASE_NAME_LEN];
    int payload_len = 0;
    
    //Creo il timestamp
    timestamp_to_string(&timestamp, timestamp_str[0]);
    strcpy(timestamp_str[1], timestamp_str[0]);
    // Create the base name
    get_base_name(base_name);
     
    // Inizializzo i valori delle risorse
    data[0].name = names[0];
    data[0].unit = UNIT;
    data[0].time = timestamp_str[0];
    data[0].v.v = temperature;
    data[0].type = V_FLOAT;

    data[1].name = names[1];
    data[1].unit = "None";
    data[1].time = timestamp_str[1];
    data[1].v.vd = active_hvac;
    data[1].type = V_INT;

    // Creo il JSON SenML
    js_senml.base_name = base_name;
    js_senml.base_unit = UNIT;
    js_senml.measurement_data = data;
    js_senml.num_measurements = 2;

    // Convert the JSON SenML to a payload
    payload_len = json_to_payload(&js_senml, (char*)buffer);
   
    if (payload_len < 0)
    {
      LOG_ERR("[Climate-manager] Error in the json_to_payload function\n");
      coap_set_status_code(response, INTERNAL_SERVER_ERROR_5_00);
      coap_set_payload(response, buffer, 0);
      return;
    }else if (payload_len > preferred_size)
    {
      LOG_ERR("[Climate-manager] Buffer overflow\n");
      coap_set_status_code(response, INTERNAL_SERVER_ERROR_5_00);
      coap_set_payload(response, buffer, 0);
      return;
    }

    // Prepare the response
    coap_set_header_content_format(response, APPLICATION_JSON);
    coap_set_header_etag(response, (uint8_t *)&payload_len, 1);
    coap_set_payload(response, buffer, payload_len);

    // Print sended data for debug
    LOG_INFO("[Climate-manager] Sending data: %s with size: %d\n", buffer, payload_len);
}

static void res_event_handler(void) {
    char timestamp_str[TIMESTAMP_STRING_LEN];
    timestamp_to_string(&timestamp, timestamp_str);
    // Sample the temperature
    old_temperature = temperature;
    temperature = fake_temp_sensing(temperature);
    LOG_INFO("[Climate-manager]------------------------NEW-EVENT------------------ \n");
    LOG_INFO("[Climate-Manager] New sample at time %s\n", timestamp_str);
    LOG_INFO("[Climate-manager] Sampled Temperature: %f\n", temperature);
    // Making Decision
    manage_hvac();
    LOG_INFO("[Climate-manager] Active HVAC: %d\n", active_hvac);
    // Notify the observers
    coap_notify_observers(&res_temperature_HVAC);
}
