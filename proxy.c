#include "csapp.h"

/* Recommended max cache and object sizes */
#define HOSTNAME_MAX_LEN 63
#define PORT_MAX_LEN 10
#define HEADER_NAME_MAX_LEN 32
#define HEADER_VALUE_MAX_LEN 64
#define MAX_CACHE_SIZE 1049000
#define MAX_OBJECT_SIZE 102400

static const char *user_agent_hdr =
    "User-Agent: Mozilla/5.0 (X11; Linux x86_64; rv:10.0.3) Gecko/20120305 "
    "Firefox/10.0.3\r\n";

typedef struct {
  char host[HOSTNAME_MAX_LEN];
  char port[PORT_MAX_LEN];
  char path[MAXLINE];
} RequestLine;

typedef struct {
  char name[HEADER_NAME_MAX_LEN];
  char value[HEADER_VALUE_MAX_LEN];
} RequestHeader;

int main() {
  printf("%s", user_agent_hdr);
  return 0;
}
