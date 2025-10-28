#include "client_handler.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <winsock2.h>
#include "utils.h"
#include "task.h"
#include "user.h"

static queue_t *client_queue;
static queue_t *task_queue;

typedef struct {
    user_t *user;
    SOCKET sockfd;
} session_t;

static session_t sessions[MAX_USERS];
static pthread_mutex_t sessions_mutex = PTHREAD_MUTEX_INITIALIZER;

void client_handler_init(queue_t *cq, queue_t *tq) {
    client_queue = cq;
    task_queue = tq;
    memset(sessions, 0, sizeof(sessions));
}

user_t *get_user_by_socket(SOCKET sockfd) {
    pthread_mutex_lock(&sessions_mutex);
    for (int i = 0; i < MAX_USERS; i++) {
        if (sessions[i].sockfd == sockfd && sessions[i].user != NULL) {
            user_t *user = sessions[i].user;
            pthread_mutex_unlock(&sessions_mutex);
            return user;
        }
    }
    pthread_mutex_unlock(&sessions_mutex);
    return NULL;
}

void set_user_session(SOCKET sockfd, user_t *user) {
    pthread_mutex_lock(&sessions_mutex);
    for (int i = 0; i < MAX_USERS; i++) {
        if (sessions[i].sockfd == 0) {
            sessions[i].sockfd = sockfd;
            sessions[i].user = user;
            break;
        }
    }
    pthread_mutex_unlock(&sessions_mutex);
}

void clear_user_session(SOCKET sockfd) {
    pthread_mutex_lock(&sessions_mutex);
    for (int i = 0; i < MAX_USERS; i++) {
        if (sessions[i].sockfd == sockfd) {
            sessions[i].sockfd = 0;
            sessions[i].user = NULL;
            break;
        }
    }
    pthread_mutex_unlock(&sessions_mutex);
}

