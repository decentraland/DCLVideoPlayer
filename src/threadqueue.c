#include "threadqueue.h"

QueueContext *queue_create() {
  QueueContext *queue = (QueueContext *) calloc(1, sizeof(QueueContext));
  queue->first = NULL;
  queue->last = NULL;
  queue->count = 0;
  return queue;
}

int queue_is_empty(QueueContext *queue) {
  if (queue->count == 0)
    return 1;
  else
    return 0;
}

void queue_clean(QueueContext *queue) {
  pthread_t item = NULL_THREAD;
  do {
    item = queue_pop_front(queue);
    if (item == NULL_THREAD)
      break;
  } while (1);
}

pthread_t queue_peek_front(QueueContext *queue) {
  if (queue->first) {
    return queue->first->thread_id;
  }
  return NULL_THREAD;
}

pthread_t queue_pop_front(QueueContext *queue) {
  if (queue->first) {
    Item *first_item = queue->first;
    pthread_t item = first_item->thread_id;
    queue->first = first_item->next_item;
    if (first_item->next_item == NULL) {
      queue->last = first_item->next_item;
    }
    --queue->count;
    free(first_item);
    return item;
  }
  return NULL_THREAD;
}

void queue_push(QueueContext *queue, pthread_t item) {
  Item *first_item = (Item *) calloc(1, sizeof(Item));
  first_item->next_item = NULL;
  first_item->thread_id = item;

  if (queue->first == NULL) {
    queue->first = first_item;
  }

  if (queue->last == NULL) {
    queue->last = first_item;
  } else {
    queue->last->next_item = first_item;
    queue->last = first_item;
  }
  ++queue->count;
}

void queue_destroy(QueueContext **queueRef) {
  QueueContext *queue = *queueRef;

  queue_clean(queue);

  free(queue);
  *queueRef = NULL;
}