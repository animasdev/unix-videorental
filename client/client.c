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
#define RENTABLE 5

/*
    send a request (as a string) to the server listening on socket and write the response in the given array.
    Return 0 in case of error, 1 in case of success.
*/
int send_request(int socket, const char* request, char* response);
int parse_user_response(const char* response,int* usr_id,int* usr_is_admin);
int parse_add_movie_response(const char* response, int* movie_id);
Video* parse_search_movie_response_item(const char* response);
int parse_search_movie_response(const char* response);
int parse_rent_movie_response(const char* response, char* due_date);

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
    else if (strcmp(response, "REGISTER") == 0) {
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
                    }else{
                        int movie_id = 0;
                        if (parse_add_movie_response(response,&movie_id)) {
                            printf("added movie \"%s\" with id %d, number of copies: %s\n",title,movie_id,buffer);
                        }
                    }
                    break;
                }
                case '2':
                    printf("You chose option 2 (fff)\n");
                    break;
                case 'q':
                case 'Q':
                    exit(0);
                default:
                    printf("Invalid choice. Please try again.\n");
            }
        }
        break;
    }
    case USER_ROLE: {
        char input[10];
            while(1) {
                printf("\nChoose an option:\n");
                printf("1. Search movie\n");
                printf("2. Rent movie\n");
                printf("3. Return movie\n");
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
                        printf("Enter word to search in the title: ");
                        if (fgets(title, sizeof(title), stdin) == NULL) {
                            printf("No title input.\n");
                            close(sock);
                            return 1;
                        }
                        size_t len = strlen(title);
                        if (len > 0 && title[len-1] == '\n') {
                            title[len-1] = '\0';
                        }
                        snprintf(request, sizeof(request), "MOVIE SEARCH %s",title);
                        if (!send_request(sock,request,response)) {
                            perror("send");
                            close(sock);
                            return 1;
                        }else{
                            char item_resp[BUFFER_SIZE];
                            int found = parse_search_movie_response(response);
                            printf("Found %d results:\n",found);
                            for (int i = 0; i < found; ++i) {
                                if (!send_request(sock,"NEXT",item_resp)) {
                                    perror("send");
                                    close(sock);
                                    return 1;
                                }
                                Video* vid = parse_search_movie_response_item(item_resp);
                                if (vid == NULL){
                                    printf("Error parsing video number %d\n",i);
                                } else {
                                    printf("%d. %s currently available?: %s \n",
                                        vid->id,vid->title,vid->is_rentable == 1 ? "yes" : "no");
                                    free(vid->title);
                                    free(vid);
                                }
                            }
                            printf("ok\n");
                        }
                        break;
                    }
                    case '2':{
                        int wanted[RENTABLE];
                        int current = 0;
                        printf("You can rent %d movies at once. Insert 'q' to stop adding\n",RENTABLE);
                        while (current < RENTABLE){
                            printf("Enter number of movie to add it to the cart: ");
                            if (fgets(buffer, sizeof(buffer), stdin) == NULL) {
                                printf("No number input.\n");
                                close(sock);
                                return 1;
                            }
                            size_t len = strlen(buffer);
                            if (len > 0 && buffer[len-1] == '\n') {
                                buffer[len-1] = '\0';
                            }
                            if (strcmp(buffer,"q") == 0){
                                break;
                            }else {
                                printf("inserted %d\n",atoi(buffer));
                                wanted[current]=atoi(buffer);
                                printf("inserted %d\n",wanted[current]);
                                current++;
                            }
                        }
                        if (current == 0){
                            printf("No movies inserted.\n");
                            break;
                        }
                        cart:
                        printf("\nYour current cart:\n");
                        for (int i=0; i<current; i++){
                            if (wanted[i] == -1) continue;
                            snprintf(request, sizeof(request), "MOVIE GET %d",wanted[i]);
                            if (!send_request(sock,request,response)) {
                                perror("send");
                                close(sock);
                                return 1;
                            }
                            const Video* video=parse_search_movie_response_item(response);
                            printf("Nr.: %d Title: \"%s\"\n",i,video->title);
                        }
                        printf("Options:\n");
                        printf("1. Remove movie from cart\n");
                        printf("2. Send rent request for full cart\n");
                        printf("insert another character to go back\n");
                        printf("Enter your choice: ");
                        if (fgets(input, sizeof(input), stdin) == NULL) {
                            printf("Input error.\n");
                            continue;
                        }
                        input[strcspn(input, "\n")] = 0;
                        switch(input[0]){
                            case '1': {
                                printf("Enter the Nr. to remove: ");
                                if (fgets(input, sizeof(input), stdin) == NULL) {
                                    printf("Input error.\n");
                                    continue;
                                }
                                int nr = atoi(input);
                                if (nr>current){
                                    printf("Invalid choice\n");
                                }else{
                                    wanted[nr]=-1;
                                    printf("Removed!\n");
                                    goto cart;
                                }
                                break;
                            }
                            case '2': {
                                for (int i=0; i<current; i++){
                                    if (wanted[i] == -1) continue;
                                    snprintf(request, sizeof(request), "RENT ADD %d %d",usr_id,wanted[i]);
                                    if (!send_request(sock,request,response)) {
                                        perror("send");
                                        close(sock);
                                        return 1;
                                    }
                                    char due_date[BUFFER_SIZE];
                                    if (strcmp(response,"KO") == 0){
                                        printf("The movie with id %d is not available anymore!",wanted[i]);
                                        continue;
                                    }
                                    int rent_id = parse_rent_movie_response(response,due_date);
                                    snprintf(request, sizeof(request), "MOVIE GET %d",wanted[i]);
                                    if (!send_request(sock,request,response)) {
                                        perror("send");
                                        close(sock);
                                        return 1;
                                    }
                                    const Video* video=parse_search_movie_response_item(response);
                                    printf("Rented \"%s\" untill %s\n",video->title,due_date);
                                }
                                break;
                            }
                        }
                        
                        
                        break;
                    }
                    case 'q':
                    case 'Q':
                        exit(0);
                    default:
                        printf("Invalid choice. Please try again.\n");
                }
            }
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

    ssize_t bytes = recv(socket, response, BUFFER_SIZE - 1, 0);
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
    char** tokens;
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

