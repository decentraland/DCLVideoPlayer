#include "framequeue.h"

QueueContext* queueCreate(int maxCount)
{
  QueueContext* queue = (QueueContext*)calloc(1, sizeof(QueueContext));
  queue->first = NULL;
  queue->last = NULL;
  queue->count = 0;
  queue->maxCount = maxCount;
  return queue;
}

int queueFull(QueueContext* queue)
{
  if (queue->count >= queue->maxCount)
    return 1;
  else
    return 0;
}

void queueClean(QueueContext* queue)
{
  AVFrame* frame = NULL;
  do {
    frame = queuePopFront(queue);
    if (frame != NULL)
      av_frame_free(&frame);
    else
      break;
  } while (1);
}

AVFrame* queuePeekFront(QueueContext* queue)
{
  if (queue->first)
  {
    AVFrame* frame = queue->first->frame;
    return frame;
  }
  return NULL;
}

AVFrame* queuePopFront(QueueContext* queue)
{
  if (queue->first)
  {
    FrameBuffer* firstFrameBuffer = queue->first;
    AVFrame* frame = firstFrameBuffer->frame;
    queue->first = firstFrameBuffer->nextFrame;
    if (firstFrameBuffer->nextFrame == NULL)
    {
      queue->last = firstFrameBuffer->nextFrame;
    }
    --queue->count;
    free(firstFrameBuffer);
    return frame;
  }
  return NULL;
}

void queuePush(QueueContext* queue, AVFrame* frame)
{
  FrameBuffer* frameBuffer = (FrameBuffer*)calloc(1, sizeof(FrameBuffer));
  frameBuffer->nextFrame = NULL;
  frameBuffer->frame = frame;

  if (queue->first == NULL) {
    queue->first = frameBuffer;
  }

  if (queue->last == NULL) {
    queue->last = frameBuffer;
  } else {
    queue->last->nextFrame = frameBuffer;
    queue->last = frameBuffer;
  }
  ++queue->count;
}

void queueDestroy(QueueContext** queueRef)
{
  QueueContext* queue = *queueRef;

  queueClean(queue);

  free(queue);
  *queueRef = NULL;
}