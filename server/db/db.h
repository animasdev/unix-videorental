#ifndef DB_H
#define DB_H

#include <sqlite3.h>

sqlite3 *get_db();

void close_db();
int setup_db();
#endif 