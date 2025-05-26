#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sqlite3.h>
#include "videodb.h"

#define ERR_SIZE 256
extern sqlite3 *get_db(const char* db_path); 

const char* build_video_query(const char* where_clause, char* buffer, size_t size);

int video_insert(const char *title, const int copies, char* errors) {
    const char* db_path = getenv("DB_PATH");
    sqlite3* db = get_db(db_path);
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
    const char* db_path = getenv("DB_PATH");
    sqlite3* db = get_db(db_path);
    sqlite3_stmt* stmt;
    int count = 0;
    char sql[512];
    build_video_query("WHERE title LIKE ?", sql, sizeof(sql));

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
        vid->rented_copies = sqlite3_column_int(stmt, 4);
        printf("%d \"%s\" %d %d %d\n",vid->id,vid->title, vid->av_copies,vid->is_rentable, vid->rented_copies);

        results[count++] = vid;
    }

    sqlite3_finalize(stmt);
    sqlite3_close(db);
    return count;
}

Video* find_video_by_id(const int id) {
    const char* db_path = getenv("DB_PATH");
    sqlite3* db = get_db(db_path);
    sqlite3_stmt* stmt;
    char sql[512];
    build_video_query("WHERE v.id = ?", sql, sizeof(sql));

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
    const char* db_path = getenv("DB_PATH");
    sqlite3* db = get_db(db_path);
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

int find_rentals_by_username(const char* username, Rental* results[], int max_results, int include_returned) {
    const char* db_path = getenv("DB_PATH");
    sqlite3* db = get_db(db_path);
    sqlite3_stmt* stmt;
    int count = 0;
    const char *base = "SELECT id, video_id, username, due_date, rented_at, returned_at, reminder FROM Rentals WHERE username = ?";
    const char *active_clause = " AND returned_at IS NULL";
    
    char sql[512];
    if (!include_returned) {
        snprintf(sql, sizeof(sql), "%s%s", base, active_clause);
    } else {
        snprintf(sql, sizeof(sql), "%s", base);
    }
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
        rental->id = sqlite3_column_int(stmt, 0);
        rental->id_movie = sqlite3_column_int(stmt, 1);
        rental->reminder = sqlite3_column_int(stmt, 6);
        const unsigned char *username = sqlite3_column_text(stmt, 2);
        const unsigned char *due_date = sqlite3_column_text(stmt, 3);
        const unsigned char* start_date = sqlite3_column_text(stmt, 4);
        const unsigned char* end_date = sqlite3_column_text(stmt, 5);
        rental->username=strdup((const char*)username);
        rental->start_date=strdup((const char*)start_date);
        rental->due_date=strdup((const char*)due_date);
        rental->end_date=end_date ? strdup((const char*)end_date) : NULL;

        results[count++] = rental;
    }

    sqlite3_finalize(stmt);
    sqlite3_close(db);
    return count;
}

Rental* find_rental_by_id(int rental_id) {
    const char* db_path = getenv("DB_PATH");
    sqlite3* db = get_db(db_path);
    const char *sql = "SELECT id, video_id, username, due_date, rented_at, returned_at, reminder FROM Rentals WHERE id = ?";
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
        rental->reminder = sqlite3_column_int(stmt, 6);
        const unsigned char *username = sqlite3_column_text(stmt, 2);
        const unsigned char *due_date = sqlite3_column_text(stmt, 3);
        const unsigned char* start_date = sqlite3_column_text(stmt, 4);
        const unsigned char* end_date = sqlite3_column_text(stmt, 5);
        rental->username=strdup((const char*)username);
        rental->start_date=strdup((const char*)start_date);
        rental->due_date=strdup((const char*)due_date);
        rental->end_date=end_date ? strdup((const char*)end_date) : NULL;
    } 

    sqlite3_finalize(stmt);
    printf("after finalize\n");
    sqlite3_close(db);
    printf("after close\n");
    return rental;
}

Rental* find_rental_by_username_and_movie(const char* username, const int movie_id){
    const char* db_path = getenv("DB_PATH");
    sqlite3* db = get_db(db_path);
    const char *sql = "SELECT id, video_id, username, due_date, rented_at, returned_at, reminder FROM Rentals WHERE video_id = ? AND username = ?";
    sqlite3_stmt *stmt;
    Rental *rental = NULL;

    if (sqlite3_prepare_v2(db, sql, -1, &stmt, NULL) != SQLITE_OK) {
        fprintf(stderr, "Failed to prepare statement: %s\n", sqlite3_errmsg(db));
        free(rental);
        return NULL;
    }

    sqlite3_bind_int(stmt, 1, movie_id);
    sqlite3_bind_text(stmt, 2, username, -1, SQLITE_TRANSIENT);
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
        rental = malloc(sizeof(Rental));
        if (!rental) {
            perror("malloc");
            sqlite3_finalize(stmt);
            sqlite3_close(db);
            return NULL;
        }
        rental->id = sqlite3_column_int(stmt, 0);
        rental->id_movie = sqlite3_column_int(stmt, 1);
        rental->reminder = sqlite3_column_int(stmt, 6);
        const unsigned char *username = sqlite3_column_text(stmt, 2);
        const unsigned char *due_date = sqlite3_column_text(stmt, 3);
        const unsigned char* start_date = sqlite3_column_text(stmt, 4);
        const unsigned char* end_date = sqlite3_column_text(stmt, 5);
        rental->username=strdup((const char*)username);
        rental->start_date=strdup((const char*)start_date);
        rental->due_date=strdup((const char*)due_date);
        rental->end_date=end_date ? strdup((const char*)end_date) : NULL;
    } 

    sqlite3_finalize(stmt);
    sqlite3_close(db);
    return rental;

}

