#ifndef TASK_H
#define TASK_H

#include <winsock2.h>
#include <windows.h>
#include <pthread.h>
#include "user.h"      // ✅ Include the real definition, no typedef duplication

#define ROOT_DIR "server_storage"
#define TASK_BUFFER_SIZE 8192

#define TASK_UPLOAD   1
#define TASK_DOWNLOAD 2
#define TASK_DELETE   3
#define TASK_LIST     4

typedef struct task {
    int type;
    user_t *user;
    char filename[256];
    char *data;
    long size;
    char result_status[1024];
    int done;

    pthread_mutex_t completion_mutex;
    pthread_cond_t completion_cond;
} task_t;

void task_init(task_t *task);
void task_destroy(task_t *task);
void task_process_upload(task_t *task);
void task_process_download(task_t *task);
void task_process_delete(task_t *task);
void task_process_list(task_t *task);

int send_all(SOCKET sock, const char *data, int len);
int recv_line(SOCKET sock, char *buf, int maxlen);

#endif /* TASK_H */
