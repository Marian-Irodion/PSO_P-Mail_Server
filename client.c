#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <arpa/inet.h>
#include <unistd.h>

#define PORT 55555
#define MAX_MESSAGE_SIZE 1024

void login(int socket_desc) {
    char username[100], password[100];
    char server_message[200];
    char trash[2];
    fgets(trash, sizeof(trash), stdin);

    printf("Enter username: ");
    fgets(username, sizeof(username), stdin);
    //username[strcspn(username, "\n")] = '\0'; // Elimină newline-ul de la sfârșit

    printf("Enter password: ");
    fgets(password, sizeof(password), stdin);
    //password[strcspn(password, "\n")] = '\0'; // Elimină newline-ul de la sfârșit

    // Concatenare username și parolă într-un singur mesaj de trimis la server
    snprintf(server_message, sizeof(server_message), "%s%s", username, password);

    // Trimitere mesaj la server
    if (send(socket_desc, server_message, strlen(server_message), 0) < 0) {
        printf("Unable to send message\n");
        return;
    }

    // Primire răspuns de la server
    char server_response[200];
    if (recv(socket_desc, server_response, sizeof(server_response), 0) < 0) {
        printf("Unable to receive message\n");
        return;
    }

    if(strstr(server_response, "User authenticated") == NULL) {
        login(socket_desc);
    }
}

void sendMessage(int socket_desc) {

    char server_message[MAX_MESSAGE_SIZE+200], client_message[MAX_MESSAGE_SIZE];
    char recipient[100], title[100];
    char trash[2];
    fgets(trash, sizeof(trash), stdin);
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
        return;
    }
}

void readSentMessage(int socket_desc) {
    char server_message[200];

    // Primire răspuns de la server
    char server_response;
    while(1) {
        recv(socket_desc, &server_response, sizeof(server_response), 0);
        if(server_response == '^') {
            return;
        }
        printf("%c", server_response);
    }
}

void readReceivedMessage(int socket_desc) {}

void deleteMessage(int socket_desc) {}

void createAcc(int socket_desc) {
    char username[100], password[100];
    char server_message[200];
    char trash[2];
    fgets(trash, sizeof(trash), stdin);

    printf("Enter username: ");
    fgets(username, sizeof(username), stdin);
    username[strcspn(username, "\n")] = '\0'; // Elimină newline-ul de la sfârșit

    printf("Enter password: ");
    fgets(password, sizeof(password), stdin);
    password[strcspn(password, "\n")] = '\0'; // Elimină newline-ul de la sfârșit

    // Concatenare username și parolă într-un singur mesaj de trimis la server
    snprintf(server_message, sizeof(server_message), "%s\n%s", username, password);

    // Trimitere mesaj la server
    if (send(socket_desc, server_message, strlen(server_message), 0) < 0) {
        printf("Unable to send message\n");
        return;
    }

    // Primire răspuns de la server
    char server_response[200];
    if (recv(socket_desc, server_response, sizeof(server_response), 0) < 0) {
        printf("Unable to receive message\n");
        return;
    }

    if(strstr(server_response, "User created") == NULL) {
        createAcc(socket_desc);
    }
}

void menu(int socket_desc) {
    char mode[100];
    char option;
    printf("1. Send message\n");
    printf("2. Read sent message\n");
    printf("3. Read received message\n");
    printf("4. Delete message\n");
    printf("5. Exit\n");
    printf("Choose an option: ");
    option=getchar();
    switch (option) {
        case '1':
            fflush(stdin);
            snprintf(mode, sizeof(mode), "%s", "ss\0");
            send(socket_desc, mode, strlen(mode), 0);
            sendMessage(socket_desc);
            break;
        case '2':
            fflush(stdin);
            snprintf(mode, sizeof(mode), "%s", "sm\0");
            send(socket_desc, mode, strlen(mode), 0);
            readSentMessage(socket_desc);
            break;
        case '3':
            fflush(stdin);
            snprintf(mode, sizeof(mode), "%s", "rm\0");
            send(socket_desc, mode, strlen(mode), 0);
            readReceivedMessage(socket_desc);
            break;
        case '4':
            fflush(stdin);
            snprintf(mode, sizeof(mode), "%s", "dm\0");
            send(socket_desc, mode, strlen(mode), 0);
            deleteMessage(socket_desc);
            break;
        case '5':
            fflush(stdin);
            exit(0);
        default:
            printf("Invalid option\n");
            break;
    }
}

void antemenu(int socket_desc) {
    char option;
    char mode[100];
    printf("1. Login\n");
    printf("2. Register\n");
    printf("3. Exit\n");
    printf("Choose an option: ");
    option=getchar();
    switch (option) {
        case '1':
            fflush(stdin);
            snprintf(mode, sizeof(mode), "%s", "lg\0");
            send(socket_desc, mode, strlen(mode), 0);
            login(socket_desc);
            break;
        case '2':
            fflush(stdin);
            snprintf(mode, sizeof(mode), "%s", "rg\0");
            send(socket_desc, mode, strlen(mode), 0);
            createAcc(socket_desc);
            break;
        case '3':
            fflush(stdin);
            exit(0);
        default:
            printf("Invalid option\n");
            break;
    }
}

int main(void) {
    int socket_desc;
    struct sockaddr_in server_addr;

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
    server_addr.sin_addr.s_addr = inet_addr("192.168.222.116"); // Adresa IP a serverului

    // Conectare la server
    if (connect(socket_desc, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0) {
        printf("Unable to connect\n");
        return -1;
    }
    printf("Connected with server successfully\n");

    //menu(socket_desc);
    system("clear");
    printf("-------------------------------------------------------------------------\n\n\n");
    printf("\t\t\tBine ai venit in aplicatia noastra de mail!\n\n\n");
    printf("-------------------------------------------------------------------------\n\n\n");
    antemenu(socket_desc);
    system("clear");
    menu(socket_desc);

    return 0;
}
