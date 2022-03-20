#include "player.h"
#include "logger.h"
#include <libavformat/avformat.h>

typedef struct ThreadArgs {
    MediaPlayerContext *vpc;
    const char* url;
} ThreadArgs;

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

void* _run_decoder(void* args) {
  ThreadArgs *thread_args = (ThreadArgs *)args;
  MediaPlayerContext *vpc = thread_args->vpc;
  const char* url = thread_args->url;
  free(thread_args);

  vpc->dectx = decoder_create(url);

  if (vpc->dectx == NULL) {
    vpc->failed = 1;
    pthread_exit(NULL);
    return NULL;
  }

  vpc->loading = 0;
  while(vpc->thread_running == 1) {
    int res = -1;
    if (safe_queue_is_full(vpc->video_queue) == 0 && safe_queue_is_full(vpc->audio_queue) == 0) {
      ProcessOutput processOutput;
      res = decoder_process_frame(vpc->dectx, &processOutput);
      if (res == 0) {
        if (processOutput.videoFrame) {
          safe_queue_push(vpc->video_queue, processOutput.videoFrame);
        }

        if (processOutput.audioFrame) {
          safe_queue_push(vpc->audio_queue, processOutput.audioFrame);
        }

        //logging("videoCount=%d audioCount=%d", vpc->video_queue->count, vpc->audio_queue->count);
      }
    }
  }

  decoder_destroy(vpc->dectx);
  safe_queue_destroy(&vpc->video_queue);
  safe_queue_destroy(&vpc->audio_queue);
  free(vpc);

  logging("THREAD EXIT!!");
  pthread_exit(NULL);
  return NULL;
}

void player_join_threads()
{
  while(1) {
    pthread_t thread = queue_pop_front(thread_queue);
    if (thread != NULL) {
      int res = pthread_join(thread, NULL);
      logging("thread exit with code=%d", res);
    } else {
      break;
    }
  }
}

MediaPlayerContext *player_create(const char *url) {
  logging("player_create %s", url);
  MediaPlayerContext *vpc = (MediaPlayerContext *) calloc(1, sizeof(MediaPlayerContext));

  vpc->failed = 0;
  vpc->loading = 1;
  vpc->dectx = NULL;
  vpc->video_queue = safe_queue_create(64);
  vpc->audio_queue = safe_queue_create(128);

  vpc->start_time = get_time_in_seconds();
  vpc->playing = 0;
  vpc->loop = 0;
  vpc->buffering = 1;
  vpc->video_progress_time = 0.0;
  vpc->last_video_frame_time = 0.0f;
  vpc->thread_running = 1;

  pthread_t thread_id;
  ThreadArgs* thread_args = (ThreadArgs*)calloc(1, sizeof(ThreadArgs));
  thread_args->vpc = vpc;
  thread_args->url = url;

  int err = pthread_create(&(thread_id), NULL, &_run_decoder, (void*)thread_args);
  if (err != 0) {
    vpc->failed = 1;
    logging("[ERROR] can't create thread :[%s]", strerror(err));
  } else {
    if (thread_queue == NULL)
      thread_queue = queue_create();
    queue_push(thread_queue, thread_id);
  }
  return vpc;
}

void player_destroy(MediaPlayerContext *vpc) {
  logging("player_destroy");
  vpc->thread_running = 0;
}

void player_play(MediaPlayerContext *vpc) {
  if (vpc->playing == 1) return;
  vpc->start_time = get_time_in_seconds() - vpc->video_progress_time;
  vpc->playing = 1;
  logging("player_play");
}

void player_stop(MediaPlayerContext *vpc) {
  if (vpc->playing == 0) return;
  vpc->playing = 0;
  vpc->video_progress_time = get_time_in_seconds() - vpc->start_time;
  logging("player_stop");
}

int player_failed(MediaPlayerContext *vpc) {
  return vpc->failed;
}

int player_is_loading(MediaPlayerContext *vpc) {
  return vpc->loading;
}

int player_is_buffering(MediaPlayerContext *vpc) {
  return vpc->buffering;
}

int player_is_playing(MediaPlayerContext *vpc) {
  return vpc->playing;
}

void player_set_paused(MediaPlayerContext *vpc, int paused) {
  if (paused == 1) {
    player_play(vpc);
  } else {
    player_stop(vpc);
  }
}

void player_set_loop(MediaPlayerContext *vpc, int loop) {
  vpc->loop = loop;
  vpc->dectx->loop = loop;
}

int player_has_loop(MediaPlayerContext *vpc) {
  return vpc->loop;
}

float player_get_length(MediaPlayerContext *vpc) {
  double ctxDuration = (double) (vpc->dectx->av_format_ctx->duration) / AV_TIME_BASE;
  double vsDuration = vpc->dectx->video_avs->duration;
  return vpc->dectx->video_avs->duration <= 0 ? ctxDuration : vsDuration * av_q2d(vpc->dectx->video_avs->time_base);
}

