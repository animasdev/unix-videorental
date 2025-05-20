#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <sys/un.h>
#include "../common/common.h"

#define SOCKET_PATH "/tmp/mysocket"
#define BUFFER_SIZE 256
#define MAX_TOKENS 100 
#define ADMIN_ROLE 1
#define USER_ROLE 0
/*
    send a request (as a string) to the server listening on socket and write the response in the given array.
    Return 0 in case of error, 1 in case of success.
*/
int send_request(int socket, const char* request, char* response);
int parse_user_response(const char* response,int* usr_id,int* usr_is_admin);
int main() {
    int sock;
    struct sockaddr_un addr;
    char title[BUFFER_SIZE];
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
    if (parse_user_response(response,&usr_id,&usr_role)){
        printf("User with id %d, logged in. is admin: %d\n",usr_id,usr_role);
    }

    switch (usr_role) {
        case ADMIN_ROLE:{
            char input[10];
            while(1) {
                printf("\nChoose an option:\n");
                printf("1. Insert new movie\n");
                printf("2. Update existing movie\n");
                printf("3. List expired rentals\n");
                printf("q. Quit\n");
                printf("Enter your choice: ");
                if (fgets(input, sizeof(input), stdin) == NULL) {
                    printf("Input error.\n");
                    continue;
                }
                input[strcspn(input, "\n")] = 0;
                if (strcmp(input, "q") == 0 || strcmp(input, "Q") == 0) {
                    printf("Exiting...\n");
                }
                switch (input[0]) {
                case '1': {
                    int nr = 0;
                    printf("Enter title: ");
                    if (fgets(title, sizeof(title), stdin) == NULL) {
                        printf("No title input.\n");
                        close(sock);
                        return 1;
                    }
                    size_t len = strlen(title);
                    if (len > 0 && title[len-1] == '\n') {
                        title[len-1] = '\0';
                    }
                    printf("Enter number of copies: ");
                    if (fgets(buffer, sizeof(buffer), stdin) == NULL) {
                        printf("No number input.\n");
                        close(sock);
                        return 1;
                    }
                    size_t len2 = strlen(buffer);
                    if (len2 > 0 && buffer[len2-1] == '\n') {
                        buffer[len2-1] = '\0';
                    }
                    snprintf(request, sizeof(request), "MOVIE ADD %s %s",title, buffer);
                    if (!send_request(sock,request,response)) {
                        perror("send");
                        close(sock);
                        return 1;
                    }
                    break;
                }
                case '2':
                    printf("You chose option 2 (fff)\n");
                    break;
                default:
                    printf("Invalid choice. Please try again.\n");
            }
        }
        break;
    }
    case USER_ROLE: {
        break;
    }
    default:
        printf("Role %d not recognized!\n",usr_role);
        break;
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

int parse_user_response(const char* response,int* usr_id,int* usr_is_admin){
    const char** tokens;
    if (parse_command(response,tokens)>0){
        if (strcmp(tokens[0], "OK") == 0){
            const int id = atoi(tokens[1]);
            const int is_admin = atoi(tokens[2]);
            (*usr_id)=id;
            (*usr_is_admin)=is_admin;
            return 1;
        }
    }
    return 0;
}