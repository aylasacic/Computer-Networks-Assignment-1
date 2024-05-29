#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <sys/select.h>

#define PORT 9007
#define BUFFSIZE 1024

typedef struct {
    char username[50];
    char password[50];
    char curr_path[1024];
    // add state: current directory, earlier authentication
    // 
} User;

User *users;
int user_count = 0;

void read_user_credentials() {
    char ch;
    int lines = 0;
    int lastLine = 0;
    User user1;

    FILE *file = fopen("users.txt", "r");
    if (!file) {
        perror("Could not open users.txt");
        exit(EXIT_FAILURE);
    }

    while ((ch = fgetc(file)) != EOF) {
        if (ch == '\n') {
            lines++;
            lastLine = 1; 
        } else {
            lastLine = 0; 
        }
    }

    if (!lastLine) {
        lines++;
    } 

    while((ch=fgetc(file)) != EOF) {
        if(ch == '\n'){
            lines++;
        }
    }

    printf("%d \n", lines);

    //dynamically create the users array after getting number of lines in file
    users = (User*) malloc(lines * sizeof(User));

    if (!users) {
        perror("Could not allocate memory for users");
        fclose(file);
        exit(EXIT_FAILURE);
    }

    rewind(file);

    char line[100];
    while (fgets(line, sizeof(line), file)) {
        sscanf(line, "%[^:]:%s", users[user_count].username, users[user_count].password);
        char command[100] = "mkdir ";
        if (strlen(command) + strlen(users[user_count].username) < sizeof(command)) {
            FILE *fp;
            char file[1035];

            fp = popen("ls", "r");
            if (fp == NULL) {
                printf("Failed to run command\n");
                exit(1);
            }

            int file_exists = 0;
            while (fgets(file, sizeof(file), fp) != NULL) {
                //remove the newline character from the file name so that comparison works
                file[strcspn(file, "\n")] = 0;
                
                if (strcmp(file, users[user_count].username) == 0) {
                    // printf("exists\n");
                    file_exists = 1;
                    break;
                }
            }
            pclose(fp);

            //if file exists, don't make new directory
            //otherwise make a new one
            if (!file_exists) {
                strcat(command, users[user_count].username);
                // printf("%s\n", command);
                system(command);
            }

        } else {
            printf("Error: Buffer overflow detected!\n");
        }
        user_count++;
    }

    fclose(file);
}

int authenticate_user(const char *username, const char *password) {
    for (int i = 0; i < user_count; i++) {
        if (strcmp(users[i].username, username) == 0 && strcmp(users[i].password, password) == 0) {
            return 1;
        }
    }
    return 0;
}

// void handle_clear(char *buffer, int client_fd, char *current_user){
//     char response[] = "Clear";
//     send(client_fd, response, strlen(response), 0);
// }

void handle_user_command(char *buffer, int client_fd, char *current_user) {
    char username[50];
    sscanf(buffer, "USER %s", username);

    // Check if the username exists
    for (int i = 0; i < user_count; i++) {
        if (strcmp(users[i].username, username) == 0) {
            strcpy(current_user, username);
            char response[] = "331 Username OK, need password.\n";
            printf("Successful username verification \n");
            send(client_fd, response, strlen(response), 0);
            return;
        }
    }

    char response[] = "530 Not logged in.\n";
    printf("Unsuccessful username verification \n");
    send(client_fd, response, strlen(response), 0);
}

void handle_pass_command(char *buffer, int client_fd, char *current_user) {
    char password[50];
    sscanf(buffer, "PASS %s", password);

    if (authenticate_user(current_user, password)) {
        char response[] = "230 User logged in, proceed.\n";
        printf("Successful login\n");
        send(client_fd, response, strlen(response), 0);

        FILE *fo;
        FILE  *fp;
        char file[1035];
        char path[1035];

        fo = popen("cd", "r");
        if (fo == NULL) {
            printf("Failed to run command\n");
            exit(1);
        }
        
        fp = popen("pwd", "r");
        if (fp == NULL) {
            printf("Failed to run command\n");
            exit(1);
        }

        while(fgets(path, sizeof(path) - 1, fp) != NULL){
            printf("%s", path);
        }

        path[strcspn(path, "\n")] = 0; 

        char command[100] = "cd ";
        if (strlen(command) + strlen(current_user) < sizeof(command)) {
            strcat(path, "/");
            strcat(path, current_user);
            strcat(command, path);

            printf("%s\n", path);
            printf("%s\n", command);

            system(command);

            for (int i = 0; i < user_count; i++) {
            if (strcmp(users[i].username, current_user) == 0) {
                strcpy(users[i].curr_path, path);
                break;
            }
        }
        } else {
            printf("Error: Buffer overflow detected!\n");
        }

        pclose(fp);
        pclose(fo);
    } else {
        char response[] = "530 Not logged in.\n";
        printf("Incorrect password\n");
        send(client_fd, response, strlen(response), 0);
    }
}

