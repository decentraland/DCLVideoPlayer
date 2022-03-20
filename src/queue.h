#pragma once

#include <stdlib.h>

typedef struct Item {
    void *item;
    struct Item *next_item;
} Item;

typedef struct QueueContext {
    Item *first;
    Item *last;
    int count;
} QueueContext;

QueueContext *queue_create();

int queue_is_full(QueueContext *queue);

int queue_is_empty(QueueContext *queue);

void *queue_pop_front(QueueContext *queue);

void *queue_peek_front(QueueContext *queue);

void queue_push(QueueContext *queue, void *item);

void queue_clean(QueueContext *queue);

void queue_destroy(QueueContext **queueRef);