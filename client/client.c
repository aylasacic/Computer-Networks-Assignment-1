#include <stdio.h>
#include <string.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netinet/in.h>

#include <unistd.h>
#include <stdlib.h>

#define PORT 9007

void print_intro(){
    printf("Hello!! Plase Authenticate to run server commands \n");
    printf("1. type \"USER\" followed by a space and your username \n");
    printf("2. type \"PASS\" followed by a space and your password \n\n");

    printf("\"QUIT\" to close connection at any moment \n");
    printf("Once Authenticated \n");
    printf("this is the list of commands : \n");
    printf("\"STOR\" + space + filename |to send a file to the server \n");
    printf("\"RETR\" + space + filename |to download a file from the server \n");
    printf("\"LIST\" |to list all the files under the current server directory \n");
    printf("\"CWD\" + space + directory|to change the current server directory \n");
    printf("\"PWD\" to display the current server directory \n");
    printf("Add \"!\" begore the last three commands to apply them locally");

    printf("220 Service ready for new user.\n");
}


int main()
{
    //create a socket
    int network_socket;
    network_socket = socket(AF_INET , SOCK_STREAM, 0);

    //check for fail error
    if (network_socket == -1) {
        printf("socket creation failed..\n");
        exit(EXIT_FAILURE);
    }

    //setsock
    int value  = 1;
    setsockopt(network_socket,SOL_SOCKET,SO_REUSEADDR,&value,sizeof(value)); //&(int){1},sizeof(int)
    
    struct sockaddr_in server_address;
    bzero(&server_address,sizeof(server_address));
    server_address.sin_family = AF_INET;
    server_address.sin_port = htons(PORT);
    server_address.sin_addr.s_addr = INADDR_ANY;

    //connect
    if(connect(network_socket,(struct sockaddr*)&server_address,sizeof(server_address))<0)
    {
        perror("connect");
        exit(EXIT_FAILURE);
    }
    
    char buffer[256];

    print_intro();
    while(1)
    {
        bzero(buffer, sizeof(buffer));
        printf("ftp> ");
        
        fgets(buffer, sizeof(buffer), stdin);
        buffer[strcspn(buffer, "\n")] = 0;  //remove trailing newline char from buffer, fgets does not remove it
        if(strcmp(buffer, "QUIT") == 0)
        {
            printf("closing the connection to server \n");
            close(network_socket);
            break;
        }
        
        if(send(network_socket, buffer, strlen(buffer), 0) < 0)
        {
            perror("send");
            exit(EXIT_FAILURE);
        }
        
        recv(network_socket, buffer, sizeof(buffer), 0);
        if(strcmp(buffer, "!pwd") == 0){
            system("pwd");
        } else {
            printf("%s \n", buffer);
        }
    }

    return 0;
}
