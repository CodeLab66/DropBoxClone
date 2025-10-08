#ifndef USER_H
#define USER_H

#include <pthread.h>
#include "config.h"  // For MAX_USERS, DEFAULT_QUOTA, ROOT_DIR

typedef struct {
    char username[32];
    char password[32];
    long quota;
    long used;
    pthread_mutex_t mutex;  // Per-user lock for concurrency
} user_t;

extern user_t users[MAX_USERS];
extern int num_users;
extern pthread_mutex_t users_mutex;

user_t *user_signup(const char *username, const char *password);
int user_login(const char *username, const char *password);
user_t *user_find(const char *username);
void user_update_used(user_t *user, long delta);
void user_ensure_dir(const char *username);

#endif
