#ifndef CLIENT_HANDLER_H
#define CLIENT_HANDLER_H

#include <winsock2.h>
#include <windows.h>
#include <pthread.h>
#include "queue.h"
#include "user.h"
#include "task.h"
#include "utils.h"

#define BUFFER_SIZE 8192
#define MAX_USERS   100

/* --- Initialization --- */
void client_handler_init(queue_t *client_queue, queue_t *task_queue);

/* --- Session management --- */
user_t *get_user_by_socket(SOCKET sockfd);
void set_user_session(SOCKET sockfd, user_t *user);
void clear_user_session(SOCKET sockfd);

/* --- Main client handler thread function --- */
void *client_handler_thread(void *arg);

#endif /* CLIENT_HANDLER_H */
