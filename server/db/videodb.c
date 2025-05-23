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


int find_videos_by_title(const char* search_term, Video* results[], int max_results) {
    sqlite3* db = get_db();
    sqlite3_stmt* stmt;
    int count = 0;
    const char* sql = "SELECT v.id, v.title , v.available_copies ,count(r.id)<v.available_copies as rentable FROM Videos v "
                      "left join Rentals r on r.video_id =v.id "
                      "WHERE title LIKE ? "
                      "group by v.id"; 

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
        Video* vid = malloc(sizeof(Video));
        if (!vid) break;

        const unsigned char* title = sqlite3_column_text(stmt, 1);
        vid->title = strdup((const char*)title);
        vid->id = sqlite3_column_int(stmt, 0);
        vid->av_copies = sqlite3_column_int(stmt, 2);
        vid->is_rentable = sqlite3_column_int(stmt, 3);

        results[count++] = vid;
    }

    sqlite3_finalize(stmt);
    sqlite3_close(db);
    return count;
}

Video* find_video_by_id(const int id) {
    sqlite3* db = get_db();
    sqlite3_stmt* stmt;
    const char* sql = "SELECT v.id, v.title , v.available_copies ,count(r.id)<v.available_copies as rentable FROM Videos v "
                      "left join Rentals r on r.video_id =v.id "
                      "WHERE v.id = ? "
                      "group by v.id"; 

    if (sqlite3_prepare_v2(db, sql, -1, &stmt, NULL) != SQLITE_OK) {
        fprintf(stderr, "Failed to prepare query: %s\n", sqlite3_errmsg(db));
        sqlite3_close(db);
        return NULL;
    }

    sqlite3_bind_int(stmt, 1, id);
    const char *expanded = sqlite3_expanded_sql(stmt);
    if (expanded) {
        printf("Executing SQL: %s\n", expanded);
        sqlite3_free((void *)expanded);
    }
    Video* vid = NULL;
    int rc =sqlite3_step(stmt);
    if (rc != SQLITE_ROW && rc != SQLITE_DONE) {
        fprintf(stderr, "sqlite3_step failed: %s (code %d, extended %d)\n",
                sqlite3_errmsg(db), rc, sqlite3_extended_errcode(db));
    }
    if ( rc == SQLITE_ROW) {
        vid = malloc(sizeof(Video));
        const unsigned char* title = sqlite3_column_text(stmt, 1);
        vid->title = strdup((const char*)title);
        vid->id = sqlite3_column_int(stmt, 0);
        vid->av_copies = sqlite3_column_int(stmt, 2);
        vid->is_rentable = sqlite3_column_int(stmt, 3);

    }
    sqlite3_finalize(stmt);
    sqlite3_close(db);
    return vid;
}

int rent_video(const char* username,const int movie_id){
    sqlite3* db = get_db();
    sqlite3_stmt* stmt;
    
    const char* sql = "INSERT INTO Rentals (video_id, username, due_date) "
                      "VALUES(?, ?, date('now', '+7 days'))";

    if (sqlite3_prepare_v2(db, sql, -1, &stmt, NULL) != SQLITE_OK) {
        fprintf(stderr, "Failed to prepare query: %s\n", sqlite3_errmsg(db));
        sqlite3_close(db);
        return -1;
    }

    sqlite3_bind_int(stmt, 1, movie_id);
    sqlite3_bind_text(stmt, 2, username, -1, SQLITE_TRANSIENT);
    const char *expanded = sqlite3_expanded_sql(stmt);
    if (expanded) {
        printf("Executing SQL: %s\n", expanded);
        sqlite3_free((void *)expanded);
    }
    int rc = sqlite3_step(stmt);
    sqlite3_finalize(stmt);
    if (rc != SQLITE_DONE) {
        const char* err= sqlite3_errmsg(db);
        fprintf(stderr, "Insert failed: %s\n", err);
        sqlite3_close(db);
        return -1;
    }

    int last_id = (int) sqlite3_last_insert_rowid(db);
    sqlite3_close(db);
    return last_id;
}

int find_rentals_by_username(const char* username, Rental* results[], int max_results) {
    sqlite3* db = get_db();
    sqlite3_stmt* stmt;
    int count = 0;
    const char *sql = "SELECT id, video_id, username, due_date, rented_at FROM Rentals WHERE username = ?";
    
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
    while (sqlite3_step(stmt) == SQLITE_ROW && count < max_results) {
        Rental* rental = malloc(sizeof(Rental));
        if (!rental) break;

        const unsigned char* name = sqlite3_column_text(stmt, 2);
        const unsigned char* due_date = sqlite3_column_text(stmt, 3);
        const unsigned char* start_date = sqlite3_column_text(stmt, 4);
        rental->username = strdup((const char*)name);
        rental->id = sqlite3_column_int(stmt, 0);
        rental->id_movie = sqlite3_column_int(stmt, 1);
        rental->start_date=strdup((const char*)start_date);
        rental->due_date=strdup((const char*)due_date);

        results[count++] = rental;
    }

    sqlite3_finalize(stmt);
    sqlite3_close(db);
    return count;
}

Rental* find_rental_by_id(int rental_id) {
    sqlite3* db = get_db();
    const char *sql = "SELECT id, video_id, username, due_date, rented_at FROM Rentals WHERE id = ?";
    sqlite3_stmt *stmt;
    Rental *rental = malloc(sizeof(Rental));
    if (!rental) {
        perror("malloc");
        return NULL;
    }

    if (sqlite3_prepare_v2(db, sql, -1, &stmt, NULL) != SQLITE_OK) {
        fprintf(stderr, "Failed to prepare statement: %s\n", sqlite3_errmsg(db));
        free(rental);
        return NULL;
    }

    sqlite3_bind_int(stmt, 1, rental_id);
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
    if ( rc == SQLITE_ROW) {
        rental->id = sqlite3_column_int(stmt, 0);
        rental->id_movie = sqlite3_column_int(stmt, 1);

        const unsigned char *username = sqlite3_column_text(stmt, 2);
        const unsigned char *due_date = sqlite3_column_text(stmt, 3);
        const unsigned char* start_date = sqlite3_column_text(stmt, 4);
        rental->username=strdup((const char*)username);
        rental->start_date=strdup((const char*)start_date);
        rental->due_date=strdup((const char*)due_date);
    } 

    sqlite3_finalize(stmt);
    printf("after finalize\n");
    sqlite3_close(db);
    printf("after close\n");
    return rental;
}