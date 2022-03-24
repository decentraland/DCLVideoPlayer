#ifndef H_SAFEFRAMEQUEUE
#define H_SAFEFRAMEQUEUE

#include <stdlib.h>
#include "libavutil/frame.h"
#include <pthread.h>

typedef struct FrameBuffer {
    AVFrame *frame;
    uint8_t loop_id;
    struct FrameBuffer *next_frame;
} FrameBuffer;

typedef struct SafeQueueContext {
    FrameBuffer *first;
    FrameBuffer *last;
    int count;
    int maxCount;
    pthread_mutex_t lock;
} SafeQueueContext;

SafeQueueContext *safe_queue_create(int maxCount);

int safe_queue_is_full(SafeQueueContext *queue);

int safe_queue_is_empty(SafeQueueContext *queue);

AVFrame *safe_queue_pop_front(SafeQueueContext *queue);

AVFrame *safe_queue_peek_front(SafeQueueContext *queue);

uint8_t safe_queue_peek_front_loop_id(SafeQueueContext *queue);

void safe_queue_push(SafeQueueContext *queue, AVFrame *frame, uint8_t loop_id);

void safe_queue_clean(SafeQueueContext *queue);

void safe_queue_destroy(SafeQueueContext **queueRef);

#endif