#ifndef JSONSENML_H
#define JSONSENML_H
#include <stdio.h>
#include <stdint.h>
#include <string.h>
#ifndef COOJA
#include <nrfx.h>
#endif

#define BASE_NAME_LEN 32
#define MAX_PAYLOAD_LEN 256
typedef union {
    float v;    // float value
    int vd;     // int value
    char* vs;   // string value
} value;

typedef struct{
    char* name;
    char* unit;
    char* time;
    value v;
} MeasurementData;


typedef struct {
    char* base_name;
    char* base_unit;
    MeasurementData* measurement_data;
    int num_measurements;

} json_senml;

void get_base_name(char *mac_str);
int json_to_payload(json_senml* js, char* string);
#endif // JSONSENML_H