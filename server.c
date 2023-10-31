#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <arpa/inet.h>
#include <unistd.h>

#define PORT 55555
#define MAX_MESSAGE_SIZE 1024

void handle_client(int client_socket) {
    char buffer[MAX_MESSAGE_SIZE];
    int message_len;

    while(1) {
        message_len = recv(client_socket, buffer, MAX_MESSAGE_SIZE, 0);

        if(message_len <= 0) {
            printf("Client disconnected.\n");
            break;
        }

        buffer[message_len] = '\0'; // Add null terminator to treat received data as string

        // Process the message (e.g., check recipient, format, etc.)
        printf("Received message: %s\n", buffer);

        // Assuming the message is valid, forward it to the client application
        send(client_socket, buffer, strlen(buffer), 0);
    }

    close(client_socket);
}

int main() {
    int server_socket, client_socket;
    struct sockaddr_in server_addr, client_addr;
    socklen_t addr_len = sizeof(struct sockaddr);

    // Create socket
    server_socket = socket(AF_INET, SOCK_STREAM, 0);

    // Set up server address
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(PORT);
    server_addr.sin_addr.s_addr = htonl(INADDR_ANY);

    // Bind
    bind(server_socket, (struct sockaddr *)&server_addr, sizeof(server_addr));

    // Listen for incoming connections
    listen(server_socket, 5);

    printf("Server listening on port %d...\n", PORT);

    while(1) {
        // Accept connection
        client_socket = accept(server_socket, (struct sockaddr *)&client_addr, &addr_len);

        printf("New connection accepted\n");

        // Handle client
        handle_client(client_socket);
    }

    close(server_socket);

    return 0;
}