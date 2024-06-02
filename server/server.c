#include "../code/commands.h"
#include "../code/selectfns.h"

#define PORT 4444
#define DATA_PORT 2709

int data_channels = 0;

static volatile int keepRunning = 1;

void catchC(int dummy) {
    keepRunning = 0;
}

int handle_port_cmd(char* request_content, int client_fd){
	/* increment data channels: (N+1) on client side */
    data_channels++;

    /* get current client */
	Client *client = getClient(client_fd);

    /* send the data channel to the current client */
	send(client_fd, &data_channels, sizeof(int), 0);

    /* 
        create a socket
	    use an int to hold the fd for the socket
    */
    int data_sock;
    /*
        1st argument: domain/family of socket. For Internet family of IPv4 addresses, we use AF_INET 
	    2nd: type of socket TCP
	    3rd: protocol for connection
    */
	data_sock = socket(AF_INET, SOCK_STREAM, 0);
	if (data_sock < 0){
		printf("socket creation failed..\n");
        return -1;
	}

    /* connect to another socket on the other side */

	/* specify an address for the socket we want to connect to */
	struct sockaddr_in data_server_addr;

    /* specify address family */
	memset(&data_server_addr, 0, sizeof(data_server_addr));  
	data_server_addr.sin_family = AF_INET;	   

    /* 
        specify the port we want to connect to remotely
        htons converts integer port to right format for the structure
        9002 just some port we know the OS is not using  
    */       
	data_server_addr.sin_port = htons(DATA_PORT);

    /* 
        specify the actual IP address
        connect to our local machine
        INADDR_ANY gets any IP address used on our local machine
        INADDR_ANY is an IP address that is used when we don't want to bind a socket to any specific IP. 

        basically, while implementing communication, we need to bind our socket to an IP address. 
        when we don't know the IP address of our machine, we can use the special IP address INADDR_ANY. 
        it allows our server to receive packets that have been targeted by any of the interfaces.
    */
	data_server_addr.sin_addr.s_addr = INADDR_ANY;

    /* 
        enable the SO_REUSEADDR option on the socket.
        this allows the server to bind to an address that is already in use.
        It is useful for allowing a server to be restarted quickly without waiting
        for the previous socket to time out.
    */
	setsockopt(data_sock, SOL_SOCKET, SO_REUSEADDR, &(int){1},sizeof(int)); 
	

    /* 
        attempt to bind the socket to the specified address and port.
        if the bind operation fails, send an error message to the client 
        indicating that the data connection cannot be opened.
    */
	if(bind(data_sock, (struct sockaddr *)&data_server_addr, sizeof(data_server_addr)) < 0) 
	{
        send(client_fd, "425 Can't open data connection", strlen("425 Can't open data connection"), 0);
        perror("Bind failed!");
		return -1;
	}
	
    /* 
        receive data from the client through the socket client_fd and store it 
        in the port_request buffer. 
    */
	char port_request[1024];
	recv(client_fd, port_request, 1024, 0);

	int h1, h2, h3, h4;
    int p1, p2;

    /*
        PORT request numbers parsed into values based on the assignment description of
        port command (RFC 959 - DATA PORT)
        
        " The argument is a HOST-PORT specification for the data port
        to be used in data connection.  There are defaults for both
        the user and server data ports, and under normal
        circumstances this command and its reply are not needed.  If
        this command is used, the argument is the concatenation of a
        32-bit internet host address and a 16-bit TCP port address.
        This address information is broken into 8-bit fields and the
        value of each field is transmitted as a decimal number (in
        character string representation).  The fields are separated
        by commas.  A port command would be:

               PORT h1,h2,h3,h4,p1,p2

        where h1 is the high order 8 bits of the internet host
        address."


    */
	sscanf(port_request, "PORT %d,%d,%d,%d,%d,%d", &h1, &h2, &h3, &h4, &p1, &p2);
    printf("%s\n", port_request);

    /*
        get client IP and PORT from the parsed parameters
    */
	char client_ip[24];
	sprintf(client_ip, "%d.%d.%d.%d", h1, h2, h3, h4);
	int client_port = p1 * 256 + p2;
	
    /*
        declare variable to hold the address information for data exchange with the client.
        set the address family to AF_INET (IPv4).
        set the port number to client_port, converting it to network byte order using htons.
        set the IP address to client_ip, converting it to a format suitable for struct in_addr using inet_addr.
    */
	struct sockaddr_in data_exchange; 
	memset(&data_exchange, 0, sizeof(data_exchange)); 
	data_exchange.sin_family = AF_INET;
	data_exchange.sin_port = htons(client_port);
	data_exchange.sin_addr.s_addr = inet_addr(client_ip);

    // same as above
	if (setsockopt(data_sock, SOL_SOCKET, SO_REUSEADDR, &(int){1}, sizeof(int)) < 0){
		perror("Socket reusable action failed!");
	}

    /* 
        attempt to connect the socket to the specified address and port.
        if the connect operation fails, send an error message to the client indicating 
        that the data connection cannot be opened.
    */
	if (connect(data_sock, (struct sockaddr *)&data_exchange, sizeof(data_exchange)) < 0){
        send(client_fd, "425 Can't open data connection", strlen("425 Can't open data connection"), 0);
        perror("Connect failed!");
		return -1;
	}

    /* 
        save the data socket into the client's unique identifier 
    */
	client -> data_socket = data_sock;

    /* if no error send success message */
    send(client_fd, "200 PORT command successful.\n", strlen("200 PORT command successful.\n"), 0);
            
	return 1;
}

