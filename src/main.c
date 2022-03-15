//#include "decoder.h"
#include "player.h"
#include "logger.h"
#include <time.h>

double get_time_in_seconds()
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
  double checkpointTime = get_time_in_seconds();
  logging("Hello world");

  MediaPlayerContext* vpc = player_create(
          "http://commondatastorage.googleapis.com/gtv-videos-bucket/sample/BigBuckBunny.mp4");

  player_play(vpc);
  player_set_loop(vpc, 1);
  player_seek(vpc, 588.0f);
  int last_second = 0;
  int testStatus = 0;
  while(1) {
    double checkpoint_time_in_seconds = get_time_in_seconds() - checkpointTime;
    double current_time_in_seconds = player_get_playback_position(vpc);
    if (last_second != (int)current_time_in_seconds) {
      last_second = (int)current_time_in_seconds;
      logging("Second: %d %d", last_second, (int)checkpoint_time_in_seconds);
    }
    player_process(vpc);

    void* release_ptr = NULL;
    uint8_t* videoData = NULL;
    do {
      videoData = NULL;
      player_grab_video_frame(vpc, &release_ptr, &videoData);
      if (videoData != NULL) {
        logging("New frame! %f", player_get_playback_position(vpc));
        player_release_frame(vpc, release_ptr);
      }
    } while(videoData != NULL);

    uint8_t *audio_data = NULL;
    int frame_size = 0;
    do {
      audio_data = NULL;
      player_grab_audio_frame(vpc, &release_ptr, &audio_data, &frame_size);
      if (audio_data != NULL)
        player_release_frame(vpc, release_ptr);
    } while(audio_data != NULL);

    /*if (testStatus == 0) {
      if (checkpoint_time_in_seconds >= 6.0f) {
        logging("############### TEST STATUS 1 ############################");
        testStatus = 1;
        //playerSeek(vpc, 588.0f);
      }
    } else if (testStatus == 1) {
      if (checkpoint_time_in_seconds >= 500.0f) {
        logging("############### TEST STATUS 2 ############################");
        //player_seek(vpc, 0.0f);
        //player_play(vpc);
        testStatus = 2;
      }
    } else if (testStatus == 2) {
      if (checkpoint_time_in_seconds >= 600.0f) {
        logging("############### TEST STATUS 3 ############################");
        break;
      }
    }*/
  }

  player_destroy(vpc);
  return 0;
}