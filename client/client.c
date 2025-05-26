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
#define MAX_QUERY_RESULT 100

/*
    send a request (as a string) to the server listening on socket and write the response in the given array.
    Return 0 in case of error, 1 in case of success.
*/
int send_request(int socket, const char* request, char* response);
int parse_user_response(const char* response,int* usr_id,int* usr_is_admin);
int parse_add_movie_response(const char* response, int* movie_id);
Video* parse_search_movie_response_item(const char* response);
int parse_search_movie_response(const char* response);
int parse_rent_movie_response(const char* response, char* due_date, int* is_new);
Cart* parse_get_cart_response_item(const char* response);
int handle_movie_search(int socket);
int retrieve_cart(int socket, const char*username, Cart cart[], int max_results);
int retrieve_rentals(int socket, const char*username, Rental rentals[], int max_results, const int include_returned);
int return_movie(int socket, const int rent_id);
Video* retrieve_movie(int socket, const int movie_id);
int movie_list(int socket, Video videos[], int max_results);
int reminder_movie(int socket, const int rent_id, const int reminder);
void check_reminders(const int socket,const char* username);

int debug = 0;
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
    int max_rentable =0;
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

    ssize_t bytes = recv(sock, response, BUFFER_SIZE - 1, 0);
    if (bytes <= 0) {
        printf("Server unavailable.\n");
        close(sock);
        return 0;
    }
    response[bytes] = '\0';
    load_env_file(".env");
    const char* log_level=getenv("LOG_LEVEL");
    debug=log_level != NULL && strcmp(log_level,"debug") == 0;

    if (debug) printf("\n[DEBUG] Server response: %s\n\n", response);
    max_rentable=atoi(response);


    printf("Welcome to Unix Video Rental portal! by current policy users can rent up to %d videos. Have a good day\n",max_rentable);
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
        if (debug) printf("\n[DEBUG] User with id %d, logged in. is admin: %d\n\n",usr_id,usr_role);
    }

    switch (usr_role) {
        case ADMIN_ROLE:{
            char input[10];
            while(1) {
                printf("\nChoose an option:\n");
                printf("1. Insert new movie\n");
                printf("2. Movies List\n");
                printf("3. Active Rentals List\n");
                printf("4. Rentals Archive\n");
                printf("5. Send reminder\n");
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
                case '2': {
                    Video videos[MAX_QUERY_RESULT];
                    int found = movie_list(sock,videos,MAX_QUERY_RESULT);
                    for (int i=0;i<found;i++){
                        printf("ID: %d Title: \"%s\" Total Copies: %d Rented Copies: %d Available Copies: %d\n",
                        videos[i].id,videos[i].title,videos[i].av_copies,videos[i].rented_copies,videos[i].av_copies-videos[i].rented_copies);
                    }
                    break;
                }
                case '3': {
                    Rental rentals[MAX_QUERY_RESULT];
                    int found = retrieve_rentals(sock,"ALL",rentals,MAX_QUERY_RESULT,0);
                    for (int i=0;i<found;i++){
                        Video* video=retrieve_movie(sock,rentals[i].id_movie);
                        int is_expired=is_date_passed(rentals[i].due_date);
                        char* expired= is_expired ? "* This rental is overdue!" : "";
                        printf("ID: %d Username: \"%s\" Title: \"%s\" Rented at: %s Due to: %s%s\n",
                        rentals[i].id, rentals[i].username, video->title,rentals[i].start_date, rentals[i].due_date,expired);
                    }
                    break;
                }
                case '4': {
                    Rental rentals[MAX_QUERY_RESULT];
                    int found = retrieve_rentals(sock,"ALL",rentals,MAX_QUERY_RESULT,1);
                    for (int i=0;i<found;i++){
                        if (strcmp(rentals[i].end_date,"(null)")){
                            Video* video=retrieve_movie(sock,rentals[i].id_movie);
                            printf("ID: %d Username: \"%s\" Title: \"%s\" Rented at: %s Returned at: %s\n",
                            rentals[i].id, rentals[i].username, video->title,rentals[i].start_date, rentals[i].end_date);
                        }
                    }
                    break;
                }
                case '5': {
                    char choice[10];
                    printf("insert the rental id you want to send reminder for: ");
                    if (fgets(choice, sizeof(choice), stdin) == NULL) {
                        printf("choice error.\n");
                        continue;
                    }
                    reminder_movie(sock,atoi(choice),1);
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
    case USER_ROLE: {
        char input[10];
            while(1) {
                check_reminders(sock,username);
                printf("\nChoose an option:\n");
                printf("1. Search Movie\n");
                printf("2. Manage Cart\n");
                printf("3. Return Rentals\n");
                printf("4. See Rentals Archive\n");
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
                    case '1':
                        handle_movie_search(sock);
                        break;
                    case '2':{
                        Cart carts[MAX_QUERY_RESULT];
                        int nr_items = 0;
                        cart:
                        printf("\n");
                        nr_items = retrieve_cart(sock,username,carts,MAX_QUERY_RESULT);
                        if (nr_items == -1){
                            printf("Server error. check if server is up or try again.\n");
                            break;
                        }
                        printf("Your cart currently has %d items.\n",nr_items);
                        printf("Please, choose an option:\n");
                        if (nr_items > 0) printf("1. View Cart\n");
                        printf("2. Add movie to cart\n");
                        if (nr_items > 0) printf("3. Remove movie from cart\n");
                        if (nr_items > 0) printf("4. Rent movies in your cart\n");
                        printf("insert another character to go back\n");
                        printf("Enter your choice: ");
                        if (fgets(input, sizeof(input), stdin) == NULL) {
                            printf("Input error.\n");
                            continue;
                        }
                        input[strcspn(input, "\n")] = 0;
                        switch(input[0]){
                            case '1':
                                for (int i=0;i <nr_items; i++){
                                    printf("id: %d - title \"%s\"\n",carts[i].id,carts[i].movie->title);
                                }
                                break;
                            case '2': {
                                Rental rentals[MAX_QUERY_RESULT];
                                int nr = retrieve_rentals(sock,username,rentals,MAX_QUERY_RESULT,0);
                                if (nr == max_rentable ) {
                                    printf("You already have the maximum number of rented movies at the same time.\nReturn something to rent another movie!\n");
                                    break;
                                }
                                printf("Enter id of movie to add it to the cart: ");
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
                                    snprintf(request, sizeof(request), "CART ADD %s %d",username,atoi(buffer));
                                    if (!send_request(sock,request,response)) {
                                        perror("send");
                                        close(sock);
                                        return 1;
                                    }
                                }
                                break;
                            }
                            case '3': {
                                printf("Enter id of item to remove it to the cart: ");
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
                                    snprintf(request, sizeof(request), "CART DEL %d",atoi(buffer));
                                    if (!send_request(sock,request,response)) {
                                        perror("send");
                                        close(sock);
                                        return 1;
                                    }
                                }
                                break;
                            }
                            case '4': {
                                for (int i=0; i<nr_items; i++){
                                    snprintf(request, sizeof(request), "RENT ADD %d %d",usr_id,carts[i].movie->id);
                                    if (!send_request(sock,request,response)) {
                                        perror("send");
                                        close(sock);
                                        return 1;
                                    }
                                    char due_date[BUFFER_SIZE];
                                    if (strcmp(response,"KO UNAVAILABLE") == 0){
                                        printf("The movie \"%s\" is not available anymore!\n",carts[i].movie->title);
                                        continue;
                                    }
                                    if (strcmp(response,"KO MAX") == 0){
                                        printf("You already have the maximum number of rented movies at the same time.\nReturn something to rent another movie!\n");
                                        break;
                                    }
                                    int is_new;
                                    int rent_id = parse_rent_movie_response(response,due_date,&is_new);
                                    snprintf(request, sizeof(request), "CART DEL %d",carts[i].id);
                                    if (!send_request(sock,request,response)) {
                                        perror("send");
                                        close(sock);
                                        return 1;
                                    }
                                    printf("%s \"%s\" untill %s. Removed from cart\n",is_new ? "Rented" : "You already rented", carts[i].movie->title,due_date);
                                }
                                break;
                            }
                        }
                        
                        
                        break;
                    }
                    case '3' : {
                        char choice[10];
                        Rental rentals[MAX_QUERY_RESULT];
                        int nr_items = retrieve_rentals(sock,username,rentals,MAX_QUERY_RESULT,0);
                        if (nr_items == 0) {
                            printf("You have no rented movie yet.\n");
                            break;
                        }
                        printf("You currently have %d rented movies:\n",nr_items);
                       for (int i=0;i <nr_items; i++){
                            Video* video = retrieve_movie(sock,rentals[i].id_movie);
                            printf("Rental id: %d - title \"%s\" - to return due %s\n",rentals[i].id,video->title,rentals[i].due_date);
                            
                            free(video->title);
                            free(video);
                        }
                        printf("insert the rental id you want to return: ");
                        if (fgets(choice, sizeof(choice), stdin) == NULL) {
                            printf("choice error.\n");
                            continue;
                        }
                        choice[strcspn(choice, "\n")] = 0;
                        if (strcmp(choice, "q") == 0 || strcmp(choice, "Q") == 0) {
                            printf("Exiting...\n");
                        } else {
                            int ok = return_movie(sock,atoi(choice));
                            if (!ok) printf("Error returning the movie!\n");
                        }
                        break;
                    }
                    case '4': {
                        Rental rentals[MAX_QUERY_RESULT];
                        int nr_items = retrieve_rentals(sock,username,rentals,MAX_QUERY_RESULT,1);
                        printf("List of the previously rented movies:\n");
                       for (int i=0;i <nr_items; i++){
                            Video* video = retrieve_movie(sock,rentals[i].id_movie);
                            if (strcmp(rentals[i].end_date,"(null)")){
                                printf("Rental id: %d - title \"%s\" - taken on \"%s\" - returned on \"%s\"\n",rentals[i].id,video->title,rentals[i].start_date,rentals[i].end_date);
                            }
                            free(video->title);
                            free(video);
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
     if (debug) printf("\n[DEBUG] request: '%s'\n\n",request);
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
    if (debug) printf("\n[DEBUG]Server response: %s\n\n", response);
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
If there was an error it returns -1.
*/
int parse_search_movie_response(const char* response) {
    char* tokens[MAX_TOKENS];
    if (parse_command(response,tokens)>0){
        if (strcmp(tokens[0], "OK") == 0){
            return atoi(tokens[1]);
        }else{
            printf("Error: %s\n",tokens[1]);
        }
    }
    return -1;
}

int check_response_ok(const char* response) {
    char* tokens[MAX_TOKENS];
    if (parse_command(response,tokens)>0){
        return strcmp(tokens[0], "OK") == 0;
    }
    return 0;
}

int parse_rent_movie_response(const char* response, char* due_date, int* is_new) {
    char* tokens[MAX_TOKENS];
    if (parse_command(response,tokens)>0) {
        if (strcmp(tokens[0],"KO") == 0){
            *is_new = 0;
        } else {
            *is_new = 1;
        }
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
        vid->rented_copies = atoi(tokens[4]);
        return vid;
    }
    
    return NULL;
}

Cart* parse_get_cart_response_item(const char* response) {
    char* tokens[MAX_TOKENS];
    Cart* cart = malloc(sizeof(Cart));
    cart->movie = malloc(sizeof(Video));
    if (parse_command(response,tokens)>0) {
        cart->id=atoi(tokens[0]);
        cart->username=tokens[1];
        cart->movie->id=atoi(tokens[2]);
        const char* title = tokens[3];
        cart->movie->title = strdup((const char*)title);;
        cart->movie->av_copies= atoi(tokens[4]);
        cart->movie->is_rentable= atoi(tokens[5]);
        return cart;
    }
    
    return NULL;
}

int handle_movie_search(int socket){
    char title[BUFFER_SIZE];
    char request[BUFFER_SIZE];
    char response[BUFFER_SIZE];
    int nr = 0;
    printf("Enter word to search in the title: ");
    if (fgets(title, sizeof(title), stdin) == NULL) {
        printf("No title input.\n");
        return 0;
    }
    size_t len = strlen(title);
    if (len > 0 && title[len-1] == '\n') {
        title[len-1] = '\0';
    }
    snprintf(request, sizeof(request), "MOVIE SEARCH %s",title);
    if (!send_request(socket,request,response)) {
        perror("send");
        return 0;
    }else{
        char item_resp[BUFFER_SIZE];
        int found = parse_search_movie_response(response);
        printf("Found %d results:\n",found);
        for (int i = 0; i < found; ++i) {
            if (!send_request(socket,"NEXT",item_resp)) {
                perror("send");
                return 0;
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
    }
    return 1;
}

int movie_list(int socket, Video videos[], int max_results){
    char title[BUFFER_SIZE];
    char request[BUFFER_SIZE];
    char response[BUFFER_SIZE];
    int found = 0;
    snprintf(request, sizeof(request), "MOVIE SEARCH %c",'%');
    if (!send_request(socket,request,response)) {
        perror("send");
        return 0;
    }else{
        char item_resp[BUFFER_SIZE];
        found = parse_search_movie_response(response);
        for (int i = 0; i < found; ++i) {
            if (!send_request(socket,"NEXT",item_resp)) {
                perror("send");
                return 0;
            }
            Video* vid = parse_search_movie_response_item(item_resp);
            if (vid == NULL){
                printf("Error parsing video number %d\n",i);
            } else {
                videos[i]=*vid;
            }

        }
    }
    return found;
}

int retrieve_cart(int socket, const char*username, Cart cart[], int max_results){
    char request[BUFFER_SIZE];
    char response[BUFFER_SIZE];
    snprintf(request, sizeof(request), "CART GET %s",username);
    if (!send_request(socket,request,response)) {
        perror("send");
        return -1;
    }
    char item_resp[max_results];
    int found = parse_search_movie_response(response);
    for (int i=0; i<found; i++){
        snprintf(request, sizeof(request), "NEXT");
        if (!send_request(socket,request,response)) {
            perror("send");
            return -1;
        }
        Cart* item=parse_get_cart_response_item(response);
        cart[i]=*item;
    }
    return found;
}

Rental* parse_search_rental_response_item(const char* response) {
    char* tokens[MAX_TOKENS];
    Rental* rental = malloc(sizeof(Rental));
    if (parse_command(response,tokens)>0) {
        rental->id=atoi(tokens[0]);
        rental->username=tokens[1];
        rental->id_movie=atoi(tokens[2]);
        rental->reminder=atoi(tokens[6]);
        const char* start_date = tokens[3];
        rental->start_date= strdup((const char*)start_date);
        const char* due_date = tokens[4];
        rental->due_date= strdup((const char*)due_date);
        const char* end_date = tokens[5];
        rental->end_date= strdup((const char*)end_date);
        return rental;
    }
    
    return NULL;
}

int retrieve_rentals(int socket, const char*username, Rental rentals[], int max_results, const int include_returned){
    char request[BUFFER_SIZE];
    char response[BUFFER_SIZE];
    snprintf(request, sizeof(request), "RENT SEARCH %s %d",username, include_returned);
    if (!send_request(socket,request,response)) {
        perror("send");
        return -1;
    }
    char item_resp[max_results];
    int found = parse_search_movie_response(response);
    for (int i=0; i<found; i++){
        snprintf(request, sizeof(request), "NEXT");
        if (!send_request(socket,request,response)) {
            perror("send");
            return -1;
        }
        Rental* item=parse_search_rental_response_item(response);
        rentals[i]=*item;
    }
    return found;
}

Video* retrieve_movie(int socket, const int movie_id) {
    char request[BUFFER_SIZE];
    char response[BUFFER_SIZE];
    snprintf(request, sizeof(request), "MOVIE GET %d",movie_id);
    if (!send_request(socket,request,response)) {
        perror("send");
        return NULL;
    }else {
        return parse_search_movie_response_item(response);
    }
}

int return_movie(int socket, const int rent_id) {
    char request[BUFFER_SIZE];
    char response[BUFFER_SIZE];
    snprintf(request, sizeof(request), "RENT PUT %d",rent_id);
    if (!send_request(socket,request,response)) {
        perror("send");
        return 0;
    }else {
        return check_response_ok(response);
    }
}

int reminder_movie(int socket, const int rent_id, const int reminder) {
    char request[BUFFER_SIZE];
    char response[BUFFER_SIZE];
    snprintf(request, sizeof(request), "RENT REMIND %d %d",rent_id,reminder);
    if (!send_request(socket,request,response)) {
        perror("send");
        return 0;
    }else {
        return check_response_ok(response);
    }
}



void check_reminders(const int socket,const char* username) {
    Rental rentals[MAX_QUERY_RESULT];
    char buffer[BUFFER_SIZE];
    int nr = retrieve_rentals(socket,username,rentals,MAX_QUERY_RESULT,0);
    for (int i=0;i<nr;i++){
        if (rentals[i].reminder == 1) {
            Video *video = retrieve_movie(socket,rentals[i].id_movie);
            printf("\n");
            snprintf(buffer,sizeof(buffer),"Attention! don't forget to return the movie \"%s\" before %s",video->title,rentals[i].due_date);
            printf("%s\n",buffer);
            reminder_movie(socket,rentals[i].id,0);
        }
    }
}