#include "utils.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <direct.h>
#include <io.h>
#include <windows.h>

int send_response(int sockfd, const char *response) {
    if (sockfd == INVALID_SOCKET || response == NULL) return -1;
    int total = 0;
    int len = (int)strlen(response);
    while (total < len) {
        int sent = send(sockfd, response + total, len - total, 0);
        if (sent <= 0) return -1;
        total += sent;
    }
    return 0;
}

char *read_file(const char *username, const char *filename, long *size) {
    if (username == NULL || filename == NULL || size == NULL) return NULL;
    char path[BUFFER_SIZE];
    snprintf(path, sizeof(path), "%s/%s/%s", ROOT_DIR, username, filename);
    FILE *file = fopen(path, "rb");
    if (file == NULL) return NULL;
    fseek(file, 0, SEEK_END);
    long file_size = ftell(file);
    rewind(file);
    char *buffer = malloc(file_size);
    if (buffer == NULL) {
        fclose(file);
        return NULL;
    }
    size_t read = fread(buffer, 1, file_size, file);
    fclose(file);
    if ((long)read != file_size) {
        free(buffer);
        return NULL;
    }
    *size = file_size;
    return buffer;
}

int write_file(const char *username, const char *filename, const char *data, long size) {
    if (username == NULL || filename == NULL || data == NULL || size < 0) return -1;
    char path[BUFFER_SIZE];
    snprintf(path, sizeof(path), "%s/%s/%s", ROOT_DIR, username, filename);
    user_ensure_dir(username);
    FILE *file = fopen(path, "wb");
    if (file == NULL) return -1;
    size_t written = fwrite(data, 1, size, file);
    fclose(file);
    return (written == (size_t)size) ? 0 : -1;
}

int delete_file(const char *username, const char *filename) {
    if (username == NULL || filename == NULL) return -1;
    char path[BUFFER_SIZE];
    snprintf(path, sizeof(path), "%s/%s/%s", ROOT_DIR, username, filename);
    if (_access(path, 0) == 0) {
        return _unlink(path);
    }
    return -1;
}

char *list_files(const char *username) {
    char path[512];
    snprintf(path, sizeof(path), "%s/%s/*", ROOT_DIR, username);

    struct _finddata_t fileinfo;
    intptr_t handle = _findfirst(path, &fileinfo);
    if (handle == -1) {
        char *msg = malloc(128);
        strcpy(msg, "OK Files in storage:\n(Empty)\n");
        return msg;
    }

    // Allocate large dynamic buffer and track length
    size_t capacity = 8192;
    size_t length = 0;
    char *list = malloc(capacity);
    if (!list) {
        _findclose(handle);
        return NULL;
    }

    length += snprintf(list + length, capacity - length, "OK Files in storage:\n");

    do {
        if (!(fileinfo.attrib & _A_SUBDIR)) {
            size_t needed = strlen(fileinfo.name) + 4; // "- name\n"
            if (length + needed + 1 >= capacity) {
                capacity *= 2;
                char *newptr = realloc(list, capacity);
                if (!newptr) break;
                list = newptr;
            }
            length += snprintf(list + length, capacity - length, "- %s\n", fileinfo.name);
        }
    } while (_findnext(handle, &fileinfo) == 0);

    _findclose(handle);

    return list;
}

