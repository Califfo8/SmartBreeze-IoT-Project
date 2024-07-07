#ifndef RESREQUEST_H
#define RESREQUEST_H
#include "contiki.h"
#include "coap-engine.h"
#include "coap-blocking-api.h"
#include "sys/log.h"
#include "etimer.h"
#include <stdio.h>

void resource_request(char* str_server_ep, char* res_url, char* payload, int max_attempts)
{
    static coap_endpoint_t server_ep;
    static coap_message_t request[1]; // This way the packet can be treated as pointer as usual

    while (max_attempts != 0) {
        // Populate the coap endpoint structure
        coap_endpoint_parse(str_server_ep, strlen(SERVER_EP), &server_ep);
        // Prepare the request
        coap_init_message(request, COAP_TYPE_CON, COAP_POST, 0);
        coap_set_header_uri_path(request, res_url);
        // Set the payload
        coap_set_payload(request, (uint8_t *)payload, strlen(payload));
        // Send the request and wait for the response
        COAP_BLOCKING_REQUEST(&server_ep, request, registration_chunk_handler);
        // Something goes wrong with the registration, the node sleeps and tries again
        if (max_attempts == -1) {
            etimer_set(&sleep_timer, CLOCK_SECOND * SLEEP_INTERVAL);
            PROCESS_WAIT_EVENT_UNTIL(etimer_expired(&sleep_timer));
            max_attempts = MAX_REGISTER_ATTEMPTS;
        }
      
    }
}



#endif // RESREQUEST_H