void *client_handler_thread(void *arg) {
    while (1) {
        int *sockfd_ptr = (int *)queue_dequeue(client_queue);
        if (sockfd_ptr == NULL || *sockfd_ptr < 0) {
            if (sockfd_ptr) free(sockfd_ptr);
            break;
        }

        SOCKET sockfd = *sockfd_ptr;
        free(sockfd_ptr);

        char buffer[BUFFER_SIZE];
        int bytes_received;
        user_t *current_user = NULL;

        while ((bytes_received = recv(sockfd, buffer, BUFFER_SIZE - 1, 0)) > 0) {
            buffer[bytes_received] = '\0';
            buffer[strcspn(buffer, "\n\r")] = 0;

            printf("Received command: %s\n", buffer);

            char command[32] = {0};
            char arg1[256] = {0};
            char arg2[256] = {0};

            if (sscanf(buffer, "%31s %255s %255s", command, arg1, arg2) >= 1) {
                if (strcmp(command, "SIGNUP") == 0) {
                    if (strlen(arg1) > 0 && strlen(arg2) > 0) {
                        user_t *user = user_signup(arg1, arg2);
                        if (user) {
                            send_response(sockfd, "OK Signed up successfully\n");
                            printf("User %s signed up\n", arg1);
                        } else {
                            send_response(sockfd, "ERROR User exists or limit reached\n");
                        }
                    } else {
                        send_response(sockfd, "ERROR Invalid signup format\n");
                    }
                } else if (strcmp(command, "LOGIN") == 0) {
                    if (strlen(arg1) > 0 && strlen(arg2) > 0) {
                        int success = user_login(arg1, arg2);
                        if (success) {
                            current_user = user_find(arg1);
                            set_user_session(sockfd, current_user);
                            send_response(sockfd, "OK Logged in successfully\n");
                            printf("User %s logged in\n", arg1);
                        } else {
                            send_response(sockfd, "ERROR Invalid credentials\n");
                        }
                    } else {
                        send_response(sockfd, "ERROR Invalid login format\n");
                    }
                } else if (strcmp(command, "UPLOAD") == 0) {
                        current_user = get_user_by_socket(sockfd);
                        if (current_user == NULL) {
                            send_response(sockfd, "ERROR Please login first\n");
                            continue;
                        }
                        if (strlen(arg1) == 0) {
                            send_response(sockfd, "ERROR Invalid upload format\n");
                            continue;
                        }

                        // Direct upload (no SEND_SIZE/SEND_DATA handshake)
                        send_response(sockfd, "OK Ready to receive file\n");

                        // Receive file data directly from client
                        char buffer[BUFFER_SIZE];
                        char *data = malloc(BUFFER_SIZE * 10); // temporary buffer for data
                        if (!data) {
                            send_response(sockfd, "ERROR Memory allocation failed\n");
                            continue;
                        }

                        long total_received = 0;
                        int bytes_received;

                        // Receive until client stops sending (or socket closes)
                        while ((bytes_received = recv(sockfd, buffer, sizeof(buffer), 0)) > 0) {
                            memcpy(data + total_received, buffer, bytes_received);
                            total_received += bytes_received;
                            if (bytes_received < sizeof(buffer)) break; // assume file ended
                        }

                        if (total_received == 0) {
                            send_response(sockfd, "ERROR Empty upload\n");
                            free(data);
                            continue;
                        }

                        // Create a task for worker
                        task_t *task = malloc(sizeof(task_t));
                        task_init(task);
                        task->type = TASK_UPLOAD;
                        task->user = current_user;
                        strncpy(task->filename, arg1, sizeof(task->filename) - 1);
                        task->filename[sizeof(task->filename) - 1] = '\0';
                        task->data = data;
                        task->size = total_received;

                        queue_enqueue(task_queue, task);

                        // Wait for worker to complete
                        pthread_mutex_lock(&task->completion_mutex);
                        while (!task->done) {
                            pthread_cond_wait(&task->completion_cond, &task->completion_mutex);
                        }
                        pthread_mutex_unlock(&task->completion_mutex);

                        send_response(sockfd, task->result_status);
                        printf("Upload completed for file: %s by user: %s (%ld bytes)\n",
                               arg1, current_user->username, total_received);

                        task_destroy(task);
                        free(task);
                    } else if (strcmp(command, "DOWNLOAD") == 0) {
                        current_user = get_user_by_socket(sockfd);
                        if (current_user == NULL) {
                            send_response(sockfd, "ERROR Please login first\n");
                            continue;
                        }
                        if (strlen(arg1) == 0) {
                            send_response(sockfd, "ERROR Invalid download format\n");
                            continue;
                        }

                        // Prepare download task
                        task_t *task = malloc(sizeof(task_t));
                        task_init(task);
                        task->type = TASK_DOWNLOAD;
                        task->user = current_user;
                        strncpy(task->filename, arg1, sizeof(task->filename) - 1);
                        task->filename[sizeof(task->filename) - 1] = '\0';
                        task->data = NULL;
                        task->size = 0;

                        // Enqueue task
                        queue_enqueue(task_queue, task);

                        // Wait for worker
                        pthread_mutex_lock(&task->completion_mutex);
                        while (!task->done) {
                            pthread_cond_wait(&task->completion_cond, &task->completion_mutex);
                        }
                        pthread_mutex_unlock(&task->completion_mutex);

                        // If worker failed
                        if (strncmp(task->result_status, "OK", 2) != 0 || task->data == NULL) {
                            send_response(sockfd, task->result_status);
                            task_destroy(task);
                            free(task);
                            continue;
                        }

                        // Send file size
                        char header[64];
                        snprintf(header, sizeof(header), "SIZE %ld\n", task->size);
                        send_response(sockfd, header);

                        // Wait for READY from client
                        char buffer[BUFFER_SIZE];
                        memset(buffer, 0, BUFFER_SIZE);
                        int bytes_received = recv(sockfd, buffer, BUFFER_SIZE - 1, 0);
                        if (bytes_received <= 0) {
                            printf("Client disconnected before READY\n");
                            free(task->data);
                            task_destroy(task);
                            free(task);
                            continue;
                        }

                        buffer[bytes_received] = '\0';
                        buffer[strcspn(buffer, "\r\n")] = 0;
                        if (strcmp(buffer, "READY") != 0) {
                            send_response(sockfd, "ERROR Expected READY\n");
                            free(task->data);
                            task_destroy(task);
                            free(task);
                            continue;
                        }

                        // Send file bytes safely
                        long sent_total = 0;
                        while (sent_total < task->size) {
                            int chunk = (int)((task->size - sent_total) > BUFFER_SIZE
                                                  ? BUFFER_SIZE
                                                  : (task->size - sent_total));
                            if (send_all(sockfd, task->data + sent_total, chunk) == SOCKET_ERROR) {
                                printf("Error sending file data\n");
                                break;
                            }
                            sent_total += chunk;
                        }

                        // Notify client and cleanup
                        send_response(sockfd, "DOWNLOAD OK\n");
                        printf("Download completed for file: %s by user: %s (%ld bytes)\n",
                               arg1, current_user->username, sent_total);

                        free(task->data);
                        task->data = NULL;
                        task_destroy(task);
                        free(task);
                    } else if (strcmp(command, "LIST") == 0) {
                    current_user = get_user_by_socket(sockfd);
                    if (current_user == NULL) {
                        send_response(sockfd, "ERROR Please login first\n");
                        continue;
                    }
                    task_t *task = malloc(sizeof(task_t));
                    task_init(task);
                    task->type = TASK_LIST;
                    task->user = current_user;
                    queue_enqueue(task_queue, task);

                    /* wait for worker */
                    pthread_mutex_lock(&task->completion_mutex);
                    while (!task->done) {
                        pthread_cond_wait(&task->completion_cond, &task->completion_mutex);
                    }
                    pthread_mutex_unlock(&task->completion_mutex);

                    if (task->data && task->size > 0) {
                        send_all(sockfd, task->data, (int)task->size);
                        free(task->data);
                    } else {
                        send_response(sockfd, task->result_status);
                    }

                    task_destroy(task);
                    free(task);
                    } else if (strcmp(command, "DELETE") == 0) {
                        current_user = get_user_by_socket(sockfd);
                        if (current_user == NULL) {
                            send_response(sockfd, "ERROR Please login first\n");
                            continue;
                        }
                        if (strlen(arg1) == 0) {
                            send_response(sockfd, "ERROR Invalid delete format\n");
                            continue;
                        }

                        // Prepare delete task
                        task_t *task = malloc(sizeof(task_t));
                        task_init(task);
                        task->type = TASK_DELETE;
                        task->user = current_user;
                        strncpy(task->filename, arg1, sizeof(task->filename) - 1);
                        task->filename[sizeof(task->filename) - 1] = '\0';
                        task->data = NULL;
                        task->size = 0;

                        queue_enqueue(task_queue, task);

                        // Wait for completion
                        pthread_mutex_lock(&task->completion_mutex);
                        while (!task->done) {
                            pthread_cond_wait(&task->completion_cond, &task->completion_mutex);
                        }
                        pthread_mutex_unlock(&task->completion_mutex);

                        send_response(sockfd, task->result_status);
                        printf("Deleted file: %s by user: %s\n", arg1, current_user->username);

                        task_destroy(task);
                        free(task);
                    } else if (strcmp(command, "EXIT") == 0) {
                        send_response(sockfd, "OK Goodbye\n");
                        break;
                    } else {
                        send_response(sockfd, "ERROR Unknown command\n");
                    }
            } else {
                send_response(sockfd, "ERROR Invalid command format\n");
            }
        }

        clear_user_session(sockfd);
        closesocket(sockfd);
        printf("Client disconnected\n");
    }
    return NULL;
}
