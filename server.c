#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <fcntl.h>
#include <time.h>
#include <sodium.h>
#include <pthread.h>
#include <sys/file.h>

#define PORT 55555
#define MAX_MESSAGE_SIZE 1024

#define MAX_EVENTS 10
#define MAX_THREADS 5

int indexCount = 0;
int thread_count = 0;
char globalUser[100];
pthread_mutex_t messages_mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t visibility_list_mutex = PTHREAD_MUTEX_INITIALIZER;
unsigned char public_key[crypto_box_PUBLICKEYBYTES];
unsigned char secret_key[crypto_box_SECRETKEYBYTES];

// Structura unui nod din lista de vizibilitate a utilizatorilor
struct Node
{
    char username[100];
    struct Node *next;
};

// Structura listei de vizibilitate a utilizatorilor
struct visibilityUsers
{
    struct Node *head;
};

typedef struct
{
    int client_socket;
    int terminate;
} ClientInfo;

// Funcție pentru a inițializa o listă nouă de vizibilitate a utilizatorilor
struct visibilityUsers *createVisibilityUsers()
{
    pthread_mutex_lock(&visibility_list_mutex);

    struct visibilityUsers *list = (struct visibilityUsers *)malloc(sizeof(struct visibilityUsers));
    list->head = NULL;

    pthread_mutex_unlock(&visibility_list_mutex);
    return list;
}

// Funcție pentru a adăuga un utilizator în lista de vizibilitate a utilizatorilor
void addUser(struct visibilityUsers *list, const char *username)
{
    pthread_mutex_lock(&visibility_list_mutex);

    struct Node *newNode = (struct Node *)malloc(sizeof(struct Node));
    strncpy(newNode->username, username, sizeof(newNode->username) - 1);
    newNode->username[sizeof(newNode->username) - 1] = '\0';
    newNode->next = list->head;
    list->head = newNode;

    pthread_mutex_unlock(&visibility_list_mutex);
}

// Funcție pentru a obține toate string-urile din lista de vizibilitate
char *getAllVisibilityUsers(struct visibilityUsers *list)
{
    pthread_mutex_lock(&visibility_list_mutex);

    struct Node *current = list->head;
    char *result = malloc(sizeof(char) * 100); // dimensiunea este exemplificativă
    memset(result, '\0', sizeof(result));

    while (current != NULL)
    {
        strcat(result, current->username);
        strcat(result, " ");
        current = current->next;
    }
    result[strlen(result) - 1] = '\n'; // Elimină ultimul spațiu din șirul rezultat
    pthread_mutex_unlock(&visibility_list_mutex);

    return result;
}

// Funcție pentru a șterge un utilizator din lista de vizibilitate a utilizatorilor
void removeUser(struct visibilityUsers *list, const char *username)
{
    struct Node *current = list->head;
    struct Node *prev = NULL;

    while (current != NULL)
    {
        if (strcmp(current->username, username) == 0)
        {
            if (prev == NULL)
            {
                list->head = current->next;
            }
            else
            {
                prev->next = current->next;
            }
            free(current);
            return;
        }
        prev = current;
        current = current->next;
    }
}

// Funcție pentru a elibera memoria ocupată de lista de vizibilitate a utilizatorilor
void freeVisibilityUsers(struct visibilityUsers *list)
{
    struct Node *current = list->head;
    struct Node *temp;

    while (current != NULL)
    {
        temp = current->next;
        free(current);
        current = temp;
    }
    free(list);
}

// o structura care sa retina username-ul celui care a trimis mesajul, username-ul destinatarului, titlul si mesajul
struct Message
{
    char ID[100];
    char sender[100];
    char recipient[100];
    char title[100];
    char message[100];
    struct visibilityUsers *visibilityList;
};

struct Message *loadMessages()
{
    pthread_mutex_lock(&messages_mutex);

    int fd = open("messages", O_RDONLY);
    lseek(fd, 0, SEEK_SET);
    char buffer[16384];
    char character;
    int bufPos = 0;
    int bytesRead;

