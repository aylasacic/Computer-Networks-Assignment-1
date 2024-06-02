#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <unistd.h>
#include <stdlib.h>
#include <sys/time.h>
#include <sys/select.h>

int client_num = 0;

typedef struct{
    char username[1024];
    int login; 
    int isAuthenticated; 
    char ip_addr[16];
    char curr_dir[1024];
    
    int client_fd; 
    int ftp_port;
    int data_socket;
} Client;

Client clients[100];

Client *getClient(int fd) {
    for (int i = 0; i < 100; i++) {
        if (clients[i].client_fd == fd) {
            return &clients[i];
        }
    }
    return NULL;
}

int storeStateAt(){
	for (int i = 0; i < 100; i++){
		if(clients[i].client_fd == -1){
            client_num++;
			return i;
		}
	}
	return -1;
}

void addClients() {
    for (int i = 0; i < 100; i++) {
        strcpy(clients[i].username, "");
	clients[i].login = 0;
        clients[i].isAuthenticated = 0;
	strcpy(clients[i].ip_addr, "");
	strcpy(clients[i].curr_dir, "");
		
        clients[i].client_fd = -1;
	clients[i].ftp_port = -1;
	clients[i].data_socket = -1;
    }
}

void removeClient(int client_fd) {
    for (int i = 0; i < 100; i++) {
        if(clients[i].client_fd == client_fd){
            strcpy(clients[i].username, "");
		    clients[i].login = 0;
            clients[i].isAuthenticated = 0;
		    strcpy(clients[i].ip_addr, "");
		    strcpy(clients[i].curr_dir, "");
		
		    clients[i].ftp_port = -1;
		    clients[i].data_socket = -1;

            if(clients[i].client_fd != -1){
                close(clients[i].client_fd);
            }

            clients[i].client_fd = -1;
        }  
    }
}

int authenticateClient(int client_fd){
	Client *client = getClient(client_fd);
	
    	//set current directory
	char client_directory[1024];
	getcwd(client_directory, sizeof(client_directory));
    	strcat(client_directory, "/");
    	strcat(client_directory, client->username);
	strcpy(client->curr_dir, client_directory);

	client->isAuthenticated = 1;
	client->data_socket = -1;	
	return 1;
}
