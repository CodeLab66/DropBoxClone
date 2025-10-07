#include "client_handler.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>  // For close (though winsock2.h overrides for Windows)
#include <winsock2.h> // For Windows socket handling
#include "utils.h"    // For send_response
#include "task.h"     // For task_init, task_destroy

static queue_t *client_queue;
static queue_t *task_queue;

void client_handler_init(queue_t *cq, queue_t *tq) {
    client_queue = cq;
    task_queue = tq;
}

void *client_handler_thread(void *arg) {
    while (1) {
        int sockfd = *(int *)queue_dequeue(client_queue);
        if (sockfd < 0) break;  // Signal to exit

        char buffer[BUFFER_SIZE];
        int bytes_received = recv(sockfd, buffer, BUFFER_SIZE - 1, 0);
        if (bytes_received <= 0) {
            closesocket(sockfd);  // Windows: closesocket instead of close
            continue;
        }
        buffer[bytes_received] = '\0';

        // Simple command parsing (e.g., "SIGNUP username password")
        char command[32], arg1[256], arg2[256];
        if (sscanf(buffer, "SIGNUP %31s %31s", arg1, arg2) == 2) {
            user_t *user = user_signup(arg1, arg2);
            const char *response = (user) ? "OK Signed up\n" : "ERROR User exists or limit reached\n";
            send_response(sockfd, response);
        } else if (sscanf(buffer, "LOGIN %31s %31s", arg1, arg2) == 2) {
            int success = user_login(arg1, arg2);
            const char *response = (success) ? "OK Logged in\n" : "ERROR Invalid credentials\n";
            send_response(sockfd, response);
        } else if (sscanf(buffer, "UPLOAD %255s", arg1) == 1) {
            // Assume data follows (simplified, real impl needs multi-packet)
            char *data = strdup(buffer + strlen(arg1) + 7);  // Rough offset
            task_t *task = malloc(sizeof(task_t));
            task_init(task);  // Initialize task
            task->type = TASK_UPLOAD;
            task->user = user_find(arg1);  // Placeholder: needs login tracking
            strncpy(task->filename, arg1, sizeof(task->filename) - 1);
            task->data = data;
            task->size = strlen(data);
            queue_enqueue(task_queue, task);
            pthread_mutex_lock(&task->completion_mutex);
            while (!task->done) pthread_cond_wait(&task->completion_cond, &task->completion_mutex);
            send_response(sockfd, task->result_status);
            pthread_mutex_unlock(&task->completion_mutex);
            task_destroy(task);
            free(task);
        } else {
            send_response(sockfd, "ERROR Unknown command\n");
        }

        closesocket(sockfd);  // Windows: closesocket
    }
    return NULL;
}