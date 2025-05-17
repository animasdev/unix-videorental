// server_local_fork.c
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <signal.h>

#define SOCKET_PATH "/tmp/mysocket"
#define BUFFER_SIZE 1024
#define MAX_TOKENS 100 
#define MAX_TOKEN_LEN 100 


void cleanup(int signum);
int user_exists(const char *username);
int parse_command(const char *input, char *tokens[]);
int command_type(const char *cmd);

void handle_client(int client_fd) {
    char buffer[BUFFER_SIZE];
    ssize_t bytes_read;

    while ((bytes_read = read(client_fd, buffer, BUFFER_SIZE - 1)) > 0) {
        buffer[bytes_read] = '\0';
        printf("Client (%d) says: %s\n", client_fd, buffer);

        const char *prefix_check = "check user ";
        const char *prefix_login = "LOGIN ";
        char **tokens;
        if (parse_command(buffer,tokens) ) {printf("parse token ok\n");}
        switch (command_type(tokens[0]))
        {
        case 1:
            printf("check\n");
            break;
        
        default:
            break;
        }
        if (strncmp(buffer, prefix_check, strlen(prefix_check)) == 0) {
            char *username = buffer + strlen(prefix_check);
            size_t len = strlen(username);
            if (len > 0 && username[len - 1] == '\n') {
                username[len - 1] = '\0';
            }

            if (user_exists(username)) {
                send(client_fd, "LOGIN", 5, 0);
            } else {
                send(client_fd, "REGISTER", 9, 0);
            }
        }
        else if (strncmp(buffer, prefix_login, strlen(prefix_login)) == 0) {
            send(client_fd, "LOGIN OK\n", 9, 0);
        }
        else {
            const char *msg = "ERROR: Unknown command\n";
            send(client_fd, msg, strlen(msg), 0);
        }
    }

    if (bytes_read == 0) {
        printf("Client (%d) disconnected.\n", client_fd);
    } else {
        perror("read");
    }
    close(client_fd);
    exit(0);
}

int main() {
    int server_fd, client_fd;
    struct sockaddr_un addr;

    signal(SIGINT, cleanup);

    server_fd = socket(AF_LOCAL, SOCK_STREAM, 0);
    if (server_fd == -1) {
        perror("socket");
        exit(EXIT_FAILURE);
    }

    unlink(SOCKET_PATH);

    memset(&addr, 0, sizeof(addr));
    addr.sun_family = AF_LOCAL;
    strncpy(addr.sun_path, SOCKET_PATH, sizeof(addr.sun_path) - 1);

    if (bind(server_fd, (struct sockaddr *)&addr, sizeof(addr)) == -1) {
        perror("bind");
        close(server_fd);
        exit(EXIT_FAILURE);
    }

    if (listen(server_fd, 5) == -1) {
        perror("listen");
        close(server_fd);
        exit(EXIT_FAILURE);
    }

    printf("Server listening at %s\n", SOCKET_PATH);

    while (1) {
        client_fd = accept(server_fd, NULL, NULL);
        if (client_fd == -1) {
            perror("accept");
            continue;
        }

        pid_t pid = fork();
        if (pid < 0) {
            perror("fork");
            close(client_fd);
        } else if (pid == 0) {
            close(server_fd);
            handle_client(client_fd);
        } else {
            close(client_fd);
        }
    }
    return 0;
}

int parse_command(const char *input, char *tokens[]) {
    char *copy = strdup(input);  // make a modifiable copy
    if (!copy) {
        perror("strdup");
        return -1;
    }

    int count = 0;
    char *token = strtok(copy, " \t\n");
    while (token != NULL && count < MAX_TOKENS) {
        printf("%s\n",token);
        tokens[count] = strdup(token);  // allocate and copy each token
        if (!tokens[count]) {
            perror("strdup");
            break;
        }
        count++;
        token = strtok(NULL, " \t\n");
        
    }

    free(copy);  // original copy is no longer needed
    return count;
}

int command_type(const char *cmd) {
    if (strcmp(cmd, "CHECK") == 0) return 1;
    if (strcmp(cmd, "LOGIN") == 0) return 2;
    if (strcmp(cmd, "REGISTER") == 0) return 3;
    if (strcmp(cmd, "LIST") == 0) return 4;
    if (strcmp(cmd, "RENT") == 0) return 5;
    if (strcmp(cmd, "CLOSE") == 0) return 6;
    return 0;
}

int user_exists(const char *username) {
    return strcmp(username, "alice") == 0;
}

void cleanup(int signum) {
    unlink(SOCKET_PATH);
    exit(0);
}
