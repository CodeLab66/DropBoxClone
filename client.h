#ifndef CLIENT_H
#define CLIENT_H

#include <winsock2.h>
#include <windows.h>

#define SERVER_IP "127.0.0.1"
#define SERVER_PORT 8081
#define CLIENT_BUFFER_SIZE 8192

/* client helpers */
int init_winsock();
void cleanup_winsock();
SOCKET create_connection();

/* file operations */
int send_all(SOCKET sock, const char *data, int len);
int recv_line(SOCKET sock, char *buf, int maxlen);
void upload_file(SOCKET sock, const char *localpath, const char *remote_filename);
void download_file(SOCKET sock, const char *remote_filename, const char *localpath);

/* interactive loop */
void client_loop(SOCKET sock);

#endif /* CLIENT_H */
