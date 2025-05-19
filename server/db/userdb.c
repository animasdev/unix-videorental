#include <stdio.h>
#include <string.h>
#include <sqlite3.h>
#include "userdb.h"

#define MAX_MEM_USRS 50

struct user {
    const char* username;
    const char* password;
    int role;
};

struct user users[MAX_MEM_USRS];
int curr_insert = 0;

int user_exists(const char *username) {
    printf("checking if user %s exists\n",username);
    if (curr_insert == 0 ) return -1;
    for (int i=0;i<curr_insert;i++){
        const struct user usr = users[i];
        printf("user name %s\n",usr.username);
        printf("user pass %s\n",usr.password);
        printf("user role %d\n",usr.role);
        if (strcmp(username,usr.username) == 0)
            return i;
    }
    return -1;
}

int user_login(const int usr_id, const char *password){
    if (curr_insert == 0 ) return 0;
    if (usr_id < MAX_MEM_USRS){
        printf("yes\n");
        const struct user usr = users[usr_id];
        printf("user name %s\n",usr.username);
        printf("user pass %s\n",usr.password);
        printf("user role %d\n",usr.role);
        return strcmp(password,users[usr_id].password);
    }
    return 0;
}

int user_register(const char *username, const char *password){
    if (user_exists(username) == -1){
        struct user new_usr;
        new_usr.username=username;
        new_usr.password=password;
        new_usr.role=0;
        users[curr_insert] = new_usr;
        return curr_insert++;
    }
    return -1;
}