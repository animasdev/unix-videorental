#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <sys/un.h>

#define SOCKET_PATH "/tmp/mysocket"
#define BUFFER_SIZE 256

int main() {
    int sock;
    struct sockaddr_un addr;
    char buffer[BUFFER_SIZE];
    char input[100];
    char username[100];

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

    // Prompt for username
    printf("Enter username: ");
    if (fgets(username, sizeof(username), stdin) == NULL) {
        printf("No username input.\n");
        close(sock);
        return 1;
    }

    // Remove trailing newline
    size_t len = strlen(username);
    if (len > 0 && username[len-1] == '\n') {
        username[len-1] = '\0';
    }

    // Build the message "check user <username>"
    snprintf(buffer, sizeof(buffer), "check user %s", username);

    // Send it to server
    if (send(sock, buffer, strlen(buffer), 0) == -1) {
        perror("send");
        close(sock);
        return 1;
    }

    // Wait for response from server
    ssize_t bytes = recv(sock, buffer, sizeof(buffer) - 1, 0);
    if (bytes <= 0) {
        printf("Server disconnected or no response.\n");
        close(sock);
        return 1;
    }

    buffer[bytes] = '\0';
    printf("Server response: %s\n", buffer);
    printf("strcmp(buffer, LOGIN) %d\n",strcmp(buffer, "LOGIN"));
    // Now based on server response you can proceed...
    if (strcmp(buffer, "LOGIN") == 0) { 
        printf("Enter password: ");
        if (fgets(input, sizeof(input), stdin) == NULL) {
            printf("No username input.\n");
            close(sock);
            return 1;
        }
        // Remove trailing newline
        size_t len = strlen(input);
        if (len > 0 && input[len-1] == '\n') {
            input[len-1] = '\0';
        }
        snprintf(buffer, sizeof(buffer), "LOGIN %s %s",username, input);
        if (send(sock, buffer, strlen(buffer), 0) == -1) {
        perror("send");
        close(sock);
        return 1;
        }

        // Wait for response from server
        ssize_t bytes = recv(sock, buffer, sizeof(buffer) - 1, 0);
        if (bytes <= 0) {
            printf("Server disconnected or no response.\n");
            close(sock);
            return 1;
        }

        buffer[bytes] = '\0';
        printf("Server response: %s\n", buffer);
    }

    close(sock);
    return 0;
}
