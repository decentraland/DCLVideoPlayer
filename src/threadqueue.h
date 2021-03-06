#ifndef H_THREADQUEUE
#define H_THREADQUEUE

#include <stdlib.h>
#include <pthread.h>

#define NULL_THREAD ((pthread_t) 0)

typedef struct Item {
    pthread_t thread_id;
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

pthread_t queue_pop_front(QueueContext *queue);

pthread_t queue_peek_front(QueueContext *queue);

void queue_push(QueueContext *queue, pthread_t thread_id);

void queue_clean(QueueContext *queue);

void queue_destroy(QueueContext **queueRef);

#endif