#include "csapp.h"

/* Recommended max cache and object sizes */
#define HOSTNAME_MAX_LEN 63
#define PORT_MAX_LEN 16
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

void doit(int fd);
void parse_request(int fd, RequestLine *request_line, RequestHeader *headers,
                   int *num_hds);
void parse_uri(char *uri, RequestLine *request_line);
RequestHeader parse_header(char *line);
int send_to_server(RequestLine *request_line, RequestHeader *headers,
                   int num_hds);
void *thread(void *vargp);
int reader(int fd, char *uri);
void writer(char *uri, char *buf);

int main(int argc, char **argv) {
  int listenfd, *connfd;
  pthread_t tid;
  char hostname[MAXLINE], port[MAXLINE];
  socklen_t clientlen;
  struct sockaddr_storage clientaddr;

  if (argc != 2) {
    fprintf(stderr, "usage: %s <port>\n", argv[0]);
    exit(1);
  }

  listenfd = Open_listenfd(argv[1]);
  while (1) {
    clientlen = sizeof(clientaddr);
    connfd = Malloc(sizeof(int));
    *connfd = Accept(listenfd, (SA *)&clientaddr, &clientlen);
    Getnameinfo((SA *)&clientaddr, clientlen, hostname, MAXLINE, port, MAXLINE,
                0);
    printf("Accepted connection from (%s, %s)\n", hostname, port);
    Pthread_create(&tid, NULL, thread, connfd);
  }
}

void *thread(void *vargp) {
  int connfd = *((int *)vargp);
  Pthread_detach(pthread_self());
  Free(vargp);
  doit(connfd);
  Close(connfd);
  return NULL;
}

void doit(int fd) {
  char buf[MAXLINE], uri[MAXLINE], object_buf[MAX_OBJECT_SIZE];
  int total_size, connfd;
  rio_t rio;
  RequestLine request_line;
  RequestHeader headers[20];
  int num_hds, n;
  parse_request(fd, &request_line, headers, &num_hds);

  strcpy(uri, request_line.host);
  strcpy(uri + strlen(uri), request_line.path);
  if (reader(fd, uri)) {
    fprintf(stdout, "%s from cache\n", uri);
    fflush(stdout);
    return;
  }

  total_size = 0;
  connfd = send_to_server(&request_line, headers, num_hds);
  Rio_readinitb(&rio, connfd);
  while ((n = Rio_readlineb(&rio, buf, MAXLINE))) {
    Rio_writen(fd, buf, n);
    strcpy(object_buf + total_size, buf);
    total_size += n;
  }
  if (total_size < MAX_OBJECT_SIZE)
    writer(uri, object_buf);
  Close(connfd);
}

void parse_request(int fd, RequestLine *request_line, RequestHeader *headers,
                   int *num_hds) {
  char buf[MAXLINE], method[MAXLINE], uri[MAXLINE], version[MAXLINE];
  rio_t rio;

  Rio_readinitb(&rio, fd);
  if (!Rio_readlineb(&rio, buf, MAXLINE))
    return;

  sscanf(buf, "%s %s %s", method, uri, version);
  parse_uri(uri, request_line);

  *num_hds = 0;
  Rio_readlineb(&rio, buf, MAXLINE);
  while (strcmp(buf, "\r\n")) {
    headers[(*num_hds)++] = parse_header(buf);
    Rio_readlineb(&rio, buf, MAXLINE);
  }
}

void parse_uri(char *uri, RequestLine *request_line) {
  if (strstr(uri, "http://") != uri) {
    fprintf(stderr, "Error: invalid uri!\n");
    exit(1);
  }
  uri += strlen("http://");
  char *c = strstr(uri, ":");
  *c = '\0';
  strcpy(request_line->host, uri);
  uri = c + 1;
  c = strstr(uri, "/");
  *c = '\0';
  strcpy(request_line->port, uri);
  *c = '/';
  strcpy(request_line->path, c);
}

RequestHeader parse_header(char *line) {
  RequestHeader header;
  char *c = strstr(line, ":");
  if (c == NULL) {
    fprintf(stderr, "Error: invalid header: %s\n", line);
    exit(1);
  }
  *c = '\0';
  strcpy(header.name, line);
  strcpy(header.value, c + 2);
  return header;
}

int send_to_server(RequestLine *line, RequestHeader *header, int num_hds) {
  int clientfd;
  char buf[MAXLINE], *buf_head = buf;
  rio_t rio;

  clientfd = Open_clientfd(line->host, line->port);
  Rio_readinitb(&rio, clientfd);
  sprintf(buf_head, "GET %s HTTP/1.0\r\n", line->path);
  buf_head = buf + strlen(buf);
  for (int i = 0; i < num_hds; ++i) {
    sprintf(buf_head, "%s : %s", header[i].name, header[i].value);
    buf_head = buf + strlen(buf);
  }
  sprintf(buf_head, "\r\n");
  Rio_writen(clientfd, buf, MAXLINE);
  return clientfd;
}