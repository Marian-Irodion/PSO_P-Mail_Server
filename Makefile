CFLAGS = -g -Wall
LDFLAGS = -g
CC = gcc
LIBS_PATH = -L.
LDLIBS = $(LIBS_PATH) -lrsa -lm

# List of source files
SERVER_SRC = server.c
CLIENT_SRC = client.c
RSA_SRC = rsa.c

# List of object files
SERVER_OBJ = server.o
CLIENT_OBJ = client.o
RSA_OBJ = rsa.o

# Executables
SERVER_EXE = server
CLIENT_EXE = client

all: clean $(SERVER_EXE) $(CLIENT_EXE)

$(SERVER_EXE): $(SERVER_OBJ) librsa.a
	$(CC) $(LDFLAGS) -o $@ $(SERVER_OBJ) $(LDLIBS)

$(CLIENT_EXE): $(CLIENT_OBJ) librsa.a
	$(CC) $(LDFLAGS) -o $@ $(CLIENT_OBJ) $(LDLIBS)

librsa.a: $(RSA_OBJ)
	ar rc librsa.a $(RSA_OBJ)
	ranlib librsa.a

$(SERVER_OBJ): $(SERVER_SRC) rsa.h
	$(CC) -c $(CFLAGS) $(SERVER_SRC)

$(CLIENT_OBJ): $(CLIENT_SRC) rsa.h
	$(CC) -c $(CFLAGS) $(CLIENT_SRC)

$(RSA_OBJ): $(RSA_SRC) rsa.h
	$(CC) -c $(CFLAGS) $(RSA_SRC)

.PHONY: clean

clean:
	rm -f *.o $(SERVER_EXE) $(CLIENT_EXE) librsa.a