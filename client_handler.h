#ifndef CLIENT_HANDLER_H
#define CLIENT_HANDLER_H

#include <pthread.h>
#include "queue.h"
#include "task.h"
#include "user.h"

void client_handler_init(queue_t *client_queue, queue_t *task_queue);
void *client_handler_thread(void *arg);

#endif