int find_all_rentals(Rental* results[], int max_results, int include_returned) {
    const char* db_path = getenv("DB_PATH");
    sqlite3* db = get_db(db_path);
    sqlite3_stmt* stmt;
    int count = 0;
    const char *base = "SELECT id, video_id, username, due_date, rented_at, returned_at, reminder FROM Rentals";
    const char *active_clause = " WHERE returned_at IS NULL";
    
    char sql[512];
    if (!include_returned) {
        snprintf(sql, sizeof(sql), "%s%s", base, active_clause);
    } else {
        snprintf(sql, sizeof(sql), "%s", base);
    }
    if (sqlite3_prepare_v2(db, sql, -1, &stmt, NULL) != SQLITE_OK) {
        fprintf(stderr, "Failed to prepare query: %s\n", sqlite3_errmsg(db));
        sqlite3_close(db);
        return -1;
    }

    const char *expanded = sqlite3_expanded_sql(stmt);
        if (expanded) {
            printf("Executing SQL: %s\n", expanded);
            sqlite3_free((void *)expanded);
        }
    while (sqlite3_step(stmt) == SQLITE_ROW && count < max_results) {
        Rental* rental = malloc(sizeof(Rental));
        if (!rental) break;
        rental->id = sqlite3_column_int(stmt, 0);
        rental->id_movie = sqlite3_column_int(stmt, 1);
        rental->reminder = sqlite3_column_int(stmt, 6);
        const unsigned char *username = sqlite3_column_text(stmt, 2);
        const unsigned char *due_date = sqlite3_column_text(stmt, 3);
        const unsigned char* start_date = sqlite3_column_text(stmt, 4);
        const unsigned char* end_date = sqlite3_column_text(stmt, 5);
        rental->username=strdup((const char*)username);
        rental->start_date=strdup((const char*)start_date);
        rental->due_date=strdup((const char*)due_date);
        rental->end_date=end_date ? strdup((const char*)end_date) : NULL;

        results[count++] = rental;
    }

    sqlite3_finalize(stmt);
    sqlite3_close(db);
    return count;
}


const char* build_video_query(const char* where_clause, char* buffer, size_t size) {
    const char* base = 
        "SELECT v.id, v.title, v.available_copies, count(r.id) < v.available_copies as rentable, count(r.id) as rented_copies "
        "FROM Videos v "
        "LEFT JOIN Rentals r ON r.video_id = v.id and r.returned_at is NULL "
        "%s "
        "GROUP BY v.id";
    snprintf(buffer, size, base, where_clause ? where_clause : "");
    return buffer;
}


int set_rental_return_date(const int rental_id){
    const char* db_path = getenv("DB_PATH");
    sqlite3* db = get_db(db_path);
    sqlite3_stmt *stmt;
    const char *sql = "UPDATE Rentals set returned_at = CURRENT_DATE WHERE id = ?";

    if (sqlite3_prepare_v2(db, sql, -1, &stmt, NULL) != SQLITE_OK) {
        fprintf(stderr, "Failed to prepare insert: %s\n", sqlite3_errmsg(db));
        sqlite3_close(db);
        return 0;
    }

    sqlite3_bind_int(stmt,1,rental_id);

    int rc = sqlite3_step(stmt);
    sqlite3_finalize(stmt);
    if (rc != SQLITE_DONE) {
        const char* err= sqlite3_errmsg(db);
        fprintf(stderr, "Update failed: %s\n", err);
        sqlite3_close(db);
        return 0;
    }

    sqlite3_close(db);
    return 1;
}

int set_rental_reminder(const int rental_id, const int reminder){
    const char* db_path = getenv("DB_PATH");
    sqlite3* db = get_db(db_path);
    sqlite3_stmt *stmt;
    const char *sql = "UPDATE Rentals set reminder = ? WHERE id = ?";

    if (sqlite3_prepare_v2(db, sql, -1, &stmt, NULL) != SQLITE_OK) {
        fprintf(stderr, "Failed to prepare UPDATE: %s\n", sqlite3_errmsg(db));
        sqlite3_close(db);
        return 0;
    }

    sqlite3_bind_int(stmt,1,reminder);
    sqlite3_bind_int(stmt,2,rental_id);

    int rc = sqlite3_step(stmt);
    sqlite3_finalize(stmt);
    if (rc != SQLITE_DONE) {
        const char* err= sqlite3_errmsg(db);
        fprintf(stderr, "Update failed: %s\n", err);
        sqlite3_close(db);
        return 0;
    }

    sqlite3_close(db);
    return 1;
}