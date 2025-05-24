#ifndef CART_DB_H
#define CART_DB_H

#include <sqlite3.h>
#include "../../common/common.h"


int cart_insert(const char* username,const int movie_id, char* errors);
int cart_remove(const int cart_id);
int find_cart_by_username(const char* username,Cart* cart_items[], int max_results);
#endif