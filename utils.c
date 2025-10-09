#include "utils.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <direct.h>
#include <io.h>
#include <windows.h>

int send_response(int sockfd, const char *response) {
    if (sockfd < 0 || response == NULL) return -1;
    ssize_t total = 0, len = strlen(response);
    while (total < len) {
        ssize_t sent = send(sockfd, response + total, len - total, 0);
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
    fread(buffer, 1, file_size, file);
    fclose(file);
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
    return (written == size) ? 0 : -1;
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
    if (username == NULL) return NULL;
    char dir_path[BUFFER_SIZE];
    snprintf(dir_path, sizeof(dir_path), "%s/%s/*", ROOT_DIR, username);
    WIN32_FIND_DATA find_data;
    HANDLE hFind = FindFirstFile(dir_path, &find_data);
    if (hFind == INVALID_HANDLE_VALUE) {
        return strdup("ERROR No files found or directory error\n");
    }
    char *result = malloc(BUFFER_SIZE);
    if (result == NULL) {
        FindClose(hFind);
        return NULL;
    }
    size_t remaining = BUFFER_SIZE - 1;
    snprintf(result, BUFFER_SIZE, "OK Files in %s's storage:\n", username);
    remaining -= strlen(result);
    do {
        if (!(find_data.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)) {
            strncat(result, "- ", remaining);
            remaining -= 2;
            strncat(result, find_data.cFileName, remaining);
            remaining -= strlen(find_data.cFileName);
            strncat(result, "\n", remaining);
            remaining -= 1;
            if (remaining <= 10) break;  // Safety to prevent overflow
        }
    } while (FindNextFile(hFind, &find_data));
    FindClose(hFind);
    strncat(result, "END\n", remaining);
    return result;
}