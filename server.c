#include "server.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <winsock2.h>
#include <signal.h>
#include "queue.h"
#include "client_handler.h"
#include "worker.h"
#include "config.h"

volatile int running = 1;

void server_init(void) {
    WSADATA wsaData;
    if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) {
        printf("WSAStartup failed: %d\n", WSAGetLastError());
        exit(1);
    }
}

static void signal_handler(int sig) {
    printf("Shutdown signal received (%d), exiting...\n", sig);
    running = 0;
}

int server_main(void) {
    server_init();

    queue_t client_queue, task_queue;
    queue_init(&client_queue);
    queue_init(&task_queue);

    pthread_t client_threads[NUM_CLIENT_THREADS];

    // Initialize client handler
    client_handler_init(&client_queue, &task_queue);

    // Create client threads
    for (int i = 0; i < NUM_CLIENT_THREADS; i++) {
        if (pthread_create(&client_threads[i], NULL, client_handler_thread, NULL) != 0) {
            printf("Failed to create client thread %d\n", i);
            exit(1);
        }
    }

    // Initialize worker pool
    worker_pool_t worker_pool;
    worker_init(&worker_pool, &task_queue, NUM_WORKER_THREADS);

    SOCKET server_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (server_fd == INVALID_SOCKET) {
        printf("Socket creation failed: %d\n", WSAGetLastError());
        WSACleanup();
        return 1;
    }

    int opt = 1;
    if (setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, (char*)&opt, sizeof(opt)) == SOCKET_ERROR) {
        printf("Setsockopt failed: %d\n", WSAGetLastError());
    }

    struct sockaddr_in addr = {0};
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = INADDR_ANY;
    addr.sin_port = htons(PORT);

    if (bind(server_fd, (struct sockaddr*)&addr, sizeof(addr)) == SOCKET_ERROR) {
        printf("Bind failed: %d\n", WSAGetLastError());
        closesocket(server_fd);
        WSACleanup();
        return 1;
    }

    if (listen(server_fd, SOMAXCONN) == SOCKET_ERROR) {
        printf("Listen failed: %d\n", WSAGetLastError());
        closesocket(server_fd);
        WSACleanup();
        return 1;
    }

    printf("Server listening on port %d...\n", PORT);

    signal(SIGINT, signal_handler);

    while (running) {
        struct sockaddr_in client_addr;
        int addr_len = sizeof(client_addr);
        SOCKET client_fd = accept(server_fd, (struct sockaddr*)&client_addr, &addr_len);
        if (client_fd == INVALID_SOCKET) {
            if (running) {
                printf("Accept failed: %d\n", WSAGetLastError());
            }
            continue;
        }

        printf("New client connected\n");
        int *sockfd_ptr = malloc(sizeof(int));
        *sockfd_ptr = client_fd;
        queue_enqueue(&client_queue, sockfd_ptr);
    }

    printf("Shutting down server...\n");

    // Signal client threads to exit
    for (int i = 0; i < NUM_CLIENT_THREADS; i++) {
        int *exit_signal = malloc(sizeof(int));
        *exit_signal = -1;
        queue_enqueue(&client_queue, exit_signal);
    }

    // Signal worker threads to exit
    for (int i = 0; i < NUM_WORKER_THREADS; i++) {
        queue_enqueue(&task_queue, NULL);
    }

    // Wait for client threads to finish
    for (int i = 0; i < NUM_CLIENT_THREADS; i++) {
        pthread_join(client_threads[i], NULL);
    }

    // Wait for worker threads to finish
    for (int i = 0; i < NUM_WORKER_THREADS; i++) {
        pthread_join(worker_pool.threads[i], NULL);
    }

    // Cleanup
    free(worker_pool.threads);
    queue_destroy(&client_queue);
    queue_destroy(&task_queue);
    closesocket(server_fd);
    WSACleanup();

    printf("Server shutdown complete\n");
    return 0;
}

int main() {
    return server_main();
}