#ifndef DB_H
#define DB_H

#include <sqlite3.h>

// crea le tabelle necessarie per il funzionamente del programma



int setup_db();
int insert_user();
// autenticazione utente
int check_user_login( const char *username, const char *password);

// registrazione utente
int register_user( const char *username, const char *password);

// Check if a video is available for rent
int rent_video( int video_id, const char *username);

// Return a rented video
int return_video( int video_id, const char *username);

#endif 
