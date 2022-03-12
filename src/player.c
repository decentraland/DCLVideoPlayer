#include "player.h"
#include "logger.h"
#include <libavformat/avformat.h>

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

VideoPlayerContext* playerCreate(const char* url)
{
  logging("playerCreate %s\n", url);
  VideoPlayerContext* vpc = (VideoPlayerContext*)calloc(1, sizeof(VideoPlayerContext));
  vpc->dectx = decoder_create(url);
  vpc->videoQueue = queueCreate(64);
  vpc->audioQueue = queueCreate(128);
  vpc->lockVideoFrame = NULL;
  vpc->lockAudioFrame = NULL;

  vpc->startTime = 0.0;
  vpc->pausedTime = 0.0;
  vpc->playing = 0;
  vpc->loop = 0;
  return vpc;
}

void playerDestroy(VideoPlayerContext* vpc)
{
  logging("playerDestroy\n");
  decoder_destroy(vpc->dectx);
  queueDestroy(&vpc->videoQueue);
  queueDestroy(&vpc->audioQueue);
  free(vpc);
}

void playerPlay(VideoPlayerContext* vpc)
{
  if (vpc->playing == 1) return;
  vpc->startTime = getTimeInSeconds() - (vpc->pausedTime - vpc->startTime);
  vpc->playing = 1;
  logging("playerPlay\n");
}

void playerStop(VideoPlayerContext* vpc)
{
  if (vpc->playing == 0) return;
  vpc->playing = 0;
  vpc->pausedTime = getTimeInSeconds();
  logging("playerStop\n");
}

int playerIsPlaying(VideoPlayerContext* vpc)
{
  logging("playerIsPlaying\n");
  return 0;
}

void playerSetPaused(VideoPlayerContext* vpc, int paused)
{
  if (paused == 1) {
    playerPlay(vpc);
  } else {
    playerStop(vpc);
  }
}

int playerIsPaused(VideoPlayerContext* vpc)
{
  return vpc->playing == 0;
}

void playerSetLoop(VideoPlayerContext* vpc, int loop)
{
  vpc->loop = loop;
}

int playerHasLoop(VideoPlayerContext* vpc)
{
  return vpc->loop;
}

float playerGetLength(VideoPlayerContext* vpc)
{
  return 0.0f;
}

float playerGetPlaybackPosition(VideoPlayerContext* vpc)
{
  if (vpc->playing == 1) {
    return (float) (getTimeInSeconds() - vpc->startTime);
  } else {
    return (float) (vpc->pausedTime - vpc->startTime);
  }
}

void playerSeek(VideoPlayerContext* vpc, float time)
{
  decoder_seek(vpc->dectx, time);
  queueClean(vpc->videoQueue);
  queueClean(vpc->audioQueue);
  if (vpc->playing == 1)
    vpc->startTime = getTimeInSeconds() - time;
  else
    vpc->pausedTime = vpc->startTime + time;
}

void playerProcess(VideoPlayerContext* vpc)
{
  if (queueFull(vpc->videoQueue) == 0 && queueFull(vpc->audioQueue) == 0) {
    ProcessOutput processOutput;
    int res = decoder_process_frame(vpc->dectx, &processOutput);
    if (res == 0) {
      if (processOutput.videoFrame) {
        queuePush(vpc->videoQueue, processOutput.videoFrame);
      }

      if (processOutput.audioFrame) {
        queuePush(vpc->audioQueue, processOutput.audioFrame);
      }

      //logging("videoCount=%d audioCount=%d\n", vpc->videoQueue->count, vpc->audioQueue->count);
    }
  }
}

void grabAVFrame(AVFrame** lockFrame, QueueContext* queue, uint8_t** data, float currentTimeInSec, AVRational time_base)
{
  if (*lockFrame != NULL) {
    logging("[ERROR] Release video frame before grabbing another one!\n");
    return;
  }

  AVFrame* frame = queuePeekFront(queue);
  if (frame != NULL) {
    double timeInSec = (double)(av_q2d(time_base) * (double)frame->best_effort_timestamp);
    logging("grabAVFrame %lf VS %f VS %lld\n", timeInSec, currentTimeInSec, frame->best_effort_timestamp);
    if (timeInSec <= currentTimeInSec) {
      *lockFrame = frame;
      *data = frame->data[0];
      queuePopFront(queue);
    } else {
      *data = NULL;
    }
  } else {
    *data = NULL;
  }
}

void playerGrabVideoFrame(VideoPlayerContext* vpc, uint8_t** data)
{
  if (vpc->playing == 1) {
    float currentTimeInSec = (float) (getTimeInSeconds() - vpc->startTime);
    grabAVFrame(&vpc->lockVideoFrame, vpc->videoQueue, data, currentTimeInSec, vpc->dectx->video_avs->time_base);
  }
}

void playerReleaseVideoFrame(VideoPlayerContext* vpc)
{
  if (vpc->lockVideoFrame) {
    av_frame_free(&vpc->lockVideoFrame);
    vpc->lockVideoFrame = NULL;
  }
}

void playerGrabAudioFrame(VideoPlayerContext* vpc, uint8_t** data)
{
  if (vpc->playing == 1) {
    float currentTimeInSec = (float) (getTimeInSeconds() - vpc->startTime);
    grabAVFrame(&vpc->lockAudioFrame, vpc->audioQueue, data, currentTimeInSec, vpc->dectx->audio_avs->time_base);
  }
}

void playerReleaseAudioFrame(VideoPlayerContext* vpc)
{
  if (vpc->lockAudioFrame) {
    av_frame_free(&vpc->lockAudioFrame);
    vpc->lockAudioFrame = NULL;
  }
}