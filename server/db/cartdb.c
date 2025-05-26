#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sqlite3.h>
#include "cartdb.h"
#include "videodb.h"
#define ERR_SIZE 256
extern sqlite3 *get_db(const char* db_path);

int cart_insert(const char* username,const int movie_id,char* errors){
    const char* db_path = getenv("DB_PATH");
    sqlite3 *db = get_db(db_path);
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
int find_cart_by_username(const char* username, Cart* cart_items[], int max_results){
    const char* db_path = getenv("DB_PATH");
    sqlite3* db = get_db(db_path);
    sqlite3_stmt* stmt;
    const char* sql = "SELECT id, username, video_id FROM Carts WHERE username = ?"; 

    if (sqlite3_prepare_v2(db, sql, -1, &stmt, NULL) != SQLITE_OK) {
        fprintf(stderr, "Failed to prepare query: %s\n", sqlite3_errmsg(db));
        sqlite3_close(db);
        return -1;
    }

    sqlite3_bind_text(stmt, 1, username, -1, SQLITE_TRANSIENT);
    const char *expanded = sqlite3_expanded_sql(stmt);
    if (expanded) {
        printf("Executing SQL: %s\n", expanded);
        sqlite3_free((void *)expanded);
    }
    int count = 0;
    while (sqlite3_step(stmt) == SQLITE_ROW && count < max_results) {
        Cart* cart = malloc(sizeof(Cart));
        if (!cart) break;

        cart->id = sqlite3_column_int(stmt, 0);
        const unsigned char* username = sqlite3_column_text(stmt, 1);
        cart->username = strdup((const char*)username);
        cart->movie = find_video_by_id(sqlite3_column_int(stmt, 2));
        cart_items[count++] = cart;
    }

    sqlite3_finalize(stmt);
    sqlite3_close(db);
    return count;
}
int cart_delete_by_id(int cart_id) {
    const char* db_path = getenv("DB_PATH");
    sqlite3* db = get_db(db_path);
    sqlite3_stmt *stmt;
    const char *sql = "DELETE FROM Carts WHERE id = ?";

    if (sqlite3_prepare_v2(db, sql, -1, &stmt, NULL) != SQLITE_OK) {
        fprintf(stderr, "Failed to prepare query: %s\n", sqlite3_errmsg(db));
        sqlite3_close(db);
        return 0;
    }

    sqlite3_bind_int(stmt, 1, cart_id);

    int rc = sqlite3_step(stmt);
    sqlite3_finalize(stmt);

    if (rc != SQLITE_DONE) {
        const char* err = sqlite3_errmsg(db);
        fprintf(stderr, "Delete failed: %s\n", err);
        sqlite3_close(db);
        return 0;
    }

    sqlite3_close(db);
    return 1;
}