#ifndef UTILS_H
#define UTILS_H

#include <winsock2.h>
#include <windows.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "config.h"
#include "user.h"

/* Default buffer size for I/O */
#define BUFFER_SIZE 8192

/* --- Response helpers --- */
int send_response(int sockfd, const char *response);

/* --- File operations --- */
char *read_file(const char *username, const char *filename, long *size);
int write_file(const char *username, const char *filename, const char *data, long size);
int delete_file(const char *username, const char *filename);
char *list_files(const char *username);

/* --- Utility for ensuring directory exists (declared here, implemented in user.c) --- */
void user_ensure_dir(const char *username);

#endif /* UTILS_H */
