#ifndef ENV_READER_H
#define ENV_READER_H

// defined in code, declared here
extern char *env;

void trim_trailing_whitespace(char *str);

int env_init();
void env_close();
char *env_get(const char *key);

#endif /* ENV_READER_H */