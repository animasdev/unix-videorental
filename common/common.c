#include <stdio.h>
#include <stdlib.h>
#include <string.h>
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