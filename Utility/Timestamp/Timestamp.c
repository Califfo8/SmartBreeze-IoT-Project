#include "Timestamp.h"

//Advance the time by a certain number of hours
//the application do not consider the minutes so it will be always the same
void advance_time(Timestamp* ts, int hours)
{
  ts->hour = ts->hour + hours;
  if (ts->hour == 24)
  {
    ts->hour = 0;
    ts->day++;
    if (ts->month == 2 && ts->day == 28) // February
    {
      ts->day = 1;
      ts->month++;
  
    }
    else if ((ts->month == 4 || ts->month == 6 || ts->month == 9 || ts->month == 11) && ts->day == 31) // April, June, September, November
    {
      ts->day = 1;
      ts->month++;
    }
    else if (ts->day == 32) // January, March, May, July, August, October, December
    {
      ts->day = 1;
      ts->month++;
      if (ts->month == 13)
      {
        ts->month = 1;
        ts->year++;
      }
    }
  }
}

void convert_to_feature(Timestamp* ts, float* float_ts)
{
  float_ts[0] = ts->month;
  float_ts[0] = ts->day;
  float_ts[1] = ts->hour;
}

void copy_timestamp(Timestamp* ts, Timestamp* new_ts)
{
  ts->year = new_ts->year;
  ts->month = new_ts->month;
  ts->day = new_ts->day;
  ts->hour = new_ts->hour;
  ts->minute = new_ts->minute;
}

void timestamp_to_string(Timestamp* ts, char* string)
{
  static char str_month[13];
  static char str_day[13];
  static char str_hour[13];
  static char str_minute[13];
  if (ts->month < 10)
    sprintf(str_month, "0%d", ts->month);
  else
    sprintf(str_month, "%d", ts->month);

  if (ts->day < 10)
    sprintf(str_day, "0%d", ts->day);
  else
    sprintf(str_day, "%d", ts->day);

  if (ts->hour < 10)
    sprintf(str_hour, "0%d", ts->hour);
  else
    sprintf(str_hour, "%d", ts->hour);
  
  sprintf(string, "%d-%s-%sT%s:%sZ", ts->year, str_month, str_day, str_hour, str_minute);
}