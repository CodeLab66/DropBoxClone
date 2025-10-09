#ifndef UTILS_H
#define UTILS_H

#include "user.h"
#include "task.h"

int send_response(int sockfd, const char *response);
char *read_file(const char *username, const char *filename, long *size);
int write_file(const char *username, const char *filename, const char *data, long size);
int delete_file(const char *username, const char *filename);
char *list_files(const char *username);

#endif