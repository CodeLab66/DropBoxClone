#ifndef TASK_H
#define TASK_H

#include <pthread.h>
#include "user.h"

typedef enum {
    TASK_UPLOAD,
    TASK_DOWNLOAD,
    TASK_DELETE,
    TASK_LIST
} task_type_t;

typedef struct task {
    task_type_t type;
    user_t *user;
    char filename[256];
    char *data;
    long size;
    char *result_status;
    char *result_data;
    long result_size;
    pthread_mutex_t completion_mutex;
    pthread_cond_t completion_cond;
    int done;
    struct task *next;
} task_t;

void task_init(task_t *task);
void task_destroy(task_t *task);
void task_process_upload(task_t *task);
void task_process_download(task_t *task);
void task_process_delete(task_t *task);
void task_process_list(task_t *task);

#endif