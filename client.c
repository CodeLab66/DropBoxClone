#include "client.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "config.h"
#include <winsock2.h>

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
    server_addr.sin_addr.s_addr = inet_addr("127.0.0.1");
    server_addr.sin_port = htons(PORT);

    if (connect(client_fd, (struct sockaddr*)&server_addr, sizeof(server_addr)) == SOCKET_ERROR) {
        printf("Connection failed: %d\n", WSAGetLastError());
        closesocket(client_fd);
        WSACleanup();
        return 1;
    }

    printf("Connected to server at port %d\n", PORT);
    printf("Available commands:\n");
    printf("  SIGNUP <username> <password>\n");
    printf("  LOGIN <username> <password>\n");
    printf("  LIST\n");
    printf("  UPLOAD <local_filename>\n");
    printf("  EXIT\n\n");

    char buffer[BUFFER_SIZE];
    while (1) {
        printf("> ");
        fflush(stdout);

        if (fgets(buffer, BUFFER_SIZE, stdin) == NULL) break;

        buffer[strcspn(buffer, "\n")] = 0;

        if (strcmp(buffer, "EXIT") == 0) {
            send(client_fd, buffer, strlen(buffer), 0);
            break;
        }

        if (strlen(buffer) == 0) {
            continue;
        }

        if (send(client_fd, buffer, strlen(buffer), 0) == SOCKET_ERROR) {
            printf("Send failed: %d\n", WSAGetLastError());
            break;
        }

        if (strncmp(buffer, "UPLOAD ", 7) == 0) {
            // Special handling for UPLOAD
            char response[BUFFER_SIZE] = {0};
            int bytes_received = recv(client_fd, response, BUFFER_SIZE - 1, 0);
            if (bytes_received <= 0) {
                printf("Receive failed or connection closed\n");
                break;
            }
            response[bytes_received] = 0;
            printf("Server: %s", response);
            if (strcmp(response, "SEND_SIZE\n") != 0) {
                continue;
            }
            // Extract filename
            char filename[256] = {0};
            sscanf(buffer + 7, "%255s", filename);
            FILE *file = fopen(filename, "rb");
            if (!file) {
                printf("Local file open failed: %s\n", filename);
                continue;
            }
            fseek(file, 0, SEEK_END);
            long size = ftell(file);
            rewind(file);
            char size_buf[32];
            snprintf(size_buf, sizeof(size_buf), "%ld\n", size);
            if (send(client_fd, size_buf, strlen(size_buf), 0) == SOCKET_ERROR) {
                fclose(file);
                printf("Send size failed\n");
                break;
            }
            memset(response, 0, BUFFER_SIZE);
            bytes_received = recv(client_fd, response, BUFFER_SIZE - 1, 0);
            if (bytes_received <= 0) {
                fclose(file);
                printf("Receive failed\n");
                break;
            }
            response[bytes_received] = 0;
            printf("Server: %s", response);
            if (strcmp(response, "SEND_DATA\n") != 0) {
                fclose(file);
                continue;
            }
            char *data = malloc(size);
            if (!data) {
                fclose(file);
                printf("Memory allocation failed\n");
                continue;
            }
            fread(data, 1, size, file);
            fclose(file);
            long sent = 0;
            while (sent < size) {
                int bytes_sent = send(client_fd, data + sent, size - sent, 0);
                if (bytes_sent <= 0) break;
                sent += bytes_sent;
            }
            free(data);
            if (sent != size) {
                printf("File send incomplete\n");
                continue;
            }
            // Receive final response
            memset(response, 0, BUFFER_SIZE);
            bytes_received = recv(client_fd, response, BUFFER_SIZE - 1, 0);
            if (bytes_received <= 0) {
                printf("Receive failed\n");
                break;
            }
            response[bytes_received] = 0;
            printf("Server: %s", response);
        } else {
            // Normal response handling for other commands
            char response[BUFFER_SIZE] = {0};
            int bytes_received = recv(client_fd, response, BUFFER_SIZE - 1, 0);
            if (bytes_received <= 0) {
                printf("Receive failed or connection closed\n");
                break;
            }
            response[bytes_received] = 0;
            printf("Server: %s", response);
        }
    }

    closesocket(client_fd);
    WSACleanup();
    printf("Client shutdown complete\n");
    return 0;
}

int main() {
    return client_run();
}