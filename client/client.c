#include <stdio.h>
#include <string.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <unistd.h>
#include <stdlib.h>
#include <dirent.h>
#include <errno.h>
#include <time.h>

#define PORT 4444

void log_error(const char *message, int errnum) {
    fprintf(stderr, "%s: %s\n", message, strerror(errnum));
}

int isAuthenticated = 0;

long int findFileSize(const char *fileName) {
    FILE *file = fopen(fileName, "rb"); // Open the file in binary mode
    if (file == NULL) {
        printf("Could not open file %s\n", fileName);
        return -1;
    }
    
    fseek(file, 0, SEEK_END); // Move the file pointer to the end of the file
    long int size = ftell(file); // Get the current position of the file pointer (file size)
    fclose(file); // Close the file
    
    return size;
}


void print_intro() {
    printf("%s", "Hello!! Please Authenticate to run server commands \n");
    printf("%s", "1. Type \"USER\" followed by a space and your username \n");
    printf("%s", "2. Type \"PASS\" followed by a space and your password \n\n");

    printf("%s", "\"QUIT\" to close connection at any moment \n");
    printf("%s", "Once Authenticated \n");
    printf("%s", "This is the list of commands : \n");
    printf("%s", "\"STOR\" + space + filename | to send a file to the server \n");
    printf("%s", "\"RETR\" + space + filename | to download a file from the server \n");
    printf("%s", "\"LIST\" | to list all the files under the current server directory \n");
    printf("%s", "\"CWD\" + space + directory | to change the current server directory \n");
    printf("%s", "\"PWD\" to display the current server directory \n");
    printf("%s", "Add \"!\" before the last three commands to apply them locally\n");

    printf("%s", "220 Service ready for new user.\n");
}
    

