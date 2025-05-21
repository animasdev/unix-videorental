#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sqlite3.h>
#include "videodb.h"

#define ERR_SIZE 256
extern sqlite3 *get_db(); 



int video_insert(const char *title, const int copies, char* errors) {
    sqlite3 *db = get_db();
    sqlite3_stmt *stmt;
    const char *sql = "INSERT INTO Videos (title, available_copies) VALUES (?, ?)";

    if (sqlite3_prepare_v2(db, sql, -1, &stmt, NULL) != SQLITE_OK) {
        fprintf(stderr, "Failed to prepare insert: %s\n", sqlite3_errmsg(db));
        sqlite3_close(db);
        return -1;
    }

    sqlite3_bind_text(stmt, 1, title, -1, SQLITE_TRANSIENT);
    sqlite3_bind_int(stmt,2,copies);

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


int find_videos_by_title(const char* search_term, struct Video* results[], int max_results) {
    sqlite3* db = get_db();
    sqlite3_stmt* stmt;
    int count = 0;
    const char* sql = "SELECT id,title, available_copies, (borrowed_copies) AS rented_copies "
                      "FROM Videos WHERE title LIKE ?";

    if (sqlite3_prepare_v2(db, sql, -1, &stmt, NULL) != SQLITE_OK) {
        fprintf(stderr, "Failed to prepare query: %s\n", sqlite3_errmsg(db));
        sqlite3_close(db);
        return -1;
    }

    char pattern[256];
    snprintf(pattern, sizeof(pattern), "%%%s%%", search_term); 
    sqlite3_bind_text(stmt, 1, pattern, -1, SQLITE_TRANSIENT);
    const char *expanded = sqlite3_expanded_sql(stmt);
        if (expanded) {
            printf("Executing SQL: %s\n", expanded);
            sqlite3_free((void *)expanded);
        }
    while (sqlite3_step(stmt) == SQLITE_ROW && count < max_results) {
        struct Video* vid = malloc(sizeof(struct Video));
        if (!vid) break;

        const unsigned char* title = sqlite3_column_text(stmt, 1);
        vid->title = strdup((const char*)title);
        vid->id = sqlite3_column_int(stmt, 0);
        vid->av_copies = sqlite3_column_int(stmt, 2);
        vid->rt_copies = sqlite3_column_int(stmt, 3);

        results[count++] = vid;
    }

    sqlite3_finalize(stmt);
    sqlite3_close(db);
    return count;
}