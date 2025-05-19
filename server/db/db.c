#include "db.h"
#include <stdio.h>

sqlite3 *get_db() {
    static sqlite3 *db = NULL;

    if (!db) {
        if (sqlite3_open("../db/videoteca.db", &db) != SQLITE_OK) {
            fprintf(stderr, "Can't open DB: %s\n", sqlite3_errmsg(db));
            return NULL;
        }
    }
    return db;
}

void close_db() {
    sqlite3 *db = get_db();
    if (db) sqlite3_close(db);
}

int setup_db() {
    const sqlite3 *db = get_db();
    const char *create_users_table =
        "CREATE TABLE IF NOT EXISTS Users ("
        "id INTEGER PRIMARY KEY AUTOINCREMENT, "
        "username TEXT UNIQUE NOT NULL, "
        "password TEXT NOT NULL, "
        "is_admin INTEGER NOT NULL DEFAULT 0);";

    const char *create_videos_table =
        "CREATE TABLE IF NOT EXISTS Videos ("
        "id INTEGER PRIMARY KEY AUTOINCREMENT, "
        "title TEXT NOT NULL, "
        "available_copies INTEGER NOT NULL, "
        "borrowed_copies INTEGER NOT NULL DEFAULT 0);";

    const char *create_rentals_table =
        "CREATE TABLE IF NOT EXISTS Rentals ("
        "id INTEGER PRIMARY KEY AUTOINCREMENT, "
        "video_id INTEGER NOT NULL, "
        "username TEXT NOT NULL, "
        "rented_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP, "
        "due_date TIMESTAMP NOT NULL, "
        "FOREIGN KEY(video_id) REFERENCES Videos(id), "
        "FOREIGN KEY(username) REFERENCES Users(username));";

    sqlite3_stmt *stmt;

    // Create Users table
    if (sqlite3_prepare_v2(db, create_users_table, -1, &stmt, 0) != SQLITE_OK)
    {
        fprintf(stdout, "Failed to prepare create Users table: %s\n", sqlite3_errmsg(db));
        return 0;
    }
    if (sqlite3_step(stmt) != SQLITE_DONE)
    {
        fprintf(stdout, "Failed to create Users table: %s\n", sqlite3_errmsg(db));
        sqlite3_finalize(stmt);
        return 0;
    }
    sqlite3_finalize(stmt);

    // Create Videos table
    if (sqlite3_prepare_v2(db, create_videos_table, -1, &stmt, 0) != SQLITE_OK)
    {
        fprintf(stdout, "Failed to prepare create Videos table: %s\n", sqlite3_errmsg(db));
        return 0;
    }
    if (sqlite3_step(stmt) != SQLITE_DONE)
    {
        fprintf(stdout, "Failed to create Videos table: %s\n", sqlite3_errmsg(db));
        sqlite3_finalize(stmt);
        return 0;
    }
    sqlite3_finalize(stmt);

    if (sqlite3_prepare_v2(db, create_rentals_table, -1, &stmt, 0) != SQLITE_OK)
    {
        fprintf(stdout, "Failed to prepare create Rentals table: %s\n", sqlite3_errmsg(db));
        return 0;
    }
    if (sqlite3_step(stmt) != SQLITE_DONE)
    {
        fprintf(stdout, "Failed to create Rentals table: %s\n", sqlite3_errmsg(db));
        sqlite3_finalize(stmt);
        return 0;
    }
    sqlite3_finalize(stmt);
    sqlite3_close_v2(db);
    return 1;
}