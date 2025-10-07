#include "worker.h"
#include "task.h"
#include "utils.h"
#include <stdio.h>
#include <stdlib.h>

static queue_t *task_queue;

void worker_init(queue_t *tq) {
    task_queue = tq;
}

void *worker_thread(void *arg) {
    while (1) {
        task_t *task = (task_t *)queue_dequeue(task_queue);
        if (task == NULL) break;

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
        }
    }
    return NULL;
}