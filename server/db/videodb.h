#ifndef VIDEO_DB_H
#define VIDEO_DB_H

#include <sqlite3.h>

int video_insert(const char *title, const int copies);

#endif 
