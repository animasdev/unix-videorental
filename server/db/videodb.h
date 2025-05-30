#ifndef VIDEO_DB_H
#define VIDEO_DB_H

#include <sqlite3.h>
#include "../../common/common.h"

int video_insert(const char *title, const int copies, char* errors);
int find_videos_by_title(const char* search_term,Video* results[], int max_results);
Video* find_video_by_id(const int id);
int rent_video(const char* username,const int movie_id);
Rental* find_rental_by_id(int rental_id);
int find_rentals_by_username(const char* username, Rental* results[], int max_results, int include_returned);
Rental* find_rental_by_username_and_movie(const char* username, const int movie_id);
int set_rental_return_date(const int rental_id);
int find_all_rentals(Rental* results[], int max_results, int include_returned);
int set_rental_reminder(const int rental_id, const int reminder);
#endif 
