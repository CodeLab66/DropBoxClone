#include "utils.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <direct.h>
#include <io.h>      // For _access, _unlink on Windows
#include <windows.h> // For CreateFile, etc., if needed

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
    user_ensure_dir(username);  // Ensure dir exists
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
    if (_access(path, 0) == 0) {  // Check if file exists
        return _unlink(path);     // Windows equivalent of unlink
    }
    return -1;
}

char *list_files(const char *username) {
    if (username == NULL) return NULL;
    char path[BUFFER_SIZE];
    snprintf(path, sizeof(path), "%s/%s", ROOT_DIR, username);
    char *result = malloc(BUFFER_SIZE);  // Simple buffer, expand if needed
    if (result == NULL) return NULL;
    strcpy(result, "OK Files:\n");
    // Placeholder: Simulate listing (real impl needs dir iteration)
    strcat(result, "file1.txt\nfile2.txt\nEND\n");
    return result;
}