int main(){
    //create a socket
    int network_socket;
    network_socket = socket(AF_INET, SOCK_STREAM, 0);

    //check for fail error
    if (network_socket < 0) {
        printf("%s", "socket creation failed..\n");
        exit(EXIT_FAILURE);
    }

    //setsock
    int value  = 1;
    setsockopt(network_socket, SOL_SOCKET, SO_REUSEADDR, &value, sizeof(value));

    struct sockaddr_in server_address;
    bzero(&server_address, sizeof(server_address));
    server_address.sin_family = AF_INET;
    server_address.sin_port = htons(PORT);
    server_address.sin_addr.s_addr = INADDR_ANY;

    printf("%u", server_address.sin_addr.s_addr);

    //connect
    if (connect(network_socket, (struct sockaddr*)&server_address, sizeof(server_address)) < 0) {
        perror("connect");
        exit(EXIT_FAILURE);
    }

    printf("Connected to server. \n");

    char buffer[256];

    print_intro();

    while (1)
    {
        //get input from user
        printf("ftp> ");
        fgets(buffer, sizeof(buffer), stdin);
        buffer[strcspn(buffer, "\n")] = 0;  //remove trailing newline char from buffer, fgets does not remove it

        if (strncmp(buffer, "QUIT", 4) == 0) {
            send(network_socket, buffer, strlen(buffer), 0);
            bzero(buffer, sizeof(buffer));
            int message = recv(network_socket, buffer, sizeof(buffer), 0);

            if (message <= 0) {
                printf("Server has shutdown\n");
                close(network_socket);
                return 0;
            }

            buffer[message] = '\0';
            printf("%s\n", buffer);
            close(network_socket);
            break;
        } else if (strncmp(buffer, "USER", 4) == 0) {
            // printf("%s\n", buffer);
            send(network_socket, buffer, strlen(buffer), 0);

            
            bzero(buffer, sizeof(buffer));
            int message = recv(network_socket, buffer, sizeof(buffer), 0);

            if (message <= 0) {
                printf("Server has shutdown\n");
                close(network_socket);
                return 0;
            }

            buffer[message] = '\0';
            printf("%s\n", buffer);
        } else if (strncmp(buffer, "PASS", 4)==0){

			send(network_socket, buffer, sizeof(buffer), 0);
			bzero(buffer, sizeof(buffer));

			int rec_bytes = recv(network_socket, buffer, sizeof(buffer), 0);
			if (rec_bytes<=0){
				printf("Server has shutdown\n");
				return 0;
			}
			printf("%s\n", buffer);

            if (strstr(buffer, "230") != NULL) {
                isAuthenticated = 1;
            }
		} else if (strncmp(buffer, "PWD", 3)==0){

			send(network_socket, buffer, sizeof(buffer), 0);
			bzero(buffer, sizeof(buffer));

			int rec_bytes = recv(network_socket, buffer, sizeof(buffer), 0);
			if (rec_bytes<=0){
				printf("Server has shutdown\n");
				return 0;
			}
			printf("%s\n", buffer);
		} else if(strncmp(buffer, "!PWD", 4)==0){

			char pwd[1024];
			getcwd(pwd, sizeof(pwd));

			printf("%s\n", pwd);

		} else if (strncmp(buffer, "CWD", 3)==0){

			send(network_socket, buffer, sizeof(buffer), 0);
			bzero(buffer, sizeof(buffer));

			int rec_bytes = recv(network_socket, buffer, sizeof(buffer), 0);
			if (rec_bytes<=0){
				printf("Server has shutdown\n");
				return 0;
			}
			printf("%s\n", buffer);
		} else if (strncmp(buffer, "!LIST", 5) == 0) {
			/* open current directory for user (on client) */
			DIR *directory;
			struct dirent *dir;
			directory = opendir(".");


			/*
			if directory is good,
			print out the content
			*/
			if (directory) {
				while ((dir = readdir(directory)) != NULL) {
					/* skip directories "." and ".." */
					if (strcmp(dir -> d_name, ".") == 0 || strcmp(dir->d_name, "..") == 0) {
						continue;
					}
					printf("%s\n", dir->d_name);
				}
				closedir(directory);
			} else {
				printf("!LIST failed");
				continue;
			}
		} else if (strncmp(buffer, "!CWD", 4) == 0) {
			char* command = strtok(buffer, " ");
			char* argument = strtok(NULL, " ");
			if (argument) {
				if (chdir(argument) == 0) {
			    		printf("250 Directory successfully changed.\r\n");
				} else {
			    		printf("550 Failed to change directory.\r\n");
				}
			} else {
				printf("501 Syntax error in parameters or arguments.\r\n");
			}
			continue;
        } //if PORT required
		else if(strncmp(buffer, "STOR", 4)==0 || strncmp(buffer, "RETR", 4)==0 || strncmp(buffer, "LIST", 4)==0)
		{
			/*
				since PORT runs automatically, we need to stop user who is not logged in 
				from using these functions as it messes up the code after.
				
			*/
			if (!isAuthenticated) {
				printf("530 Not logged in!\n");
				continue;
		    	}
		    	
			if (strncmp(buffer, "STOR", 4) == 0 || strncmp(buffer, "RETR", 4) == 0){
				char buff[1024];
				strcpy(buff, buffer);

				char* filename = strtok(buff, " ");
				filename = strtok(NULL, " ");

				if (filename == NULL) {
				    printf("Please provide a file name!\n");
				    continue;
				}
			}

			/* start the PORT command */
			send(network_socket, "PORT 12345", sizeof("PORT 12345"), 0);
			
			int channel;
			
            		/* receive channel from server */
			int message = recv(network_socket, &channel, sizeof(channel), 0);

			if (message <= 0){
				printf("%s", "Server has shutdown\n");
				return 0;
			}

			/* specify an address for the socket we want to connect to */
			struct sockaddr_in cli_addr;
			socklen_t cli_addr_size = sizeof(cli_addr);

			
			if (getsockname(network_socket, (struct sockaddr*) &cli_addr, &cli_addr_size) != 0) {
				log_error("Error in getting socket:", errno);
				continue;
			}

			
			char port_request[1024];
			int h1, h2, h3, h4;
			
			/*  
				specify the port we want to connect to remotely
				htons converts integer port to right format for the structure
				just some port we know the OS is not using 
				add channel increment to the port such that we do not use a port which is already
				in use
				channel increment in server code
			*/
			
			int client_port = ntohs(cli_addr.sin_port) + channel;
			
			/* same thing for IP */
			char client_ip[INET_ADDRSTRLEN];

			const char* ip_result = inet_ntop(AF_INET, &(cli_addr.sin_addr), client_ip, INET_ADDRSTRLEN);

			if (ip_result == NULL) {
			    perror("inet_ntop failed");
			    continue;
			} 

			/*
				get p1 and p2 based on assignment description

				p1 = port / 256
				p2 = port % 256
				// convert p1 and p2 to port (done on server side)
				port = (p1 * 256) + p2
			*/
			int p1 = client_port / 256;
			int p2 = client_port % 256;
			
			//sscanf(client_ip,IP_ADDR_FORMAT,&h1,&h2,&h3,&h4);
			//snprintf(port_request, 1024, "PORT %d,%d,%d,%d,%d,%d", h1, h2, h3, h4, p1, p2);
			
			if (sscanf(client_ip, "%d.%d.%d.%d", &h1, &h2, &h3, &h4) == 4) {
				snprintf(port_request, sizeof(port_request), "PORT %d,%d,%d,%d,%d,%d", h1, h2, h3, h4, p1, p2);
			} else {
				fprintf(stderr, "Invalid IP address format\n");
				return 1;
			}

 
			/*
				send PORT request 
			*/ 
			send(network_socket, port_request, sizeof(port_request), 0);

			/*
				create socket for data exchange
				on server side we create the addressing only not the socket itself
				the client creates the socket
			*/ 
			int data_sock = socket(AF_INET,SOCK_STREAM,0);

			if (data_sock < 0){
				perror("data sock: ");
				continue;
			}
			
			/*
				declare variable to hold the address information for data exchange with the client.
				set the address family to AF_INET (IPv4).
				set the port number to client_port, converting it to network byte order using htons.
				set the IP address to client_ip, converting it to a format suitable for struct in_addr using inet_addr.
			*/
			struct sockaddr_in data_addr;
			data_addr.sin_family = AF_INET;
			data_addr.sin_port = htons(client_port);
			data_addr.sin_addr.s_addr = inet_addr(client_ip);

			/* 
				enable the SO_REUSEADDR option on the socket.
				this allows the server to bind to an address that is already in use.
				It is useful for allowing a server to be restarted quickly without waiting
				for the previous socket to time out.
			*/
			if (setsockopt(data_sock, SOL_SOCKET, SO_REUSEADDR, &(int){1}, sizeof(int)) < 0)
			{
				perror("setsockopt");
			}
				
			/*
				classic TCP procedure from here:
				1. bind
				2. listen
				3. accept
			*/
			if (bind(data_sock, (struct sockaddr *)&data_addr, sizeof(data_addr)) < 0)
			{
				perror("Bind error: ");
				continue;
			}
			
			/* 
				attempt to mark the socket as a passive socket, that is,
				one that will be used to accept incoming connection requests,
				with a backlog of up to 5 pending connections.
				if the listen operation fails, print an error message and close 
				the data socket to release the resources associated with it.
			*/
			
			if (listen(data_sock, 5) < 0)
			{
				perror("Lissten error: ");
				close(data_sock);
				continue;
			}
			
			/*
				receive PORT response from server
				if not successfull, terminate current loop an start next one
			*/
			char response[1024];
			recv(network_socket, response, 1024, 0);
			
			printf("%s \r\n", response);
			
			
			if ((strncmp(response, "200", 3)) != 0) {
				printf("PORT response failed...\n");
				continue;
			}
			
			/*
				accept data packet from server
				and check if there are errors
				data socket cannot be 0
			*/
			int data_socket = accept(data_sock, 0, 0);

			if (data_socket < 0)
			{
				perror("Accept error: ");
				continue;
			}

			
			if(strncmp(buffer, "STOR", 4)==0){
				//TBD				
			
			}

			if(strncmp(buffer, "RETR", 4)==0){
			 	/* create a receiver buffer to store the incoming file name */
				char receiver[1024];
				strcpy(receiver, buffer);
				char* filename = strchr(receiver, ' ');
				if (filename != NULL) {
				 	/* 
				 		extract the file name from the receiver buffer 
				 		not sure why just using buffer does not work 
				 		
				 	*/
					filename = strtok(filename + 1, " ");
				}

				/* send the buffer contents through the network socket */
				send(network_socket,buffer,sizeof(buffer),0);
			
				/* 
					receive the size of the incoming data and 
					the actual response data 
				*/
				int size;
				recv(data_socket, &size, sizeof(int), 0);
				char response[size];
				memset(response, 0, sizeof(response)); // clear buffer just for error mitigation
				recv(data_socket, response, sizeof(response), 0);
				
				/* check if the response indicates an error (550 code) */
				if (strncmp(response, "550", 3)==0){
					printf("%s\r\n", response);
					close(data_socket);
					continue;
				} else {
					char str[] = "150 File status okay; about to open data connection.\n";
					/*
						printing out just the response ends up printing some random buffer values as well. not sure why so I print out just the string
					*/
					if(strncmp(str, response, strlen(str)) == 0){
						printf("%s\r\n", str);
					}
					
				}

				/* 
					prepare a buffer for file data and initialize it to zero 
					make sure its the same size as the buffer on other end
				*/
				char file_buffer[1024];
				bzero(file_buffer, sizeof(file_buffer));

				/* 
					create a temp file name using the channel number 
					as per assignment details
				*/
				char temp[1024];
				int rnd = rand();
				sprintf(temp, "rand%d.tmp", rnd);

				/* open a temo file for writing in binary mode */
				FILE* file = fopen(temp, "wb");

				int b;
				
				/* receive the file data in chunks and write it to the temporary file */
				while ((b = recv(data_socket, file_buffer, sizeof(file_buffer), 0)) > 0) {
				    if (b < 1024) {
				    	if (b != 0){
				    		/* last iteration */
				    		fwrite(file_buffer, 1, b, file);
				    	} 
					break;
				    }
				    fwrite(file_buffer, 1, b, file);

				    
				}
				
				/* close all files and disconnect from data*/
				fclose(file);
				rename(temp, filename);
				close(data_socket);
				
				/* print out successful data retrieval */
				memset(response, 0, sizeof(response));
				memset(receiver, 0, sizeof(receiver));
				recv(network_socket, receiver, sizeof(receiver), 0);
				printf("%s \n", receiver);
			}
			if (strncmp(buffer, "LIST", 4) == 0){
			
				char receiver[1024] = {0}; // for receiving data response
                		int size; // receive size of incoming data
                		char directory_content[1024]; // receive directory content

				/* attempt to receive the response from the server.*/
				send(network_socket, buffer, sizeof(buffer), 0);
				
				recv(data_socket, receiver, sizeof(receiver), 0);
				printf("%s\n", receiver);

				/*
					attempt to receive the size of the incoming data.
					this is needed to properly record the directory content size incoming next
				*/
				recv(data_socket, &size, sizeof(int), 0);
				
				/* attempt to receive the directory content based on the incoming size. */
				recv(data_socket, directory_content, size, 0);
				printf("%s\n", directory_content);
				
				/*
					reset receiver to save memory
					attempt to receive the response from the server.
				*/
				memset(receiver, 0, sizeof(receiver));
				recv(network_socket, receiver, sizeof(receiver), 0);
				printf("%s\n", receiver);
			}
			
		} else {
            send(network_socket, buffer, strlen(buffer), 0);
            bzero(buffer, sizeof(buffer));

            int message = recv(network_socket, buffer, sizeof(buffer), 0);

            if (message <= 0) {
                printf("Server has shutdown\n");
                close(network_socket);
                return 0;
            }

            buffer[message] = '\0';
            printf("%s\n", buffer);
        }
    }
    return 0;
}

