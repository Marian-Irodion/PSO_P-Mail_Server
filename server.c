#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <fcntl.h>

#define PORT 55555
#define MAX_MESSAGE_SIZE 1024

int indexCount = 0;
char globalUser[100];

//o structura care sa retina username-ul celui care a trimis mesajul, username-ul destinatarului, titlul si mesajul
struct Message {
    char ID[100];
    char sender[100];
    char recipient[100];
    char title[100];
    char message[100];
};

struct Message *loadMessages() {
    int fd = open("messages", O_RDONLY);
    char buffer[16384];
    char character;
    int bufPos=0;
    int bytesRead;
    struct Message *messages = malloc(sizeof(struct Message));
    while(1)
    {
    start:
    if(bytesRead = read(fd, &character, 1) > 0) {
        if (character == '^') {
            buffer[bufPos] = character;
            read(fd, &character, 1);
            bufPos = 0;
            break;
        } else {
            buffer[bufPos] = character;
            bufPos++;
        }
    } else {
        return messages;
    }
    }
    char *ID = strtok(buffer, " ");
    char *sender = strtok(NULL, " ");
    char *recipient = strtok(NULL, " ");
    char *title = strtok(NULL, "\n");
    char *message = strtok(NULL, "^");
    strcpy(messages[indexCount].ID, ID);
    strcpy(messages[indexCount].sender, sender);
    strcpy(messages[indexCount].recipient, recipient);
    strcpy(messages[indexCount].title, title);
    strcpy(messages[indexCount].message, message);
    indexCount++;
    messages = realloc(messages, (indexCount + 1) * sizeof(struct Message));
    goto start; 

    return messages;
}

//o functie care scrie ce tot ce este in structura Message intr-un fisier messages
void saveMessages(struct Message *messages) {
    int fd = open("messages", O_RDWR | O_CREAT | O_TRUNC, 0644);
    for(int i=0; i<indexCount; i++) {
        char buffer[8192];
        snprintf(buffer, sizeof(buffer), "%s %s %s %s\n%s^\n", messages[i].ID, messages[i].sender, messages[i].recipient, messages[i].title, messages[i].message);
        write(fd, buffer, strlen(buffer));
    }
    close(fd);
}



//o functie implementata pe server care sa caute in fisierul credentials.txt daca exista username-ul si parola introduse de utilizator
//daca exista, returneaza 1, altfel returneaza 0
//si sa folosesti doar file descriptors si apeluri de sistem
int checkCredentias(char *username, char *password) {
    int fd = open("credentials", O_RDONLY);
    char buffer[1024];
    int bytesRead;
    int bufferPos = 0;
    char character;
    char *user, *pass;
    while (1) {
        start:
        bytesRead = read(fd, &character, 1);
        if(bytesRead < 0) {
            return 0;
        }
        if (character == '\n') {
            buffer[bufferPos] = '\0';
            bufferPos = 0;
            //memset(buffer, '\0', sizeof(buffer));
            break;
        } else {
            buffer[bufferPos] = character;
            bufferPos++;
        }
    }
    user = strtok(buffer, " ");
    pass = strtok(NULL, "\n");
    if (strcmp(user, username) == 0 && strcmp(pass, password) == 0)
        return 1;
    goto start;
}


void handle_client(int client_socket) {
    char buffer[MAX_MESSAGE_SIZE];
    int credentials_len;

    while (1) {
        credentials_len = recv(client_socket, buffer, MAX_MESSAGE_SIZE, 0);

        if (credentials_len <= 0) {
            printf("Client disconnected.\n");
            break;
        }

        buffer[credentials_len] = '\0'; // Adaugă terminatorul null pentru a trata datele primite ca șir

        // Separă mesajul în destinatar, titlu și conținut
        char *username = strtok(buffer, "\n");
        char *password = strtok(NULL, "\n");

        if (username != NULL && password != NULL) {
            printf("Received message:\nUsername: %s\nParola: %s\n", username, password);
            if (checkCredentias(username, password) == 1) {
                printf("User authenticated\n");
                send(client_socket, "User authenticated", strlen("User authenticated"), 0);
                strcpy(globalUser, username);
                printf("\nUser connected: %s\n", globalUser);
                break;
            } else {
                printf("User not authenticated\n");
                send(client_socket, "User not authenticated", strlen("User not authenticated"), 0);
            }
        } else {
            printf("Invalid message format.\n");
        }
    }

    //close(client_socket);
}

//functie care sa genereze un ID unic format din 32 caractere(cifre) pentru fiecare mesaj
char* generateID() {
    char *ID = malloc(14 * sizeof(char));
    int i;
    for (i = 0; i < 13; i++) {
        ID[i] = rand() % 10 + '0';
    }
    ID[13]='\0';
    return ID;
}


//functie care sa adauge o intrare in structura messages
void addMessage(struct Message **messages, char *sender, char *recipient, char *title, char *message) {
    strcpy((*messages)[indexCount].ID, generateID());
    strcpy((*messages)[indexCount].sender, sender);
    strcpy((*messages)[indexCount].recipient, recipient);
    strcpy((*messages)[indexCount].title, title);
    strcpy((*messages)[indexCount].message, message);
    indexCount++;
    *messages = realloc(*messages, (indexCount + 1) * sizeof(struct Message));
}

void handle_message(struct Message **messages, int client_socket) {
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
            addMessage(messages, globalUser, recipient, title, message);
            break;
            // Aici poți adăuga logica pentru a trata mesajul cum dorești
            // De exemplu, poți trimite un răspuns înapoi clientului sau poți face altceva cu mesajul primit
        } else {
            printf("Invalid message format.\n");
        }
    }

    //close(client_socket);
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
    // Modifică adresa IP a serverului în codul clientului
    server_addr.sin_addr.s_addr = inet_addr("192.168.222.116");


    // Legare
    bind(server_socket, (struct sockaddr *)&server_addr, sizeof(server_addr));

    // Ascultare pentru conexiuni de intrare
    listen(server_socket, 5);

    printf("Server listening on port %d...\n", PORT);
    struct Message *messages = loadMessages();

    while (1) {
        // Acceptare conexiune

        client_socket = accept(server_socket, (struct sockaddr *)&client_addr, &addr_len);

        printf("New connection accepted\n");

        // Manipulare client
        handle_client(client_socket);
        handle_message(&messages, client_socket);
        saveMessages(messages);
    }


    close(server_socket);

    return 0;
}
