#include "../include/https.h"
#include "../include/common.h"
#include "../include/download.h"
#include "../include/parser.h"
#include "../include/net.h"
#include "../include/http.h"
#include "../include/progress.h"
#include "../include/utils.h"
#ifdef WITH_OPENSSL

// 全局初始化标志
static int openssl_initialized = 0;

int init_openssl() {
  if (openssl_initialized) {
    return 0; // 已初始化
  }

  // 初始化 OpenSSL 库
  SSL_load_error_strings();
  OpenSSL_add_ssl_algorithms();
  SSL_library_init();

  openssl_initialized = 1;
  // printf("OpenSSL 库初始化成功\n");
  return 0;
}

void cleanup_openssl() {
  if (!openssl_initialized) {
    return;
  }

  // 清理 OpenSSL 库
  EVP_cleanup();
  ERR_free_strings();

  openssl_initialized = 0;
  // printf("OpenSSL 库清理完成\n");
}

SSL_CTX* create_ssl_context() {
  if (!openssl_initialized) {
    fprintf(stderr, "错误: OpenSSL 库未初始化\n");
    return NULL;
  }

  // 创建 SSL 上下文，使用 TLS 客户端方法
  SSL_CTX* ctx = SSL_CTX_new(TLS_client_method());
  if (!ctx) {
    fprintf(stderr, "错误: 无法创建 SSL 上下文\n");
    ERR_print_errors_fp(stderr);
    return NULL;
  }

  // 设置验证模式
  SSL_CTX_set_verify(ctx, SSL_VERIFY_NONE, NULL);

  // printf("SSL 上下文创建成功\n");
  return ctx;
}

void close_https_connection(HttpsConnection* https_connection) {
  if (!https_connection) return;

  if (https_connection->ssl) {
    SSL_shutdown(https_connection->ssl);
    SSL_free(https_connection->ssl);
  }

  if (https_connection->sockfd >= 0) {
    close(https_connection->sockfd);
  }

  if (https_connection->ctx) {
    SSL_CTX_free(https_connection->ctx);
  }

  free(https_connection);
  // printf("HTTPS 连接已关闭\n");
}

