#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#define MAX_TOKENS 100

/*
split the given input in tokens by whitespaces. use doublquotes (") to parse sentences in one token.
If a doublequote is open and not closed, the rest of the input is gonna be considered part of the token that opened it.
*/
int parse_command(const char *input, char *tokens[]) {
    char *copy = strdup(input);
    if (!copy) {
        perror("strdup");
        return -1;
    }

    int count = 0;
    char *p = copy;
    while (*p && count < MAX_TOKENS) {
        // Skip leading whitespace
        while (*p == ' ' || *p == '\t' || *p == '\n') p++;

        if (*p == '\0') break;

        if (*p == '"') {
            p++;
            char *start = p;
            while (*p && *p != '"') p++;
            if (*p == '"') {
                *p = '\0';
                p++;
            } else {
                fprintf(stderr, "Warning: unterminated quote, auto-closing\n");
            }
            tokens[count++] = strdup(start);
        } else {
            char *start = p;
            while (*p && *p != ' ' && *p != '\t' && *p != '\n') p++;
            if (*p) *p++ = '\0';
            tokens[count++] = strdup(start);
        }
    }

    free(copy);
    return count;
}
/*
Returns 1 if the given date in format Y-m-d is passed; 0 otherwise.
*/
int is_date_passed(const char* date_str) {
    struct tm input_tm = {0};
    if (strptime(date_str, "%Y-%m-%d", &input_tm) == NULL) {
        fprintf(stderr, "Invalid date format: %s\n", date_str);
        return -1; 
    }

    time_t input_time = mktime(&input_tm);
    if (input_time == -1) {
        fprintf(stderr, "Failed to convert input date\n");
        return -1;
    }

    time_t now = time(NULL);
    if (now == -1) {
        perror("time");
        return -1;
    }
    return difftime(now, input_time) > 0;
}