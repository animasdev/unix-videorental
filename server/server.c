// server_local_fork.c
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <signal.h>
#include "userdb.h"
#include "db.h"

#define SOCKET_PATH "/tmp/mysocket"
#define BUFFER_SIZE 1024
#define MAX_TOKENS 100 
#define MAX_TOKEN_LEN 100 

#define CHECK_COMMAND 1
#define LOGIN_COMMAND 2
#define REGISTER_COMMAND 3 
#define LIST_COMMAND 4
#define RENT_COMMAND 5
#define CLOSE_COMMAND 6
#define DB_PATH "../db/videoteca.db"


void cleanup(int signum);

int parse_command(const char *input, char *tokens[]);
int command_type(const char *cmd);
int db_setup();

void handle_client(int client_fd) {
    char buffer[BUFFER_SIZE];
    ssize_t bytes_read;

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
                    printf("registered new user %s - %s with id %d\n",tokens[1],tokens[2],new_id);
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
                    char id_str[16];
                    snprintf(id_str, sizeof(id_str), "%d", id);
                    send(client_fd, id_str, strlen(id_str), 0);
                } else {
                    send(client_fd, "KO", 2, 0);
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


    if (!db_setup()){
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

int parse_command(const char *input, char *tokens[]) {
    char *copy = strdup(input);
    if (!copy) {
        perror("strdup");
        return -1;
    }

    int count = 0;
    char *token = strtok(copy, " \t\n");
    while (token != NULL && count < MAX_TOKENS) {
        printf("%s\n",token);
        tokens[count] = strdup(token); 
        if (!tokens[count]) {
            perror("strdup");
            break;
        }
        count++;
        token = strtok(NULL, " \t\n");
        
    }

    free(copy); 
    return count;
}

int command_type(const char *cmd) {
    if (strcmp(cmd, "CHECK") == 0) return CHECK_COMMAND;
    if (strcmp(cmd, "LOGIN") == 0) return LOGIN_COMMAND;
    if (strcmp(cmd, "REGISTER") == 0) return REGISTER_COMMAND;
    if (strcmp(cmd, "LIST") == 0) return LIST_COMMAND;
    if (strcmp(cmd, "RENT") == 0) return RENT_COMMAND;
    if (strcmp(cmd, "CLOSE") == 0) return CLOSE_COMMAND;

    return 0;
}



void cleanup(int signum) {
    unlink(SOCKET_PATH);
    exit(0);
}

int db_setup(){
    sqlite3 *db;
    int ret = 0;
    setup_db();
    db=get_db();
    if (!admin_exists()){
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