    struct Message *messages = (struct Message *)malloc((indexCount + 1) * sizeof(struct Message));
    while (1)
    {
    start:
        if (bytesRead = read(fd, &character, 1) > 0)
        {
            if (character == '^')
            {
                buffer[bufPos] = character;
                read(fd, &character, 1);
                bufPos = 0;
                break;
            }
            else
            {
                buffer[bufPos] = character;
                bufPos++;
            }
        }
        else
        {
            close(fd);
            pthread_mutex_unlock(&messages_mutex);
            return messages;
        }
    }
    char *ID = strtok(buffer, " ");
    char *sender = strtok(NULL, " ");
    char *recipient = strtok(NULL, " ");
    char *title = strtok(NULL, "\n");
    char *firstUser = strtok(NULL, " ");
    char *secondUser = strtok(NULL, "\n");
    char *message = strtok(NULL, "^");
    strncpy(messages[indexCount].ID, ID, strlen(ID));
    strncpy(messages[indexCount].sender, sender, strlen(sender));
    strncpy(messages[indexCount].recipient, recipient, strlen(recipient));
    strncpy(messages[indexCount].title, title, strlen(title));
    strncpy(messages[indexCount].message, message, strlen(message));
    messages[indexCount].visibilityList = createVisibilityUsers();
    addUser(messages[indexCount].visibilityList, firstUser);
    addUser(messages[indexCount].visibilityList, secondUser);
    indexCount++;
    messages = realloc(messages, (indexCount + 1) * sizeof(struct Message));
    goto start;

    close(fd);
    pthread_mutex_unlock(&messages_mutex);

    return messages;
}

// o functie care scrie ce tot ce este in structura Message intr-un fisier messages
void saveMessages(struct Message *messages)
{
    pthread_mutex_lock(&messages_mutex);

    int fd = open("messages", O_RDWR | O_CREAT | O_TRUNC, 0644);
    for (int i = 0; i < indexCount; i++)
    {
        char buffer[8192];
        char *allVisibileUsers = getAllVisibilityUsers(messages[i].visibilityList);
        snprintf(buffer, sizeof(buffer), "%s %s %s %s\n%s%s^\n", messages[i].ID, messages[i].sender, messages[i].recipient, messages[i].title, allVisibileUsers, messages[i].message);
        write(fd, buffer, strlen(buffer));
    }
    close(fd);

    pthread_mutex_unlock(&messages_mutex);
}

// o functie implementata pe server care sa caute in fisierul credentials.txt daca exista username-ul si parola introduse de utilizator
// daca exista, returneaza 1, altfel returneaza 0
// si sa folosesti doar file descriptors si apeluri de sistem
int checkCredentias(char *username, char *password)
{
    int fd = open("credentials", O_RDONLY);
    char buffer[1024];
    int bytesRead;
    int bufferPos = 0;
    char character;
    char *user, *pass;
    while (1)
    {
    start:
        bytesRead = read(fd, &character, 1);
        if (bytesRead <= 0)
        {
            return 0;
        }
        if (character == '\n')
        {
            buffer[bufferPos] = '\0';
            bufferPos = 0;
            // memset(buffer, '\0', sizeof(buffer));
            break;
        }
        else
        {
            buffer[bufferPos] = character;
            bufferPos++;
        }
    }
    user = strtok(buffer, " ");
    pass = strtok(NULL, "\n");
    if (user == NULL || pass == NULL)
    {
        return 0;
    }
    if (strcmp(user, username) == 0 && strcmp(pass, password) == 0)
        return 1;

    goto start;
}

void handle_client(int client_socket)
{
    char buffer[MAX_MESSAGE_SIZE];
    int credentials_len;

    while (1)
    {
        credentials_len = recv(client_socket, buffer, MAX_MESSAGE_SIZE, 0);

        if (credentials_len <= 0)
        {
            printf("Client disconnected.\n");
            break;
        }

        buffer[credentials_len] = '\0'; // Adaugă terminatorul null pentru a trata datele primite ca șir

        // Separă mesajul în destinatar, titlu și conținut
        char *username = strtok(buffer, "\n");
        char *password = strtok(NULL, "\n");

        if (username != NULL && password != NULL)
        {
            printf("Received message:\nUsername: %s\nParola: %s\n", username, password);
            if (checkCredentias(username, password) == 1)
            {
                printf("User authenticated\n");
                send(client_socket, "User authenticated", strlen("User authenticated"), 0);
                strcpy(globalUser, username);
                printf("\nUser connected: %s\n", globalUser);
                break;
            }
            else
            {
                printf("User not authenticated\n");
                send(client_socket, "User not authenticated", strlen("User not authenticated"), 0);
            }
        }
        else
        {
            printf("Invalid message format.\n");
        }
    }

    // close(client_socket);
}

