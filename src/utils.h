#ifndef H_UTILS
#define H_UTILS

#include <stdint.h>

double get_time_in_seconds();

/* milisleep(): Sleep for the requested number of milliseconds. */
int milisleep(double msec);

uint8_t get_next_id(uint8_t* id);

#endif