#ifndef QUEUE_H
#define QUEUE_H

#include <pthread.h>
#include "config.h"

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
int queue_enqueue(queue_t *q, void *item);
void *queue_dequeue(queue_t *q);

#endif