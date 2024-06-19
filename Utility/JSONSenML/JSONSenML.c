#include "JSONSenML.h"


void get_base_name(char *mac_str) {
    
    uint8_t mac[6];
    #ifndef COOJA
    mac[0] = (NRF_FICR->DEVICEADDR[1] >> 8) & 0xFF;
    mac[1] = (NRF_FICR->DEVICEADDR[1] >> 0) & 0xFF;
    mac[2] = (NRF_FICR->DEVICEADDR[0] >> 24) & 0xFF;
    mac[3] = (NRF_FICR->DEVICEADDR[0] >> 16) & 0xFF;
    mac[4] = (NRF_FICR->DEVICEADDR[0] >> 8) & 0xFF;
    mac[5] = (NRF_FICR->DEVICEADDR[0] >> 0) & 0xFF;
    #else
    // Generates a fake MAC address for simulation in Cooja
    // UAA (Universally Administered Address) with the LSB bit set to 0
    mac[0] = 0x02;
    mac[1] = 0x00;
    mac[2] = 0x00;
    mac[3] = 0x00;
    mac[4] = 0x00;
    mac[5] = 0x01;
    #endif
    
    snprintf(mac_str, BASE_NAME_LEN, "mac:dev:coap:%02X%02X%02X%02X%02X%02X:", 
             mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
}


int json_to_payload(json_senml* js, char* payload)
{
    char comodo[MAX_PAYLOAD_LEN] = "";
    // Creo il base name con il mac
    sprintf(payload, "{{\"bn\":\"%s\",\"bu\":\"%s\"},", js->base_name, js->base_unit);
    for (int i = 0; i < js->num_measurements; i++)
    {
        sprintf(comodo, "{\"n\":\"%s\",\"u\":\"%s\",\"t\":\"%s\",\"v\":", 
         js->measurement_data[i].name, 
         js->measurement_data[i].unit, 
         js->measurement_data[i].time);
        strcat(payload, comodo);
        if (js->measurement_data[i].type == V_FLOAT)
        {

            sprintf(comodo, "%f},", js->measurement_data[i].v.v);
        }
        else if (js->measurement_data[i].type == V_INT)
        {
            sprintf(comodo, "%d},", js->measurement_data[i].v.vd);
        }
        else if (js->measurement_data[i].type == V_STRING)
        {
            sprintf(comodo, "\"%s\"},", js->measurement_data[i].v.vs);
        }else{
            return -1;
        }
        strcat(payload, comodo);
    }
    strcat(payload, "}");
    return sizeof(payload);
}

int payload_to_json(const char* payload, json_senml* js, int num_measurements) {
    sscanf(payload,"{\"bn\":\"%[^\"]\"",js->base_name);
    sscanf(payload,"{\"bu\":\"%[^\"]\"",js->base_unit);
    for (int i = 0; i < num_measurements; i++)
    {
        sscanf(payload,"{\"n\":\"%[^\"]\",\"u\":\"%[^\"]\",\"t\":\"%[^\"]\",\"v\":%f}", 
         js->measurement_data[i].name, 
         js->measurement_data[i].unit, 
         js->measurement_data[i].time,
         &js->measurement_data[i].v.v);
    }

    return 0;
}