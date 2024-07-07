#ifndef TIMESTAMP_H
#define TIMESTAMP_H
#include <stdio.h>
#include <string.h>
#define TIMESTAMP_STRING_LEN 18
typedef struct {
    int year;
    int month;
    int day;
    int hour;
    int minute;
} Timestamp;

// Advance the time by the given hours
void advance_time(Timestamp* ts, int hours);
void advance_time_m(Timestamp* ts, int minutes);
void convert_to_feature(Timestamp* ts, float* float_ts);
void copy_timestamp(Timestamp* ts, Timestamp* new_ts);
int timestamp_to_string(Timestamp* ts, char* string);
void string_to_timestamp(char* string, Timestamp* ts);
#endif // TIMESTAMP_H