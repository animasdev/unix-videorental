#ifndef USER_DB_H
#define USER_DB_H

#include <sqlite3.h>

int user_exists(const char *username);
int user_login(const int usr_id, const char *password);
int user_register(const char *username, const char *password);
int user_is_admin(const int id);
#endif 