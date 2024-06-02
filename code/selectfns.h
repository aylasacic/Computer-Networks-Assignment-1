#include "../code/clients.h"
#include "../code/users.h"

#include <stdio.h>
#include <string.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <unistd.h>
#include <stdlib.h>
#include <sys/time.h>
#include <sys/select.h>
#include <signal.h>
#include <dirent.h>
#include <time.h>

long int findFileSize(const char *fileName) {
    FILE *file = fopen(fileName, "rb"); 
    if (file == NULL) {
        printf("Could not open file %s\n", fileName);
        return -1;
    }
    
    fseek(file, 0, SEEK_END); 
    long int size = ftell(file);
    fclose(file); 
    
    return size;
}

int handle_quit_command(int client_fd){
    const char *quit_msg = "221 Service closing control connection.\n";

    send(client_fd, quit_msg, strlen(quit_msg), 0);

    removeClient(client_fd);
    
    client_num--;

    close(client_fd);

    printf("User disconnected from fd %d\n", client_fd);

    return 1;
}

int handle_user_command(char *buffer, int client_fd) {

	/* get username */
    	char delim[] = " ";
	char *username = strtok(buffer, delim);
	username = strtok(NULL, delim);

	/* see if username is provided at all */
	if (username == NULL){
		char response[] = "530 Not logged in.\n";
		printf("Username not provided \n");
		send(client_fd, response, strlen(response), 0);
		return 0;
	}
	
	/*
		check if the username corresponds to any username in users array
		if it does, return successful login and require password
	*/
	for (int i = 0; i < 100; i++) {
		if (strcmp(users[i].username, username) == 0) {
		    	Client *client = getClient(client_fd);
			strcpy(client->username, username);
		    	client->login = 1;
		    	printf("Successful username verification \n");
		    	send(client_fd, "331 Username OK, need password.", strlen("331 Username OK, need password."), 0);
		    return 1;
		}
	}

	/* otherwise uer not logged in -> non-existing username */
	char response[] = "530 Not logged in.\n";
	printf("Unsuccessful username verification \n");
	send(client_fd, response, strlen(response), 0);
	return 0;
}

int handle_pass_command(char *buffer, int client_fd) {

	/* get username */
    	char delim[] = " ";
	char *password = strtok(buffer, delim);
	password = strtok(NULL, delim);
	
	/* check if password provided */
	if (password == NULL){
		char response[] = "530 Not logged in.\n";
		printf("Password not provided \n");
		send(client_fd, response, strlen(response), 0);
		return 0;
	}

	/* get client_fd, if password provided */
    	Client *client = getClient(client_fd);

	/* 
		check if the username matches the username of the client 
		check if correct password is given for respective user
	*/
	for (int i = 0; i < 100; i++) {
		if (strncmp(users[i].username, client->username, strlen(clients->username)) == 0) {
	    		if(strncmp(users[i].password, password, strlen(password))==0){
	    			
	    			/*
	    				authorize the user
	    				set authorization flag to true (both client and user)
	    				get their current server directory (by adding/[username) 
	    				to the current directory
	    				
	    				
	    			*/
				users[i].isAuthenticated = 1;

				char directory[1024];
				getcwd(directory, sizeof(directory));
				strcat(directory, "/");
				strcat(directory, client->username);
				strcpy(client->curr_dir, directory);

				client->isAuthenticated = 1;
				client->data_socket = -1; // initialize data socket

				printf("Successful login \n");
				send(client_fd, "230 User logged in, proceed.", strlen("230 User logged in, proceed."), 0);
				return 1;
	    		}
		}
	}

	/* otherwise remove login flag and require login again */
	client -> login = 0;
	char response[] = "530 Not logged in.\n";
	printf("Unsuccessful username verification \n");
	send(client_fd, response, strlen(response), 0);
	return 0;
}


int handle_pwd_command(int client_fd){
	
	/* get current client */
    	Client *client = getClient(client_fd);

	/* get their current server directory */
	char current_directory[2048];
	strcpy(current_directory, client->curr_dir);
	
	/* 
		set keyword and initalize substring to store only the 
		client relevant directory information
	*/
	char *keyword = "server";
    	char *substring;

	/* substring set to everything after /server in pwd */
	substring = strstr(current_directory, keyword);

	/* error checking */
	if (substring != NULL) {
		substring += strlen(keyword);
		// printf("%s\n", substring);
	} else {
		printf("Keyword not found in the path.\n");
	}

	/* successful response */
	char response[1024];
	snprintf(response, 1024, "257 %s", substring);
	send(client_fd, response, strlen(response), 0);	
	return 1;
}

