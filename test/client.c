// client_local.c (updated to receive responses)
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <sys/un.h>

#define SOCKET_PATH "/tmp/mysocket"
#define BUFFER_SIZE 1024

int main() {
    int sock;
    struct sockaddr_un addr;
    char buffer[BUFFER_SIZE];
    char response[BUFFER_SIZE];

    sock = socket(AF_LOCAL, SOCK_STREAM, 0);
    if (sock == -1) {
        perror("socket");
        exit(EXIT_FAILURE);
    }

    memset(&addr, 0, sizeof(addr));
    addr.sun_family = AF_LOCAL;
    strncpy(addr.sun_path, SOCKET_PATH, sizeof(addr.sun_path) - 1);

    if (connect(sock, (struct sockaddr *)&addr, sizeof(addr)) == -1) {
        perror("connect");
        close(sock);
        exit(EXIT_FAILURE);
    }

    printf("Connected to server. Type messages:\n");

    while (fgets(buffer, BUFFER_SIZE, stdin) != NULL) {
        send(sock, buffer, strlen(buffer), 0);

        int bytes = recv(sock, response, BUFFER_SIZE - 1, 0);
        if (bytes > 0) {
            response[bytes] = '\0';
            printf("Server replied: %s", response);
        }
    }

    close(sock);
    return 0;
}
