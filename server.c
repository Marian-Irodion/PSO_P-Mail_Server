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

    while (1) {
        message_len = recv(client_socket, buffer, MAX_MESSAGE_SIZE, 0);

        if (message_len <= 0) {
            printf("Client disconnected.\n");
            break;
        }

        buffer[message_len] = '\0'; // Adaugă terminatorul null pentru a trata datele primite ca șir

        // Separă mesajul în destinatar, titlu și conținut
        char *recipient = strtok(buffer, "\n");
        char *title = strtok(NULL, "\n");
        char *message = strtok(NULL, "\n");

        if (recipient != NULL && title != NULL && message != NULL) {
            printf("Received message:\nRecipient: %s\nTitle: %s\nMessage: %s\n", recipient, title, message);

            // Aici poți adăuga logica pentru a trata mesajul cum dorești
            // De exemplu, poți trimite un răspuns înapoi clientului sau poți face altceva cu mesajul primit
        } else {
            printf("Invalid message format.\n");
        }
    }

    close(client_socket);
}

int main() {
    int server_socket, client_socket;
    struct sockaddr_in server_addr, client_addr;
    socklen_t addr_len = sizeof(struct sockaddr);

    // Creare socket
    server_socket = socket(AF_INET, SOCK_STREAM, 0);

    // Setare adresa serverului
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(PORT);
    server_addr.sin_addr.s_addr = htonl(INADDR_ANY);

    // Legare
    bind(server_socket, (struct sockaddr *)&server_addr, sizeof(server_addr));

    // Ascultare pentru conexiuni de intrare
    listen(server_socket, 5);

    printf("Server listening on port %d...\n", PORT);

    while (1) {
        // Acceptare conexiune
        client_socket = accept(server_socket, (struct sockaddr *)&client_addr, &addr_len);

        printf("New connection accepted\n");

        // Manipulare client
        handle_client(client_socket);
    }

    close(server_socket);

    return 0;
}
