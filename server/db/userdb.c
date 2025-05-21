#include <stdio.h>
#include <string.h>
#include <sqlite3.h>
#include <stdlib.h>
#include "userdb.h"

extern sqlite3 *get_db(); 
/*
if user of the given username exists, it returns the id. It returns -1 if the user is not found.
*/
int user_exists(const char *username) {
    sqlite3 *db = get_db();
    sqlite3_stmt *stmt;
    const char *sql = "SELECT id FROM Users WHERE username = ?";

    if (sqlite3_prepare_v2(db, sql, -1, &stmt, NULL) != SQLITE_OK) {
        fprintf(stderr, "Failed to prepare statement: %s\n", sqlite3_errmsg(db));
        sqlite3_close(db);
        return -1;
    }

    sqlite3_bind_text(stmt, 1, username, -1, SQLITE_TRANSIENT);
    const char *expanded = sqlite3_expanded_sql(stmt);
    if (expanded) {
        printf("Executing SQL: %s\n", expanded);
        sqlite3_free((void *)expanded);
    }
    int result = -1;
    int rc =sqlite3_step(stmt);
    if (rc != SQLITE_ROW && rc != SQLITE_DONE) {
        fprintf(stderr, "sqlite3_step failed: %s (code %d, extended %d)\n",
                sqlite3_errmsg(db), rc, sqlite3_extended_errcode(db));
    }

    if ( rc == SQLITE_ROW) {
        result = sqlite3_column_int(stmt, 0);
        printf("result: %d\n",result);
    }

    sqlite3_finalize(stmt);
    sqlite3_close(db);
    return result;
}

int user_login(const int usr_id, const char *password) {
    sqlite3 *db = get_db();
    sqlite3_stmt *stmt;
    const char *sql = "SELECT password FROM Users WHERE id = ?";

    if (sqlite3_prepare_v2(db, sql, -1, &stmt, NULL) != SQLITE_OK) {
        fprintf(stderr, "Failed to prepare login check: %s\n", sqlite3_errmsg(db));
        sqlite3_close(db);
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
    sqlite3_close(db);
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
        sqlite3_close(db);
        return -1;
    }

    sqlite3_bind_text(stmt, 1, username, -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 2, password, -1, SQLITE_TRANSIENT);

    int rc = sqlite3_step(stmt);
    sqlite3_finalize(stmt);
    sqlite3_close(db);
    return (rc == SQLITE_DONE) ? user_exists(username) : -1;
}

int user_is_admin(const int usr_id){
    sqlite3 *db = get_db();
    sqlite3_stmt *stmt;
    const char *sql = "SELECT is_admin FROM Users WHERE id = ?";

    if (sqlite3_prepare_v2(db, sql, -1, &stmt, NULL) != SQLITE_OK) {
        fprintf(stderr, "Failed to prepare is_admin check: %s\n", sqlite3_errmsg(db));
        sqlite3_close(db);
        return -1;
    }

    sqlite3_bind_int(stmt, 1, usr_id);

    int result = -1;
    if (sqlite3_step(stmt) == SQLITE_ROW) {
        result = sqlite3_column_int(stmt, 0);
    }

    sqlite3_finalize(stmt);
    sqlite3_close(db);
    return result;
}

User* find_user_by_id(const int id) {
    sqlite3 *db = get_db();
    sqlite3_stmt *stmt;
    const char *sql = "SELECT id, username, is_admin FROM Users WHERE id = ?";

    if (sqlite3_prepare_v2(db, sql, -1, &stmt, NULL) != SQLITE_OK) {
        fprintf(stderr, "Failed to prepare statement: %s\n", sqlite3_errmsg(db));
        sqlite3_close(db);
        return NULL;
    }

    sqlite3_bind_int(stmt, 1,id);
    const char *expanded = sqlite3_expanded_sql(stmt);
    if (expanded) {
        printf("Executing SQL: %s\n", expanded);
        sqlite3_free((void *)expanded);
    }
    int rc =sqlite3_step(stmt);
    if (rc != SQLITE_ROW && rc != SQLITE_DONE) {
        fprintf(stderr, "sqlite3_step failed: %s (code %d, extended %d)\n",
                sqlite3_errmsg(db), rc, sqlite3_extended_errcode(db));
    }
    User* result = NULL;
    if ( rc == SQLITE_ROW) {
        result = malloc(sizeof(User));
        result->id = sqlite3_column_int(stmt, 0);
        result->username = strdup((const char*)sqlite3_column_text(stmt, 1));
        result->is_admin = sqlite3_column_int(stmt, 2);
    }
    sqlite3_finalize(stmt);
    sqlite3_close(db);
    return result;
}
