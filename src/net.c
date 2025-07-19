#include "../include/net.h"



int create_tcp_connection(const char* ip_str, int port) {
  int sockfd = socket(AF_INET, SOCK_STREAM, 0);
  if (sockfd < 0) {
    perror("Socket Created Failure");
    return -1;
  }

  // 设置连接超时
  struct timeval timeout;
  timeout.tv_sec = 30;  // 30秒超时
  timeout.tv_usec = 0;
  setsockopt(sockfd, SOL_SOCKET, SO_RCVTIMEO, &timeout, sizeof(timeout));
  setsockopt(sockfd, SOL_SOCKET, SO_SNDTIMEO, &timeout, sizeof(timeout));

  struct sockaddr_in server_addr;
  memset(&server_addr, 0, sizeof(server_addr));
  server_addr.sin_family = AF_INET;
  server_addr.sin_port = htons(port);

  if (inet_pton(AF_INET, ip_str, &server_addr.sin_addr) <= 0) {
    close(sockfd);
    return -1;
  }

  if (connect(sockfd, (struct sockaddr*)&server_addr, sizeof(server_addr)) < 0) {
    perror("Connection Failure");
    close(sockfd);
    return -1;
  }

  return sockfd;
}

int send_full_data(int sockfd, const char* buffer, size_t length) {
  size_t total_sent = 0;

  while (total_sent < length) {
    ssize_t sent = send(sockfd, buffer + total_sent, length - total_sent, 0);
    if (sent <= 0) {
      perror("send");
      return -1;
    }
    total_sent += sent;
  }

  return 0;
}

