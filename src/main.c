//#include "decoder.h"
#include "player.h"
#include "logger.h"
#include <time.h>

double getTimeInSeconds()
{
  struct timespec tms;

  /* The C11 way */
  /* if (! timespec_get(&tms, TIME_UTC)) { */

  /* POSIX.1-2008 way */
  if (clock_gettime(CLOCK_REALTIME,&tms)) {
    return -1;
  }
  /* seconds, multiplied with 1 million */
  int64_t micros = tms.tv_sec * 1000000;
  /* Add full microseconds */
  micros += tms.tv_nsec/1000;
  /* round up if necessary */
  if (tms.tv_nsec % 1000 >= 500) {
    ++micros;
  }
  double seconds = ((double)micros) / 1000000.0;
  return seconds;
}

/*int runDecoder()
{
    logging("Running Decoder\n");

    DecoderContext* dectx = create("http://commondatastorage.googleapis.com/gtv-videos-bucket/sample/BigBuckBunny.mp4");
    if (dectx == NULL)
    {
        logging("error on create");
        return -1;
    }

    int i = 50000000;
    while(i >= 0)
    {
        ProcessOutput processOutput;
        int res = process_frame(dectx, &processOutput);

        if (processOutput.videoFrame) {
            av_frame_free(&processOutput.videoFrame);
        }

        if (processOutput.audioFrame) {
            av_frame_free(&processOutput.audioFrame);
        }

        if (res >= 0) {
            --i;
        }
    }

    destroy(dectx);
    return 0;
}*/

int main()
{
  double checkpointTime = getTimeInSeconds();
  logging("Hello world\n");

  VideoPlayerContext* vpc = playerCreate("http://commondatastorage.googleapis.com/gtv-videos-bucket/sample/BigBuckBunny.mp4");

  playerPlay(vpc);
  int lastSecond = 0;
  int testStatus = 0;
  while(1) {
    float checkpointTimeInSeconds = getTimeInSeconds() - checkpointTime;
    float currentTimeInSeconds = playerGetPlaybackPosition(vpc);
    if (lastSecond != (int)currentTimeInSeconds) {
      lastSecond = (int)currentTimeInSeconds;
      logging("Second: %d %d\n", lastSecond, (int)checkpointTimeInSeconds);
    }
    playerProcess(vpc);

    uint8_t* videoData = NULL;
    playerGrabVideoFrame(vpc, &videoData);
    if (videoData != NULL) {
      logging("New frame! %f\n", playerGetPlaybackPosition(vpc));
      playerReleaseVideoFrame(vpc);
    }

    uint8_t* audioData = NULL;
    playerGrabAudioFrame(vpc, &audioData);
    if (audioData != NULL)
      playerReleaseAudioFrame(vpc);

    if (testStatus == 0) {
      if (checkpointTimeInSeconds >= 3.0f) {
        testStatus = 1;
        playerStop(vpc);
      }
    } else if (testStatus == 1) {
      if (checkpointTimeInSeconds >= 8.0f) {
        playerSeek(vpc, 120.0f);
        playerPlay(vpc);
        testStatus = 2;
      }
    } else if (testStatus == 2) {
      if (checkpointTimeInSeconds >= 15.0f) {
        break;
      }
    }
  }

  playerDestroy(vpc);
  return 0;
}