// functie care sa genereze un ID unic format din 32 caractere(cifre) pentru fiecare mesaj
char *generateID()
{
    char *ID = malloc(14 * sizeof(char));
    int i;
    for (i = 0; i < 13; i++)
    {
        ID[i] = rand() % 10 + '0';
    }
    ID[13] = '\0';
    return ID;
}

// functie care sa adauge o intrare in structura messages
void addMessage(struct Message **messages, char *sender, char *recipient, char *title, char *message)
{
    pthread_mutex_lock(&messages_mutex);

    *messages = realloc(*messages, (indexCount + 1) * sizeof(struct Message));

    // Verificați dacă realocarea a avut succes
    if (*messages == NULL)
    {
        perror("Realloc failed\n");
        pthread_mutex_unlock(&messages_mutex);
        return;
    }

    // strcpy((*messages)[indexCount].ID, generateID());
    // strcpy((*messages)[indexCount].sender, sender);
    // strcpy((*messages)[indexCount].recipient, recipient);
    // strcpy((*messages)[indexCount].title, title);
    // strcpy((*messages)[indexCount].message, message);
    struct Message *newMessage = &(*messages)[indexCount];
    if (indexCount < 0 || indexCount >= INT_MAX)
    {
        perror("IndexCount exceeds limits\n");
        pthread_mutex_unlock(&messages_mutex);
        return;
    }

    snprintf(newMessage->ID, sizeof(newMessage->ID), "%s", generateID());
    newMessage->ID[strlen(newMessage->ID)] = '\0'; // Asigurați-vă că ID-ul este null-terminated

    strncpy(newMessage->sender, sender, 100);
    newMessage->sender[strlen(newMessage->sender)] = '\0'; // Asigurați-vă că sender este null-terminated

    strncpy(newMessage->recipient, recipient, 100);
    newMessage->recipient[strlen(newMessage->recipient)] = '\0'; // Asigurați-vă că recipient este null-terminated

    strncpy(newMessage->title, title, 100);
    newMessage->title[strlen(newMessage->title)] = '\0'; // Asigurați-vă că title este null-terminated

    strncpy(newMessage->message, message, 100);
    newMessage->message[strlen(newMessage->message)] = '\0';

    newMessage->visibilityList = createVisibilityUsers();
    if (newMessage->visibilityList == NULL)
    {
        perror("Failed to create visibility list\n");
        pthread_mutex_unlock(&messages_mutex);
        return;
    }

    addUser(newMessage->visibilityList, sender);
    addUser(newMessage->visibilityList, recipient);

    indexCount++;

    pthread_mutex_unlock(&messages_mutex);
}

void handle_message(struct Message **messages, int client_socket)
{
    char buffer[MAX_MESSAGE_SIZE];
    int message_len;

    while (1)
    {
        pthread_mutex_lock(&messages_mutex);

        message_len = recv(client_socket, buffer, MAX_MESSAGE_SIZE, 0);

        pthread_mutex_unlock(&messages_mutex);

        if (message_len <= 0)
        {
            printf("Client disconnected.\n");
            break;
        }

        buffer[message_len] = '\0'; // Adaugă terminatorul null pentru a trata datele primite ca șir

        // Separă mesajul în destinatar, titlu și conținut
        char *recipient = strtok(buffer, "\n");
        char *title = strtok(NULL, "\n");
        char *message = strtok(NULL, "\n");

        if (recipient != NULL && title != NULL && message != NULL)
        {
            printf("Received message:\nRecipient: %s\nTitle: %s\nMessage: %s\n", recipient, title, message);

            //pthread_mutex_lock(&messages_mutex);
            addMessage(messages, globalUser, recipient, title, message);
            saveMessages(*messages);
            //pthread_mutex_unlock(&messages_mutex);
            break;
            // Aici poți adăuga logica pentru a trata mesajul cum dorești
            // De exemplu, poți trimite un răspuns înapoi clientului sau poți face altceva cu mesajul primit
        }
        else
        {
            printf("Invalid message format.\n");
        }
    }

    // close(client_socket);
}

