#ifndef COMMON_H
#define COMMON_H


struct Video {
    int id;
    char* title;
    int av_copies; //available copies
    int rt_copies; //rented out copies
};

int parse_command(const char *input, char *tokens[]);

#endif 