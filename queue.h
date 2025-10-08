#ifndef QUEUE_H
#define QUEUE_H

#include <pthread.h>
#include "config.h"  // For MAX_QUEUE_SIZE

typedef struct {
    void *items[MAX_QUEUE_SIZE];
    int head;
    int tail;
    int count;
    pthread_mutex_t mutex;
    pthread_cond_t not_empty;
    pthread_cond_t not_full;
} queue_t;

void queue_init(queue_t *q);
void queue_destroy(queue_t *q);
int queue_enqueue(queue_t *q, void *item);  // 0 success, -1 error
void *queue_dequeue(queue_t *q);

#endif
