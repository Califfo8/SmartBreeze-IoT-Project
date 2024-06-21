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
    return strlen(payload);
}

int copy_value (char *string, char *output, char *start, char *end)
{

  char *start_ptr = strstr(string, start);
  char *end_ptr = strstr(string, end);
  //LOG_INFO("Start: %s\n End: %s\n\n", start_ptr, end_ptr);

  if (start_ptr == NULL || end_ptr == NULL)
	{
	  return -1;
	}
  // Tolgo la prima parte
  start_ptr += strlen (start);
  
  strncpy (output, start_ptr, end_ptr - start_ptr);
  output[end_ptr - start_ptr] = '\0';
  return (end_ptr - string) + strlen(end);
}

void parse_str (char *payload, json_senml * js)
{
  char* pos = payload;
  int passo = 0;
  // Copio il base_name
  passo = copy_value (pos, js->base_name, "\"bn\":", ",");
  if(passo == -1)
  {
      LOG_ERR("ERROR: Parsing base name\n");
      LOG_ERR("\t %s\n", pos);
      return;
  }
  pos += passo;
  
  // Copio la base unit
  passo = copy_value (pos, js->base_unit, "\"bu\":", "}");
  if(passo == -1)
  {
      LOG_ERR("ERROR: Parsing base unit\n");
      return;
  }
  pos += passo;
  // Skip the ",{"
  pos+=2;
  // Copio i dati
  for(int i=0; i<js->num_measurements;i++)
  {
      // name
      passo = copy_value (pos, js->measurement_data[i].name, "\"n\":", ",");
      if(passo == -1)
      {
        LOG_ERR("ERROR: Parsing name of measure %d\n", i);
        return;
      }
      pos += passo;
      // Unit
      passo = copy_value (pos, js->measurement_data[i].unit, "\"u\":", ",");
      if(passo == -1)
      {
        LOG_ERR("ERROR: Parsing unit of measure %d\n", i);
        return;
      }
      pos += passo;
      //Timestamp
      passo = copy_value (pos, js->measurement_data[i].time, "\"t\":", ",");
      if(passo == -1)
      {
        LOG_ERR("ERROR: Parsing time of measure %d\n", i);
        return;
      }
      pos += passo;
      
      //Value
      char time_value[MAX_STR_LEN];
      char *endptr;
      char *point;
      passo = copy_value (pos, time_value, "\"v\":", "}");
      if(passo == -1)
      {
        LOG_ERR("ERROR: Parsing value of measure %d\n", i);
        return;
      }
      pos += passo;
      // Replace "," to "."
      point = strstr(time_value, ",");
      if(point != NULL)
      {
          *point = '.';
      }
      // Convert to float
      js->measurement_data[i].v.v = strtof(time_value, &endptr);
       if (endptr == time_value) {
        LOG_ERR("ERROR: Convert value of measure %d\n", i);
        }
        js->measurement_data[i].type = V_FLOAT;
      // Skip the ",{"
      pos+=2;
  }
}
void print_json_senml(json_senml *senml) {
    LOG_INFO("Base Name: %s\n", senml->base_name);
    LOG_INFO("Base Unit: %s\n", senml->base_unit);
    LOG_INFO("Number of Measurements: %d\n", senml->num_measurements);

    for (int i = 0; i < senml->num_measurements; i++) {
        MeasurementData *md = &senml->measurement_data[i];
        LOG_INFO("\nMeasurement %d:\n", i + 1);
        LOG_INFO("  Name: %s\n", md->name);
        LOG_INFO("  Unit: %s\n", md->unit);
        LOG_INFO("  Time: %s\n", md->time);

        // Stampa il valore in base al tipo
        switch (md->type) {
            case V_FLOAT:
                LOG_INFO("  Value: %f (float)\n", md->v.v);
                break;
            case V_INT:
                LOG_INFO("  Value: %d (int)\n", md->v.vd);
                break;
            case V_STRING:
                LOG_INFO("  Value: %s (string)\n", md->v.vs);
                break;
            default:
                LOG_INFO("  Value: Unknown type\n");
                break;
        }
    }
}

