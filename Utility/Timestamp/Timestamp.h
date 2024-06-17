#ifndef TIMESTAMP_H
#define TIMESTAMP_H
#include <stdio.h>
#include <string.h>
#define TIMESTAMP_STRING_LEN 16
typedef struct {
    int year;
    int month;
    int day;
    int hour;
    int minute;
} Timestamp;


// Advance the time by the given hours
void advance_time(Timestamp* ts, int hours);
void convert_to_feature(Timestamp* ts, float* float_ts);
void copy_timestamp(Timestamp* ts, Timestamp* new_ts);
void timestamp_to_string(Timestamp* ts, char* string);
#endif // TIMESTAMP_H