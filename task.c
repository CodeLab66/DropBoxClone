#define _CRT_SECURE_NO_WARNINGS
#include "task.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <direct.h>
#include <windows.h>
#include "utils.h"
#define BUFFER_SIZE TASK_BUFFER_SIZE

/* send_all and recv_line (kept as helpers) */
int send_all(SOCKET sock, const char *data, int len) {
    int total = 0;
    int bytesleft = len;
    int n;
    while (total < len) {
        n = send(sock, data + total, bytesleft, 0);
        if (n == SOCKET_ERROR) return SOCKET_ERROR;
        total += n;
        bytesleft -= n;
    }
    return total;
}

int recv_line(SOCKET sock, char *buf, int maxlen) {
    int i = 0;
    char c;
    int n;
    while (i < maxlen - 1) {
        n = recv(sock, &c, 1, 0);
        if (n <= 0) return n;
        buf[i++] = c;
        if (c == '\n') break;
    }
    buf[i] = '\0';
    return i;
}


static void sanitize_filename_local(char *out, const char *in, size_t n) {
    const char *p1 = strrchr(in, '\\');
    const char *p2 = strrchr(in, '/');
    const char *name = in;
    if (p1 && p2) name = (p1 > p2) ? p1+1 : p2+1;
    else if (p1) name = p1+1;
    else if (p2) name = p2+1;

    size_t i = 0;
    for (; *name != '\0' && i + 1 < n; ++name) {
        if (*name == '.' && name[1] == '.') continue;
        if (*name == '\\' || *name == '/') continue;
        out[i++] = *name;
    }
    out[i] = '\0';
}

/* convenience to ensure user's directory exists */
static void ensure_user_dir_local(const char *username, char *out_dir, size_t n) {
    _mkdir(ROOT_DIR);
    snprintf(out_dir, n, "%s\\%s", ROOT_DIR, username);
    _mkdir(out_dir);
}


void task_process_upload(task_t *task) {
    if (!task || !task->user || strlen(task->filename) == 0 ||
        !task->data || task->size <= 0) {
        strncpy(task->result_status, "ERROR Invalid upload task\n",
                sizeof(task->result_status) - 1);
        return;
        }

    char userdir[512];
    snprintf(userdir, sizeof(userdir), "%s/%s", ROOT_DIR, task->user->username);
    _mkdir(ROOT_DIR);   // ensure base dir exists
    _mkdir(userdir);    // ensure user folder exists

    char filepath[512];
    snprintf(filepath, sizeof(filepath), "%s/%s/%s",
             ROOT_DIR, task->user->username, task->filename);

    FILE *fp = fopen(filepath, "wb");
    if (!fp) {
        snprintf(task->result_status, sizeof(task->result_status),
                 "ERROR Saving file\n");
        task->data = NULL;
        task->size = 0;
        return;
    }

    size_t written = fwrite(task->data, 1, task->size, fp);
    fclose(fp);

    if (written != (size_t)task->size) {
        snprintf(task->result_status, sizeof(task->result_status),
                 "ERROR File write incomplete\n");
        task->data = NULL;
        task->size = 0;
        return;
    }

    // Update user’s quota usage
    pthread_mutex_lock(&task->user->mutex);
    task->user->used += task->size;
    pthread_mutex_unlock(&task->user->mutex);

    snprintf(task->result_status, sizeof(task->result_status),
             "OK Upload successful\n");
}


void task_process_download(task_t *task) {
    if (!task || !task->user || strlen(task->filename) == 0) {
        if (task)
            strncpy(task->result_status, "ERROR Invalid download task\n", sizeof(task->result_status) - 1);
        return;
    }

    // Build full file path
    char filepath[512];
    snprintf(filepath, sizeof(filepath), "%s/%s/%s", ROOT_DIR, task->user->username, task->filename);

    FILE *fp = fopen(filepath, "rb");
    if (!fp) {
        snprintf(task->result_status, sizeof(task->result_status), "ERROR File not found: %s\n", task->filename);
        task->data = NULL;
        task->size = 0;
        return;
    }

    // Determine file size safely
    fseek(fp, 0, SEEK_END);
    long filesize = ftell(fp);
    rewind(fp);

    if (filesize <= 0) {
        fclose(fp);
        snprintf(task->result_status, sizeof(task->result_status), "ERROR Empty or invalid file\n");
        task->data = NULL;
        task->size = 0;
        return;
    }

    // Allocate buffer for the file content
    task->data = malloc(filesize);
    if (!task->data) {
        fclose(fp);
        snprintf(task->result_status, sizeof(task->result_status), "ERROR Memory allocation failed\n");
        task->size = 0;
        return;
    }

    // Read file content into allocated memory
    size_t read_bytes = fread(task->data, 1, filesize, fp);
    fclose(fp);

    if (read_bytes != (size_t)filesize) {
        free(task->data);
        task->data = NULL;
        snprintf(task->result_status, sizeof(task->result_status), "ERROR Reading file\n");
        task->size = 0;
        return;
    }

    // Everything OK
    task->size = filesize;
    snprintf(task->result_status, sizeof(task->result_status), "OK File ready\n");
}



void task_process_delete(task_t *task) {
    if (!task || !task->user || strlen(task->filename) == 0) {
        if (task)
            strncpy(task->result_status, "ERROR Invalid delete task\n",
                    sizeof(task->result_status) - 1);
        return;
    }

    size_t path_len = strlen(ROOT_DIR) + strlen(task->user->username) + strlen(task->filename) + 4;
    char *filepath = malloc(path_len);
    if (!filepath) {
        strncpy(task->result_status, "ERROR Memory allocation failed\n",
                sizeof(task->result_status) - 1);
        return;
    }

    snprintf(filepath, path_len, "%s/%s/%s", ROOT_DIR, task->user->username, task->filename);

    // Try to delete the file
    if (remove(filepath) == 0) {
        snprintf(task->result_status, sizeof(task->result_status),
                 "OK File deleted successfully\n");
    } else {
        snprintf(task->result_status, sizeof(task->result_status),
                 "ERROR Unable to delete file: %s\n", task->filename);
    }

    free(filepath);
    task->data = NULL;
    task->size = 0;
}




void task_process_list(task_t *task) {
    if (!task || !task->user) {
        if (task)
            strncpy(task->result_status, "ERROR Invalid list task\n",
                    sizeof(task->result_status) - 1);
        return;
    }

    char *list = list_files(task->user->username);
    if (!list) {
        strncpy(task->result_status, "ERROR Listing files\n",
                sizeof(task->result_status) - 1);
        return;
    }

    /* Append END marker */
    size_t len = strlen(list);
    char *final_list = malloc(len + 6);
    if (!final_list) {
        strncpy(task->result_status, "ERROR Memory allocation\n",
                sizeof(task->result_status) - 1);
        free(list);
        return;
    }

    snprintf(final_list, len + 6, "%sEND\n", list);
    free(list);

    task->data = final_list;
    task->size = (long)strlen(final_list);
    task->result_status[0] = '\0';
}



void task_init(task_t *task) {
    if (!task) return;
    memset(task, 0, sizeof(task_t));
    pthread_mutex_init(&task->completion_mutex, NULL);
    pthread_cond_init(&task->completion_cond, NULL);
    task->done = 0;
    task->data = NULL;
    task->size = 0;
    task->result_status[0] = '\0';
}

void task_destroy(task_t *task) {
    if (!task) return;
    /*if (task->data) {
        free(task->data);
        task->data = NULL;
    }*/
    pthread_mutex_destroy(&task->completion_mutex);
    pthread_cond_destroy(&task->completion_cond);
}

