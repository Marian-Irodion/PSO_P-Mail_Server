#include <stdio.h>
#include "rsa.h"
#include <string.h>
#include <stdlib.h>

int main(int argc, char **argv)
{
  struct public_key_class pub[1];
  struct private_key_class priv[1];
  rsa_gen_keys(pub, priv, PRIME_SOURCE_FILE);

  printf("Private Key:\n Modulus: %lld\n Exponent: %lld\n", (long long)priv->modulus, (long long) priv->exponent);
  printf("Public Key:\n Modulus: %lld\n Exponent: %lld\n", (long long)pub->modulus, (long long) pub->exponent);
  
  char message[] = "baba booey bazinga imi place femeile";
  int i;

  printf("Original:\n");
  for(i=0; i < strlen(message); i++){
    printf("%c", (long long)message[i]);
  }  
  printf("\n");
  
  long long *encrypted = rsa_encrypt(message, sizeof(message), priv);
  if (!encrypted){
    fprintf(stderr, "Error in encryption!\n");
    return 1;
  }
  printf("Encrypted:\n");
  for(i=0; i < strlen(message); i++){
    printf("%c", (long long)encrypted[i]);
  } 
  printf("\n"); 
  
  char *decrypted = rsa_decrypt(encrypted, 8*sizeof(message), pub);
  if (!decrypted){
    fprintf(stderr, "Error in decryption!\n");
    return 1;
  }
  printf("Decrypted:\n");
  for(i=0; i < strlen(message); i++){
    printf("%c", (long long)decrypted[i]);
  }  
  
  printf("\n");
  free(encrypted);
  free(decrypted);
  return 0;
}