int parse_add_movie_response(const char* response, int* movie_id) {
    char* tokens[MAX_TOKENS];
    if (parse_command(response,tokens)>0){
        if (strcmp(tokens[0], "OK") == 0){
            const int id = atoi(tokens[1]);
            (*movie_id)=id;
            return 1;
        }else{
            printf("Error: %s\n",tokens[1]);
        }
    }
    return 0;
}

/*
Parses the response for the search movie request. The expected response is 'OK n'
where n is the number of records found, which gets returned to the caller.
If there was an error it returs -1.
*/
int parse_search_movie_response(const char* response) {
    char* tokens[MAX_TOKENS];
    if (parse_command(response,tokens)>0){
        printf("token[0]: %s\n",tokens[0]);
        printf("token ok?: %d\n",strcmp(tokens[0], "OK") == 0);
        if (strcmp(tokens[0], "OK") == 0){
            return atoi(tokens[1]);
        }else{
            printf("Error: %s\n",tokens[1]);
        }
    }
    return -1;
}


int parse_rent_movie_response(const char* response, char* due_date) {
    char* tokens[MAX_TOKENS];

    if (parse_command(response,tokens)>0) {
        const int rent_id = atoi(tokens[1]);
        snprintf(due_date, BUFFER_SIZE, "%s", tokens[2]);
        return rent_id;
    }
    return -1;
}

Video* parse_search_movie_response_item(const char* response) {
    char* tokens[MAX_TOKENS];
    Video* vid = malloc(sizeof(Video));
    if (parse_command(response,tokens)>0) {
        const char* title = tokens[1];
        vid->title = strdup((const char*)title);
        vid->id = atoi(tokens[0]);
        vid->av_copies = atoi(tokens[2]);
        vid->is_rentable = atoi(tokens[3]);
        return vid;
    }
    
    return NULL;
}