void readSentMessage(struct Message *messages, int socket_desc)
{
    pthread_mutex_lock(&messages_mutex);

    for (int i = 0; i < indexCount; i++)
    {
        if (messages[i].visibilityList == NULL)
        {
            printf("Error: visibilityList is NULL for index %d\n", i);
            continue;
        }

        struct Node *current = messages[i].visibilityList->head;

        while (current != NULL)
        {
            if (strcmp(globalUser, current->username) == 0 && strcmp(globalUser, messages[i].sender) == 0)
            {
                char buffer[MAX_MESSAGE_SIZE];
                snprintf(buffer, sizeof(buffer), "ID: %s\nCatre: %s\nTitlul: %s\nContinut: %s\n\n", messages[i].ID, messages[i].recipient, messages[i].title, messages[i].message);
                send(socket_desc, buffer, strlen(buffer), 0);

                // pthread_mutex_unlock(&messages_mutex);
                // send(socket_desc, "^", strlen("^"), 0);
                break; // În momentul în care găsim utilizatorul în lista de vizibilitate, trimitem mesajul și ieșim din buclă
            }
            current = current->next;
        }
    }

    pthread_mutex_unlock(&messages_mutex);
    // Încheiere trimitere
    send(socket_desc, "^", strlen("^"), 0);
}

void readReceivedMessage(struct Message *messages, int socket_desc)
{
    pthread_mutex_lock(&messages_mutex);

    for (int i = 0; i < indexCount; i++)
    {
        if (messages[i].visibilityList == NULL)
        {
            printf("Error: visibilityList is NULL for index %d\n", i);
            continue;
        }
        struct Node *current = messages[i].visibilityList->head;
        while (current != NULL)
        {
            if (strcmp(globalUser, current->username) == 0 && strcmp(globalUser, messages[i].recipient) == 0)
            {
                char buffer[MAX_MESSAGE_SIZE];
                snprintf(buffer, sizeof(buffer), "ID: %s\nDe la: %s\nTitlul: %s\nContinut: %s\n\n", messages[i].ID, messages[i].sender, messages[i].title, messages[i].message);
                send(socket_desc, buffer, strlen(buffer), 0);

                // pthread_mutex_unlock(&messages_mutex);
                // send(socket_desc, "^", strlen("^"), 0);
                break; // Ieșire din buclă când utilizatorul este găsit în lista de vizibilitate
            }
            current = current->next;
        }
    }

    pthread_mutex_unlock(&messages_mutex);
    // Încheiere trimitere
    send(socket_desc, "^", strlen("^"), 0);
}

void deleteMessage(struct Message **messages, int socket_desc)
{
    // in aceasta functie trebuie sa stergi mesajul cu ID-ul primit de la client
    // in lista visibilityUsers a mesajului cu ID-ul primit de la client trebuie ca username-ul clientului sa fie inlocuit cu string-ul "null"
    // dupa asta, daca in lista visibilityUsers a mesajului cu ID-ul primit de la client nu mai exista niciun username diferit de "null", atunci mesajul trebuie sters din structura messages
    // si sa folosesti doar file descriptors si apeluri de sistem
    char server_message[200];
    char ID[14];
    ssize_t bytesRead = recv(socket_desc, ID, sizeof(ID) - 1, 0);
    if (bytesRead <= 0)
    {
        close(socket_desc);
        return;
    }

    printf("ID primit: %s\n", ID);
    ID[bytesRead] = '\0';

    pthread_mutex_lock(&messages_mutex);

    for (int i = 0; i < indexCount; i++)
    {
        if (strcmp(ID, (*messages)[i].ID) == 0)
        {
            struct Node *current = (*messages)[i].visibilityList->head;
            while (current != NULL)
            {
                if (strcmp(current->username, globalUser) == 0)
                {
                    strncpy(current->username, "null", strlen("null"));
                }
                current = current->next;
            }

            struct Node *aux = (*messages)[i].visibilityList->head;
            while (aux != NULL)
            {
                if (strcmp(aux->username, "null") != 0)
                {
                    pthread_mutex_unlock(&messages_mutex);
                    return;
                }
                aux = aux->next;
            }

            close(socket_desc);
            for (int j = i; j < indexCount - 1; j++)
            {
                (*messages[j]) = (*messages)[j + 1];
            }

            indexCount--;
            *messages = realloc(*messages, indexCount * sizeof(struct Message));

            break;
        }
    }

    pthread_mutex_unlock(&messages_mutex);
    saveMessages(*messages);
}

