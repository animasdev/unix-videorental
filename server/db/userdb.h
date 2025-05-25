#ifndef USER_DB_H
#define USER_DB_H

#include <sqlite3.h>


typedef struct {
    int id;
    char* username;
    int is_admin;
} User;


int user_exists(const char *username);
int user_login(const int usr_id, const char *password);
int user_register(const char *username, const char *password);
int user_is_admin(const int id);
User* find_user_by_id(const int id);
User* find_user_by_username(const char* username);
#endif 