int getCommands(int client_fd) {
    /* 
        create buffer and clear it so no errors occur
    */
    char buffer[1024];
    bzero(&buffer, sizeof(buffer));

    /*
        receive buffer from client socket
    */
    int bytes = recv(client_fd, buffer, sizeof(buffer), 0);

    /*
        check if client closed connection, i.e, diconnected
    */
    if (bytes <= 0) { // client has closed the connection
        printf("Closed!\n");
        removeClient(client_fd);
        close(client_fd);
        return -1;
    }

    /*
        get request format from commands.h
    */
    int req = getRequestType(buffer);
    Client *client = getClient(client_fd);

    /*
        generic switch statement to execute correct request reponses
    */
    switch (req) {
        case 0: // PORT
		/*
			very brute force but don't allow user to select port
			since user does not know the "secret unique identifier": 12345
			PORT can only be ecexuted automatically from client side
		*/
		if (strstr(buffer, "PORT 12345") == NULL) {
                	send(client_fd, "504 Command not implemented for that parameter.\n", strlen("504 Command not implemented for that parameter.\n"), 0);
                	return 0;
            	}

		if (client->login == 1 && client->isAuthenticated != 1) {
			send(client_fd, "503 Bad sequence of commands.\n", strlen("503 Bad sequence of commands.\n"), 0);
			client->login = 0;
			return 0;
		} else if (client->isAuthenticated != 1) {
			send(client_fd, "530 Not logged in.\n", strlen("530 Not logged in.\n"), 0);
			return 0;
		}

		int result = handle_port_cmd(buffer, client_fd);

		break;

        case 1: // USER
            if (client->isAuthenticated == 1) {
                send(client_fd, "530 Already logged in.\n", strlen("530 Already logged in.\n"), 0);
                return 0;
            } else if (client->login == 1 && client->isAuthenticated != 1) {
                send(client_fd, "503 Bad sequence of commands.\n", strlen("503 Bad sequence of commands.\n"), 0);
                return 0;
            }
            handle_user_command(buffer, client_fd);
            break;

        case 2: // PASS
            if (client->login != 1) {
                send(client_fd, "503 Bad sequence of commands.\n", strlen("503 Bad sequence of commands.\n"), 0);
                return 0;
            } else if (client->isAuthenticated == 1) {
                send(client_fd, "530 Already logged in.\n", strlen("530 Already logged in.\n"), 0);
                return 0;
            }
            handle_pass_command(buffer, client_fd);
            break;

        case 3: // STOR
            if (client->isAuthenticated != 1) {
                send(client_fd, "530 Not logged in.", strlen("530 Not logged in."), 0);
                return 0;
            }
            // TBD
            break;

        case 4: // RETR
            if (client->isAuthenticated != 1) {
                send(client_fd, "530 Not logged in.", strlen("530 Not logged in."), 0);
                return 0;
            }
            if (fork() == 0){
				int result = handle_retr_cmd(buffer, client_fd);
				if (result == 1){
					send(client_fd,"226 Transfer completed.\n", strlen("226 Transfer completed.\n"), 0);
				}
                exit(0);
			}
            break;
        case 5: // LIST
            if (client->isAuthenticated != 1) {
                send(client_fd, "530 Not logged in.", strlen("530 Not logged in."), 0);
                return 0;
            }
            if (fork() == 0) {
                int result = handle_list_command(buffer, client_fd);
                exit(0);
            }
            break;

        case 7: // CWD
            if (client->isAuthenticated != 1) {
                send(client_fd, "530 Not logged in.", strlen("530 Not logged in."), 0);
                return 0;
            } else if (client->login == 1 && client->isAuthenticated != 1) {
                send(client_fd, "503 Bad sequence of commands.\n", strlen("503 Bad sequence of commands.\n"), 0);
                return 0;
            }
            handle_cwd_command(buffer, client_fd);
            break;
        case 9: // PWD
            if (client->login == 1 && client->isAuthenticated != 1) {
                send(client_fd, "503 Bad sequence of commands.\n", strlen("503 Bad sequence of commands.\n"), 0);
                return 0;
            } else if (client->isAuthenticated != 1) {
                send(client_fd, "530 Not logged in.", strlen("530 Not logged in."), 0);
                return 0;
            }
            handle_pwd_command(client_fd);
            break;

        case 11: // QUIT
            handle_quit_cmd(client_fd);
            return -1;

        default:
            if (client->login == 1 && client->isAuthenticated != 1) {
                client->login = 0;
            }
            // report INVALID command
            send(client_fd, "202 Command not implemented.", strlen("202 Command not implemented."), 0);
            break;
    }
    return 0;
}


