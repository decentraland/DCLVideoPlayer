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

uint8_t safe_queue_peek_front_loop_id(SafeQueueContext *queue) {
  uint8_t loop_id = 0;
  pthread_mutex_lock(&queue->lock);
  if (queue->first) {
    loop_id = queue->first->loop_id;
  }
  pthread_mutex_unlock(&queue->lock);
  return loop_id;
}

AVFrame *safe_queue_pop_front(SafeQueueContext *queue) {
  AVFrame *frame = NULL;
  pthread_mutex_lock(&queue->lock);
  if (queue->first) {
    FrameBuffer *firstFrameBuffer = queue->first;
    frame = firstFrameBuffer->frame;
    queue->first = firstFrameBuffer->next_frame;
    if (firstFrameBuffer->next_frame == NULL) {
      queue->last = firstFrameBuffer->next_frame;
    }
    --queue->count;
    free(firstFrameBuffer);
  }
  pthread_mutex_unlock(&queue->lock);
  return frame;
}

void safe_queue_push(SafeQueueContext *queue, AVFrame *frame, uint8_t loop_id) {
  pthread_mutex_lock(&queue->lock);
  FrameBuffer *frame_buffer = (FrameBuffer *) calloc(1, sizeof(FrameBuffer));
  frame_buffer->next_frame = NULL;
  frame_buffer->frame = frame;
  frame_buffer->loop_id = loop_id;

  if (queue->first == NULL) {
    queue->first = frame_buffer;
  }

  if (queue->last == NULL) {
    queue->last = frame_buffer;
  } else {
    queue->last->next_frame = frame_buffer;
    queue->last = frame_buffer;
  }
  ++queue->count;
  pthread_mutex_unlock(&queue->lock);
}

void safe_queue_destroy(SafeQueueContext **queueRef) {
  SafeQueueContext *queue = *queueRef;
  safe_queue_clean(queue);
  pthread_mutex_destroy(&queue->lock);
  free(queue);
  *queueRef = NULL;
}