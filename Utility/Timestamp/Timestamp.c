#include "Timestamp.h"

void advance_time(int* ts, int hours)
{
  ts[2] = ts[2] + hours;
  if (ts[2] == 24)
  {
    ts[2] = 0;
    ts[1]++;
    if (ts[0] == 2 && ts[1] == 28) // February
    {
      ts[1] = 1;
      ts[0]++;
  
    }
    else if ((ts[0] == 4 || ts[0] == 6 || ts[0] == 9 || ts[0] == 11) && ts[1] == 31) // April, June, September, November
    {
      ts[1] = 1;
      ts[0]++;
    }
    else if (ts[1] == 32) // January, March, May, July, August, October, December
    {
      ts[1] = 1;
      ts[0]++;
      if (ts[0] == 13)
      {
        ts[0] = 1;
      }
    }
  }
}

void convert_to_float(int* ts, float* float_ts)
{
  float_ts[0] = ts[0];
  float_ts[1] = ts[1];
  float_ts[2] = ts[2];
}

void copy_timestamp(int* ts, int* new_ts)
{
  new_ts[0] = ts[0];
  new_ts[1] = ts[1];
  new_ts[2] = ts[2];
}