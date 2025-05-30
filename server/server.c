#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <signal.h>
#include "userdb.h"
#include "videodb.h"
#include "cartdb.h"
#include "db.h"
#include "../common/common.h"

#define SOCKET_PATH "/tmp/mysocket"
#define BUFFER_SIZE 1024
#define MAX_TOKENS 100 
#define MAX_TOKEN_LEN 100 
#define ERR_SIZE 256
#define MAX_QUERY_RESULTS 100

#define CHECK_COMMAND 1
#define LOGIN_COMMAND 2
#define REGISTER_COMMAND 3 
#define LIST_COMMAND 4
#define RENT_COMMAND 5
#define CLOSE_COMMAND 6
#define MOVIE_COMMAND 7
#define CART_COMMAND 8
void cleanup(int signum);

int parse_command(const char *input, char *tokens[]);
int command_type(const char *cmd);
int db_setup(const char* db_path);
int handle_movie_command(char** tokens, int client_fd);
int handle_cart_command(char** tokens, int client_fd);
int handle_rent_command(char** tokens, int client_fd, int max_rentable);

void handle_client(int client_fd) {
    char buffer[BUFFER_SIZE];
    ssize_t bytes_read;

    const char *max_rentable = getenv("MAX_RENTABLE");
    send(client_fd,max_rentable,strlen(max_rentable),0);
    while ((bytes_read = read(client_fd, buffer, BUFFER_SIZE - 1)) > 0) {
        buffer[bytes_read] = '\0';
        printf("Client (%d) says: %s\n", client_fd, buffer);

        char *tokens[MAX_TOKENS] = {0};
        if (parse_command(buffer,tokens)>0) {printf("parse token ok\n");}
        printf("First token: '%s'\n", tokens[0]);
        printf("second token: '%s'\n", tokens[1]);
        printf("third token: '%s'\n", tokens[2]);
        const int cmd_type = command_type(tokens[0]);
        printf("identified cmd %d\n",cmd_type);
        switch (cmd_type) {
            case CHECK_COMMAND:{
                char* username = tokens[2];
                printf("extracted username '%s'\n",username);
                size_t len = strlen(username);
                if (len > 0 ) {
                    username[strcspn(username, "\n")] = 0;
                }
                printf("extracted username '%s'\n",username);
                if (user_exists(username) > -1) {
                    send(client_fd, "LOGIN", 5, 0);
                } else {
                    send(client_fd, "REGISTER", 9, 0);
                }
                break;
            }
            case REGISTER_COMMAND:{
                int new_id = user_register(tokens[1],tokens[2]);
                if (new_id > -1){
                    char response[100];
                    snprintf(response, sizeof(response), "OK %d 0", new_id);
                    send(client_fd, response, strlen(response), 0);
                    printf("registered new user %s with id %d\n",tokens[1],new_id);
                }else {
                    printf("error registering new user %s %s\n",tokens[1],tokens[2]);
                }
                break;
            }
            case LOGIN_COMMAND: {
                char* username = tokens[1];
                char* pass = tokens[2];
                const int id = user_exists(username);
                if (id != -1 && user_login(id,pass)){
                    char response[100];
                    int role = user_is_admin(id);
                    if (role == -1){
                        printf("Error determining role for user id %d",id);
                    }
                    snprintf(response, sizeof(response), "OK %d %d", id, role);
                    send(client_fd, response, strlen(response), 0);
                } else {
                    send(client_fd, "KO", 2, 0);
                }
                break;
            }
            case MOVIE_COMMAND: {
                if (!handle_movie_command(tokens,client_fd)){
                    printf("Error handling movie subcommand\n");
                }
                break;
            }
            case RENT_COMMAND: {
                int max=atoi(max_rentable);
                if (!handle_rent_command(tokens,client_fd,max)){
                    printf("Error handling rent subcommand\n");
                }
                break;
            }
            case CART_COMMAND: {
                if (!handle_cart_command(tokens,client_fd)){
                    printf("Error handling cart subcommand\n");
                }
                break;
            }
            default:{
                const char *msg = "ERROR: Unknown command\n";
                printf("%s\n",msg);
                send(client_fd, msg, strlen(msg), 0);
            }
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
    printf("Starting server\n");
    printf("Reading env file\n");
    const char* db_path = getenv("DB_PATH");
    if (db_path == NULL) load_env_file(".env");
    db_path = getenv("DB_PATH");
    printf("db path: %s\n",db_path == NULL ? "Not found" : db_path);

    const char* max_rentable = getenv("MAX_RENTABLE");
    printf("max_rentable: %s\n",max_rentable == NULL ? "Not found" : max_rentable);

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


    if (!db_setup(db_path)){
        printf("Error initializing db ...\n");
        return 1;
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




int command_type(const char *cmd) {
    if (strcmp(cmd, "CHECK") == 0) return CHECK_COMMAND;
    if (strcmp(cmd, "LOGIN") == 0) return LOGIN_COMMAND;
    if (strcmp(cmd, "REGISTER") == 0) return REGISTER_COMMAND;
    if (strcmp(cmd, "LIST") == 0) return LIST_COMMAND;
    if (strcmp(cmd, "RENT") == 0) return RENT_COMMAND;
    if (strcmp(cmd, "CLOSE") == 0) return CLOSE_COMMAND;
    if (strcmp(cmd, "MOVIE") == 0) return MOVIE_COMMAND;
    if (strcmp(cmd, "CART") == 0) return CART_COMMAND;


    return 0;
}



void cleanup(int signum) {
    unlink(SOCKET_PATH);
    exit(0);
}

int db_setup(const char* db_path){
    sqlite3 *db;
    int ret = 0;
    setup_db(db_path);
    db=get_db(db_path);
    if (!admin_exists(db_path)){
        char username[100];
        char password[100];

        printf("No admin found in database. Create an admin account.\n");
        printf("Enter admin username: ");
        fgets(username, sizeof(username), stdin);
        username[strcspn(username, "\n")] = 0;
        printf("Enter admin password: ");
        fgets(password, sizeof(password), stdin);
        password[strcspn(password, "\n")] = 0;
        printf("\n");
        const char *sql = "INSERT INTO Users (username, password, is_admin) VALUES (?, ?, 1);";
        sqlite3_stmt *stmt;

        if (sqlite3_prepare_v2(db, sql, -1, &stmt, 0) == SQLITE_OK) {
            sqlite3_bind_text(stmt, 1, username, -1, SQLITE_TRANSIENT);
            sqlite3_bind_text(stmt, 2, password, -1, SQLITE_TRANSIENT);
            if (sqlite3_step(stmt) == SQLITE_DONE) {
                printf("Admin user created successfully.\n");
                ret = 1;
            }
        }else {
            printf("Failed to create admin user: %s\n", sqlite3_errmsg(db));
        }
        sqlite3_finalize(stmt);
        sqlite3_close(db);
        return ret;
    }
    sqlite3_close(db);
    return 1;
}

#define ADD_SUBCOMAND 1
#define PUT_SUBCOMAND 2
#define DEL_SUBCOMAND 3
#define SEARCH_SUBCOMMAND 4
#define GET_SUBCOMMAND 5
#define REMIND_SUBCOMMAND 6

int parse_subcommand(const char* subcmd){
    if (strcmp(subcmd, "ADD") == 0) return ADD_SUBCOMAND;
    if (strcmp(subcmd, "PUT") == 0) return PUT_SUBCOMAND;
    if (strcmp(subcmd, "DEL") == 0) return DEL_SUBCOMAND;
    if (strcmp(subcmd, "SEARCH") == 0) return SEARCH_SUBCOMMAND;
    if (strcmp(subcmd, "GET") == 0) return GET_SUBCOMMAND;
    if (strcmp(subcmd, "REMIND") == 0) return REMIND_SUBCOMMAND;


    return 0;
}
int handle_movie_command(char** tokens, int client_fd){
    char response[ERR_SIZE];
    switch(parse_subcommand(tokens[1])) {
        case ADD_SUBCOMAND: {
            char* title = tokens[2];
            printf("title '%s'\n",title);
            char errors[ERR_SIZE];
            int nr = atoi(tokens[3]);
            int id = video_insert(title,nr,errors);
            printf("errors '%s'\n",errors);
            if (id > -1){
                snprintf(response, ERR_SIZE, "OK %d", id);
            } else {
                printf("errors '%s'\n",errors);
                snprintf(response, ERR_SIZE, "KO \"%s\"",errors);
            }
            send(client_fd, response, strlen(response), 0);
            return 1;
            break;
        }
        case SEARCH_SUBCOMMAND: {
            char* query = tokens[2];
            printf("query '%s'\n",query);
            Video* results[MAX_QUERY_RESULTS];
            int found = find_videos_by_title(query, results, MAX_QUERY_RESULTS);

            if (found == -1 ){
                snprintf(response, ERR_SIZE, "KO");    
            } else {
                snprintf(response, ERR_SIZE, "OK %d",found);
            }
            send(client_fd, response, strlen(response), 0);
            for (int i = 0; i < found; i++) {
                char ack_buf[32];
                int n = recv(client_fd, ack_buf, sizeof(ack_buf) - 1, 0);
                if (n <= 0) break;
                ack_buf[n] = '\0';
                if (strncmp(ack_buf, "NEXT", 4) != 0) break;
                snprintf(response, ERR_SIZE, "%d \"%s\" %d %d %d",results[i]->id,results[i]->title, results[i]->av_copies,results[i]->is_rentable, results[i]->rented_copies);
                printf("%s\n",response);
                send(client_fd, response, strlen(response), 0);
                free(results[i]->title);
                free(results[i]);
            }

            return 1;
            break;
        }
        case GET_SUBCOMMAND: {
            int id = atoi(tokens[2]);
            Video* video = find_video_by_id(id);
            snprintf(response, ERR_SIZE, "%d \"%s\" %d %d %d",video->id,video->title, video->av_copies,video->is_rentable, video->rented_copies);
            printf("%s\n",response);
            send(client_fd, response, strlen(response), 0);
            return 1;
            break;
        }
        default: {
            printf("Subcommand not found.\n");
            return 0;
            break;
        }
    }
}

int handle_rent_command(char** tokens, int client_fd, int max_rentable){
    char response[ERR_SIZE];
    switch(parse_subcommand(tokens[1])) {
        case ADD_SUBCOMAND: {
            User *user=find_user_by_id(atoi(tokens[2]));
            Video *video = find_video_by_id(atoi(tokens[3]));
            printf("User %s wants to rent movie %s\n",user->username,video->title);
            if (video->is_rentable){
                Rental* rentals[MAX_QUERY_RESULTS];
                int count = find_rentals_by_username(user->username,rentals,MAX_QUERY_RESULTS,0);
                if (count == max_rentable) {
                    snprintf(response, sizeof(response), "KO MAX");
                } else {
                    Rental *old = find_rental_by_username_and_movie(user->username,video->id);
                    if (old == NULL || old->end_date != NULL) {
                        int last_id = rent_video(user->username,video->id);
                        Rental *rental = find_rental_by_id(last_id);
                        printf("Rental: %d %d %s %s %s\n",rental->id, rental->id_movie, rental->username, rental->start_date, rental->due_date);
                        snprintf(response, sizeof(response), "OK %d %s", last_id,rental->due_date);
                    } else {
                        printf("Old: %d %d %s %s %s %s\n",old->id, old->id_movie, old->username, old->start_date, old->due_date,old->end_date);
                        snprintf(response, sizeof(response), "KO %d %s", old->id,old->due_date);
                    }
                }
                
            } else {
                snprintf(response, sizeof(response), "KO UNAVAILABLE");
            }
            printf("Response: %s\n",response);
            send(client_fd, response, strlen(response), 0);
            return 1;
            break;
        }
        case SEARCH_SUBCOMMAND: {
            char* query = tokens[2];
            const int include_returned = atoi(tokens[3]);
            printf("query '%s'\n",query);
            Rental* results[MAX_QUERY_RESULTS];
            int found = 0;
            if (strcmp(query,"ALL") == 0 ){
                found = find_all_rentals(results,MAX_QUERY_RESULTS,include_returned);
            } else {
                found = find_rentals_by_username(query, results, MAX_QUERY_RESULTS,include_returned);
            }

            if (found == -1 ){
                snprintf(response, ERR_SIZE, "KO");    
            } else {
                snprintf(response, ERR_SIZE, "OK %d",found);
            }
            send(client_fd, response, strlen(response), 0);
            for (int i = 0; i < found; i++) {
                char ack_buf[32];
                int n = recv(client_fd, ack_buf, sizeof(ack_buf) - 1, 0);
                if (n <= 0) break;
                ack_buf[n] = '\0';
                if (strncmp(ack_buf, "NEXT", 4) != 0) break;
                snprintf(response, ERR_SIZE, "%d %s %d \"%s\" \"%s\" \"%s\" %d",results[i]->id,results[i]->username,results[i]->id_movie, results[i]->start_date,results[i]->due_date,results[i]->end_date, results[i]->reminder);
                printf("%s\n",response);
                send(client_fd, response, strlen(response), 0);
                free(results[i]->username);
                free(results[i]);
            }

            return 1;
            break;
        }
        case GET_SUBCOMMAND :{
            return 1;
            break;
        }
        case PUT_SUBCOMAND: {
            int id = atoi(tokens[2]);
            if (set_rental_return_date(id)){
                snprintf(response, sizeof(response), "OK");
            } else {
                snprintf(response, sizeof(response), "KO");
            }
            printf("Response: %s\n",response);
            send(client_fd, response, strlen(response), 0);
            return 1;
            break;
        }
        case REMIND_SUBCOMMAND: {
            int id = atoi(tokens[2]);
            int reminder = atoi(tokens[3]);
            if (set_rental_reminder(id,reminder)){
                snprintf(response, sizeof(response), "OK");
            } else {
                snprintf(response, sizeof(response), "KO");
            }
            printf("Response: %s\n",response);
            send(client_fd, response, strlen(response), 0);
            return 1;
            break;
        }
        default: {
            printf("Subcommand not found.\n");
            return 0;
            break;
        }
    }
   
}

int handle_cart_command(char** tokens, int client_fd){
    char response[ERR_SIZE];
    switch(parse_subcommand(tokens[1])) {
        case ADD_SUBCOMAND: {
            char* username = tokens[2];
            char errors[ERR_SIZE];
            int video_id = atoi(tokens[3]);
            int id = cart_insert(username,video_id,errors);
            if (id > -1){
                snprintf(response, ERR_SIZE, "OK %d", id);
            } else {
                printf("errors '%s'\n",errors);
                snprintf(response, ERR_SIZE, "KO \"%s\"",errors);
            }
            send(client_fd, response, strlen(response), 0);
            return 1;
            break;
        }
        case GET_SUBCOMMAND: {
            char* username = tokens[2];
            printf("username '%s'\n",username);
            Cart* results[MAX_QUERY_RESULTS];
            int found = find_cart_by_username(username, results, MAX_QUERY_RESULTS);

            if (found == -1 ){
                snprintf(response, ERR_SIZE, "KO");    
            } else {
                snprintf(response, ERR_SIZE, "OK %d",found);
            }
            send(client_fd, response, strlen(response), 0);
            for (int i = 0; i < found; i++) {
                char ack_buf[32];
                int n = recv(client_fd, ack_buf, sizeof(ack_buf) - 1, 0);
                if (n <= 0) break;
                ack_buf[n] = '\0';
                if (strncmp(ack_buf, "NEXT", 4) != 0) break;
                Video* movie = results[i]->movie;
                snprintf(response, ERR_SIZE, "%d %s %d \"%s\" %d %d",results[i]->id,results[i]->username, movie->id, movie->title, movie->av_copies,movie->is_rentable);
                printf("%s\n",response);
                send(client_fd, response, strlen(response), 0);
                free(results[i]->username);
                free(results[i]);
            }

            return 1;
            break;
        }
        case DEL_SUBCOMAND: {
            int cart_id = atoi(tokens[2]);
            if (cart_delete_by_id(cart_id)){
                snprintf(response, ERR_SIZE, "OK");    
            } else {
                snprintf(response, ERR_SIZE, "KO");
            }
            send(client_fd, response, strlen(response), 0);
            return 1;
            break;
        }
        default: {
            printf("Subcommand not found.\n");
            return 0;
            break;
        }
    }
}