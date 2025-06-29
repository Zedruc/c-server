#ifndef SIGN_COOKIE_H
#define SIGN_COOKIE_H

char* sign_scope_token(const char *token, const char *secret);
int get_signed_cookie_value(const char* cookie_value, char** out_value); 
int verify_scope_token(const char *cookie_value, const char *secret);

#endif