
#include <arpa/inet.h>
#include <errno.h>
#include <fcntl.h>
#include <netinet/in.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/epoll.h>
#include <sys/socket.h>
#include <unistd.h>

#define MAXLINE 1024
#define MAX_EVENT 500
#define LISTENQ 100
#define SERV_PORT 8080

void get_client_ip_str(char *client_ip, int socket) {
  struct sockaddr_in addr;
  socklen_t addr_size = sizeof(struct sockaddr_in);
  int res = getpeername(socket, (struct sockaddr *)&addr, &addr_size);

  strcpy(client_ip, inet_ntoa(addr.sin_addr));
  printf("remote ip is: %s\n", client_ip);
}

void build_lenght_line(char *content_length, char *content) {
  // ensures the memory is an empty string
  content_length[0] = '\0';
  char content_length_value[3] = {'\n'};
  sprintf(content_length_value, "%d", (int)strlen(content));
  char *content_length_key = "Content-Length: ";
  strcat(content_length, content_length_key);
  strcat(content_length, content_length_value);
  strcat(content_length, "\r\n\r\n");
}

void write_socket(int socket) {
  char *response_ok = "HTTP/1.1 200 OK\r\n";
  char *response_content_type = "Content-Type: text/plain\r\n";
  char *response_close = "Connection: close\r\n";

  char client_ip[20];
  get_client_ip_str(client_ip, socket);

  char response_content_length[20];
  build_lenght_line(response_content_length, client_ip);

  // http header
  send(socket, response_ok, strlen(response_ok), 0);
  send(socket, response_content_type, strlen(response_content_type), 0);
  send(socket, response_close, strlen(response_close), 0);
  send(socket, response_content_length, strlen(response_content_length), 0);

  // http body
  send(socket, client_ip, strlen(client_ip), 0);
}

void handle_out_event(int epfd, struct epoll_event *event) {
  int sockfd = event->data.fd;
  write_socket(sockfd);

  /* epoll_ctl(epfd, EPOLL_CTL_DEL, sockfd, NULL); */
  /* close(sockfd); */
}

void handle_in_event(int epfd, struct epoll_event *event) {
  int sockfd = event->data.fd;

  char buffer[MAXLINE] = {0};

  int n;
  if ((n = read(sockfd, buffer, MAXLINE)) > 0) {
    printf("request content: \n%s\n", buffer);
    struct epoll_event ev;
    ev.data.fd = sockfd;
    ev.events = EPOLLOUT | EPOLLET | EPOLLRDHUP | EPOLLHUP;
    epoll_ctl(epfd, EPOLL_CTL_MOD, sockfd, &ev);
  } else {
    close(sockfd);
  }
}

void setnonblocking(int sock) {
  int opts;
  opts = fcntl(sock, F_GETFL);

  if (opts < 0) {
    perror("fcntl(sock, GETFL)");
    exit(1);
  }

  opts = opts | O_NONBLOCK;

  if (fcntl(sock, F_SETFL, opts) < 0) {
    perror("fcntl(sock, SETFL, opts)");
    exit(1);
  }
}

void handle_listen(int epfd, int listenfd) {
  struct sockaddr_in clientaddr;
  socklen_t clilen = sizeof(struct sockaddr_in);
  printf("accept connection, fd is %d\n", listenfd);
  int connfd = accept(listenfd, (struct sockaddr *)&clientaddr, &clilen);
  if (connfd < 0) {
    perror("connfd < 0");
    return;
  }

  setnonblocking(connfd);

  char *str = inet_ntoa(clientaddr.sin_addr);
  printf("connect from %s\n", str);
  struct epoll_event ev;
  ev.data.fd = connfd;
  ev.events = EPOLLIN | EPOLLRDHUP | EPOLLHUP;
  if (epoll_ctl(epfd, EPOLL_CTL_ADD, connfd, &ev) == -1) {
    perror("epoll_ctl: conn_sock");
    close(connfd);
  }
}

int main(int argc, char *argv[]) {
  signal(SIGPIPE, SIG_IGN);
  printf("epoll socket begins.\n");
  int listenfd, connfd, sockfd, epfd, nfds;

  struct epoll_event ev, events[MAX_EVENT];

  epfd = epoll_create1(0);
  if (epfd == -1) {
    perror("epoll_create1");
    exit(EXIT_FAILURE);
  }
  printf("epoll file: %d.\n", epfd);
  // Creating socket file descriptor
  if ((listenfd = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
    perror("socket failed");
    goto close_epollfd;
  }

  int opt;
  // Forcefully attaching socket to the port 8080
  /* if (setsockopt(listenfd, SOL_SOCKET, SO_REUSEADDR | SO_REUSEPORT, &opt, */
  /*                sizeof(opt))) { */
  /*   perror("setsockopt"); */
  /*   goto close_listenfd; */
  /* } */

  setnonblocking(listenfd);

  ev.data.fd = listenfd;
  ev.events = EPOLLIN;
  epoll_ctl(epfd, EPOLL_CTL_ADD, listenfd, &ev);

  struct sockaddr_in serveraddr;
  bzero(&serveraddr, sizeof(serveraddr));
  serveraddr.sin_family = AF_INET;
  serveraddr.sin_addr.s_addr = INADDR_ANY;
  serveraddr.sin_port = htons(SERV_PORT);

  // Forcefully attaching socket to the port 8080
  if (bind(listenfd, (struct sockaddr *)&serveraddr, sizeof(serveraddr))) {
    perror("bind failed");
    goto close_listenfd;
  }

  if (listen(listenfd, LISTENQ) < 0) {
    perror("listen");
    goto close_listenfd;
  }

  int count_listen, count_in, count_out, count_close;
  count_listen = count_in = count_out = count_close = 0;
  for (;;) {
    nfds = epoll_wait(epfd, events, MAX_EVENT, -1);
    printf("epoll wait return value: %d.\n", nfds);

    for (int i = 0; i < nfds; ++i) {
      printf("handle file is: %d\n", events[i].data.fd);
      if (events[i].events & (EPOLLRDHUP | EPOLLHUP)) {
        /* check if the connection is closing */
        printf("fd:%d is closed\n", events[i].data.fd);
        epoll_ctl(epfd, EPOLL_CTL_DEL, events[i].data.fd, NULL);
        close(events[i].data.fd);
        count_close++;
      } else if (events[i].data.fd == listenfd) {
        handle_listen(epfd, listenfd);
        count_listen++;
      } else if (events[i].events & EPOLLIN) {
        handle_in_event(epfd, events + i);
        count_in++;
      } else if (events[i].events & EPOLLOUT) {
        handle_out_event(epfd, events + i);
        count_out++;
      }
    }

    printf("epoll status: count_listen: %d, count_in: %d, count_out: %d, "
           "count_close: %d\n",
           count_listen, count_in, count_out, count_close);
  }
close_listenfd:
  close(listenfd);
close_epollfd:
  close(epfd);
}