HttpsConnection* create_https_connection(const char* hostname, int port) {
  if (!openssl_initialized) {
    fprintf(stderr, "错误: OpenSSL 库未初始化\n");
    return NULL;
  }

  // printf("正在建立 HTTPS 连接到 %s:%d...\n", hostname, port);

  // 分配内存
  HttpsConnection* https_connection = malloc(sizeof(HttpsConnection));
  if (!https_connection) {
    fprintf(stderr, "错误: 内存分配失败\n");
    return NULL;
  }

  // 初始化结构体
  https_connection->ctx = NULL;
  https_connection->ssl = NULL;
  https_connection->sockfd = -1;

  // 创建 SSL 上下文
  https_connection->ctx = create_ssl_context();
  if (!https_connection->ctx) {
    fprintf(stderr, "错误: SSL 上下文创建失败\n");
    free(https_connection);
    return NULL;
  }

  // 解析主机名为 IP 地址
  char ip_str[INET_ADDRSTRLEN];
  if (resolve_hostname(hostname, ip_str, sizeof(ip_str)) != 0) {
    fprintf(stderr, "错误: 无法解析主机名 %s\n", hostname);
    SSL_CTX_free(https_connection->ctx);
    free(https_connection);
    return NULL;
  }

  // printf("已解析 %s -> %s\n", hostname, ip_str);

  // 建立 TCP 连接
  https_connection->sockfd = create_tcp_connection(ip_str, port);
  if (https_connection->sockfd < 0) {
    fprintf(stderr, "错误: TCP 连接建立失败\n");
    SSL_CTX_free(https_connection->ctx);
    free(https_connection);
    return NULL;
  }

  // printf("TCP 连接建立成功\n");

  // 创建 SSL 对象
  https_connection->ssl = SSL_new(https_connection->ctx);
  if (!https_connection->ssl) {
    fprintf(stderr, "错误: 无法创建 SSL 对象\n");
    ERR_print_errors_fp(stderr);
    close(https_connection->sockfd);
    SSL_CTX_free(https_connection->ctx);
    free(https_connection);
    return NULL;
  }

  // 将 SSL 对象与 socket 关联
  if (SSL_set_fd(https_connection->ssl, https_connection->sockfd) != 1) {
    fprintf(stderr, "错误: 无法将 SSL 对象与 socket 关联\n");
    ERR_print_errors_fp(stderr);
    SSL_free(https_connection->ssl);
    close(https_connection->sockfd);
    SSL_CTX_free(https_connection->ctx);
    free(https_connection);
    return NULL;
  }

  // 设置 SNI (Server Name Indication)
  if (SSL_set_tlsext_host_name(https_connection->ssl, hostname) != 1) {
    fprintf(stderr, "警告: 无法设置 SNI\n");
  }

  // 执行 SSL 握手
  // printf("正在执行 SSL 握手...\n");
  int ssl_connect_result = SSL_connect(https_connection->ssl);
  if (ssl_connect_result != 1) {
    int ssl_error = SSL_get_error(https_connection->ssl, ssl_connect_result);
    fprintf(stderr, "错误: SSL 握手失败 (错误代码: %d)\n", ssl_error);

    // 错误信息
    switch (ssl_error) {
    case SSL_ERROR_WANT_READ:
      fprintf(stderr, "SSL_ERROR_WANT_READ: 需要更多数据\n");
      break;
    case SSL_ERROR_WANT_WRITE:
      fprintf(stderr, "SSL_ERROR_WANT_WRITE: 需要写入数据\n");
      break;
    case SSL_ERROR_SYSCALL:
      fprintf(stderr, "SSL_ERROR_SYSCALL: 系统调用错误\n");
      perror("系统错误");
      break;
    case SSL_ERROR_SSL:
      fprintf(stderr, "SSL_ERROR_SSL: SSL 协议错误\n");
      ERR_print_errors_fp(stderr);
      break;
    default:
      fprintf(stderr, "未知的 SSL 错误\n");
      break;
    }

    SSL_free(https_connection->ssl);
    close(https_connection->sockfd);
    SSL_CTX_free(https_connection->ctx);
    free(https_connection);
    return NULL;
  }

  // printf("✓ SSL 握手完成\n");

  // 验证证书
  // verify_certificate(https_connection, hostname);

  return https_connection;
}

int ssl_send_data(HttpsConnection* https_connection, const char* buffer, size_t length) {
  if (!https_connection || !https_connection->ssl || !buffer) {
    fprintf(stderr, "错误: 无效的参数\n");
    return -1;
  }

  size_t total_sent = 0;

  while (total_sent < length) {
    int bytes_sent = SSL_write(https_connection->ssl, buffer + total_sent, length - total_sent);

    if (bytes_sent <= 0) {
      int ssl_error = SSL_get_error(https_connection->ssl, bytes_sent);

      switch (ssl_error) {
      case SSL_ERROR_WANT_WRITE:
      case SSL_ERROR_WANT_READ:
        // 需要重试
        continue;

      case SSL_ERROR_SYSCALL:
        fprintf(stderr, "SSL 发送错误: 系统调用失败\n");
        perror("系统错误");
        return -1;

      case SSL_ERROR_SSL:
        fprintf(stderr, "SSL 发送错误: SSL 协议错误\n");
        ERR_print_errors_fp(stderr);
        return -1;

      default:
        fprintf(stderr, "SSL 发送错误: 未知错误 (代码: %d)\n", ssl_error);
        return -1;
      }
    }

    total_sent += bytes_sent;
  }

  return 0;
}

