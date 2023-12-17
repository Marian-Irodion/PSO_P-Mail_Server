#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <termios.h>
#include <sodium.h>

#define PORT 55555
#define MAX_MESSAGE_SIZE 1024

#define GREEN_BOLD "\033[1;32m"
#define WHITE "\033[0m"
#define BLUE "\033[34m"
#define RED_BOLD "\033[1;31m"
#define YELLOW_BOLD "\033[1;33m"

unsigned char public_key[crypto_box_SECRETKEYBYTES];
unsigned char secret_key[crypto_box_SECRETKEYBYTES];

// Funcție pentru activarea modului non-canonic
void enableRawMode()
{
    struct termios raw;
    tcgetattr(STDIN_FILENO, &raw);
    raw.c_lflag &= ~(ECHO | ICANON);
    tcsetattr(STDIN_FILENO, TCSAFLUSH, &raw);
}

// Funcție pentru dezactivarea modului non-canonic
void disableRawMode()
{
    struct termios raw;
    tcgetattr(STDIN_FILENO, &raw);
    raw.c_lflag |= (ECHO | ICANON);
    tcsetattr(STDIN_FILENO, TCSAFLUSH, &raw);
}

void print_title()
{

    printf(BLUE "███╗░░░███╗████████╗░█████╗░░░░░░███╗░░░███╗░█████╗░██╗██╗░░░░░\n");
    printf("████╗░████║╚══██╔══╝██╔══██╗░░░░░████╗░████║██╔══██╗██║██║░░░░░\n");
    printf("██╔████╔██║░░░██║░░░███████║░░░░░██╔████╔██║███████║██║██║░░░░░\n");
    printf("██║╚██╔╝██║░░░██║░░░██╔══██║░░░░░██║╚██╔╝██║██╔══██║██║██║░░░░░\n");
    printf("██║░╚═╝░██║░░░██║░░░██║░░██║░░░░░██║░╚═╝░██║██║░░██║██║███████╗\n");
    printf("╚═╝░░░░░╚═╝░░░╚═╝░░░╚═╝░░╚═╝░░░░░╚═╝░░░░░╚═╝╚═╝░░╚═╝╚═╝╚══════╝\n\n\n" WHITE);
}

void sendEncryptedMessage(int socket_desc, const unsigned char *encrypted_message, size_t message_length)
{
    // Trimite lungimea mesajului criptat la server
    if (send(socket_desc, &message_length, sizeof(message_length), 0) < 0)
    {
        printf("Unable to send message length\n");
        return;
    }

    // Așteaptă acknowledgement de la server
    int acknowledgement;
    if (recv(socket_desc, &acknowledgement, sizeof(acknowledgement), 0) < 0)
    {
        printf("Unable to receive acknowledgement\n");
        return;
    }

    if (acknowledgement != 1)
    {
        printf("Server did not acknowledge message length\n");
        return;
    }

    // Trimite mesajul criptat la server
    if (send(socket_desc, encrypted_message, message_length, 0) < 0)
    {
        printf("Unable to send encrypted message\n");
        return;
    }
}


void login(int socket_desc)
{
    char username[100], password[100];
    char server_message[200];
    char trash[2];
    fgets(trash, sizeof(trash), stdin);
try_login:
    printf("Enter username: ");
    fgets(username, sizeof(username), stdin);
    // username[strcspn(username, "\n")] = '\0'; // Elimină newline-ul de la sfârșit

    printf("Enter password: ");
    enableRawMode(); // activeaza modul non-canonic al terminalului pentru ca getchar() sa nu mai astepte apasarea tastei Enter
    int i = 0;
    char c = getchar();
    while (c != '\r' && c != '\n')
    {
        if (c == 127 && i > 0) // 127 = backspace in ASCII
        {
            password[i - 1] = ' ';
            printf("\b \b");
            i--;
        }

        if (c != 127)
        {
            printf("*");
            password[i] = c;
            i++;
        }
        c = getchar();
    }
    password[i] = '\0';
    disableRawMode();

    // Concatenare username și parolă într-un singur mesaj de trimis la server
    snprintf(server_message, sizeof(server_message), "%s%s", username, password);

    // Trimitere mesaj la server
    if (send(socket_desc, server_message, strlen(server_message), 0) < 0)
    {
        printf("Unable to send message\n");
        return;
    }

    // Primire răspuns de la server
    char server_response[200];
    if (recv(socket_desc, server_response, sizeof(server_response), 0) < 0)
    {
        printf("Unable to receive message\n");
        return;
    }

    if (strcmp(server_response, "User not authenticated") == 0)
    {
        // login(socket_desc);
        printf(RED_BOLD "\nFailed to login, incorrect credentials!\n" WHITE);
        sleep(2);
        system("clear");
        print_title();
        goto try_login;
    }
    else
    {
        printf(GREEN_BOLD "\nSuccessfully logged in. Loading profile...\n" WHITE);
        sleep(3);
        system("clear");
        print_title();
    }
}

