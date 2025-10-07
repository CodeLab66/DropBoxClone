#ifndef SERVER_H
#define SERVER_H

#include <winsock2.h>

void server_init(void);
int server_main(void);
static void signal_handler(int sig);

#endif