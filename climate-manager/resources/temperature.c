#include "contiki.h"
#include "coap-engine.h"
#include "sys/log.h"
#include <stdio.h>
#include <stdlib.h>
#include "../Utility/RandomNumberGenerator/RandomNumberGenerator.h"
#include "../Utility/JSONSenML/JSONSenML.h"

//----------------------------------PARAMETERS----------------------------------//
//[+] LOG CONFIGURATION
#define LOG_MODULE "App"
#define LOG_LEVEL LOG_LEVEL_INFO

//[+] TEMPERATURE PARAMETERS
#define UNIT "C°"
#define OFF 0
#define HVAC1 1
#define HVAC2 2
// Simulation parameters
#define MAX_TEMP 42
#define MIN_TEMP 10
#define HIGTH_RATE 0.3
#define LOW_RATE 0.1
#define P_HIGTH 0.3 // Probability of generating a high rate temperature
#define HVAC1_RATE 0.15
#define HVAC2_RATE 0.4
// User parameters
#define MAX_TEMP_USER 25
#define MIN_TEMP_USER 10
#define HVAC1_POWER_CONS 500
#define HVAC2_POWER_CONS 2000
float max_temp_user = MAX_TEMP_USER;
float min_temp_user = MIN_TEMP_USER;
float hvac1_power_cons = HVAC1_POWER_CONS;
float hvac2_power_cons = HVAC2_POWER_CONS;

//[+] DERIVATED PARAMETERS
float low_trashold = 0.1;

//----------------------------------RESOURCES----------------------------------//
// Sampled temperature
static float temperature = 20.0;
// Active HVAC
static int active_hvac = 0; // 0: OFF, 1: HVAC1 ON, 2: HVAC2 ON
//----------------------------FUNCTIONS DEFINITIONS----------------------------------//
static float fake_temp_sensing(float temperature);
//static void manage_hvac(float temperature);
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
/*
static void manage_hvac(float temperature)
{

    if(temperature > max_temp_user)
    {
        active_hvac = HVAC2;
    }else if(temperature < min_temp_user)
    {
        active_hvac = HVAC1;
    }else
    {
        active_hvac = OFF;
    }

}*/

/* Define the resource handler function */
void res_get_handler(coap_message_t *request, coap_message_t *response,
                          uint8_t *buffer, uint16_t preferred_size, int32_t *offset) {
        char message[12];
        int length = 12;

        snprintf(message, length, "%f", temperature);

        // Prepare the response
        // Copy the response in the transmission buffer
        memcpy(buffer, message, length);
        coap_set_header_content_format(response, TEXT_PLAIN);
        coap_set_header_etag(response, (uint8_t *)&length, 1);
        coap_set_payload(response, buffer, length);
}

static void res_event_handler(void) {
    LOG_INFO("[Climate-manager] New sample \n");
    // Sample the solar energy
    temperature = fake_temp_sensing(temperature);
    LOG_INFO("[Energy-manager] \t-Sampled Temperature: %f\n", temperature);
    // Making Decision
    
    // Notify the observers
    coap_notify_observers(&res_temperature_HVAC);
}
