#include "worker.h"
#include <stdio.h>
#include <stdlib.h>
#include "task.h"

void worker_init(worker_pool_t *pool, queue_t *task_queue, int num_threads) {
    pool->task_queue = task_queue;
    pool->num_threads = num_threads;
    pool->threads = malloc(num_threads * sizeof(pthread_t));

    for (int i = 0; i < num_threads; i++) {
        pthread_create(&pool->threads[i], NULL, worker_thread, pool);
    }
}

void *worker_thread(void *arg) {
    worker_pool_t *pool = (worker_pool_t *)arg;

    while (1) {
        task_t *task = (task_t *)queue_dequeue(pool->task_queue);
        if (task == NULL) break;  // Exit signal

        printf("Worker processing task type: %d\n", task->type);

        switch (task->type) {
        case TASK_UPLOAD:
            task_process_upload(task);
            break;
        case TASK_DOWNLOAD:
            task_process_download(task);
            break;
        case TASK_DELETE:
            task_process_delete(task);
            break;
        case TASK_LIST:
            task_process_list(task);
            break;
        default:
            printf("Unknown task type: %d\n", task->type);
            break;
        }

        // Signal task completion
        pthread_mutex_lock(&task->completion_mutex);
        task->done = 1;
        pthread_cond_signal(&task->completion_cond);
        pthread_mutex_unlock(&task->completion_mutex);
    }

    return NULL;
}