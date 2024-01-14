#include <sodium.h>
#include <stdio.h>
#include <string.h>

#define MESSAGE_MAX_LENGTH 100

int main() {
    if (sodium_init() < 0) {
        printf("Error initializing libsodium\n");
        return 1;
    }

    unsigned char public_key[crypto_box_PUBLICKEYBYTES];
    unsigned char secret_key[crypto_box_SECRETKEYBYTES];

    crypto_box_keypair(public_key, secret_key);

    printf("Public key: ");
    for (int i = 0; i < crypto_box_PUBLICKEYBYTES; i++) {
        printf("%02x", public_key[i]);
    }
    printf("\n");

    printf("Secret key: ");
    for (int i = 0; i < crypto_box_SECRETKEYBYTES; i++) {
        printf("%02x", secret_key[i]);
    }
    printf("\n");

    char message[MESSAGE_MAX_LENGTH];
    printf("Enter a message to encrypt: ");
    fgets(message, MESSAGE_MAX_LENGTH, stdin);
    message[strcspn(message, "\n")] = '\0'; // Remove trailing newline character

    unsigned char encrypted_message[crypto_box_SEALBYTES + strlen(message)];
    if (crypto_box_seal(encrypted_message, (const unsigned char*)message, strlen(message), public_key) != 0) {
        printf("Error encrypting message\n");
        return 1;
    }

    printf("Encrypted message: ");
    for (int i = 0; i < sizeof(encrypted_message); i++) {
        printf("%02x", encrypted_message[i]);
    }
    printf("\n");

    unsigned char decrypted_message[strlen(encrypted_message)];
    if (crypto_box_seal_open(decrypted_message, encrypted_message, sizeof(encrypted_message), public_key, secret_key) != 0) {
        printf("Error decrypting message\n");
        return 1;
    }

    decrypted_message[strlen(message)] = '\0'; // Remove trailing newline character

    printf("Decrypted message: %s\n", decrypted_message);

    return 0;
}