void handle_local_pwd(char *buffer, int client_fd, char *current_user) {
    send(client_fd, "!pwd", strlen("!pwd"), 0);
}

void handle_pwd(char *buffer, int client_fd, char *current_user){
    for (int i = 0; i < user_count; i++) {
        if (strcmp(users[i].username, current_user) == 0) {
            char response[1024];
            snprintf(response, sizeof(response), "257 \"%s\"\n", users[i].curr_path);
            send(client_fd, response, strlen(response), 0);
            break;
        }
    }
}

void handle_port_command(char *buffer, int client_fd) {
    int h1, h2, h3, h4, p1, p2;
    char ip[16];
    int port;

    sscanf(buffer, "PORT %d,%d,%d,%d,%d,%d", &h1, &h2, &h3, &h4, &p1, &p2);

    snprintf(ip, sizeof(ip), "%d.%d.%d.%d", h1, h2, h3, h4);
    port = (p1 * 256) + p2;

    printf("Received PORT command with IP: %s and Port: %d\n", ip, port);

    char response[] = "200 PORT command successful.\n";
    send(client_fd, response, strlen(response), 0);
}

int main() {
    // Read user credentials
    read_user_credentials();

    int server_socket = socket(AF_INET, SOCK_STREAM, 0);
    // printf("Server fd = %d \n", server_socket);

    if (server_socket < 0) {
        perror("socket:");
        exit(EXIT_FAILURE);
    }

    int value = 1;
    setsockopt(server_socket, SOL_SOCKET, SO_REUSEADDR, &value, sizeof(value));

    struct sockaddr_in server_address;
    bzero(&server_address, sizeof(server_address));
    server_address.sin_family = AF_INET;
    server_address.sin_port = htons(PORT);
    server_address.sin_addr.s_addr = INADDR_ANY;

    if (bind(server_socket, (struct sockaddr*)&server_address, sizeof(server_address)) < 0) {
        perror("bind failed");
        exit(EXIT_FAILURE);
    }

    if (listen(server_socket, 5) < 0) {
        perror("listen failed");
        close(server_socket);
        exit(EXIT_FAILURE);
    }

    fd_set all_sockets;
    fd_set ready_sockets;

    FD_ZERO(&all_sockets);
    FD_SET(server_socket, &all_sockets);

    char current_user[50] = {0};

    while (1) {
        ready_sockets = all_sockets;

        if (select(FD_SETSIZE, &ready_sockets, NULL, NULL, NULL) < 0) {
            perror("select error");
            exit(EXIT_FAILURE);
        }

        for (int fd = 0; fd < FD_SETSIZE; fd++) {
            if (FD_ISSET(fd, &ready_sockets)) {
                if (fd == server_socket) {
                    struct sockaddr_in client_address;
                    socklen_t client_address_len = sizeof(client_address);
                    int client_sd = accept(server_socket, (struct sockaddr*)&client_address, &client_address_len);

                    if (client_sd < 0) {
                        perror("Accept error");
                        continue;
                    }

                    char client_ip[INET_ADDRSTRLEN];
                    inet_ntop(AF_INET, &client_address.sin_addr, client_ip, sizeof(client_ip));
                    int client_port = ntohs(client_address.sin_port);
                    printf("Connection established with user %d \n", client_sd);

                    printf("Their port: %d \n", client_port);

                    FD_SET(client_sd, &all_sockets);
                } else {
                    char buffer[256];
                    bzero(buffer, sizeof(buffer));
                    int bytes = recv(fd, buffer, sizeof(buffer), 0);
                    if (bytes == 0) {
                        printf("connection closed from client side fd = %d \n", fd);
                        close(fd);
                        FD_CLR(fd, &all_sockets);
                    } else if (strncmp(buffer, "USER", 4) == 0) {
                        handle_user_command(buffer, fd, current_user);
                    } else if (strncmp(buffer, "PASS", 4) == 0) {
                        handle_pass_command(buffer, fd, current_user);
                    } 
                    else if (strncmp(buffer, "!PWD", 5) == 0) {
                        handle_local_pwd(buffer, fd, current_user);
                    } 
                    else if (strncmp(buffer, "PWD", 5) == 0) {
                        handle_pwd(buffer, fd, current_user);
                    } 
                    else {
                        printf("%s \n", buffer);
                    }
                }
            }
        }
    }

    close(server_socket);
    return 0;
}
