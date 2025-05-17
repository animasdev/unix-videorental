// server_local_fork.c
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <signal.h>
#include "videodb.h"

#define SOCKET_PATH "/tmp/mysocket"
#define BUFFER_SIZE 1024
#define RESPONSE "Message received\n"

void cleanup(int signum) {
    unlink(SOCKET_PATH);
    exit(0);
}

void handle_client(int client_fd) {
    char buffer[BUFFER_SIZE];
    int bytes_read;

    while ((bytes_read = read(client_fd, buffer, BUFFER_SIZE)) > 0) {
        buffer[bytes_read] = '\0';
        printf("Client (%d) says: %s", client_fd, buffer);

        if (send(client_fd, RESPONSE, strlen(RESPONSE), 0) < 0) {
            perror("send");
            break;
        }
    }

    printf("Client (%d) disconnected.\n", client_fd);
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
