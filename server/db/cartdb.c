#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sqlite3.h>
#include "cartdb.h"

#define ERR_SIZE 256
extern sqlite3 *get_db();

int cart_insert(const char* username,const int movie_id,char* errors){
    sqlite3 *db = get_db();
    sqlite3_stmt *stmt;
    const char *sql = "INSERT INTO Carts (username, video_id) VALUES (?, ?)";

    if (sqlite3_prepare_v2(db, sql, -1, &stmt, NULL) != SQLITE_OK) {
        fprintf(stderr, "Failed to prepare insert: %s\n", sqlite3_errmsg(db));
        sqlite3_close(db);
        return -1;
    }

    sqlite3_bind_text(stmt, 1, username, -1, SQLITE_TRANSIENT);
    sqlite3_bind_int(stmt,2,movie_id);

    int rc = sqlite3_step(stmt);
    sqlite3_finalize(stmt);
    if (rc != SQLITE_DONE) {
        const char* err= sqlite3_errmsg(db);
        snprintf(errors, ERR_SIZE, "%s",err);
        fprintf(stderr, "Insert failed: %s\n", err);
        sqlite3_close(db);
        return -1;
    }

    int last_id = (int) sqlite3_last_insert_rowid(db);
    sqlite3_close(db);
    return last_id;
}
int cart_remove(const int cart_id) {
    return 0;
}
int find_cart_by_username(const char* username) {
    return 0;
}