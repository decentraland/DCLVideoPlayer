#pragma once

#include "decoder.h"
#include "framequeue.h"

typedef struct VideoPlayerContext {
    DecoderContext* dectx;
    QueueContext* videoQueue;
    QueueContext* audioQueue;

    AVFrame* lockVideoFrame;
    AVFrame* lockAudioFrame;

    double startTime;
    double pausedTime;
    int playing;
    int loop;
} VideoPlayerContext;

VideoPlayerContext* playerCreate(const char* url);
void playerDestroy(VideoPlayerContext* vpc);

void playerPlay(VideoPlayerContext* vpc);
void playerStop(VideoPlayerContext* vpc);

int playerIsPlaying(VideoPlayerContext* vpc);

void playerSetPaused(VideoPlayerContext* vpc, int paused);
int playerIsPaused(VideoPlayerContext* vpc);

void playerSetLoop(VideoPlayerContext* vpc, int loop);
int playerHasLoop(VideoPlayerContext* vpc);

float playerGetLength(VideoPlayerContext* vpc);
float playerGetPlaybackPosition(VideoPlayerContext* vpc);

void playerSeek(VideoPlayerContext* vpc, float time);

void playerProcess(VideoPlayerContext* vpc);

void playerGrabVideoFrame(VideoPlayerContext* vpc, uint8_t** data);
void playerReleaseVideoFrame(VideoPlayerContext* vpc);

void playerGrabAudioFrame(VideoPlayerContext* vpc, uint8_t** data);
void playerReleaseAudioFrame(VideoPlayerContext* vpc);