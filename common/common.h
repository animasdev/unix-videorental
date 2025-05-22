#ifndef COMMON_H
#define COMMON_H


typedef struct {
    int id;
    char* title;
    int av_copies; //available copies
    int rt_copies; //rented out copies
} Video;

typedef struct {
    int id;
    int id_movie;
    char* username;
    char* due_date; 
    char* start_date; 
} Rental;

int parse_command(const char *input, char *tokens[]);

#endif 