#pragma once

#include <stdlib.h>
#include "libavutil/frame.h"
#include <pthread.h>

typedef struct FrameBuffer {
    AVFrame *frame;
    struct FrameBuffer *nextFrame;
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

void safe_queue_push(SafeQueueContext *queue, AVFrame *frame);

void safe_queue_clean(SafeQueueContext *queue);

void safe_queue_destroy(SafeQueueContext **queueRef);