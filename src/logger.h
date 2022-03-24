#ifndef H_LOGGER
#define H_LOGGER

static void logging(const char *fmt, ...) {
  va_list args;
  fprintf(stderr, "VPLOG-");
  va_start(args, fmt);
  vfprintf(stderr, fmt, args);
  va_end(args);
  fprintf(stderr, "\n");
}

#endif