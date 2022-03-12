#pragma once
#include <stdlib.h>
#include "libavutil/frame.h"

typedef struct FrameBuffer {
    AVFrame* frame;
    struct FrameBuffer* nextFrame;
} FrameBuffer;

typedef struct QueueContext {
    FrameBuffer* first;
    FrameBuffer* last;
    int count;
    int maxCount;
} QueueContext;

QueueContext* queueCreate(int maxCount);

int queueFull(QueueContext* queue);

AVFrame* queuePopFront(QueueContext* queue);

AVFrame* queuePeekFront(QueueContext* queue);

void queuePush(QueueContext* queue, AVFrame* frame);

void queueClean(QueueContext* queue);

void queueDestroy(QueueContext** queueRef);