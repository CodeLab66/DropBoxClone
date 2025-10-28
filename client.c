#define _CRT_SECURE_NO_WARNINGS
#include "client.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define BUFFER_SIZE CLIENT_BUFFER_SIZE

int init_winsock() {
    WSADATA w;
    return (WSAStartup(MAKEWORD(2,2), &w) == 0);
}
void cleanup_winsock() { WSACleanup(); }

SOCKET create_connection() {
    SOCKET s = socket(AF_INET, SOCK_STREAM, 0);
    if (s == INVALID_SOCKET) return INVALID_SOCKET;

    struct sockaddr_in srv;
    srv.sin_family = AF_INET;
    srv.sin_port = htons(SERVER_PORT);
    srv.sin_addr.s_addr = inet_addr(SERVER_IP);

    if (connect(s, (struct sockaddr*)&srv, sizeof(srv)) == SOCKET_ERROR) {
        closesocket(s);
        return INVALID_SOCKET;
    }
    return s;
}

/* send_all: ensure full send */
int send_all(SOCKET sock, const char *data, int len) {
    int total = 0, bytesleft = len, n;
    while (total < len) {
        n = send(sock, data + total, bytesleft, 0);
        if (n == SOCKET_ERROR) return SOCKET_ERROR;
        total += n;
        bytesleft -= n;
    }
    return total;
}

/* recv_line: read until newline */
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

/* Upload a file to server */
void upload_file(SOCKET sock, const char *localpath, const char *remote_filename) {
    FILE *fp = fopen(localpath, "rb");
    if (!fp) {
        printf("Local file not found: %s\n", localpath);
        return;
    }

    char cmd[512];
    snprintf(cmd, sizeof(cmd), "UPLOAD %s\n", remote_filename);
    if (send_all(sock, cmd, (int)strlen(cmd)) == SOCKET_ERROR) {
        fclose(fp);
        printf("Send failed\n");
        return;
    }

    char buffer[BUFFER_SIZE];
    long sent = 0;
    while (!feof(fp)) {
        size_t n = fread(buffer, 1, sizeof(buffer), fp);
        if (n > 0) {
            if (send_all(sock, buffer, (int)n) == SOCKET_ERROR) {
                printf("Send data failed\n");
                break;
            }
            sent += n;
        } else break;
    }
    fclose(fp);

    // Read server confirmation
    char line[128];
    if (recv_line(sock, line, sizeof(line)) > 0) {
        printf("%s", line);
    }
    printf("Uploaded: %ld bytes\n", sent);
}


/* Main interactive client loop */
void client_loop(SOCKET sock) {
    printf("Connected to server %s:%d\n", SERVER_IP, SERVER_PORT);

    while (1) {
        printf("client>");
        fflush(stdout);

        char input[BUFFER_SIZE];
        if (!fgets(input, sizeof(input), stdin))
            break;

        input[strcspn(input, "\n")] = 0; // remove newline
        if (strlen(input) == 0)
            continue;

        // ✅ only send once (no extra newline)
        send_all(sock, input, (int)strlen(input));

        // Receive first response
        char resp[BUFFER_SIZE];
        int r = recv_line(sock, resp, sizeof(resp));
        if (r <= 0) {
            printf("Server disconnected or error\n");
            break;
        }

        /* ---------- LIST Handling ---------- */
        if (strncmp(input, "LIST", 4) == 0) {
            printf("%s", resp);
            while (1) {
                r = recv_line(sock, resp, sizeof(resp));
                if (r <= 0)
                    break;
                if (strcmp(resp, "END\n") == 0)
                    break;
                printf("%s", resp);
            }
            continue;
        }

        /* ---------- DOWNLOAD Handling ---------- */
        if (strncmp(input, "DOWNLOAD", 8) == 0) {
            if (strncmp(resp, "SIZE", 4) == 0) {
                long size = atol(resp + 5);
                if (size <= 0) {
                    printf("Invalid file size from server\n");
                    continue;
                }

                char filename[256];
                sscanf(input, "%*s %255s", filename);
                if (strlen(filename) == 0)
                    strcpy(filename, "downloaded_file.txt");

                FILE *fp = fopen(filename, "wb");
                if (!fp) {
                    printf("Error creating local file %s\n", filename);
                    continue;
                }

                send_all(sock, "READY\n", 6);

                long received = 0;
                char buf[8192];
                while (received < size) {
                    int bytes = recv(sock, buf, sizeof(buf), 0);
                    if (bytes <= 0)
                        break;
                    fwrite(buf, 1, bytes, fp);
                    received += bytes;
                }
                fclose(fp);

                printf("Downloaded %ld bytes to %s\n", received, filename);

                r = recv_line(sock, resp, sizeof(resp));
                if (r > 0)
                    printf("%s", resp);
            } else {
                printf("%s", resp);
            }
            continue;
        }

        /* ---------- Default ---------- */
        printf("%s", resp);

        if (strncmp(input, "EXIT", 4) == 0)
            break;
    }
}

/* Entry point */
int main() {
    if (!init_winsock()) {
        printf("WSAStartup failed\n");
        return 1;
    }

    SOCKET s = create_connection();
    if (s == INVALID_SOCKET) {
        printf("Failed to connect to server\n");
        cleanup_winsock();
        return 1;
    }

    client_loop(s);

    closesocket(s);
    cleanup_winsock();
    return 0;
}