float player_get_playback_position(MediaPlayerContext *vpc) {
  if (vpc->playing == 1 && vpc->buffering == 0) {
    return (float) (get_time_in_seconds() - vpc->start_time);
  } else {
    return (float) (vpc->video_progress_time);
  }
}

void player_seek(MediaPlayerContext *vpc, float time) {
  decoder_seek(vpc->dectx, time);
  safe_queue_clean(vpc->video_queue);
  safe_queue_clean(vpc->audio_queue);
  vpc->buffering = 1;
  vpc->video_progress_time = time;
  vpc->last_video_frame_time = 0.0f;
}

double _internal_grab_video_frame(MediaPlayerContext *vpc, void **release_ptr, SafeQueueContext *queue, uint8_t **data,
                                  double current_time_in_sec, AVRational time_base) {
  AVFrame *frame = safe_queue_peek_front(queue);

  if (frame != NULL) {
    double time_in_sec = (double) (av_q2d(time_base) * (double) frame->best_effort_timestamp);

    /*if (rand() % 300000 == 1)
      logging("TIMEEE: %lf VS %lf -> %lf", time_in_sec, current_time_in_sec);*/
    if (time_in_sec <= current_time_in_sec) {

      *release_ptr = frame;
      *data = frame->data[0];
      safe_queue_pop_front(queue);

      if (safe_queue_is_empty(queue)) {
        vpc->buffering = 1;
        vpc->video_progress_time = current_time_in_sec;
        vpc->last_video_frame_time = 0.0f;
        logging("START BUFFERING!!!");
      }

      return time_in_sec;
    } else {
      *release_ptr = NULL;
      *data = NULL;
    }
  } else {
    *release_ptr = NULL;
    *data = NULL;
  }
  return 0.0f;
}

double player_grab_video_frame(MediaPlayerContext *vpc, void **release_ptr, uint8_t **data) {

  if (vpc->buffering == 1) {
    if (safe_queue_is_full(vpc->video_queue) == 1 || safe_queue_is_full(vpc->audio_queue) == 1) {
      vpc->start_time = get_time_in_seconds() - vpc->video_progress_time;
      vpc->buffering = 0;
      logging("End buffering");
    }
  }

  if (vpc->playing == 1 && vpc->buffering == 0) {
    double current_time_in_sec = get_time_in_seconds() - vpc->start_time;
    double current_frame_time = 0.0;
    current_frame_time = _internal_grab_video_frame(vpc, release_ptr, vpc->video_queue, data, current_time_in_sec,
                                                    vpc->dectx->video_avs->time_base);
    if (current_frame_time != 0.0f) {
      logging("%f VS %f", current_frame_time, vpc->last_video_frame_time);

      if (vpc->loop == 1 && current_frame_time < vpc->last_video_frame_time &&
          vpc->last_video_frame_time != 0.0f) { // It's not continuos... so we made a loop
        vpc->start_time = get_time_in_seconds();
        current_time_in_sec = 0.0f;
      }
      vpc->last_video_frame_time = current_frame_time;
    }
    return current_frame_time;
  }

  *release_ptr = NULL;
  *data = NULL;
  return -1.0;
}

double player_grab_audio_frame(MediaPlayerContext *vpc, void **release_ptr, uint8_t **data, int *frame_size) {
  if (vpc->playing == 1 && vpc->buffering == 0) {
    AVFrame *frame = safe_queue_pop_front(vpc->audio_queue);
    if (frame != NULL) {
      *release_ptr = frame;
      *data = frame->data[0];
      *frame_size = frame->nb_samples;

      double time_in_sec = (double) (av_q2d(vpc->dectx->audio_avs->time_base) * (double) frame->best_effort_timestamp);
      return time_in_sec;
    }
  }
  *release_ptr = NULL;
  *data = NULL;
  return -1.0;
}

void player_release_frame(MediaPlayerContext *vpc, void *release_ptr) {
  AVFrame *frame = release_ptr;
  av_frame_free(&frame);
}

void player_get_video_format(MediaPlayerContext *vpc, int *width, int *height) {
  *width = vpc->dectx->video_avcc->width;
  *height = vpc->dectx->video_avcc->height;
}

void player_get_audio_format(MediaPlayerContext *vpc, int *frequency, int *channels) {
  *frequency = vpc->dectx->audio_frequency;
  *channels = vpc->dectx->audio_channels;
}

double player_get_start_time(MediaPlayerContext *vpc) {
  return vpc->start_time;
}

void player_set_start_time(MediaPlayerContext *vpc, double time) {
  vpc->start_time = time;
}

double player_get_global_time(MediaPlayerContext *vpc) {
  return get_time_in_seconds();
}