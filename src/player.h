#pragma once

#include "decoder.h"
#include "safeframequeue.h"
#include "threadqueue.h"

QueueContext *thread_queue = NULL;

enum State {
    StateLoading = 0,
    StateReady = 1,
    StateError = 2,
};

typedef struct MediaPlayerContext {
    DecoderContext *dectx;
    char *url;

    SafeQueueContext *video_queue;
    SafeQueueContext *audio_queue;

    double last_video_frame_time;

    double start_time;
    double video_progress_time;
    uint8_t playing;
    uint8_t loop;
    uint8_t state;
    uint8_t buffering;
    uint8_t thread_running;

    double playback_rate;
    double playback_reference;
    double playback_reference_with_rate;
} MediaPlayerContext;

void player_join_threads();

MediaPlayerContext *player_create(const char *url);

void player_destroy(MediaPlayerContext *vpc);

void player_play(MediaPlayerContext *vpc);

void player_stop(MediaPlayerContext *vpc);

int player_get_state(MediaPlayerContext *vpc);

int player_is_buffering(MediaPlayerContext *vpc);

int player_is_playing(MediaPlayerContext *vpc);

void player_set_paused(MediaPlayerContext *vpc, int paused);

void player_set_loop(MediaPlayerContext *vpc, int loop);

int player_has_loop(MediaPlayerContext *vpc);

float player_get_length(MediaPlayerContext *vpc);

float player_get_playback_position(MediaPlayerContext *vpc);

void player_set_playback_rate(MediaPlayerContext *vpc, double playback_rate);

void player_seek(MediaPlayerContext *vpc, float time);

double player_grab_video_frame(MediaPlayerContext *vpc, void **release_ptr, uint8_t **data);

double player_grab_audio_frame(MediaPlayerContext *vpc, void **release_ptr, uint8_t **data, int *frame_size);

void player_release_frame(MediaPlayerContext *vpc, void *release_ptr);

void player_get_video_format(MediaPlayerContext *vpc, int *width, int *height);

void player_get_audio_format(MediaPlayerContext *vpc, int *frequency, int *channels);

double player_get_start_time(MediaPlayerContext *vpc);

void player_set_start_time(MediaPlayerContext *vpc, double time);

double player_get_global_time(MediaPlayerContext *vpc);