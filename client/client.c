#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <sys/un.h>

#define SOCKET_PATH "/tmp/mysocket"
#define BUFFER_SIZE 256

/*
    send a request (as a string) to the server listening on socket and write the response in the given array.
    Return 0 in case of error, 1 in case of success.
*/
int send_request(int socket, const char* request, char* response);

int main() {
    int sock;
    struct sockaddr_un addr;
    char buffer[BUFFER_SIZE];
    char request[BUFFER_SIZE];
    char response[BUFFER_SIZE];
    int usr_id = 0;
    int usr_role = 0;
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

    printf("Enter username: ");
    if (fgets(username, sizeof(username), stdin) == NULL) {
        printf("No username input.\n");
        close(sock);
        return 1;
    }

    size_t len = strlen(username);
    if (len > 0 && username[len-1] == '\n') {
        username[len-1] = '\0';
    }

    snprintf(request, sizeof(request), "CHECK USER %s", username);

    if (!send_request(sock,request,response)) {
        perror("send");
        close(sock);
        return 1;
}
    if (strcmp(response, "LOGIN") == 0) { 
        printf("Enter password: ");
        if (fgets(buffer, sizeof(buffer), stdin) == NULL) {
            printf("No password input.\n");
            close(sock);
            return 1;
        }
        // Remove trailing newline
        size_t len = strlen(buffer);
        if (len > 0 && buffer[len-1] == '\n') {
            buffer[len-1] = '\0';
        }
        snprintf(request, sizeof(request), "LOGIN %s %s",username, buffer);
        if (!send_request(sock,request,response)) {
            perror("send");
            close(sock);
            return 1;
        }
        
    }
    else if (strcmp(buffer, "REGISTER") == 0) {
        printf("New user! Choose password: ");
        if (fgets(buffer, sizeof(buffer), stdin) == NULL) {
            printf("No password input.\n");
            close(sock);
            return 1;
        }
        // Remove trailing newline
        size_t len = strlen(buffer);
        if (len > 0 && buffer[len-1] == '\n') {
            buffer[len-1] = '\0';
        }
        snprintf(request, sizeof(request), "REGISTER %s %s",username, buffer);
        if (!send_request(sock,request,response)) {
            perror("send");
            close(sock);
            return 1;
        }
    }

    close(sock);
    return 0;
}

int send_request(int socket, const char* request, char* response){
    printf("request: '%s'\n",request);
    if (send(socket, request, strlen(request), 0) == -1) {
        perror("send");
        close(socket);
        return 0;
    }

    ssize_t bytes = recv(socket, response, sizeof(response) - 1, 0);
    if (bytes <= 0) {
        printf("Server disconnected or no response.\n");
        close(socket);
        return 0;
    }
    response[bytes] = '\0';
    printf("Server response: %s\n", response);
    return 1;
}