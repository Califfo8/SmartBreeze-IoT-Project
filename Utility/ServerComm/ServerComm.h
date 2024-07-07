#ifndef SERVER_COMM_H
#define SERVER_COMM_H

#include "contiki.h"
#include "etimer.h"

#include "coap-engine.h"
#include "coap-blocking-api.h"
#include "coap-observe-client.h"

#define SLEEP_INTERVAL 30 // in seconds

int max_attempts = -1;
struct etimer wait_timer;

void server_request(coap_endpoint_t* server_ep, int req_type ,const char* service_url, const char* payload, int attemps, void (*handler)(coap_message_t *response));
void up_attempts();
void stop_attempts();

#endif // SERVER_COMM_H