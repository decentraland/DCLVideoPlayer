#include "utils.h"

#include <time.h>
#include <errno.h>
#include <math.h>

double get_time_in_seconds() {
  struct timespec tms;

  /* The C11 way */
  /* if (! timespec_get(&tms, TIME_UTC)) { */

  /* POSIX.1-2008 way */
  if (clock_gettime(CLOCK_REALTIME, &tms)) {
    return -1;
  }
  /* seconds, multiplied with 1 million */
  int64_t micros = tms.tv_sec * 1000000;
  /* Add full microseconds */
  micros += tms.tv_nsec / 1000;
  /* round up if necessary */
  if (tms.tv_nsec % 1000 >= 500) {
    ++micros;
  }
  double seconds = ((double) micros) / 1000000.0;
  return seconds;
}

int milisleep(long msec) {
  struct timespec ts;
  int res;

  if (msec < 0)
  {
    errno = EINVAL;
    return -1;
  }

  ts.tv_sec = msec / 1000;
  ts.tv_nsec = (msec % 1000) * 1000000;

  do {
    res = nanosleep(&ts, &ts);
  } while (res && errno == EINTR);

  return res;
}

uint8_t get_next_id(uint8_t* id) {
  uint8_t new_id = *id;
  if (*id == UINT8_MAX) {
    *id = 0;
  } else {
    *id += 1;
  }
  return new_id;
}