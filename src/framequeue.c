#include "framequeue.h"

QueueContext* queue_create(int maxCount)
{
  QueueContext* queue = (QueueContext*)calloc(1, sizeof(QueueContext));
  queue->first = NULL;
  queue->last = NULL;
  queue->count = 0;
  queue->maxCount = maxCount;
  return queue;
}

int queue_is_full(QueueContext* queue)
{
  if (queue->count >= queue->maxCount)
    return 1;
  else
    return 0;
}

int queue_is_empty(QueueContext* queue)
{
  if (queue->count == 0)
    return 1;
  else
    return 0;
}

void queue_clean(QueueContext* queue)
{
  AVFrame* frame = NULL;
  do {
    frame = queue_pop_front(queue);
    if (frame != NULL)
      av_frame_free(&frame);
    else
      break;
  } while (1);
}

AVFrame* queue_peek_front(QueueContext* queue)
{
  if (queue->first)
  {
    AVFrame* frame = queue->first->frame;
    return frame;
  }
  return NULL;
}

AVFrame* queue_pop_front(QueueContext* queue)
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

void queue_push(QueueContext* queue, AVFrame* frame)
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

void queue_destroy(QueueContext** queueRef)
{
  QueueContext* queue = *queueRef;

  queue_clean(queue);

  free(queue);
  *queueRef = NULL;
}