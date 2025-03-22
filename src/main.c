// Server side C program to demonstrate Socket
// programming
#include <arpa/inet.h>
#include <netinet/in.h>
#include <omp.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>
#define PORT 8080
#define LISTENQ 200

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

void read_socket(int socket) {
  char buffer[1024] = {0};
  ssize_t valread = read(socket, buffer, 1024 - 1);
  printf("request content: \n%s\n", buffer);
}

void handling(int socket) {
  write_socket(socket);
  read_socket(socket);
  // closing the connected socket
  close(socket);
}

int main(int argc, char const *argv[]) {
  int server_fd, new_socket;
  struct sockaddr_in address;
  int opt = 1;
  socklen_t addrlen = sizeof(address);

  // Creating socket file descriptor
  if ((server_fd = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
    perror("socket failed");
    exit(EXIT_FAILURE);
  }

  // Forcefully attaching socket to the port 8080
  if (setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR | SO_REUSEPORT, &opt,
                 sizeof(opt))) {
    perror("setsockopt");
    exit(EXIT_FAILURE);
  }
  address.sin_family = AF_INET;
  address.sin_addr.s_addr = INADDR_ANY;
  address.sin_port = htons(PORT);

  // Forcefully attaching socket to the port 8080
  if (bind(server_fd, (struct sockaddr *)&address, sizeof(address)) < 0) {
    perror("bind failed");
    exit(EXIT_FAILURE);
  }
  if (listen(server_fd, LISTENQ) < 0) {
    perror("listen");
    exit(EXIT_FAILURE);
  }
  // OpenMP parallel region
#pragma omp parallel
  {
#pragma omp single nowait
    {
      while (1) {
        // Accept a new client
        new_socket = accept(server_fd, (struct sockaddr *)&address, &addrlen);
        if (new_socket < 0) {
          perror("Accept failed");
          continue;
        }

// Handle client in a new OpenMP task
#pragma omp task firstprivate(new_socket)
        {
          handling(new_socket);
        }
      }
    }
  }
  // closing the listening socket
  close(server_fd);
  return 0;
}
