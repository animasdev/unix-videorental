#ifndef DB_H
#define DB_H

#include <sqlite3.h>

sqlite3 *get_db(const char* db_path);

int setup_db(const char* db_path);
int admin_exists(const char* db_path);
#endif 