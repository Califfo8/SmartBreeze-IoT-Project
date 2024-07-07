#include "ServerComm.h"

void server_request(coap_endpoint_t* server_ep, int req_type ,const char* service_url, const char* payload, int attemps, void (*handler)(coap_message_t *response))
{
    static coap_message_t request[1]; // This way the packet can be treated as pointer as usual
    max_attempts = attemps;

    while (max_attempts != 0) {
        // Prepare the request
        coap_init_message(request, COAP_TYPE_CON, req_type, 0);
        coap_set_header_uri_path(request, service_url);
        // Set the payload
        coap_set_payload(request, (uint8_t *)payload, strlen(payload));
        // Send the request and wait for the response
        COAP_BLOCKING_REQUEST(&server_ep, request, handler);
        // Something goes wrong with the registration, the node sleeps and tries again
        if (max_attempts == -1) {
            etimer_set(&wait_timer, CLOCK_SECOND * SLEEP_INTERVAL);
            PROCESS_WAIT_EVENT_UNTIL(etimer_expired(&wait_timer));
            max_attempts = attemps;
        }
      
    }
    max_attempts = -1;

}

void up_attempts()
{
    // retry registration
    max_attempts--;
    // if max attempts are reached, signal to wait
    if(max_attempts == 0) {
        max_attempts = -1;
    }
}

void stop_attempts()
{
    max_attempts = 0;
}