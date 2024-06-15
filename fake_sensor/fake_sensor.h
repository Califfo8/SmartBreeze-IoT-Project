#ifndef FAKE_SENSOR_H
#define FAKE_SENSOR_H
#include "random.h"

float read_sensor_value(float old_value, int max_value, int min_value,float step_size, int increases);
#endif // FAKE_SENSOR_H