void handle_createAccount(int socket_desc)
{
    char buffer[MAX_MESSAGE_SIZE];
    int credentials_len;

    while (1)
    {
        credentials_len = recv(socket_desc, buffer, MAX_MESSAGE_SIZE, 0);

        if (credentials_len <= 0)
        {
            printf("Client disconnected.\n");
            break;
        }

        buffer[credentials_len] = '\0'; // Adaugă terminatorul null pentru a trata datele primite ca șir

        // Separă mesajul în destinatar, titlu și conținut
        char *username = strtok(buffer, "\n");
        char *password = strtok(NULL, "\0");

        if (username != NULL && password != NULL)
        {
            printf("Received message:\nUsername: %s\nParola: %s\n", username, password);
            if (checkCredentias(username, password) == 1)
            {
                printf("User already exists\n");
                send(socket_desc, "User already exists", strlen("User already exists"), 0);
            }
            else
            {
                printf("User created\n");
                send(socket_desc, "User created", strlen("User created"), 0);
                int fd = open("credentials", O_RDWR | O_APPEND, 0644);
                char buffer[1024];
                snprintf(buffer, sizeof(buffer), "%s %s\n", username, password);
                write(fd, buffer, strlen(buffer));
                close(fd);
                strncpy(globalUser, username, strlen(username));
                printf("\nUser connected: %s\n", globalUser);
                break;
            }
        }
        else
        {
            printf("Invalid message format.\n");
        }
    }

    // close(client_socket);
}

void *client_handler(void *arg)
{
    ClientInfo *ci = (ClientInfo *)arg;
    int client_socket = ci->client_socket;
    struct Message *messages = loadMessages();
    char mode[100];
    while (!ci->terminate)
    {
        // handle_client(client_socket);
        recv(client_socket, mode, 100, 0);
        if (strcmp(mode, "lg") == 0)
        {
            handle_client(client_socket);
        }
        else if (strcmp(mode, "rg") == 0)
        {
            handle_createAccount(client_socket);
        }
        else if (strcmp(mode, "ex") == 0)
        {
            printf("Client disconnected.\n");
            ci->terminate = 1;
            close(client_socket);
            break;
        }

        while (!ci->terminate)
        {
            recv(client_socket, mode, 100, 0);
            printf("Mode: %s\n", mode);
            if (strcmp(mode, "ss") == 0)
            {
                handle_message(&messages, client_socket);
            }
            else if (strcmp(mode, "sm") == 0)
            {
                readSentMessage(messages, client_socket);
            }
            else if (strcmp(mode, "rm") == 0)
            {
                readReceivedMessage(messages, client_socket);
            }
            else if (strcmp(mode, "dm") == 0)
            {
                deleteMessage(&messages, client_socket);
            }
            else if (strcmp(mode, "ex") == 0)
            {
                printf("Client exited.\n");
                // saveMessages(messages);
                ci->terminate = 1;
                close(client_socket);
                break;
            }
        }
    }

    close(client_socket);
    thread_count--;
    free(ci);
    return NULL;
}

int main()
{
    int server_socket;
    struct sockaddr_in server_addr;
    socklen_t addr_len = sizeof(struct sockaddr);
    srand(time(NULL));

    // Creare socket
    server_socket = socket(AF_INET, SOCK_STREAM, 0);

    // Setare adresa serverului
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(PORT);
    // Modifică adresa IP a serverului în codul clientului
    server_addr.sin_addr.s_addr = inet_addr("192.168.225.119");

    // Legare
    bind(server_socket, (struct sockaddr *)&server_addr, sizeof(server_addr));

    // Ascultare pentru conexiuni de intrare
    listen(server_socket, 5);

    printf("Server listening on port %d...\n", PORT);

    pthread_t client_threads[MAX_THREADS];

    while (1)
    {
        struct sockaddr_in client_addr;
        socklen_t client_len = sizeof(client_addr);

        crypto_box_keypair(public_key, secret_key);
        int client_socket = accept(server_socket, (struct sockaddr *)&client_addr, &client_len);
        if (client_socket == -1)
        {
            goto end;
        }

        if (thread_count < MAX_THREADS)
        {
            ClientInfo *ci = (ClientInfo *)malloc(sizeof(ClientInfo));
            ci->client_socket = client_socket;
            ci->terminate = 0;

            pthread_t tid;
            if (pthread_create(&tid, NULL, client_handler, ci) != 0)
            {
                perror("Failed to create thread for client.\n");
                close(client_socket);
                free(ci);
                continue;
            }

            client_threads[thread_count++] = tid;

            pthread_detach(tid);

            send(client_socket, public_key, strlen(public_key), 0);
            send(client_socket, secret_key, strlen(secret_key), 0);

            printf("Client %d connected.\n", thread_count);
        }
        else
        {
            printf("Max client limit reached. Connection refused.\n");
            close(client_socket);
        }
    end:
    }

    for (int i = 0; i < thread_count; ++i)
    {
        pthread_join(client_threads[i], NULL);
    }

    close(server_socket);
    return 0;
}
