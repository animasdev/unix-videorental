#include <stdio.h>
#include <string.h>
#include <sqlite3.h>
#include "userdb.h"

extern sqlite3 *get_db();  // Make sure to provide this function in a db.c file or here
                    // Example: extern sqlite3 *get_db(); if in another file

int user_exists(const char *username) {
    sqlite3 *db = get_db();
    sqlite3_stmt *stmt;
    const char *sql = "SELECT id FROM Users WHERE username = ?";

    if (sqlite3_prepare_v2(db, sql, -1, &stmt, NULL) != SQLITE_OK) {
        fprintf(stderr, "Failed to prepare statement: %s\n", sqlite3_errmsg(db));
        return -1;
    }

    sqlite3_bind_text(stmt, 1, username, -1, SQLITE_STATIC);

    int result = -1;
    if (sqlite3_step(stmt) == SQLITE_ROW) {
        result = sqlite3_column_int(stmt, 0); // return user id
    }

    sqlite3_finalize(stmt);
    return result;
}

int user_login(const int usr_id, const char *password) {
    sqlite3 *db = get_db();
    sqlite3_stmt *stmt;
    const char *sql = "SELECT password FROM Users WHERE id = ?";

    if (sqlite3_prepare_v2(db, sql, -1, &stmt, NULL) != SQLITE_OK) {
        fprintf(stderr, "Failed to prepare login check: %s\n", sqlite3_errmsg(db));
        return 0;
    }

    sqlite3_bind_int(stmt, 1, usr_id);

    int result = 0;
    if (sqlite3_step(stmt) == SQLITE_ROW) {
        const unsigned char *db_password = sqlite3_column_text(stmt, 0);
        if (db_password && strcmp((const char *)db_password, password) == 0) {
            result = 1;
        }
    }

    sqlite3_finalize(stmt);
    return result;
}

int user_register(const char *username, const char *password) {
    if (user_exists(username) != -1) {
        return -1; // already exists
    }

    sqlite3 *db = get_db();
    sqlite3_stmt *stmt;
    const char *sql = "INSERT INTO Users (username, password) VALUES (?, ?)";

    if (sqlite3_prepare_v2(db, sql, -1, &stmt, NULL) != SQLITE_OK) {
        fprintf(stderr, "Failed to prepare insert: %s\n", sqlite3_errmsg(db));
        return -1;
    }

    sqlite3_bind_text(stmt, 1, username, -1, SQLITE_STATIC);
    sqlite3_bind_text(stmt, 2, password, -1, SQLITE_STATIC);

    int rc = sqlite3_step(stmt);
    sqlite3_finalize(stmt);

    return (rc == SQLITE_DONE) ? user_exists(username) : -1;
}
