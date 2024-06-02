# Define the compiler
CC = gcc

# Define the directories
CLIENT_DIR = client
CODE_DIR = code
SERVER_DIR = server

# Define the source files
CLIENT_SRC = $(CLIENT_DIR)/client.c
SERVER_SRC = $(SERVER_DIR)/server.c

# Define the header files
HEADERS = $(CODE_DIR)/clients.h $(CODE_DIR)/commands.h $(CODE_DIR)/users.h $(CODE_DIR)/selectfns.h

# Define the output executables
CLIENT_EXEC = $(CLIENT_DIR)/client
SERVER_EXEC = $(SERVER_DIR)/server

# Define the compilation flags
CFLAGS = -I$(CODE_DIR)

all: $(CLIENT_EXEC) $(SERVER_EXEC)

$(CLIENT_EXEC): $(CLIENT_SRC) $(HEADERS)
	$(CC) $(CFLAGS) -o $(CLIENT_EXEC) $(CLIENT_SRC)

$(SERVER_EXEC): $(SERVER_SRC) $(HEADERS)
	$(CC) $(CFLAGS) -o $(SERVER_EXEC) $(SERVER_SRC)

clean:
	rm -f $(CLIENT_EXEC) $(SERVER_EXEC)

.PHONY: all clean

