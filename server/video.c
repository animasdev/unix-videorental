// video.c (Handle video management, rentals, returns, etc.)
#include <stdio.h>
#include <sqlite3.h>

int rent_video(sqlite3 *db, int video_id, const char *username) {
    const char *sql = "UPDATE Videos SET borrowed_copies = borrowed_copies + 1 WHERE id = ? AND available_copies > borrowed_copies;";
    sqlite3_stmt *stmt;
    if (sqlite3_prepare_v2(db, sql, -1, &stmt, 0) != SQLITE_OK) {
        fprintf(stderr, "Failed to prepare statement: %s\n", sqlite3_errmsg(db));
        return 0;
    }
    sqlite3_bind_int(stmt, 1, video_id);
    if (sqlite3_step(stmt) != SQLITE_DONE) {
        sqlite3_finalize(stmt);
        return 0;
    }
    sqlite3_finalize(stmt);
    return 1;
}
