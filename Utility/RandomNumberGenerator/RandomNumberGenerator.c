#include "RandomNumberGenerator.h"

float generate_random_float(float old_value, int max_value, int min_value, float step_size, int increases)
{
    int step = step_size * (max_value - min_value);
    int delta_i = random_rand() % step;
    int delta_d = delta_i % 100;
    float delta = (float)delta_d / 100 + (float)delta_i; 
    float new_value = 0;
    if (increases)
        new_value = old_value + delta;
    else
        new_value = old_value - delta;
    
    if (new_value > (float)max_value)
        new_value = (float)max_value;
    if (new_value < (float)min_value)
        new_value = (float)min_value;
    return new_value;
}

// Return true with probability p, false otherwise
bool flip_coin(float probability)
{
    float random_value = (float)rand() / RAND_MAX;
    if (random_value < probability)
        return true;
    else
        return false;
}