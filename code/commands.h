#include <stdio.h>
#include <stdlib.h>
#include <string.h> 

#include <string.h>
#include <stdio.h>

#define BUFFSIZE 1024

typedef enum {
    PORT = 0,
    USER = 1,
    PASS = 2,
    STOR = 3,
    RETR = 4,
    LIST = 5,
    LOCAL_LIST = 6,
    CWD = 7,
    LOCAL_CWD = 8,
    PWD = 9,    
    LOCAL_PWD = 10,
    QUIT = 11,
    INVALID = -1,
} CommandType;

CommandType getCommandType(const char* command) {
    if (strcmp(command, "PORT") == 0) return PORT;
    if (strcmp(command, "USER") == 0) return USER;
    if (strcmp(command, "PASS") == 0) return PASS;
    if (strcmp(command, "STOR") == 0) return STOR;
    if (strcmp(command, "RETR") == 0) return RETR;
    if (strcmp(command, "LIST") == 0) return LIST;
    if (strcmp(command, "!LIST") == 0) return LOCAL_LIST;
    if (strcmp(command, "CWD") == 0) return CWD;
    if (strcmp(command, "!CWD") == 0) return LOCAL_CWD;
    if (strcmp(command, "PWD") == 0) return PWD;
    if (strcmp(command, "!PWD") == 0) return LOCAL_PWD;
    if (strcmp(command, "QUIT") == 0) return QUIT;
    return INVALID;
}

int getRequestType(char* request_buffer) {
    char copy_buffer[1024];
	strcpy(copy_buffer, request_buffer);
    
	char* command = strtok(copy_buffer, " ");
    
    CommandType cmd_type = getCommandType(command);

    // printf("%d", cmd_type);

    if (cmd_type == INVALID) {
        printf("Invalid command\n");
    }
    
    return cmd_type;
}
