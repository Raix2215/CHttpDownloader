#include "./common.h"
#include "../include/parser.h"


int resolve_hostname(const char* hostname, char* ip_str, size_t ip_str_len) {
  struct addrinfo hints, * result;
  int status;

  // 初始化hints结构
  memset(&hints, 0, sizeof(hints));
  hints.ai_family = AF_INET;      // IPv4
  hints.ai_socktype = SOCK_STREAM; // TCP

  // 进行域名解析
  status = getaddrinfo(hostname, "80", &hints, &result);
  if (status != 0) {
    fprintf(stderr, "getaddrinfo error: %s\n", gai_strerror(status));
    return -1;
  }

  // 获取第一个结果的IP地址
  struct sockaddr_in* addr_in = (struct sockaddr_in*)result->ai_addr;
  const char* ip = inet_ntoa(addr_in->sin_addr);

  // 检查缓冲区大小
  if (strlen(ip) >= ip_str_len) {
    freeaddrinfo(result);
    return -1;
  }

  // 复制IP地址字符串
  strcpy(ip_str, ip);

  // 释放结果
  freeaddrinfo(result);
  return 0;
}

int parse_url(const char* url, URLInfo* info) {
  info->port = -1;
  strcpy(info->path, "/");
  strcpy(info->scheme, "http");
  info->query[0] = '\0';

  // 提取协议
  const char* protocol_end = strstr(url, "://");
  if (protocol_end) {
    size_t protocol_len = protocol_end - url;
    if (protocol_len == 4)
      info->protocol_type = PROTOCOL_HTTP;
    else
      info->protocol_type = PROTOCOL_HTTPS;
    url = protocol_end + 3;
  }
  else {
    // 默认协议为http
    info->protocol_type = PROTOCOL_HTTP;
  }

  //解析host
  const char* host_start = url;
  const char* host_end = NULL;
  const char* port_ptr = strchr(url, ':');
  const char* path_ptr = strchr(url, '/');
  const char* query_ptr = strchr(url, '?');
  if (port_ptr && (!path_ptr || port_ptr < path_ptr)) {
    host_end = port_ptr;
  }
  else if (path_ptr) {
    host_end = path_ptr;
  }
  else if (query_ptr) {
    host_end = query_ptr;
  }
  else {
    host_end = url + strlen(url);
  }
  size_t host_len = host_end - host_start;
  strncpy(info->host, host_start, host_len);
  info->host[host_len] = '\0';

  url = host_end;

  //解析port
  if (*url == ':') {
    url++;  // 跳过冒号

    // 提取端口数字
    const char* port_start = url;
    while (*url && isdigit(*url)) {
      url++;
    }
    size_t port_len = url - port_start;

    if (port_len > 0) {
      char port_str[6];
      strncpy(port_str, port_start, port_len);
      port_str[port_len] = '\0';
      info->port = atoi(port_str);
    }
  }

  // 解析path
  if (*url == '/') {
    const char* path_start = url;
    const char* path_end = strchr(url, '?');

    if (!path_end) {
      path_end = url + strlen(url);
    }

    size_t path_len = path_end - path_start;
    strncpy(info->path, path_start, path_len);
    info->path[path_len] = '\0';

    url = path_end;
  }

  // 解析query参数
  if (*url == '?') {
    url++;
    size_t query_len = strlen(url);
    if (query_len > 0) {
      strcpy(info->query, url);
    }
  }

  if (info->port == (-1)) {
    if (info->protocol_type == PROTOCOL_HTTPS) {
      info->port = 443;
    }
    else {
      info->port = 80;
    }
  }
  // 判断url类型
  info->host_type = is_domain_or_ipv4(info->host);

  return 0;
}

int is_domain_or_ipv4(const char* str) {
  if (str == NULL || *str == '\0') {
    return 0;
  }

  // IPv4解析
  int part_count = 0;       // 部分计数
  int current_part = 0;     // 当前部分的数值
  int digit_count = 0;      // 当前部分的数字位数
  int has_alpha = 0;        // 是否有字母字符
  int has_hyphen = 0;       // 是否有连字符

  const char* p = str;

  while (*p) {
    if (*p == '.') {
      if (digit_count == 0) {
      }
      part_count++;
      if (part_count > 3) {
        break;
      }
      if (current_part < 0 || current_part > 255) {
        return 0;
      }
      current_part = 0;
      digit_count = 0;
    }
    else if (isdigit((unsigned char)*p)) {
      current_part = current_part * 10 + (*p - '0');
      digit_count++;

      if (digit_count > 3) {
        return 0;
      }
    }
    else if (isalpha((unsigned char)*p) || *p == '-') {
      has_alpha = 1;
      if (*p == '-') {
        has_hyphen = 1;
      }

      if (part_count > 0 || digit_count > 0) {
        break;
      }
    }
    else {
      return 0;
    }
    p++;
  }

  if (digit_count > 0) {
    part_count++;
    if (current_part < 0 || current_part > 255) {
      return 0;
    }
  }

  if (part_count == 4 && !has_alpha && !has_hyphen) {
    return 1; // IPv4地址
  }

  // // 域名解析
  if (has_alpha) {
    int len = strlen(str);
    if (len > 253) {
      return 0;
    }

    int label_len = 0;        // 当前标签长度
    int label_start = 1;      // 是否是标签开始
    int last_char_hyphen = 0; // 上一个字符是连字符

    for (int i = 0; i <= len; i++) {
      char c = str[i];

      if (c == '.' || c == '\0') {
        // 标签结束
        if (label_len == 0) {
          // 空标签无效
          return 0;
        }

        if (last_char_hyphen) {
          // 标签以连字符结尾无效
          return 0;
        }

        if (label_len > 63) {
          return 0;
        }

        // 重置标签计数器
        label_len = 0;
        label_start = 1;
        last_char_hyphen = 0;
      }
      else if (c == '-') {
        if (label_start) {
          // 标签以连字符开头无效
          return 0;
        }

        label_len++;
        label_start = 0;
        last_char_hyphen = 1;
      }
      else if (isalnum((unsigned char)c)) {
        label_len++;
        label_start = 0;
        last_char_hyphen = 0;
      }
      else {
        return 0;
      }
    }
    return 2;
  }
  return 0;
}