int main()
{
	/*
        check for Ctrl + C
    */
    signal(SIGINT, catchC);

    /*
        read in users from users.txt
        the function is in users.h
        the format of users is: username password (seperator it " ")
        if the file opening fails: wrong format or file does not exist, stop program
    */
    if (readUsers() == -1){
		return -1;
	}

    /*
        initialize clients by adding fd's for each user
    */
	addClients();
    
    /* code underneath from LAB */
    int server_socket = socket(AF_INET,SOCK_STREAM,0);
	printf("Server fd = %d \n", server_socket);
	
	//check for fail error
	if(server_socket<0)
	{
		perror("socket:");
		exit(EXIT_FAILURE);
	}

	//setsock
	int value  = 1;
	setsockopt(server_socket, SOL_SOCKET, SO_REUSEADDR, &value,sizeof(value));
	
	//define server address structure
	struct sockaddr_in server_address;
	bzero(&server_address,sizeof(server_address));
	server_address.sin_family = AF_INET;
	server_address.sin_port = htons(PORT);
	server_address.sin_addr.s_addr = INADDR_ANY;


	//bind
	if(bind(server_socket, (struct sockaddr*)&server_address,sizeof(server_address))<0)
	{
		perror("bind failed");
		exit(EXIT_FAILURE);
	}

	//listen
	if(listen(server_socket,5)<0)
	{
		perror("listen failed");
		close(server_socket);
		exit(EXIT_FAILURE);
	}
	

	//DECLARE 2 fd sets (file descriptor sets : a collection of file descriptors)
	fd_set all_sockets;
	fd_set ready_sockets;


	//zero out/iniitalize our set of all sockets
	FD_ZERO(&all_sockets);

	//adds one socket (the current socket) to the fd set of all sockets
	FD_SET(server_socket,&all_sockets);


	printf("Server is listening...\n");

	while(1)
	{		
		//notice so far, we have created 2 fd_sets : all_sockets , ready_sockets
		//but we have only used the all_sockets and didn't touch the ready_sockets
		//that is because select() is destructive: it's going to change the set we pass in 
		//so we need a temporary copy; that is what the other fd_set ready_sockets is for
	
		//so that is why each iteration of the loop, we copy the all_sockets set into that temp fd_set
		ready_sockets = all_sockets;


		//now call select()
		//1st argument: range of file descriptors to check  [the highest file descriptor plus one] 
		//The maximum number of sockets supported by select() has an upper limit, represented by FD_SETSIZE (typically 1024).
		//you can use any number of max connections depending on your context/requirements

		//2nd argument: set of file descriptors to check for reading (the ones we want select to keep an eye on)
		//3rd argument: set of file descriptors to check for writing (usually NULL)
		//4th argument: set of file descriptors to check for errors/exceptions (usually NULL)
		//5th argument: optional timeout value specifying the time to wait for select to compplete
		if(select(FD_SETSIZE,&ready_sockets,NULL,NULL,NULL)<0)
		{
			perror("select error");
			exit(EXIT_FAILURE);
		}

		//when select returns, we know that one of our file descriptors has work for us to do
		//but which one??
		//select returns the fd_set containing JUST the file descriptors ready for reading
		//(because select is destructive, so that is why we made the temp fd_set ready_sockets copy because we didn't want to lose the original set of file descriptors that we are watching)
		
		//to know which ones are ready, we have to loop through and check
		//go from 0 to FD_SETSIZE (the largest number of file descriptors that we can store in an fd_set)
		for(int fd = 0 ; fd < FD_SETSIZE; fd++)
		{
			//check to see if that fd is SET
			if(FD_ISSET(fd,&ready_sockets))
			{
				//if it is set, that means that fd has data that we can read right now
				//when this happens, we are interested in TWO CASES
				
				//1st case: the fd is our server socket
				//that means it is telling us there is a NEW CONNECTION that we can accept
				if(fd==server_socket)
				{
					int newClientFd = storeStateAt();

                    if(newClientFd == -1){
                        printf("Network full! \n");
                        break;
                    }
                    //accept that new connection
					int client_sd = accept(server_socket,NULL,NULL);
					printf("Client Connected fd = %d \n", client_sd);
					
					//add the newly accepted socket to the set of all sockets that we are watching
					FD_SET(client_sd, &all_sockets);

                    clients[newClientFd].client_fd = client_sd;
					
				}

				//2nd case: when the socket that is ready to read from is one from the all_sockets fd_set
				//in this case, we just want to read its data
				else if(getCommands(fd) < 0)
				{


					//once we are done handling the connection, remove the socket from the list of file descriptors that we are watching
                    FD_CLR(fd, &all_sockets);

                    

					//displaying the message received 
					// printf("%s \n", buffer);
				}
			}
		}

	}

	//close
	close(server_socket);
	return 0;
}

