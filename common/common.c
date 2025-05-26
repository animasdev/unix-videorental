#define _XOPEN_SOURCE
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>    
#include <fcntl.h>    
#include <sys/types.h>
#include <sys/stat.h>
#include <time.h>
#define MAX_LINE 256
#define MAX_ENV 64
#define MAX_TOKENS 100


char* strdup(const char* s){
    char* copy = malloc(strlen(s)+1);
    if (copy) strcpy(copy,s);
}
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


void load_env_file(const char *filename) {
    int fd = open(filename, O_RDONLY, 0);
    if (fd < 0) {
        perror("Failed to open .env file");
        return;
    }

    char buffer[4096];
    ssize_t bytes_read = read(fd, buffer, sizeof(buffer) - 1);
    if (bytes_read < 0) {
        perror("Failed to read .env file");
        close(fd);
        return;
    }
    buffer[bytes_read] = '\0';
    close(fd);

    char *line = strtok(buffer, "\n");
    while (line != NULL) {
        if (line[0] == '#' || line[0] == '\0') {
            line = strtok(NULL, "\n");
            continue;
        }

        char *equal = strchr(line, '=');
        if (!equal) {
            line = strtok(NULL, "\n");
            continue;
        }

        *equal = '\0';
        char *key = line;
        char *value = equal + 1;
        
        char *env_entry = malloc(strlen(key) + strlen(value) + 2);
        if (!env_entry) {
            perror("malloc failed");
            return;
        }

        sprintf(env_entry, "%s=%s", key, value);
        putenv(env_entry);

        line = strtok(NULL, "\n");
    }
}