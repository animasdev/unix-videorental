#include <stdio.h>
#include <sqlite3.h>
#include "videodb.h"

extern sqlite3 *get_db(); 
int video_insert(const char *title, const int copies) {
    sqlite3 *db = get_db();
    sqlite3_stmt *stmt;
    const char *sql = "INSERT INTO Videos (title, available_copies) VALUES (?, ?)";

    if (sqlite3_prepare_v2(db, sql, -1, &stmt, NULL) != SQLITE_OK) {
        fprintf(stderr, "Failed to prepare insert: %s\n", sqlite3_errmsg(db));
        sqlite3_close(db);
        return 0;
    }

    sqlite3_bind_text(stmt, 1, title, -1, SQLITE_TRANSIENT);
    sqlite3_bind_int(stmt,2,copies);

    int rc = sqlite3_step(stmt);
    sqlite3_finalize(stmt);
    sqlite3_close(db);
    return 1;
}