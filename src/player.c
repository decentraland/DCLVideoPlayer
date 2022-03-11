#include "player.h"
#include "logger.h"

VideoPlayerContext* playerCreate(const char* url)
{
  logging("playerCreate %s\n", url);
  VideoPlayerContext* vpc = (VideoPlayerContext*)calloc(1, sizeof(VideoPlayerContext));
  vpc->dectx = create(url);
  return vpc;
}

void playerDestroy(VideoPlayerContext* vpc)
{
  logging("playerDestroy\n");
  free(vpc);
}

void playerPlay(VideoPlayerContext* vpc)
{
  logging("playerPlay\n");
}

void playerStop(VideoPlayerContext* vpc)
{
  logging("playerStop\n");
}

int playerIsPlaying(VideoPlayerContext* vpc)
{
  logging("playerIsPlaying\n");
  return 0;
}

void playerSetPaused(VideoPlayerContext* vpc, int paused)
{
  logging("playerSetPaused\n");
}

int playerIsPaused(VideoPlayerContext* vpc)
{
  return 0;
}

void playerSetLoop(VideoPlayerContext* vpc, int loop)
{

}

int playerHasLoop(VideoPlayerContext* vpc)
{
  return 0;
}

float playerGetLength(VideoPlayerContext* vpc)
{
  return 0.0f;
}

float playerGetPlaybackPosition(VideoPlayerContext* vpc)
{
  return 0.0f;
}

void playerSeek(VideoPlayerContext* vpc, float time)
{

}

void playerProcess(VideoPlayerContext* vpc)
{

}