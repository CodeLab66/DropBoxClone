#include "task.h"
#include "utils.h"
#include <stdlib.h>
#include <string.h>

void task_process_upload(task_t *task) {
    if (task == NULL || task->user == NULL) return;
    pthread_mutex_lock(&task->completion_mutex);
    if (task->user->used + task->size > task->user->quota) {
        task->result_status = strdup("ERROR Quota exceeded\n");
    } else if (write_file(task->user->username, task->filename, task->data, task->size) == 0) {
        task->result_status = strdup("OK Uploaded\n");
        user_update_used(task->user, task->size);
    } else {
        task->result_status = strdup("ERROR Write failed\n");
    }
    task->done = 1;
    pthread_cond_signal(&task->completion_cond);
    pthread_mutex_unlock(&task->completion_mutex);
}

void task_process_download(task_t *task) {
    if (task == NULL || task->user == NULL) return;
    pthread_mutex_lock(&task->completion_mutex);
    long size;
    task->result_data = read_file(task->user->username, task->filename, &size);
    if (task->result_data == NULL) {
        task->result_status = strdup("ERROR File not found\n");
    } else {
        task->result_status = strdup("OK");
        task->result_size = size;
    }
    task->done = 1;
    pthread_cond_signal(&task->completion_cond);
    pthread_mutex_unlock(&task->completion_mutex);
}

void task_process_delete(task_t *task) {
    if (task == NULL || task->user == NULL) return;
    pthread_mutex_lock(&task->completion_mutex);
    long size;
    char *data = read_file(task->user->username, task->filename, &size);
    if (delete_file(task->user->username, task->filename) == 0) {
        task->result_status = strdup("OK Deleted\n");
        if (data) user_update_used(task->user, -size);
        free(data);
    } else {
        task->result_status = strdup("ERROR Delete failed\n");
    }
    task->done = 1;
    pthread_cond_signal(&task->completion_cond);
    pthread_mutex_unlock(&task->completion_mutex);
}

void task_process_list(task_t *task) {
    if (task == NULL || task->user == NULL) return;
    pthread_mutex_lock(&task->completion_mutex);
    task->result_status = list_files(task->user->username);
    if (task->result_status == NULL) {
        task->result_status = strdup("ERROR Listing failed\n");
    }
    task->done = 1;
    pthread_cond_signal(&task->completion_cond);
    pthread_mutex_unlock(&task->completion_mutex);
}

void task_init(task_t *task) {
    if (task == NULL) return;
    pthread_mutex_init(&task->completion_mutex, NULL);
    pthread_cond_init(&task->completion_cond, NULL);
    task->done = 0;
    task->data = NULL;
    task->result_status = NULL;
    task->result_data = NULL;
    task->result_size = 0;
}

void task_destroy(task_t *task) {
    if (task == NULL) return;
    pthread_mutex_destroy(&task->completion_mutex);
    pthread_cond_destroy(&task->completion_cond);
    free(task->data);
    free(task->result_status);
    free(task->result_data);
}