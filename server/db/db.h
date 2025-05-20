#ifndef DB_H
#define DB_H

#include <sqlite3.h>

sqlite3 *get_db();

int setup_db();
int admin_exists();
#endif 