void sendMessage(int socket_desc)
{

    char server_message[MAX_MESSAGE_SIZE + 200], client_message[MAX_MESSAGE_SIZE];
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
    if (send(socket_desc, server_message, strlen(server_message), 0) < 0)
    {
        printf("Unable to send message\n");
        return;
    }
}

void readSentMessage(int socket_desc)
{
    // Primire răspuns de la server
    char server_response;
    while (1)
    {
        recv(socket_desc, &server_response, sizeof(server_response), 0);
        if (server_response == '^')
        {
            return;
        }
        printf("%c", server_response);
    }
}

void readReceivedMessage(int socket_desc)
{
    // Primire răspuns de la server
    char server_response;
    while (1)
    {
        recv(socket_desc, &server_response, sizeof(server_response), 0);
        if (server_response == '^')
        {
            return;
        }
        printf("%c", server_response);
    }
}

void deleteMessage(int socket_desc)
{
    char server_message[200];
    char trash[2];
    fgets(trash, sizeof(trash), stdin);

    printf("Enter message ID: ");
    fgets(server_message, sizeof(server_message), stdin);
    server_message[strcspn(server_message, "\n")] = '\0'; // Elimină newline-ul de la sfârșit

    // Trimitere mesaj la server
    if (send(socket_desc, server_message, strlen(server_message), 0) < 0)
    {
        printf("Unable to send message\n");
        return;
    }
}

void createAcc(int socket_desc)
{
    char username[100], password[100];
    char server_message[200];
    char trash[2];
    fgets(trash, sizeof(trash), stdin);

    printf("Enter username: ");
    fgets(username, sizeof(username), stdin);
    username[strcspn(username, "\n")] = '\0'; // Elimină newline-ul de la sfârșit

    printf("Enter password: ");
    enableRawMode(); // activeaza modul de baza al terminalului pentru ca getchar() sa nu mai astepte apasarea tastei Enter
    int i = 0;
    char c = getchar();
    while (c != '\r' && c != '\n')
    {
        if (c == 127 && i > 0)
        {
            password[i - 1] = ' ';
            printf("\b \b");
            i--;
        }

        if (c != 127)
        {
            printf("*");
            password[i] = c;
            i++;
        }
        c = getchar();
    }

    password[i] = '\0';
    disableRawMode();

    // Concatenare username și parolă într-un singur mesaj de trimis la server
    snprintf(server_message, sizeof(server_message), "%s\n%s", username, password);

    // Trimitere mesaj la server
    if (send(socket_desc, server_message, strlen(server_message), 0) < 0)
    {
        printf(RED_BOLD "\nUnable to send message\n" WHITE);
        return;
    }

    // Primire răspuns de la server
    char server_response[200];
    if (recv(socket_desc, server_response, sizeof(server_response), 0) < 0)
    {
        printf(RED_BOLD "\nUnable to send message\n" WHITE);
        return;
    }

    if (strstr(server_response, "User created") == NULL)
    {
        printf(GREEN_BOLD "\nAccount created successfully!\n" WHITE);
        sleep(2);
        createAcc(socket_desc);
    }
}

