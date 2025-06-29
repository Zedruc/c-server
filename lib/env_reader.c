#include <stdio.h>
#include <stdlib.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "env_reader.h"

char *env = NULL; // Actual definition of the 'env' variable declared in env_reader.h

void trim_trailing_whitespace(char *str)
{
  if (!str)
    return;
  size_t len = strlen(str);
  while (len > 0 && (str[len - 1] == '\n' || str[len - 1] == '\r' || str[len - 1] == ' ' || str[len - 1] == '\t'))
  {
    str[len - 1] = '\0';
    len--;
  }
}

int env_init(const char *db_path)
{
  FILE *fp = fopen(".env", "r");
  if (!fp)
  {
    perror("Error opening .env file");
    return 0;
  }

  // Seek to end to get file size
  fseek(fp, 0, SEEK_END);
  long length = ftell(fp);
  rewind(fp);

  // Allocate memory (+1 for null terminator)
  env = malloc(length + 1);
  if (!env)
  {
    fclose(fp);
    fprintf(stderr, "Failed to allocate memory for .env file\n");
    return 0;
  }

  // Read file into env
  size_t read = fread(env, 1, length, fp);
  env[read] = '\0'; // Null-terminate
  fclose(fp);

  return 1;
}

char *env_get(const char *key)
{
  if (!env || !key)
    return NULL;

  size_t key_len = strlen(key);
  char *pos = env;

  while (pos)
  {
    // Find the key in the env buffer
    char *line_start = strstr(pos, key);
    if (!line_start)
      return NULL;

    // Check if it's a full key (key=) and at start of line or after \n
    if ((line_start == env || *(line_start - 1) == '\n') && line_start[key_len] == '=')
    {
      // Found the key line

      // Pointer to value right after '='
      char *value_start = line_start + key_len + 1;

      // Find the end of line
      char *line_end = strchr(value_start, '\n');
      if (!line_end)
        line_end = env + strlen(env); // End of buffer

      size_t value_len = line_end - value_start;

      // Allocate trimmed value string
      char *value = malloc(value_len + 1);
      if (!value)
        return NULL;

      memcpy(value, value_start, value_len);
      value[value_len] = '\0';

      // Trim trailing whitespace here if you want
      trim_trailing_whitespace(value);

      return value; // caller must free this
    }

    pos = line_start + key_len;
  }

  return NULL;
}

void env_close()
{
  if (env)
  {
    free(env);
    env = NULL;
  }
}