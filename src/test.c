//#include "decoder.h"
#include "player.h"
#include "logger.h"
#include "testutil.h"
#include <assert.h>

void test_decoder() {
  logging("Running Decoder\n");

  DecoderContext *dectx = decoder_create(
          "http://commondatastorage.googleapis.com/gtv-videos-bucket/sample/BigBuckBunny.mp4");
  if (dectx == NULL) {
    logging("error on create");
    return;
  }

  for (int i = 0; i < 10; ++i) {
    ProcessOutput processOutput;
    int res = decoder_process_frame(dectx, &processOutput);
    assert(res == 0);
    if (processOutput.videoFrame) {
      av_frame_free(&processOutput.videoFrame);
    }

    if (processOutput.audioFrame) {
      av_frame_free(&processOutput.audioFrame);
    }
  }

  decoder_destroy(dectx);
}

void test_format(const char *test_name, const char *url, uint8_t expected_status) {
  logging(test_name);
  double timeout = get_time_in_seconds() + 30.0;
  MediaPlayerContext *vpc = player_create(url);

  while(player_get_status(vpc) == StatusLoading) {}

  assert(player_get_status(vpc) == expected_status);

  player_play(vpc);

  int video_frames = 0;
  int audio_frames = 0;
  while (audio_frames < 10 || video_frames < 10) {

    void *release_ptr = NULL;
    uint8_t *videoData = NULL;
    do {
      videoData = NULL;
      player_grab_video_frame(vpc, &release_ptr, &videoData);
      if (videoData != NULL) {
        player_release_frame(vpc, release_ptr);
        ++video_frames;
      }
    } while (videoData != NULL);

    uint8_t *audio_data = NULL;
    int frame_size = 0;
    do {
      audio_data = NULL;
      player_grab_audio_frame(vpc, &release_ptr, &audio_data, &frame_size);
      if (audio_data != NULL) {
        player_release_frame(vpc, release_ptr);
        ++audio_frames;
      }
    } while (audio_data != NULL);

    msleep(1);

    if (get_time_in_seconds() >= timeout) {
      assert(0 == 1);
    }
  }

  player_destroy(vpc);
  player_join_threads();
}

void test_loop() {

}

void test_seek() {

}

int main() {
  logging("DCLVideoPlayer Tests");

  test_decoder();

  test_format("HTTP+MP4", "http://commondatastorage.googleapis.com/gtv-videos-bucket/sample/BigBuckBunny.mp4", StatusReady);

  test_format("HTTPS+MP4", "https://storage.googleapis.com/gtv-videos-bucket/sample/BigBuckBunny.mp4", StatusReady);

  test_format("HTTP+WEBM", "http://techslides.com/demos/sample-videos/small.webm", StatusReady);

  test_format("HTTP+OGV", "http://techslides.com/demos/sample-videos/small.ogv", StatusReady);

  test_format("Invalid URL", "", StatusError);

  return 0;
}