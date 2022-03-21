#include "safeframequeue.h"
#include "logger.h"

SafeQueueContext *safe_queue_create(int maxCount) {
  SafeQueueContext *queue = (SafeQueueContext *) calloc(1, sizeof(SafeQueueContext));
  queue->first = NULL;
  queue->last = NULL;
  queue->count = 0;
  queue->maxCount = maxCount;

  if (pthread_mutex_init(&queue->lock, NULL) != 0) {
    logging("ERROR mutex init failed\n");
    return NULL;
  }

  return queue;
}

int safe_queue_is_full(SafeQueueContext *queue) {
  pthread_mutex_lock(&queue->lock);
  int res = queue->count >= queue->maxCount;
  pthread_mutex_unlock(&queue->lock);
  if (res)
    return 1;
  else
    return 0;
}

int safe_queue_is_empty(SafeQueueContext *queue) {
  pthread_mutex_lock(&queue->lock);
  int count = queue->count;
  pthread_mutex_unlock(&queue->lock);
  if (count == 0)
    return 1;
  else
    return 0;
}

void safe_queue_clean(SafeQueueContext *queue) {
  AVFrame *frame = NULL;
  do {
    frame = safe_queue_pop_front(queue);
    if (frame != NULL)
      av_frame_free(&frame);
    else
      break;
  } while (1);
}

AVFrame *safe_queue_peek_front(SafeQueueContext *queue) {
  AVFrame *frame = NULL;
  pthread_mutex_lock(&queue->lock);
  if (queue->first) {
    frame = queue->first->frame;
  }
  pthread_mutex_unlock(&queue->lock);
  return frame;
}

AVFrame *safe_queue_pop_front(SafeQueueContext *queue) {
  AVFrame *frame = NULL;
  pthread_mutex_lock(&queue->lock);
  if (queue->first) {
    FrameBuffer *firstFrameBuffer = queue->first;
    frame = firstFrameBuffer->frame;
    queue->first = firstFrameBuffer->nextFrame;
    if (firstFrameBuffer->nextFrame == NULL) {
      queue->last = firstFrameBuffer->nextFrame;
    }
    --queue->count;
    free(firstFrameBuffer);
  }
  pthread_mutex_unlock(&queue->lock);
  return frame;
}

void safe_queue_push(SafeQueueContext *queue, AVFrame *frame) {
  pthread_mutex_lock(&queue->lock);
  FrameBuffer *frameBuffer = (FrameBuffer *) calloc(1, sizeof(FrameBuffer));
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
  pthread_mutex_unlock(&queue->lock);
}

void safe_queue_destroy(SafeQueueContext **queueRef) {
  SafeQueueContext *queue = *queueRef;
  safe_queue_clean(queue);

  free(queue);
  *queueRef = NULL;
  pthread_mutex_destroy(&queue->lock);
}