int handle_cwd_command(char *buffer, int client_fd){
	
	/* get client */
    	Client *client = getClient(client_fd);

	char directory[1024];
	strcpy(directory, buffer);
	char* new_dir = strchr(directory, ' ');
	if (new_dir != NULL) {
	 	/* 
	 		extract the directroy name from the directory buffer 
	 		not sure why just using buffer does not work 
	 		
	 	*/
		new_dir = strtok(new_dir + 1, " ");
	}
	
	if (new_dir == NULL){
		send(client_fd,	"451 Requested action aborted. Local error in processing.\n", strlen("451 Requested action aborted. Local error in processing.\n"),0);
		return -1;
	}
	
	/* create temp_dir to hold current directory */
	char temp_dir[4096];
	
	/* not to be confused with current directroy which is the one we are switching to */
	char current_directory[2048];
	getcwd(temp_dir, sizeof(temp_dir));
	
	/*
		this part essentially check whether the user is trying to go beyond /server,
		that is trying to access restricted data or other directories
	*/
	if(strncmp(new_dir, "../", 3) == 0){
		
		/* save new directory */
		strcpy(current_directory, temp_dir);
		strcat(current_directory, "/");
		strcat(current_directory, client->username);
		chdir(current_directory);
		
		char *keyword = "server";
		char *substring;
		
		char check[1024];
		
		/* 
			check if temp directory is same as base directory 
			if it is, sent the directory to base user directroy of:
			/[username]
		*/
		getcwd(check, sizeof(check));
		if(strncmp(check, temp_dir, sizeof(temp_dir)) == 0){
			getcwd(client->curr_dir, sizeof(client->curr_dir));
			strcpy(client->curr_dir, check);
	
			substring = strstr(check, keyword);
		
			/*
				as in PWD, print out only the allowed user directry path
			*/

			if (substring != NULL) {
				substring += strlen(keyword);
			} else {
				printf("Keyword not found in the path.\n");
			}
			
			char response[1024];
			snprintf(response, 1024, "257 %s", substring);
			send(client_fd, response, strlen(response), 0);	
			return -1;
		}
		
		/*
			as in PWD, print out only the allowed user directry path
		*/
		substring = strstr(current_directory, keyword);
		
		printf("%s\r\n", substring);
		
		getcwd(client->curr_dir, sizeof(client->curr_dir));
		strcpy(client->curr_dir, current_directory);
	

		if (substring != NULL) {
			substring += strlen(keyword);
		} else {
			printf("Keyword not found in the path.\n");
		}
		char response[1024];
		snprintf(response, 1024, "257 %s", substring);
		send(client_fd, response, strlen(response), 0);	
		printf("\nClient working directory changed \n");
		return 1;
		
	}
	
	/* get path to new directory change */
	strcpy(current_directory, client->curr_dir);
	strcat(current_directory, "/");
	strcat(current_directory, new_dir);

	/* check if the directroy exists*/
	if(chdir(current_directory) < 0){
		send(client_fd,	"550 No such file or directory.", strlen("550 No such file or directory."),0);
		return -1;
	}

	/* restore server path */
	chdir(temp_dir);
	
	/* save directory change to client */
	getcwd(client->curr_dir, sizeof(client->curr_dir));
	strcpy(client->curr_dir, current_directory);
	
	/*
		the code underneath just prints out the path after "server"
		so that the users know where they are on the sever only
	*/
	
	char *keyword = "server";
	char *substring;
	substring = strstr(current_directory, keyword);

	/* error check */
	if (substring != NULL) {
		substring += strlen(keyword);
	} else {
		printf("Keyword not found in the path.\n");
	}

	/* send successful response back */
	char response[1024];
	snprintf(response, 1024, "257 %s", substring);
	send(client_fd, response, strlen(response), 0);	
	printf("\nClient working directory changed \n");
	return 1;
}

int handle_list_command(char* buffer, int client_fd){

	/* get client */
	Client *client = getClient(client_fd);

	/* get data socket and send successful response */
	int data_sock = client->data_socket;
	send(data_sock, "150 File status okay; about to open. data connection.\n", strlen("150 File status okay; about to open. data connection.\n"), 0);
	
	/* get current directroy */
	DIR* directory;
	struct dirent *dir;
	directory = opendir(client->curr_dir);

	/* if directroy exists, print out the contents one by one */
	if (directory){
		int fileCount = 0;
		char directoryContent[1024] = {0};
		while((dir = readdir(directory)) != NULL){
			/* skip parent directories */
			if (strncmp(dir -> d_name, ".", 1) == 0  || strncmp(dir->d_name, "..", 2) == 0) {
		        	continue;
		    	}
		    	
		    	/* calculate number of files */
		    	fileCount++;
		    	
		    	//printf("%s", dir->d_name);
			
			/* add contents to buffer */
		    	strcat(directoryContent, dir->d_name);
		    	strcat(directoryContent, "\n");
		}

		closedir(directory);
		
		int size_list = sizeof(directoryContent);
		
		/* if 0 files in directory, state that it empty */
		if(fileCount == 0){
			strcat(directoryContent, "\n[Directory empty]\n");
		}
		
		printf("Listing directory\n");
		
		/* send reponse back to server */
		send(data_sock, &size_list, sizeof(int), 0);
		send(data_sock, directoryContent, size_list, 0);
		close(data_sock);
	}
	else{
		/* error checking */
		send(client_fd, "550 No such file or directory.", strlen("550 No such file or directory."), 0);
		
		return -1;
	}
	/* send confirmation of transfer */
	printf("226 Transfer completed.\n\n");
	send(client_fd, "226 Transfer completed.", strlen("226 Transfer completed."), 0);
	return 1;
}

