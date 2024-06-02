#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>

typedef struct {
    char username[1024];
    char password[1024];
    int isAuthenticated;
} User;

User users[100];
int user_count = 0;

int readUsers() {
    FILE *file = fopen("../users.txt", "r"); 
    if (!file) {
        perror("Could not open users.txt");
        return -1;
    }

    char line[1024]; 
    while (fgets(line, sizeof(line), file) && user_count < 100) {
        line[strcspn(line, "\n")] = '\0';

        if (sscanf(line, "%s %s", users[user_count].username, users[user_count].password) == 2) {
            user_count++;
        }
    }

    for(int i = 0; i < user_count; i++){

        users[i].isAuthenticated = 0;

        char client_directory[1024];
        getcwd(client_directory, sizeof(client_directory));

        strcat(client_directory, "/");
        strcat(client_directory, users[i].username);

        int dir_result = mkdir(client_directory, 0755);
        if(dir_result < 0){
            printf("Directory already exists!\n");
        }
        else{
            printf("Directory created!\n");
        }
        
    }

    fclose(file);
    return 1;
}
