#include "../include/http.h"

int read_line_from_socket(HttpReadBuffer* read_buf, char* line_buffer, size_t line_buffer_size) {
  while (1) {
    for (size_t i = read_buf->parse_position; i + 1 < read_buf->data_length; i++) {
      if (read_buf->buffer[i] == '\r' && read_buf->buffer[i + 1] == '\n') {
        // 找到完整行
        size_t actual_line_length = i - read_buf->parse_position;

        if (actual_line_length >= line_buffer_size) {
          printf("行过长错误: %zu >= %zu\n", actual_line_length, line_buffer_size);
          return -1;
        }

        memcpy(line_buffer, read_buf->buffer + read_buf->parse_position, actual_line_length);
        line_buffer[actual_line_length] = '\0';

        // printf("成功读取HTTP行 (长度: %zu): [%s]\n", actual_line_length, line_buffer);

        read_buf->parse_position = i + 2; // 跳过\r\n
        return actual_line_length;
      }
    }

    // 处理非标准HTTP响应(只有\n结尾)
    for (size_t i = read_buf->parse_position; i < read_buf->data_length; i++) {
      if (read_buf->buffer[i] == '\n') {
        size_t actual_line_length = i - read_buf->parse_position;

        // 如果前一个字符是\r，则去掉它
        if (actual_line_length > 0 && read_buf->buffer[i - 1] == '\r') {
          actual_line_length--;
        }

        if (actual_line_length >= line_buffer_size) {
          printf("行过长错误: %zu >= %zu\n", actual_line_length, line_buffer_size);
          return -1;
        }

        memcpy(line_buffer, read_buf->buffer + read_buf->parse_position, actual_line_length);
        line_buffer[actual_line_length] = '\0';

        // printf("成功读取HTTP行 (仅\\n结尾, 长度: %zu): [%s]\n", actual_line_length, line_buffer);

        read_buf->parse_position = i + 1; // 跳过\n
        return actual_line_length;
      }
    }

    // 需要读取更多数据
    if (read_buf->data_length == READ_BUFFER_SIZE) {
      // 缓冲区已满，移动未处理数据
      size_t remaining = read_buf->data_length - read_buf->parse_position;

      // 如果剩余数据占满了整个缓冲区，说明单行过长
      if (remaining == READ_BUFFER_SIZE) {
        printf("单行数据过长，超过缓冲区大小 %d\n", READ_BUFFER_SIZE);
        return -1;
      }

      // printf("缓冲区已满，移动 %zu 字节未处理数据\n", remaining);
      memmove(read_buf->buffer, read_buf->buffer + read_buf->parse_position, remaining);
      read_buf->data_length = remaining;
      read_buf->parse_position = 0;
    }

    // 从Socket读取更多数据
    // printf("尝试从Socket读取数据...\n");
    ssize_t bytes_read = recv(read_buf->sockfd, read_buf->buffer + read_buf->data_length,
      READ_BUFFER_SIZE - read_buf->data_length, 0);

    if (bytes_read < 0) {
      printf("Socket读取错误: %s (errno: %d)\n", strerror(errno), errno);
      return -1;
    }
    else if (bytes_read == 0) {
      printf("连接被服务器关闭，当前缓冲区有 %zu 字节数据\n", read_buf->data_length);

      // 检查是否还有未处理的数据（处理最后一行没有换行符的情况）
      if (read_buf->parse_position < read_buf->data_length) {
        size_t remaining_length = read_buf->data_length - read_buf->parse_position;
        if (remaining_length < line_buffer_size) {
          memcpy(line_buffer, read_buf->buffer + read_buf->parse_position, remaining_length);
          line_buffer[remaining_length] = '\0';
          // printf("处理最后一行数据 (无换行符): [%s]\n", line_buffer);
          read_buf->parse_position = read_buf->data_length;
          return remaining_length;
        }
        else {
          printf("最后一行数据过长: %zu >= %zu\n", remaining_length, line_buffer_size);
          return -1;
        }
      }
      return 0; // 连接关闭，没有更多数据
    }
    else {
      // printf("从Socket成功读取 %zd 字节数据\n", bytes_read);
      // // 原始数据
      // printf("原始数据: ");
      // for (ssize_t i = 0; i < bytes_read; i++) {
      //   char c = read_buf->buffer[read_buf->data_length + i];
      //   if (c >= 32 && c <= 126) {
      //     printf("%c", c);
      //   }
      //   else {
      //     printf("\\x%02x", (unsigned char)c);
      //   }
      // }
      // printf("\n");
      read_buf->data_length += bytes_read;
    }
  }
}

