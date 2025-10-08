#include "client.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "config.h"    // For PORT and BUFFER_SIZE

int client_run(void) {
    WSADATA wsaData;
    if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) {
        printf("WSAStartup failed: %d\n", WSAGetLastError());
        return 1;
    }

    SOCKET client_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (client_fd == INVALID_SOCKET) {
        printf("Socket creation failed: %d\n", WSAGetLastError());
        WSACleanup();
        return 1;
    }

    struct sockaddr_in server_addr = {0};
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = inet_addr("127.0.0.1");  // localhost
    server_addr.sin_port = htons(PORT);

    if (connect(client_fd, (struct sockaddr*)&server_addr, sizeof(server_addr)) == SOCKET_ERROR) {
        printf("Connection failed: %d\n", WSAGetLastError());
        closesocket(client_fd);
        WSACleanup();
        return 1;
    }

    printf("Connected to server at port %d. Enter commands (e.g., 'SIGNUP user pass', 'EXIT' to quit):\n", PORT);

    char buffer[BUFFER_SIZE];
    while (1) {
        printf("> ");
        fflush(stdout);
        if (fgets(buffer, BUFFER_SIZE, stdin) == NULL) break;

        buffer[strcspn(buffer, "\n")] = 0;  // Remove newline

        if (strcmp(buffer, "EXIT") == 0) break;

        if (send(client_fd, buffer, strlen(buffer), 0) == SOCKET_ERROR) {
            printf("Send failed: %d\n", WSAGetLastError());
            break;
        }

        char response[BUFFER_SIZE] = "";  // Use empty string initialization instead of {0}
        int bytes_received = recv(client_fd, response, BUFFER_SIZE - 1, 0);
        if (bytes_received <= 0) {
            printf("Receive failed or connection closed\n");
            break;
        }
        response[bytes_received] = 0;
        printf("Server: %s\n", response);
    }

    closesocket(client_fd);
    WSACleanup();
    return 0;
}

int main() {
    return client_run();
}
