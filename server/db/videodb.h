#ifndef VIDEO_DB_H
#define VIDEO_DB_H

#include <sqlite3.h>
#include "../../common/common.h"

int video_insert(const char *title, const int copies, char* errors);
int find_videos_by_title(const char* search_term, struct Video* results[], int max_results);
#endif 
