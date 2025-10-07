#ifndef WORKER_H
#define WORKER_H

#include <pthread.h>
#include "queue.h"  // For queue_t
#include "task.h"   // For task_t

void worker_init(queue_t *task_queue);
void *worker_thread(void *arg);

#endif