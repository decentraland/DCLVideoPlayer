#pragma once

#ifdef _WIN32
#  define export __declspec(dllexport)
#else
#  define export
#endif

#include "decoder.h"
#include "framequeue.h"

typedef struct MediaPlayerContext {
    DecoderContext *dectx;
    QueueContext *video_queue;
    QueueContext *audio_queue;

    double last_video_frame_time;

    double start_time;
    double video_progress_time;
    int playing;
    int loop;
    int buffering;
} MediaPlayerContext;

export MediaPlayerContext *player_create(const char *url);

export void player_destroy(MediaPlayerContext *vpc);

export void player_play(MediaPlayerContext *vpc);

export void player_stop(MediaPlayerContext *vpc);

export int player_is_buffering(MediaPlayerContext *vpc);

export int player_is_playing(MediaPlayerContext *vpc);

export void player_set_paused(MediaPlayerContext *vpc, int paused);

export void player_set_loop(MediaPlayerContext *vpc, int loop);

export int player_has_loop(MediaPlayerContext *vpc);

export float player_get_length(MediaPlayerContext *vpc);

export float player_get_playback_position(MediaPlayerContext *vpc);

export void player_seek(MediaPlayerContext *vpc, float time);

export void player_process(MediaPlayerContext *vpc);

export double player_grab_video_frame(MediaPlayerContext *vpc, void **release_ptr, uint8_t **data);

export double player_grab_audio_frame(MediaPlayerContext *vpc, void **release_ptr, uint8_t **data, int *frame_size);

export void player_release_frame(MediaPlayerContext *vpc, void *release_ptr);

export void player_get_video_format(MediaPlayerContext *vpc, int *width, int *height);

export void player_get_audio_format(MediaPlayerContext *vpc, int *frequency, int *channels);

export double player_get_start_time(MediaPlayerContext *vpc);

export void player_set_start_time(MediaPlayerContext *vpc, double time);

export double player_get_global_time(MediaPlayerContext *vpc);