void menu(int socket_desc)
{
    char mode[100];
    char option;
    printf("1. Send message\n");
    printf("2. Read sent message\n");
    printf("3. Read received message\n");
    printf("4. Delete message\n");
    printf("5. Exit\n");
    printf("Choose an option: ");
    option = getchar();
    while (option == '\n' || option == EOF)
    {
        option = getchar();
    }
    switch (option)
    {
    case '1':
        fflush(stdin);
        snprintf(mode, sizeof(mode), "%s", "ss\0");
        send(socket_desc, mode, strlen(mode), 0);
        system("clear");
        print_title();
        sendMessage(socket_desc);
        system("clear");
        print_title();
        break;
    case '2':
        fflush(stdin);
        snprintf(mode, sizeof(mode), "%s", "sm\0");
        send(socket_desc, mode, strlen(mode), 0);
        system("clear");
        print_title();
        readSentMessage(socket_desc);
        printf("\n---------------------\n");
        printf("\n---------------------\n");
        printf("\n---------------------\n\n");
        break;
    case '3':
        fflush(stdin);
        snprintf(mode, sizeof(mode), "%s", "rm\0");
        send(socket_desc, mode, strlen(mode), 0);
        system("clear");
        print_title();
        readReceivedMessage(socket_desc);
        printf("\n---------------------\n");
        printf("\n---------------------\n");
        printf("\n---------------------\n\n");
        break;
    case '4':
        fflush(stdin);
        snprintf(mode, sizeof(mode), "%s", "dm\0");
        send(socket_desc, mode, strlen(mode), 0);
        system("clear");
        print_title();
        deleteMessage(socket_desc);
        printf("\n---------------------\n");
        printf("\n---------------------\n");
        printf("\n---------------------\n\n");
        break;
    case '5':
        fflush(stdin);
        snprintf(mode, sizeof(mode), "%s", "ex\0");
        send(socket_desc, mode, strlen(mode), 0);
        system("clear");
        print_title();
        printf(YELLOW_BOLD "Saving data before we close out...\n" WHITE);
        sleep(3);
        close(socket_desc);
        exit(0);
    default:
        printf(RED_BOLD "Invalid option\n" WHITE);
        sleep(2);
        system("clear");
        print_title();
        break;
    }
}

void antemenu(int socket_desc)
{
start:
    char option;
    char mode[100];
    printf("1. Login\n");
    printf("2. Register\n");
    printf("3. Exit\n");
    printf("Choose an option: ");
    option = getchar();
    switch (option)
    {
    case '1':
        fflush(stdin);
        snprintf(mode, sizeof(mode), "%s", "lg\0");
        send(socket_desc, mode, strlen(mode), 0);
        system("clear");
        print_title();
        login(socket_desc);
        break;
    case '2':
        fflush(stdin);
        snprintf(mode, sizeof(mode), "%s", "rg\0");
        send(socket_desc, mode, strlen(mode), 0);
        system("clear");
        print_title();
        createAcc(socket_desc);
        break;
    case '3':
        fflush(stdin);
        system("clear");
        snprintf(mode, sizeof(mode), "%s", "ex\0");
        send(socket_desc, mode, strlen(mode), 0);
        print_title();
        printf(YELLOW_BOLD "Saving data before we close out...\n" WHITE);
        sleep(3);
        close(socket_desc);
        exit(0);
    default:
        printf(RED_BOLD "Invalid option\n" WHITE);
        sleep(2);
        system("clear");
        print_title();
        goto start;
        break;
    }
}

int main(void)
{
    int socket_desc;
    struct sockaddr_in server_addr;

    // Creare socket
    socket_desc = socket(AF_INET, SOCK_STREAM, 0);

    if (socket_desc < 0)
    {
        printf(RED_BOLD "Unable to create socket\n" WHITE);
        return -1;
    }

    printf(GREEN_BOLD "Socket created successfully\n" WHITE);
    // Setare adresa serverului
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(PORT);
    server_addr.sin_addr.s_addr = inet_addr("192.168.222.116"); // Adresa IP a serverului

    // Conectare la server
    if (connect(socket_desc, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0)
    {
        printf(RED_BOLD "Unable to connect\n" WHITE);
        return -1;
    }
    printf(GREEN_BOLD "Connected with server successfully\n" WHITE);

    recv(socket_desc, public_key, sizeof(public_key), 0);
    recv(socket_desc, secret_key, sizeof(secret_key), 0);

    // menu(socket_desc);
    system("clear");
    print_title();
    antemenu(socket_desc);
    system("clear");
    print_title();
    while (1)
        menu(socket_desc);

    return 0;
}
