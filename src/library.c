#include "library.h"

#include <stdio.h>
#include "libavcodec/version.h"
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>

void hello(void) {
  printf("Hello from Library %d!\n", LIBAVCODEC_VERSION_INT);
  avcodec_free_context(NULL);
}
