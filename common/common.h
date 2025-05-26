#ifndef COMMON_H
#define COMMON_H


typedef struct {
    int id;
    char* title;
    int av_copies;  //available copies
    int rented_copies;
    int is_rentable;//is it possible to rent
} Video;

typedef struct {
    int id;
    int id_movie;
    char* username;
    char* due_date; 
    char* start_date; 
    char* end_date; 
    int reminder;
} Rental;


typedef struct {
    int id;
    char* username;
    Video* movie;
} Cart;


int parse_command(const char *input, char *tokens[]);
int is_date_passed(const char* date_str);
#endif 