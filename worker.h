#ifndef WORKER_H
#define WORKER_H

#include "queue.h"
#include "task.h"

typedef struct {
    queue_t *task_queue;
    pthread_t *threads;
    int num_threads;
} worker_pool_t;

void worker_init(worker_pool_t *pool, queue_t *task_queue, int num_threads);
void *worker_thread(void *arg);

#endif