int parse_status_line(const char* status_line, HttpResponseInfo* response_info) {
  char http_version[16];

  // 解析状态行
  int parsed_fields = sscanf(status_line, "%15s %d %127[^\r\n]",
    http_version,
    &response_info->status_code,
    response_info->status_message);

  if (parsed_fields < 2) {
    return -1; // 解析失败
  }

  // 验证HTTP版本格式
  if (strncmp(http_version, "HTTP/", 5) != 0) {
    return -1;
  }

  // 提供默认值
  if (parsed_fields == 2) {
    strncpy(response_info->status_message, "OK", sizeof(response_info->status_message) - 1);
    response_info->status_message[sizeof(response_info->status_message) - 1] = '\0';
  }

  return 0;
}

StatusAction determine_status_action(int status_code) {
  if (status_code >= 200 && status_code < 300) {
    return STATUS_ACTION_CONTINUE; // 2xx成功
  }
  else if (status_code >= 300 && status_code < 400) {
    return STATUS_ACTION_REDIRECT; // 3xx重定向
  }
  else if (status_code >= 400 && status_code < 500) {
    return STATUS_ACTION_ERROR;    // 4xx客户端错误
  }
  else if (status_code >= 500 && status_code < 600) {
    return STATUS_ACTION_RETRY;    // 5xx服务器错误
  }
  return STATUS_ACTION_ERROR;
}

int parse_header_field(const char* header_line, HttpResponseInfo* response_info) {
  char field_name[128];
  char field_value[512];

  // 查找冒号分隔符
  const char* colon_pos = strchr(header_line, ':');
  if (!colon_pos) {
    return -1; // 无效的头部格式
  }

  // 提取字段名
  size_t name_length = colon_pos - header_line;
  if (name_length >= sizeof(field_name)) {
    return -1; // 字段名过长
  }

  strncpy(field_name, header_line, name_length);
  field_name[name_length] = '\0';

  // 跳过冒号和可选的空白字符
  const char* value_start = colon_pos + 1;
  while (*value_start == ' ' || *value_start == '\t') {
    value_start++;
  }

  // 提取字段值
  strncpy(field_value, value_start, sizeof(field_value) - 1);
  field_value[sizeof(field_value) - 1] = '\0';

  // 去除尾部空白字符
  char* end = field_value + strlen(field_value) - 1;
  while (end > field_value && (*end == ' ' || *end == '\t' || *end == '\r')) {
    *end = '\0';
    end--;
  }

  // 处理特定头部
  return process_header_field(field_name, field_value, response_info);
}