ssize_t ssl_recv_data(HttpsConnection* https_connection, void* buffer, size_t length) {
  if (!https_connection || !https_connection->ssl || !buffer) {
    fprintf(stderr, "错误: 无效的参数\n");
    return -1;
  }

  int bytes_received = SSL_read(https_connection->ssl, buffer, length);

  if (bytes_received <= 0) {
    int ssl_error = SSL_get_error(https_connection->ssl, bytes_received);

    switch (ssl_error) {
    case SSL_ERROR_WANT_READ:
    case SSL_ERROR_WANT_WRITE:
      // 需要重试
      return 0;

    case SSL_ERROR_ZERO_RETURN:
      // SSL 连接被正常关闭
      return 0;

    case SSL_ERROR_SYSCALL:
      if (bytes_received == 0) {
        // 连接被对方关闭
        return 0;
      }
      // 系统错误
      fprintf(stderr, "\nSSL 接收错误: 系统调用失败\n");
      perror("系统错误");
      return -1;

    case SSL_ERROR_SSL:
    {
      unsigned long err = ERR_get_error();
      if (ERR_GET_REASON(err) == SSL_R_UNEXPECTED_EOF_WHILE_READING) {
        return 0;
      }
      fprintf(stderr, "\nSSL 接收错误: SSL 协议错误\n");
      ERR_print_errors_fp(stderr);
      return -1;
    }

    default:
      fprintf(stderr, "\nSSL 接收错误: 未知错误 (代码: %d)\n", ssl_error);
      return -1;
    }
  }

  return bytes_received;
}

void verify_certificate(HttpsConnection* https_connection, const char* hostname) {
  if (!https_connection || !https_connection->ssl || !hostname) {
    return;
  }

  // 获取对方证书
  X509* cert = SSL_get_peer_certificate(https_connection->ssl);
  if (!cert) {
    printf("警告: 服务器没有提供证书\n");
    return;
  }

  // printf("正在验证服务器证书...\n");

  // 获取证书主题
  char* subject = X509_NAME_oneline(X509_get_subject_name(cert), 0, 0);
  char* issuer = X509_NAME_oneline(X509_get_issuer_name(cert), 0, 0);

  // printf("证书主题: %s\n", subject ? subject : "未知");
  // printf("证书颁发者: %s\n", issuer ? issuer : "未知");

  // 清理
  if (subject) OPENSSL_free(subject);
  if (issuer) OPENSSL_free(issuer);
  X509_free(cert);

  // 获取验证结果
  long verify_result = SSL_get_verify_result(https_connection->ssl);
  if (verify_result == X509_V_OK) {
    // printf("证书验证通过");
  }
  else {
    // 忽略证书错误
  }


}

int parse_https_response_headers(HttpsConnection* https_connection, HttpResponseInfo* response_info, HttpReadBuffer* remaining_buffer) {
  if (!https_connection || !response_info || !remaining_buffer) {
    fprintf(stderr, "错误: 无效的参数\n");
    return -1;
  }

  // 初始化缓冲区
  memset(remaining_buffer, 0, sizeof(HttpReadBuffer));
  memset(response_info, 0, sizeof(HttpResponseInfo));

  char line_buffer[2048];
  int line_result;

  // 读取状态行
  line_result = ssl_read_line(https_connection, remaining_buffer, line_buffer, sizeof(line_buffer));
  if (line_result <= 0) {
    fprintf(stderr, "错误: 无法读取 HTTP 状态行\n");
    return -1;
  }

  // printf("状态行: %s\n", line_buffer);

  // 解析状态行
  if (parse_status_line(line_buffer, response_info) != 0) {
    fprintf(stderr, "错误: 无法解析 HTTP 状态行\n");
    return -1;
  }

  // printf("HTTP 状态码: %d %s\n", response_info->status_code, response_info->status_message);

  // 读取头部字段
  while (1) {
    line_result = ssl_read_line(https_connection, remaining_buffer, line_buffer, sizeof(line_buffer));
    if (line_result < 0) {
      fprintf(stderr, "错误: 读取头部字段失败\n");
      return -1;
    }

    if (line_result == 0) {
      // 空行，头部结束
      break;
    }

    // 解析头部字段
    if (parse_header_field(line_buffer, response_info) != 0) {
      printf("警告: 无法解析头部字段: %s\n", line_buffer);
    }
  }

  // printf("✓ HTTP 响应头解析完成\n");
  // printf("Content-Length: %lld\n", response_info->content_length);
  // printf("Transfer-Encoding: %s\n", response_info->transfer_encoding);

  return 0;
}

