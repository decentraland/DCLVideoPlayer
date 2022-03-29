//#include "decoder.h"
#include "player.h"
#include "logger.h"
#include "utils.h"
#include <assert.h>

void test_id_generator()
{
  uint8_t id = 254;
  uint8_t new_id = 0;
  new_id = get_next_id(&id);
  assert(new_id == 254);
  assert(id == 255);

  new_id = get_next_id(&id);
  assert(new_id == 255);
  assert(id == 0);

  new_id = get_next_id(&id);
  assert(new_id == 0);
  assert(id == 1);
}

void test_decoder() {
  logging("Running Decoder\n");

  DecoderContext *dectx = decoder_create(
          "http://commondatastorage.googleapis.com/gtv-videos-bucket/sample/BigBuckBunny.mp4", 0, 1);
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

void test_format(const char *test_name, const char *url, uint8_t expected_state) {
  logging(test_name);
  double timeout = get_time_in_seconds() + 30.0;
  MediaPlayerContext *vpc = player_create(url, 1);

  while (player_get_state(vpc) == StateLoading) {
    milisleep(1);
  }

  logging("player_get_state=%d vs %d", player_get_state(vpc), expected_state);
  assert(player_get_state(vpc) == expected_state);

  if (player_get_state(vpc) != StateReady) {
    player_destroy(vpc);
    return;
  }

  player_set_loop(vpc, 1);
  player_play(vpc);

  int video_frames = 0;
  int audio_frames = 0;
  while (audio_frames < 128 || video_frames < 128) {

    void *release_ptr = NULL;
    do {
      uint8_t* videoData[3];
      player_grab_video_frame(vpc, &release_ptr, videoData);
      if (videoData[0] != NULL) {
        player_release_frame(vpc, release_ptr);
        ++video_frames;
        logging("frames: %d", video_frames);
      } else {
        break;
      }
    } while (1);

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

    milisleep(1);

    if (get_time_in_seconds() >= timeout) {
      logging("timeout error");
      assert(0 == 1);
    }
  }

  player_destroy(vpc);
  player_stop_all_threads();
}

void test_loop(const char* url) {
  logging("test_loop");
  MediaPlayerContext *vpc = player_create(url, 1);

  while (player_get_state(vpc) == StateLoading) {
    milisleep(1);
  }

  logging("player_get_state=%d vs %d", player_get_state(vpc), StateReady);
  assert(player_get_state(vpc) == StateReady);

  if (player_get_state(vpc) != StateReady) {
    player_destroy(vpc);
    return;
  }

  player_set_loop(vpc, 1);
  player_play(vpc);
  player_seek(vpc, 77.0);

  int loop_id = vpc->last_loop_id;
  int video_frames = 0;
  int audio_frames = 0;
  while (vpc->last_loop_id != loop_id) {

    void *release_ptr = NULL;
    do {
      uint8_t* videoData[3];
      player_grab_video_frame(vpc, &release_ptr, videoData);
      if (videoData[0] != NULL) {
        player_release_frame(vpc, release_ptr);
        ++video_frames;
        logging("frames: %d %f", video_frames, player_get_playback_position(vpc));
      } else {
        break;
      }
    } while (1);

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

    milisleep(1);
  }

  player_destroy(vpc);
  player_stop_all_threads();
}

void test_seek() {

}

int main() {
  logging("DCLVideoPlayer Tests");

  test_format("MP3", "https://www.soundhelix.com/examples/mp3/SoundHelix-Song-1.mp3", StateReady);

  /*
  test_format("TEST", "https://player.vimeo.com/external/552481870.m3u8?s=c312c8533f97e808fccc92b0510b085c8122a875", StateReady);
  test_format("TEST", "https://vod.dcl.guru/ethereumbuilding/index.m3u8", StateReady);
  test_format("TEST", "https://player.vimeo.com/external/691246715.m3u8?s=b737d04dfc8eb41807655f40e0a320f4459b7fd9", StateReady);
  test_format("TEST", "https://player.vimeo.com/external/691245304.m3u8?s=e71379e85190b8126ade179ef46992bbf398b4dc", StateReady);
  test_format("TEST", "https://vod.dcl.guru/cashlabs/2/polygonal_dressing_1.mp4", StateReady);
  test_format("TEST", "https://vod.dcl.guru/cashlabs/2/polygonal_dressing_2.mp4", StateReady);
  test_format("TEST", "https://vod.dcl.guru/cashlabs/2/auroboros_1.mp4", StateReady);
  test_format("TEST", "https://vod.dcl.guru/cashlabs/2/jason_e_1.mp4", StateReady);
  test_format("TEST", "https://vod.dcl.guru/cashlabs/2/antoni_tudisco_1.mp4", StateReady);
  test_format("TEST", "https://vod.dcl.guru/cashlabs/2/antoni_tudisco_2.mp4", StateReady);
  test_format("TEST", "https://vod.dcl.guru/cashlabs/2/thedematerialised_1.mp4", StateReady);
  test_format("TEST", "https://vod.dcl.guru/cashlabs/2/avavav_1.mp4", StateReady);
  test_format("TEST", "https://vod.dcl.guru/cashlabs/2/krista_kim_1.mp4", StateReady);
  test_format("TEST", "https://vod.dcl.guru/cashlabs/3/metagolden_1.mp4", StateReady);
  test_format("TEST", "https://vod.dcl.guru/cashlabs/3/metagolden_2.mp4", StateReady);
  test_format("TEST", "https://vod.dcl.guru/cashlabs/3/marjan_m_1.mp4", StateReady);
  test_format("TEST", "https://vod.dcl.guru/cashlabs/3/marjan_m_2.mp4", StateReady);
  test_format("TEST", "https://vod.dcl.guru/cashlabs/3/alex_box_1.mp4", StateReady);
  test_format("TEST", "https://vod.dcl.guru/cashlabs/3/nip.mp4", StateReady);

  // Get stuck
  test_format("TEST", "https://peer-lb.decentraland.org/content/contents/bafybeibbwqzr4vpim6xmpvo4yyxkotxn6cucffyyowcmo5n2ngooj565qa", StateReady);
  test_format("TEST", "https://player.vimeo.com/external/691387948.m3u8?s=0c2f0f7f74716ce5f5e5f0a352142dd00fc1eca3", StateReady);
  test_format("TEST", "https://dclstreams.com/hosted/live/cryptovalleycc/index.m3u8", StateReady);
  test_format("TEST", "https://video.dcl.guru/live/visual/index.m3u8", StateReady);
   */

  decoder_print_hw_available();

  test_id_generator();

  test_loop("https://player.vimeo.com/external/691621058.m3u8?s=a2aa7b62cd0431537ed53cd699109e46d0de8575");

  // Unspecified pixel format
  //test_format("HTTPS+MP4", "https://peer-lb.decentraland.org/content/contents/QmWhxckWadLR3qQE5VqBBKDyt5Uj6bkDwRSAEfJ2vU1iZR", StateReady);

  // Stream too slow...
  //test_format("HTTPS+?", "https://eu-nl-012.worldcast.tv/dancetelevisionthree/dancetelevisionthree.m3u8", StateReady);

  test_decoder();

  test_format("HTTPS+HLS 1", "https://player.vimeo.com/external/552481870.m3u8?s=c312c8533f97e808fccc92b0510b085c8122a875", StateReady);

  test_format("HTTPS+HLS 2", "https://player.vimeo.com/external/575854261.m3u8?s=d09797037b7f4f1013d337c04836d1e998ad9c80", StateReady);

  test_format("HTTP+MP4", "http://commondatastorage.googleapis.com/gtv-videos-bucket/sample/BigBuckBunny.mp4",
              StateReady);

  test_format("HTTPS+MP4", "https://storage.googleapis.com/gtv-videos-bucket/sample/BigBuckBunny.mp4", StateReady);

  test_format("HTTP+WEBM", "http://techslides.com/demos/sample-videos/small.webm", StateReady);

  test_format("HTTP+OGV", "http://techslides.com/demos/sample-videos/small.ogv", StateReady);

  test_format("HTTPS+HLS EX-CRASH", "https://player.vimeo.com/external/691415562.m3u8?s=65096902279bbd8bb19bf9e2b9391c4c7e510402", StateReady);

  test_format("Invalid URL", "", StateError);

  return 0;
}