int process_header_field(const char* name, const char* value, HttpResponseInfo* response_info) {
  // 使用strcasecmp进行大小写不敏感比较
  if (strcasecmp(name, "Content-Length") == 0) {
    char* endptr;
    response_info->content_length = strtoll(value, &endptr, 10);
    if (*endptr != '\0' || response_info->content_length < 0) {
      return -1; // 无效的Content-Length值
    }
  }
  else if (strcasecmp(name, "Content-Type") == 0) {
    strncpy(response_info->content_type, value,
      sizeof(response_info->content_type) - 1);
  }
  else if (strcasecmp(name, "Transfer-Encoding") == 0) {
    if (strcasecmp(value, "chunked") == 0) {
      response_info->chunked_encoding = 1;
    }
  }
  else if (strcasecmp(name, "Connection") == 0) {
    if (strcasecmp(value, "close") == 0) {
      response_info->connection_close = 1;
    }
  }
  else if (strcasecmp(name, "Location") == 0) {
    strncpy(response_info->location, value,
      sizeof(response_info->location) - 1);
  }
  else if (strcasecmp(name, "Server") == 0) {
    strncpy(response_info->server, value,
      sizeof(response_info->server) - 1);
  }
  else if (strcasecmp(name, "Set-Cookie") == 0) {
    if (strlen(response_info->cookies) + strlen(value) < sizeof(response_info->cookies)) {
      if (strlen(response_info->cookies) > 0) {
        strncat(response_info->cookies, "; ", sizeof(response_info->cookies) - strlen(response_info->cookies) - 1);
      }
      strncat(response_info->cookies, value, sizeof(response_info->cookies) - strlen(response_info->cookies) - 1);
    }
  }
  else {
    // 未处理的头部字段
    // printf("未处理的头部字段: %s: %s\n", name, value);
  }

  return 0;
}

int build_http_get_request(const char* host, const char* path, char* buffer, size_t buffer_size) {
  // 如果路径为空或为根路径，使用 "/"
  const char* request_path = (path == NULL || strlen(path) == 0) ? "/" : path;

  char* allocated_path = NULL;

  // 确保路径以 '/' 开头
  if (request_path[0] != '/') {
    size_t new_path_len = strlen(request_path) + 2;
    allocated_path = malloc(new_path_len);
    if (!allocated_path) {
      return -1;
    }
    snprintf(allocated_path, new_path_len, "/%s", request_path);
    request_path = allocated_path;
  }

  int written = snprintf(buffer, buffer_size,
    "GET %s HTTP/1.1\r\n"
    "Host: %s\r\n"
    "User-Agent: CHttpDownloader/1.0\r\n"
    "Connection: close\r\n"
    "Accept: */*\r\n"
    "\r\n",
    request_path, host);

  // 释放分配的内存
  if (allocated_path) {
    free(allocated_path);
  }

  // 检查是否发生截断
  if (written >= buffer_size) {
    return -1; // 缓冲区不足
  }

  return written;
}

int parse_http_response_headers(int sockfd, HttpResponseInfo* response_info, HttpReadBuffer* remaining_buffer) {
  HttpReadBuffer read_buf = { 0 };
  read_buf.sockfd = sockfd;

  char line_buffer[8192];
  int line_count = 0;

  // 初始化响应信息结构体
  memset(response_info, 0, sizeof(HttpResponseInfo));
  response_info->content_length = -1; // 未知长度

  // printf("开始解析HTTP响应头...\n");

  while (1) {
    int line_length = read_line_from_socket(&read_buf, line_buffer, sizeof(line_buffer));

    if (line_length < 0) {
      // printf("读取HTTP响应行失败\n");
      return -1; // 读取错误
    }

    if (line_length == 0) {
      // 遇到空行则解析完成
      // printf("HTTP响应头解析完成\n");
      break;
    }

    // printf("解析第%d行: %s\n", line_count, line_buffer);

    if (line_count == 0) {
      // 解析状态行
      if (parse_status_line(line_buffer, response_info) != 0) {
        // printf("状态行解析失败: %s\n", line_buffer);
        return -1;
      }
      // printf("状态码: %d, 状态消息: %s\n", response_info->status_code, response_info->status_message);
    }
    else {
      // 解析头部字段
      if (parse_header_field(line_buffer, response_info) != 0) {
        fprintf(stderr, "警告：无法解析头部字段: %s\n", line_buffer);
      }
    }

    line_count++;
  }

  // 将剩余的缓冲区数据传递给调用者
  if (remaining_buffer) {
    *remaining_buffer = read_buf;
    size_t remaining_data = read_buf.data_length - read_buf.parse_position;
    // printf("缓冲区状态: 总长度=%zu, 解析位置=%zu, 剩余数据=%zu字节\n",
    //   read_buf.data_length, read_buf.parse_position, remaining_data);
  }

  return 0;
}