int ssl_read_line(HttpsConnection* https_connection, HttpReadBuffer* read_buf, char* line_buffer, size_t line_buffer_size) {
  size_t line_pos = 0;

  while (line_pos < line_buffer_size - 1) {
    // 如果缓冲区中没有数据，从 SSL 连接读取
    if (read_buf->parse_position >= read_buf->data_length) {
      ssize_t bytes_received = ssl_recv_data(https_connection, read_buf->buffer, sizeof(read_buf->buffer));
      if (bytes_received <= 0) {
        return bytes_received; // 错误或连接关闭
      }
      read_buf->data_length = bytes_received;
      read_buf->parse_position = 0;
    }

    // 从缓冲区读取一个字符
    char ch = read_buf->buffer[read_buf->parse_position++];

    if (ch == '\n') {
      // 找到行结束符
      line_buffer[line_pos] = '\0';

      // 移除行末的 \r
      if (line_pos > 0 && line_buffer[line_pos - 1] == '\r') {
        line_buffer[line_pos - 1] = '\0';
        line_pos--;
      }

      return line_pos;
    }

    line_buffer[line_pos++] = ch;
  }

  // 行太长
  fprintf(stderr, "错误: HTTP 响应行过长\n");
  return -1;
}

int download_https_content_with_length(HttpsConnection* https_connection, FILE* output_file, long long content_length, DownloadProgress* progress, HttpReadBuffer* remaining_buffer) {
  const size_t BUFFER_SIZE = 8192;
  char buffer[BUFFER_SIZE];

  if (!progress || !output_file || !https_connection) {
    fprintf(stderr, "错误: 无效的参数\n");
    return -1;
  }

  progress->total_size = content_length;
  progress->downloaded_size = 0;
  progress->start_time = time(NULL);
  progress->last_update_time = progress->start_time;

  // 首先处理缓冲区中的剩余数据
  if (remaining_buffer && remaining_buffer->parse_position < remaining_buffer->data_length) {
    size_t remaining_data = remaining_buffer->data_length - remaining_buffer->parse_position;
    size_t bytes_to_write = remaining_data;

    // 确保不超过期望的内容长度
    if (progress->downloaded_size + bytes_to_write > content_length) {
      bytes_to_write = content_length - progress->downloaded_size;
    }

    if (fwrite(remaining_buffer->buffer + remaining_buffer->parse_position, 1, bytes_to_write, output_file) != bytes_to_write) {
      fprintf(stderr, "缓冲区数据写入文件失败\n");
      return -1;
    }

    progress->downloaded_size += bytes_to_write;
    remaining_buffer->parse_position += bytes_to_write;

    // 更新进度显示
    update_download_progress(progress);
  }

  while (progress->downloaded_size < content_length) {
    // 计算本次接收的数据量
    size_t bytes_to_receive = BUFFER_SIZE;
    long long remaining = content_length - progress->downloaded_size;
    if (remaining < BUFFER_SIZE) {
      bytes_to_receive = (size_t)remaining;
    }

    // 通过 SSL 接收数据
    ssize_t bytes_received = ssl_recv_data(https_connection, buffer, bytes_to_receive);

    if (bytes_received <= 0) {
      clear_progress_line();
      if (bytes_received == 0) {
        fprintf(stderr, "SSL 连接意外关闭，已下载 %lld/%lld 字节\n",
          progress->downloaded_size, content_length);
      }
      else {
        fprintf(stderr, "SSL 接收数据失败\n");
      }
      return -1;
    }

    // 写入文件
    size_t bytes_written = fwrite(buffer, 1, bytes_received, output_file);
    if (bytes_written != (size_t)bytes_received) {
      clear_progress_line();
      fprintf(stderr, "文件写入失败: 期望写入 %zd 字节，实际写入 %zu 字节\n",
        bytes_received, bytes_written);
      return -1;
    }

    // 更新进度
    progress->downloaded_size += bytes_received;

    // 定期更新进度显示（每接收一定数据量）
    time_t current_time = time(NULL);
    static long long last_update_size = 0;

    if (current_time > progress->last_update_time || progress->downloaded_size - last_update_size > 8192) {
      update_download_progress(progress);
      progress->last_update_time = current_time;
      last_update_size = progress->downloaded_size;
    }
  }

  // 最终进度更新
  update_download_progress(progress);

  // 确保数据写入磁盘
  if (fflush(output_file) != 0) {
    clear_progress_line();
    perror("刷新文件缓冲区失败");
    return -1;
  }

  return 0;
}

