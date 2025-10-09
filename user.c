#include "user.h"
#include <string.h>
#include <direct.h>
#include <errno.h>
#include <stdio.h>

user_t users[MAX_USERS];
int num_users = 0;
pthread_mutex_t users_mutex = PTHREAD_MUTEX_INITIALIZER;

user_t *user_signup(const char *username, const char *password) {
    pthread_mutex_lock(&users_mutex);
    int exists = 0;
    for (int i = 0; i < num_users; i++) {
        if (strcmp(users[i].username, username) == 0) {
            exists = 1;
            break;
        }
    }
    if (exists || num_users >= MAX_USERS) {
        pthread_mutex_unlock(&users_mutex);
        return NULL;
    }
    user_t *user = &users[num_users++];
    strncpy(user->username, username, sizeof(user->username) - 1);
    user->username[sizeof(user->username) - 1] = '\0';
    strncpy(user->password, password, sizeof(user->password) - 1);
    user->password[sizeof(user->password) - 1] = '\0';
    user->quota = DEFAULT_QUOTA;
    user->used = 0;
    user->sockfd = INVALID_SOCKET;
    pthread_mutex_init(&user->mutex, NULL);
    pthread_mutex_unlock(&users_mutex);
    user_ensure_dir(username);
    return user;
}

int user_login(const char *username, const char *password) {
    user_t *user = user_find(username);
    if (user && strcmp(user->password, password) == 0) {
        return 1;
    }
    return 0;
}

user_t *user_find(const char *username) {
    pthread_mutex_lock(&users_mutex);
    for (int i = 0; i < num_users; i++) {
        if (strcmp(users[i].username, username) == 0) {
            pthread_mutex_unlock(&users_mutex);
            return &users[i];
        }
    }
    pthread_mutex_unlock(&users_mutex);
    return NULL;
}

void user_update_used(user_t *user, long delta) {
    if (user == NULL) return;
    pthread_mutex_lock(&user->mutex);
    user->used += delta;
    pthread_mutex_unlock(&user->mutex);
}

void user_ensure_dir(const char *username) {
    char dir_path[BUFFER_SIZE];
    snprintf(dir_path, sizeof(dir_path), "%s/%s", ROOT_DIR, username);
    printf("Creating user directory: %s\n", dir_path);
    if (_mkdir(dir_path) == -1 && errno != EEXIST) {
        printf("mkdir failed: %s\n", strerror(errno));
    }
}