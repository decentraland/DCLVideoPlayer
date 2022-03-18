#pragma once

#include <stdlib.h>
#include "libavutil/frame.h"

typedef struct FrameBuffer {
    AVFrame *frame;
    struct FrameBuffer *nextFrame;
} FrameBuffer;

typedef struct QueueContext {
    FrameBuffer *first;
    FrameBuffer *last;
    int count;
    int maxCount;
} QueueContext;

QueueContext *queue_create(int maxCount);

int queue_is_full(QueueContext *queue);

int queue_is_empty(QueueContext *queue);

AVFrame *queue_pop_front(QueueContext *queue);

AVFrame *queue_peek_front(QueueContext *queue);

void queue_push(QueueContext *queue, AVFrame *frame);

void queue_clean(QueueContext *queue);

void queue_destroy(QueueContext **queueRef);