int download_https_content_until_close(HttpsConnection* https_connection, FILE* output_file, DownloadProgress* progress, HttpReadBuffer* remaining_buffer) {
  const size_t BUFFER_SIZE = 8192;
  char buffer[BUFFER_SIZE];
  int consecutive_zero_reads = 0;  // 连续零读取计数
  const int MAX_ZERO_READS = 3;    // 最大连续零读取次数

  if (!progress || !output_file || !https_connection) {
    fprintf(stderr, "错误: 无效的参数\n");
    return -1;
  }

  progress->total_size = -1;  // 未知大小
  progress->downloaded_size = 0;
  progress->start_time = time(NULL);
  progress->last_update_time = progress->start_time;

  // 首先处理缓冲区中的剩余数据
  if (remaining_buffer && remaining_buffer->parse_position < remaining_buffer->data_length) {
    size_t remaining_data = remaining_buffer->data_length - remaining_buffer->parse_position;

    if (fwrite(remaining_buffer->buffer + remaining_buffer->parse_position, 1, remaining_data, output_file) != remaining_data) {
      fprintf(stderr, "缓冲区数据写入文件失败\n");
      return -1;
    }

    progress->downloaded_size += remaining_data;
    remaining_buffer->parse_position = remaining_buffer->data_length;
    update_download_progress(progress);
  }

  while (1) {
    ssize_t bytes_received = ssl_recv_data(https_connection, buffer, BUFFER_SIZE);

    if (bytes_received == 0) {
      // 没有读取到数据
      consecutive_zero_reads++;
      if (consecutive_zero_reads >= MAX_ZERO_READS) {
        // 连接已关闭
        break;
      }
      // 短暂等待后继续尝试
      usleep(10000); // 等待 10ms
      continue;
    }

    if (bytes_received < 0) {
      // 发生错误，但如果已经下载了一些数据，可能是正常结束
      if (progress->downloaded_size > 0) {
        printf("\n警告: SSL 连接在下载 %lld 字节后关闭\n", progress->downloaded_size);
        break;
      }
      else {
        clear_progress_line();
        fprintf(stderr, "SSL 接收错误\n");
        return -1;
      }
    }

    // 重置连续零读取计数
    consecutive_zero_reads = 0;

    if (fwrite(buffer, 1, bytes_received, output_file) != (size_t)bytes_received) {
      clear_progress_line();
      fprintf(stderr, "文件写入失败\n");
      return -1;
    }

    progress->downloaded_size += bytes_received;

    // 定期更新进度显示
    time_t current_time = time(NULL);
    static long long last_update_size = 0;

    if (current_time > progress->last_update_time || progress->downloaded_size - last_update_size > 8192) {
      update_download_progress(progress);
      progress->last_update_time = current_time;
      last_update_size = progress->downloaded_size;
    }
  }

  // 最终进度更新
  update_download_progress(progress);

  // 确保数据写入磁盘
  if (fflush(output_file) != 0) {
    clear_progress_line();
    perror("刷新文件缓冲区失败");
    return -1;
  }

  return 0;
}

