#ifndef TIMESTAMP_H
#define TIMESTAMP_H

// Advance the time by the given hours
void advance_time(int* ts, int hours);
void convert_to_float(int* ts, float* float_ts);
void copy_timestamp(int* ts, int* new_ts);
#endif // TIMESTAMP_H