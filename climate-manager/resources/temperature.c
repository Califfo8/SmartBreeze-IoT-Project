#include "contiki.h"
#include "coap-engine.h"
#include <stdio.h>
float temperature = 20.0;

void res_temperature_handler(coap_message_t *request, coap_message_t *response,
                          uint8_t *buffer, uint16_t preferred_size, int32_t *offset);

/* Define the solar_energy resource */
RESOURCE(
        res_temperature, 
        "title=\"Temperature\";rt=\"temperature\"", 
        res_temperature_handler, 
        NULL,
        NULL,
        NULL);
    
/* Define the resource handler function */
void res_temperature_handler(coap_message_t *request, coap_message_t *response,
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