int handle_retr_command(char* buffer, int client_fd){
	/* get client */
	Client *client = getClient(client_fd);

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
	
	
	/* get data socket */
	int data_socket = client->data_socket;

	/* set temporary directroy to current directory */
	char temp_dir[4096];
	getcwd(temp_dir, sizeof(temp_dir));
	
	/* change client directroy to current directory */
	chdir(client->curr_dir);
	
	/* get file size */
	long int file_size = findFileSize(filename);
	
	/* 
		if directory doesnt exist, send response to server and terminate 
		I also send size of the string back because without it, txt file
		fills up the buffer and the loop on client end fails
	*/
	if (file_size == -1){
		int size = strlen("550 No such file or directory.\n");
		send(data_socket, &size, sizeof(int), 0);
		send(data_socket, "550 No such file or directory.\n", strlen("550 No such file or directory.\n"), 0);
		close(data_socket);
		return -1;
	}
	/* if it does exist send repsponse back */
	int size = strlen("150 File status okay; about to open. data connection.\n");
	send(data_socket, &size, sizeof(int), 0);
	send(data_socket, "150 File status okay; about to open. data connection.\n", strlen( "150 File status okay; about to open. data connection.\n"), 0);

	/* send file back to client */
	if(file_size!=-1){
		
		/* open file and read it into temporary file */
		FILE* file_to_send = fopen(filename, "rb");
		char *file_data = malloc(file_size);
		fread(file_data, 1, file_size, file_to_send); 
		
		/* send it in byte size chunks */
		int sent = 0;
		while (sent < file_size) {
		/* 
			if the file size leftover is bigger than buffer (1024),
			send back full buffer
			otherwise send remainder
		*/
		
		    int b = (file_size - sent >= 1024) ? 1024 : file_size - sent;

		    send(data_socket, file_data + sent, b, 0);

		    sent += b;
		}
	
		/* close directory and file and socket, free up allocated space */
		chdir(temp_dir);
		fclose(file_to_send);
		free(file_data);

		close(data_socket);	
	}
	printf("226 Transfer completed.\n\n");
	return 1;
}


/* reverse RETR */
int handle_stor_command(char* buffer, int client_fd){
	/* get client */
	Client *client = getClient(client_fd);
	
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

	int connection_at = client -> data_socket;
	
	/* if file name is missing, send back bad response */
	if (filename == NULL){
		send(connection_at, "550 No such file or directory.\n", strlen("550 No such file or directory.\n"), 0);
		close(connection_at);
		return -1;
	}
	send(connection_at, "150 File status okay; about to open. data connection.\n", strlen("150 File status okay; about to open. data connection.\n"), 0);
	
	/* data buffer for storing incoming packets */
	char data_buffer[1024];
	
	char temp[1024];
	int randn = rand();
	/* create temp file as per assignment instructions */
	sprintf(temp, "rand%d.tmp",randn);
	
	char temp_dir[4096];
		
	/* go to client dir where the file will be stored */
	getcwd(temp_dir, sizeof(temp_dir));
	chdir(client->curr_dir);

	/* open file for binary writing */
	FILE* file = fopen(temp, "wb");
	if (file == NULL) {
		send(connection_at, "550 Cannot create file.\n", strlen("550 Cannot create file.\n"), 0);	
		fclose(file);
		return -1;
	}
	
	/* receive file in byte sized chunks */
	int b;
	while ((b = recv(connection_at, data_buffer, sizeof(data_buffer), 0)) > 0) {
		if (b < 1024) {
		    	if (b != 0){
		    		fwrite(data_buffer, 1, b, file);
		    	} 
			break;
	    	}
	    	fwrite(data_buffer, 1, b, file);  
	}

	/* close file and socket, rename it and change directory to original directory */
	fclose(file);
	close(connection_at);
	rename(temp, filename);
	chdir(temp_dir);
   	printf("226 Transfer completed.\n\n");
	return 1;
}
