#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <arpa/inet.h>
#include <unistd.h>

#define PORT 55555
#define MAX_MESSAGE_SIZE 1024

int main(void) {
    int socket_desc;
    struct sockaddr_in server_addr;
    char server_message[MAX_MESSAGE_SIZE], client_message[MAX_MESSAGE_SIZE];
    char recipient[100], title[100];

    memset(server_message, '\0', sizeof(server_message));
    memset(client_message, '\0', sizeof(client_message));

    // Creare socket
    socket_desc = socket(AF_INET, SOCK_STREAM, 0);

    if (socket_desc < 0) {
        printf("Unable to create socket\n");
        return -1;
    }

    printf("Socket created successfully\n");

    // Setare adresa serverului
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(PORT);
    server_addr.sin_addr.s_addr = inet_addr("127.0.0.1"); // Adresa IP a serverului

    // Conectare la server
    if (connect(socket_desc, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0) {
        printf("Unable to connect\n");
        return -1;
    }
    printf("Connected with server successfully\n");

    // Introducere destinatar
    printf("Enter recipient: ");
    fgets(recipient, sizeof(recipient), stdin);
    recipient[strcspn(recipient, "\n")] = '\0'; // Elimină newline-ul de la sfârșit

    // Introducere titlu
    printf("Enter title: ");
    fgets(title, sizeof(title), stdin);
    title[strcspn(title, "\n")] = '\0'; // Elimină newline-ul de la sfârșit

    // Introducere mesaj
    printf("Enter message: ");
    fgets(client_message, sizeof(client_message), stdin);
    client_message[strcspn(client_message, "\n")] = '\0'; // Elimină newline-ul de la sfârșit

    // Concatenare destinatar, titlu și mesaj într-un singur mesaj de trimis la server
    snprintf(server_message, sizeof(server_message), "%s\n%s\n%s", recipient, title, client_message);

    // Trimitere mesaj la server
    if (send(socket_desc, server_message, strlen(server_message), 0) < 0) {
        printf("Unable to send message\n");
        return -1;
    }

    // Închidere socket
    close(socket_desc);

    return 0;
}