int download_file_https(const char* url, const char* output_filename, const char* download_dir, int redirect_count) {
#ifdef WITH_OPENSSL
  // CLI颜色定义
  const char* BLUE = "\033[34m";
  const char* CYAN = "\033[36m";
  const char* YELLOW = "\033[33m";
  const char* RESET = "\033[0m";
  const char* BOLD = "\033[1m";
  const char* RED = "\033[31m";
  const char* GREEN = "\033[32m";

  const int MAX_REDIRECTS = 10;
  const char* current_url = url;
  char redirect_url[2048];
  char full_output_path[4096];

  // 构造完整输出文件路径
  if (!output_filename) {
    fprintf(stderr, "%s错误: 输出文件名不能为空%s\n", RED, RESET);
    return DOWNLOAD_ERROR_URL_PARSE;
  }

  // 如果没有指定下载目录或目录为空，使用当前工作目录
  if (!download_dir || strlen(download_dir) == 0) {
    char current_dir[PATH_MAX];
    if (getcwd(current_dir, sizeof(current_dir)) == NULL) {
      fprintf(stderr, "%s错误: 无法获取当前工作目录: %s%s\n", RED, strerror(errno), RESET);
      return DOWNLOAD_ERROR_FILE_OPEN;
    }
    snprintf(full_output_path, sizeof(full_output_path), "%s/%s", current_dir, output_filename);
  }
  else {
    // 检查下载目录是否存在
    struct stat dir_stat;
    if (stat(download_dir, &dir_stat) != 0) {
      fprintf(stderr, "%s错误: 下载目录不存在: %s%s\n", RED, download_dir, RESET);
      return DOWNLOAD_ERROR_FILE_OPEN;
    }
    if (!S_ISDIR(dir_stat.st_mode)) {
      fprintf(stderr, "%s错误: 指定的路径不是目录: %s%s\n", RED, download_dir, RESET);
      return DOWNLOAD_ERROR_FILE_OPEN;
    }

    // 构造完整路径，处理目录末尾的斜杠
    if (download_dir[strlen(download_dir) - 1] == '/') {
      snprintf(full_output_path, sizeof(full_output_path), "%s%s", download_dir, output_filename);
    }
    else {
      snprintf(full_output_path, sizeof(full_output_path), "%s/%s", download_dir, output_filename);
    }
  }

  printf("%s下载文件将保存到: %s%s%s%s\n", BOLD, RESET, BLUE, full_output_path, RESET);

  // 初始化 OpenSSL
  if (init_openssl() != 0) {
    fprintf(stderr, "%s错误: OpenSSL 初始化失败%s\n", RED, RESET);
    return DOWNLOAD_ERROR_NETWORK;
  }

  for (int redirect_iter = 0; redirect_iter <= MAX_REDIRECTS; redirect_iter++) {
    // 变量初始化
    URLInfo url_info = { 0 };
    HttpResponseInfo response_info = { 0 };
    DownloadProgress progress = { 0 };
    HttpReadBuffer remaining_buffer = { 0 };
    HttpsConnection* https_connection = NULL;
    FILE* output_file = NULL;
    DownloadResult result = DOWNLOAD_SUCCESS;

    if (!current_url) {
      fprintf(stderr, "%s错误: 无效的URL%s\n", RED, RESET);
      cleanup_openssl();
      return DOWNLOAD_ERROR_URL_PARSE;
    }

    // URL解析
    if (parse_url(current_url, &url_info) != 0) {
      fprintf(stderr, "%s错误: 无法解析URL%s\n", RED, RESET);
      cleanup_openssl();
      return DOWNLOAD_ERROR_URL_PARSE;
    }

    // 检查协议类型
    if (url_info.protocol_type != PROTOCOL_HTTPS) {
      fprintf(stderr, "%s错误: 不支持的协议，期望 HTTPS%s\n", RED, RESET);
      cleanup_openssl();
      return DOWNLOAD_ERROR_URL_PARSE;
    }

    printf("%sHost: %s%s%s%s, %sPort: %s%s%d%s, %sPath: %s%s%s%s\n",
      BOLD, RESET, BLUE, url_info.host, RESET,
      BOLD, RESET, BLUE, url_info.port, RESET,
      BOLD, RESET, BLUE, url_info.path, RESET);

    char ip_str[INET_ADDRSTRLEN];

    // 域名解析
    if (url_info.host_type == UNALLOWED) {
      printf("%s错误：无效的域名%s\n", RED, RESET);
      result = DOWNLOAD_ERROR_DNS_RESOLVE;
      goto cleanup;
    }
    else if (url_info.host_type == DOMAIN) {
      if (resolve_hostname(url_info.host, ip_str, sizeof(ip_str)) != 0) {
        fprintf(stderr, "%s错误: 域名解析失败%s\n", RED, RESET);
        result = DOWNLOAD_ERROR_DNS_RESOLVE;
        goto cleanup;
      }
      printf("%sHostIP: %s%s%s%s\n", BOLD, RESET, BLUE, ip_str, RESET);
    }
    else {
      strncpy(ip_str, url_info.host, sizeof(ip_str) - 1);
      ip_str[sizeof(ip_str) - 1] = '\0';
      printf("%sHostIP: %s%s%s%s\n", BOLD, RESET, BLUE, ip_str, RESET);
    }

    // 建立 HTTPS 连接
    https_connection = create_https_connection(url_info.host, url_info.port);
    if (!https_connection) {
      fprintf(stderr, "%s错误: 无法连接到服务器%s\n", RED, RESET);
      result = DOWNLOAD_ERROR_CONNECTION;
      goto cleanup;
    }

    // 构造 HTTP 请求
    char request_buffer[REQUEST_BUFFER];
    int request_length = build_http_get_request(url_info.host, url_info.path, request_buffer, sizeof(request_buffer));
    if (request_length <= 0) {
      fprintf(stderr, "%s错误: 构造HTTP请求失败%s\n", RED, RESET);
      result = DOWNLOAD_ERROR_HTTP_REQUEST;
      goto cleanup;
    }

    // 发送 HTTP 请求
    if (ssl_send_data(https_connection, request_buffer, request_length) != 0) {
      fprintf(stderr, "%s错误: 发送HTTP请求失败%s\n", RED, RESET);
      result = DOWNLOAD_ERROR_HTTP_REQUEST;
      goto cleanup;
    }

    // 解析 HTTP 响应头
    if (parse_https_response_headers(https_connection, &response_info, &remaining_buffer) != 0) {
      fprintf(stderr, "%s错误: 解析HTTP响应失败%s\n", RED, RESET);
      result = DOWNLOAD_ERROR_HTTP_RESPONSE;
      goto cleanup;
    }

    printf("%sHTTP状态:%s %d %s\n", BOLD, RESET, response_info.status_code, response_info.status_message);

    // 处理重定向
    if (determine_status_action(response_info.status_code) == STATUS_ACTION_REDIRECT) {
      if (redirect_iter >= MAX_REDIRECTS) {
        fprintf(stderr, "%s错误: 重定向次数过多 (超过%d次)%s\n", RED, MAX_REDIRECTS, RESET);
        result = DOWNLOAD_ERROR_HTTP_RESPONSE;
        goto cleanup;
      }

      printf("%s警告: 服务器返回重定向状态码 %d%s\n", YELLOW, response_info.status_code, RESET);
      if (response_info.location[0]) {
        printf("%s重定向到: %s (第%d次重定向)%s\n", YELLOW, response_info.location, redirect_iter + 1, RESET);
        printf("-------------------------重定向第(%d)次--------------------------\n", redirect_iter + 1);

        // 保存重定向URL
        strncpy(redirect_url, response_info.location, sizeof(redirect_url) - 1);
        redirect_url[sizeof(redirect_url) - 1] = '\0';

        // 关闭当前连接
        if (https_connection) {
          close_https_connection(https_connection);
          https_connection = NULL;
        }

        // 更新当前URL并继续循环
        current_url = redirect_url;
        continue;
      }
      else {
        fprintf(stderr, "%s错误: 重定向但没有提供Location头%s\n", RED, RESET);
        result = DOWNLOAD_ERROR_HTTP_RESPONSE;
        goto cleanup;
      }
    }

    // 处理错误状态码
    if (determine_status_action(response_info.status_code) == STATUS_ACTION_ERROR) {
      fprintf(stderr, "%s错误: 服务器返回错误状态码 %d%s\n", RED, response_info.status_code, RESET);
      result = DOWNLOAD_ERROR_HTTP_RESPONSE;
      goto cleanup;
    }

    // 显示文件信息
    if (response_info.content_length > 0) {
      printf("%s文件大小: %s%s%lld%s\n", BOLD, RESET, BLUE, response_info.content_length, RESET);
    }
    else {
      printf("%s文件大小: %s%s未知%s\n", BOLD, RESET, YELLOW, RESET);
    }
    if (response_info.content_type[0]) {
      printf("%s文件类型: %s%s%s%s\n", BOLD, RESET, BLUE, response_info.content_type, RESET);
    }

    // 打开输出文件
    output_file = fopen(full_output_path, "wb");
    if (!output_file) {
      fprintf(stderr, "%s错误: 无法创建输出文件 %s: %s%s\n", RED, full_output_path, strerror(errno), RESET);
      result = DOWNLOAD_ERROR_FILE_OPEN;
      goto cleanup;
    }

    printf("%s开始下载到文件: %s%s%s%s\n", BOLD, RESET, BLUE, full_output_path, RESET);

    // 下载内容
    if (response_info.content_length > 0) {
      // 已知长度的下载
      if (download_https_content_with_length(https_connection, output_file,
        response_info.content_length, &progress, &remaining_buffer) != 0) {
        fprintf(stderr, "%s错误: 下载过程中发生错误%s\n", RED, RESET);
        result = DOWNLOAD_ERROR_NETWORK;
        goto cleanup;
      }
    }
    else {
      // 未知长度的下载
      if (download_https_content_until_close(https_connection, output_file, &progress, &remaining_buffer) != 0) {
        fprintf(stderr, "%s错误: 下载过程中发生错误%s\n", RED, RESET);
        result = DOWNLOAD_ERROR_NETWORK;
        goto cleanup;
      }
    }

    // 显示下载摘要
    print_download_summary(&progress, 1);

  cleanup:
    if (https_connection) {
      close_https_connection(https_connection);
    }
    if (output_file) {
      fclose(output_file);

      // 如果下载失败，删除不完整的文件
      if (result != DOWNLOAD_SUCCESS) {
        if (remove(full_output_path) == 0) {
          printf("%s已删除不完整的文件: %s\n%s", YELLOW, full_output_path, RESET);
        }
      }
    }

    // 如果成功或者非重定向错误，退出循环
    if (result == DOWNLOAD_SUCCESS ||
      determine_status_action(response_info.status_code) != STATUS_ACTION_REDIRECT) {
      cleanup_openssl();
      return result;
    }
  }

  // 重定向次数过多
  fprintf(stderr, "%s错误: 重定向次数过多%s\n", RED, RESET);
  cleanup_openssl();
  return DOWNLOAD_ERROR_HTTP_RESPONSE;

#else
  fprintf(stderr, "错误: 程序编译时未包含 HTTPS 支持\n");
  return DOWNLOAD_ERROR_NETWORK;
#endif
}

#else
int init_openssl() {
  fprintf(stderr, "错误: 程序编译时未包含 OpenSSL 支持\n");
  return -1;
}

void cleanup_openssl() {

}

#endif