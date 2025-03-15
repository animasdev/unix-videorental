#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <sqlite3.h>
#include "db/videodb.h"

#define SOCKET_PATH "/app/files/socket/videoteca.sock"

int main() {
    // apri db
    if (setup_db() == 1)
        fprintf(stdout, "Successfully set up the database.\n");
    else
        fprintf(stdout, "Failed to set up the database.\n");

/*
    if (!setup_db(db)) {
        fprintf(STDOUT_FILENO, "Failed to set up the database.\n");
        sqlite3_close(db);
        return 1;
    }

    // Initialize Unix Socket
    int server_sock, client_sock;
    struct sockaddr_un server_addr, client_addr;
    socklen_t client_len = sizeof(client_addr);

    server_sock = socket(AF_UNIX, SOCK_STREAM, 0);
    if (server_sock == -1) {
        perror("socket");
        return 1;
    }

    memset(&server_addr, 0, sizeof(struct sockaddr_un));
    server_addr.sun_family = AF_UNIX;
    strncpy(server_addr.sun_path, SOCKET_PATH, sizeof(server_addr.sun_path) - 1);

    if (bind(server_sock, (struct sockaddr *)&server_addr, sizeof(struct sockaddr_un)) == -1) {
        perror("bind");
        return 1;
    }

    if (listen(server_sock, 5) == -1) {
        perror("listen");
        return 1;
    }

    printf("Server listening on %s...\n", SOCKET_PATH);

    while (1) {
        client_sock = accept(server_sock, (struct sockaddr *)&client_addr, &client_len);
        if (client_sock == -1) {
            perror("accept");
            continue;
        }

        // Handle client connection
        // (Client handling logic here, e.g., user login, video rental)
        printf("Client connected!\n");

        // You can handle different operations based on requests here (simplified)
        handle_client(client_sock, db);

        close(client_sock);
    }

    close(server_sock);*/
    return 0;
}
