#include <openssl/hmac.h>
#include <openssl/evp.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>
#include <sign_cookie.h>
#include <str_util.h>

char* sign_scope_token(const char *token, const char *secret) {
  unsigned char* digest;
  digest = HMAC(EVP_sha256(), secret, strlen(secret), (unsigned char*)token, strlen(token), NULL, NULL);

  // Convert to hex
  char *signature = malloc(65);
  for (int i = 0; i < 32; i++) {
      sprintf(&signature[i * 2], "%02x", digest[i]);
  }

  return signature;
}

int get_signed_cookie_value(const char* cookie_value, char** out_value) {
  // Split "uuid.signature"
  const char *dot = strchr(cookie_value, '.');
  if (!dot) return 0;

  size_t token_len = dot - cookie_value;
  char *token = strndup(cookie_value, token_len);

  *out_value = token;
  return 1;
}

int verify_scope_token(const char *cookie_value, const char *secret) {
  // Split "uuid.signature"
  const char *dot = strchr(cookie_value, '.');
  if (!dot) return 0;

  size_t token_len = dot - cookie_value;
  char *token = strndup(cookie_value, token_len);
  const char *provided_sig = dot + 1;

  // Recalculate signature
  unsigned char* digest = HMAC(EVP_sha256(), secret, strlen(secret), (unsigned char*)token, token_len, NULL, NULL);

  // Convert to hex
  char expected_sig[65] = {0};
  for (int i = 0; i < 32; i++) {
      sprintf(&expected_sig[i * 2], "%02x", digest[i]);
  }

  int result = (strcmp(expected_sig, provided_sig) == 0);
  free(token);
  return result;
}