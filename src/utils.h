#ifndef H_UTILS
#define H_UTILS

#include <stdint.h>

double get_time_in_seconds();

/* msleep(): Sleep for the requested number of milliseconds. */
int msleep(double msec);

/* usleep(): Sleep for the requested number of microseconds. */
int usleep(long usec);

uint8_t get_next_id(uint8_t* id);

#endif