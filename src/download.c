#include "../include/common.h"
#include "../include/download.h"
#include "../include/parser.h"
#include "../include/net.h"
#include "../include/http.h"
#include "../include/progress.h"
#include "../include/utils.h"
ssize_t recv_data_with_timeout(int sockfd, void* buffer, size_t length, int timeout_ms) {
  struct timeval timeout;
  timeout.tv_sec = timeout_ms / 1000;
  timeout.tv_usec = (timeout_ms % 1000) * 1000;

  fd_set readfds;
  FD_ZERO(&readfds);
  FD_SET(sockfd, &readfds);

  int select_result = select(sockfd + 1, &readfds, NULL, NULL, &timeout);
  if (select_result <= 0) {
    return -1; // 超时或错误
  }

  return recv(sockfd, buffer, length, 0);
}

int recv_full_data(int sockfd, void* buffer, size_t total_length, int timeout_ms) {
  char* buf_ptr = (char*)buffer;
  size_t remaining = total_length;

  while (remaining > 0) {
    ssize_t bytes_received = recv_data_with_timeout(sockfd, buf_ptr, remaining, timeout_ms);

    if (bytes_received <= 0) {
      return -1; // 接收失败或连接关闭
    }

    buf_ptr += bytes_received;
    remaining -= bytes_received;
  }

  return 0;
}

int download_content_with_length(int sockfd, FILE* output_file, long long content_length, DownloadProgress* progress, HttpReadBuffer* remaining_buffer) {
  const size_t BUFFER_SIZE = 8192;
  char buffer[BUFFER_SIZE];

  if (!progress || !output_file) {
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

    // 接收数据
    ssize_t bytes_received = recv(sockfd, buffer, bytes_to_receive, 0);

    if (bytes_received <= 0) {
      clear_progress_line();
      if (bytes_received == 0) {
        fprintf(stderr, "连接意外关闭，已下载 %lld/%lld 字节\n",
          progress->downloaded_size, content_length);
      }
      else {
        perror("接收数据失败");
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

    // 定期更新进度显示（每100ms或每接收一定数据量）
    time_t current_time = time(NULL);
    static long long last_update_size = 0;

    if (current_time > progress->last_update_time ||
      progress->downloaded_size - last_update_size > 8192) {
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

int download_content_until_close(int sockfd, FILE* output_file, DownloadProgress* progress, HttpReadBuffer* remaining_buffer) {
  const size_t BUFFER_SIZE = 8192;
  char buffer[BUFFER_SIZE];

  if (!progress || !output_file) {
    fprintf(stderr, "错误: 无效的参数\n");
    return -1;
  }

  // 检查socket是否有效
  if (sockfd < 0) {
    fprintf(stderr, "错误: 无效的socket描述符 %d\n", sockfd);
    return -1;
  }

  // 使用fcntl检查socket是否真的有效
  int flags = fcntl(sockfd, F_GETFL);
  if (flags == -1) {
    fprintf(stderr, "错误: socket描述符无效或已关闭 %d\n", sockfd);
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
    // 清空缓冲区标记
    remaining_buffer->parse_position = remaining_buffer->data_length;

    // 更新进度显示
    update_download_progress(progress);
  }

  while (1) {
    // 再次检查socket有效性
    flags = fcntl(sockfd, F_GETFL);
    if (flags == -1) {
      clear_progress_line();
      fprintf(stderr, "错误: socket在下载过程中变为无效\n");
      return -1;
    }

    ssize_t bytes_received = recv(sockfd, buffer, BUFFER_SIZE, 0);

    if (bytes_received == 0) {
      // 连接正常关闭
      break;
    }

    if (bytes_received < 0) {
      clear_progress_line();
      fprintf(stderr, "recv错误: %s (errno: %d, sockfd: %d)\n", strerror(errno), errno, sockfd);
      return -1;
    }

    if (fwrite(buffer, 1, bytes_received, output_file) != (size_t)bytes_received) {
      clear_progress_line();
      fprintf(stderr, "文件写入失败\n");
      return -1;
    }

    progress->downloaded_size += bytes_received;

    // 定期更新进度显示
    time_t current_time = time(NULL);
    static long long last_update_size = 0;

    if (current_time > progress->last_update_time ||
      progress->downloaded_size - last_update_size > 8192) {
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
int download_file_http(const char* url, const char* output_filename, const char* download_dir, int redirect_count) {
  const int MAX_REDIRECTS = 10;
  const char* current_url = url;
  char redirect_url[2048];
  char full_output_path[4096];  // 存储完整的输出路径

  // 构造完整的输出文件路径
  if (!output_filename) {
    fprintf(stderr, "错误: 输出文件名不能为空\n");
    return DOWNLOAD_ERROR_URL_PARSE;
  }

  // 如果没有指定下载目录或目录为空，使用当前工作目录
  if (!download_dir || strlen(download_dir) == 0) {
    char current_dir[PATH_MAX];
    if (getcwd(current_dir, sizeof(current_dir)) == NULL) {
      fprintf(stderr, "错误: 无法获取当前工作目录: %s\n", strerror(errno));
      return DOWNLOAD_ERROR_FILE_OPEN;
    }
    snprintf(full_output_path, sizeof(full_output_path), "%s/%s", current_dir, output_filename);
  }
  else {
    // 检查下载目录是否存在
    struct stat dir_stat;
    if (stat(download_dir, &dir_stat) != 0) {
      fprintf(stderr, "错误: 下载目录不存在: %s\n", download_dir);
      return DOWNLOAD_ERROR_FILE_OPEN;
    }
    if (!S_ISDIR(dir_stat.st_mode)) {
      fprintf(stderr, "错误: 指定的路径不是目录: %s\n", download_dir);
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

  printf("下载文件将保存到: %s\n", full_output_path);

  for (int redirect_iter = 0; redirect_iter <= MAX_REDIRECTS; redirect_iter++) {
    // 变量初始化
    URLInfo url_info = { 0 };
    HttpResponseInfo response_info = { 0 };
    DownloadProgress progress = { 0 };
    HttpReadBuffer remaining_buffer = { 0 };
    int sockfd = -1;
    FILE* output_file = NULL;
    DownloadResult result = DOWNLOAD_SUCCESS;

    if (!current_url) {
      fprintf(stderr, "错误: 无效的URL\n");
      return DOWNLOAD_ERROR_URL_PARSE;
    }

    // URL解析
    // printf("正在解析URL: %s\n", current_url);
    if (parse_url(current_url, &url_info) != 0) {
      fprintf(stderr, "错误: 无法解析URL\n");
      return DOWNLOAD_ERROR_URL_PARSE;
    }
    printf("Host: %s, Port: %d, Path: %s\n", url_info.host, url_info.port, url_info.path);

    char ip_str[INET_ADDRSTRLEN];

    // 域名解析
    if (url_info.host_type == UNALLOWED) {
      printf("错误：无效的域名\n");
      return DOWNLOAD_ERROR_DNS_RESOLVE;
    }
    else if (url_info.host_type == DOMAIN) {
      // printf("正在解析域名: %s\n", url_info.host);
      if (resolve_hostname(url_info.host, ip_str, sizeof(ip_str)) != 0) {
        fprintf(stderr, "错误: 域名解析失败\n");
        return DOWNLOAD_ERROR_DNS_RESOLVE;
      }
      printf("HostIP: %s\n", ip_str);
    }
    else {
      strncpy(ip_str, url_info.host, sizeof(ip_str) - 1);
      ip_str[sizeof(ip_str) - 1] = '\0';
      printf("HostIP: %s\n", ip_str);
    }

    // 建立TCP连接
    // printf("正在连接服务器...\n");
    sockfd = create_tcp_connection(ip_str, url_info.port);
    if (sockfd < 0) {
      fprintf(stderr, "错误: 无法连接到服务器\n");
      return DOWNLOAD_ERROR_CONNECTION;
    }
    // printf("连接建立成功\n");

    // 构造http请求及发送
    char request_buffer[REQUEST_BUFFER];
    int request_length = build_http_get_request(url_info.host, url_info.path, request_buffer, sizeof(request_buffer));
    if (request_length <= 0) {
      fprintf(stderr, "错误: 构造HTTP请求失败\n");
      result = DOWNLOAD_ERROR_HTTP_REQUEST;
      goto cleanup_iteration;
    }

    // printf("正在发送HTTP请求...\n");
    if (send_full_data(sockfd, request_buffer, request_length) != 0) {
      fprintf(stderr, "错误: 发送HTTP请求失败\n");
      result = DOWNLOAD_ERROR_HTTP_REQUEST;
      goto cleanup_iteration;
    }

    // 解析http响应头
    // printf("正在接收响应...\n");
    if (parse_http_response_headers(sockfd, &response_info, &remaining_buffer) != 0) {
      fprintf(stderr, "错误: 解析HTTP响应失败\n");
      result = DOWNLOAD_ERROR_HTTP_RESPONSE;
      goto cleanup_iteration;
    }

    printf("HTTP状态: %d %s\n", response_info.status_code, response_info.status_message);

    // 处理重定向
    if (determine_status_action(response_info.status_code) == STATUS_ACTION_REDIRECT) {
      if (redirect_iter >= MAX_REDIRECTS) {
        fprintf(stderr, "错误: 重定向次数过多 (超过%d次)\n", MAX_REDIRECTS);
        result = DOWNLOAD_ERROR_HTTP_RESPONSE;
        goto cleanup_iteration;
      }

      printf("警告: 服务器返回重定向状态码 %d\n", response_info.status_code);
      if (response_info.location[0]) {
        printf("重定向到: %s (第%d次重定向)\n", response_info.location, redirect_iter + 1);
        printf("-------------------------重定向第(%d)次--------------------------\n", redirect_iter + 1);

        // 保存重定向URL
        strncpy(redirect_url, response_info.location, sizeof(redirect_url) - 1);
        redirect_url[sizeof(redirect_url) - 1] = '\0';

        // 关闭当前连接
        if (sockfd >= 0) {
          close(sockfd);
          sockfd = -1;
        }

        // 更新当前URL并继续循环
        current_url = redirect_url;
        continue;
      }
      else {
        fprintf(stderr, "错误: 重定向但没有提供Location头\n");
        result = DOWNLOAD_ERROR_HTTP_RESPONSE;
        goto cleanup_iteration;
      }
    }

    // 处理错误状态码
    if (determine_status_action(response_info.status_code) == STATUS_ACTION_ERROR) {
      fprintf(stderr, "错误: 服务器返回错误状态码 %d\n", response_info.status_code);
      result = DOWNLOAD_ERROR_HTTP_RESPONSE;
      goto cleanup_iteration;
    }

    // 显示文件信息
    if (response_info.content_length > 0) {
      printf("文件大小: %s\n", format_file_size(response_info.content_length));
    }
    else {
      printf("文件大小: 未知\n");
    }
    if (response_info.content_type[0]) {
      printf("文件类型: %s\n", response_info.content_type);
    }

    // 打开输出文件（使用完整路径）
    output_file = fopen(full_output_path, "wb");
    if (!output_file) {
      fprintf(stderr, "错误: 无法创建输出文件 %s: %s\n", full_output_path, strerror(errno));
      result = DOWNLOAD_ERROR_FILE_OPEN;
      goto cleanup_iteration;
    }

    printf("开始下载到文件: %s\n", full_output_path);

    // 下载内容
    if (response_info.content_length > 0) {
      // 已知长度的下载
      if (download_content_with_length(sockfd, output_file,
        response_info.content_length,
        &progress, &remaining_buffer) != 0) {
        fprintf(stderr, "错误: 下载过程中发生错误\n");
        result = DOWNLOAD_ERROR_NETWORK;
        goto cleanup_iteration;
      }
    }
    else {
      // 未知长度的下载
      if (download_content_until_close(sockfd, output_file, &progress, &remaining_buffer) != 0) {
        fprintf(stderr, "错误: 下载过程中发生错误\n");
        result = DOWNLOAD_ERROR_NETWORK;
        goto cleanup_iteration;
      }
    }

    // 显示下载摘要
    print_download_summary(&progress, 1);

  cleanup_iteration:
    if (sockfd >= 0) {
      close(sockfd);
    }
    if (output_file) {
      fclose(output_file);

      // 如果下载失败，删除不完整的文件
      if (result != DOWNLOAD_SUCCESS) {
        if (remove(full_output_path) == 0) {
          printf("已删除不完整的文件: %s\n", full_output_path);
        }
      }
    }

    // 如果成功或者非重定向错误，退出循环
    if (result == DOWNLOAD_SUCCESS ||
      determine_status_action(response_info.status_code) != STATUS_ACTION_REDIRECT) {
      return result;
    }
  }

  // 重定向次数过多
  fprintf(stderr, "错误: 重定向次数过多\n");
  return DOWNLOAD_ERROR_HTTP_RESPONSE;
}

int download_file_https(const char* url, const char* output_filename, const char* download_dir, int redirect_count) {
  //TODO
  return -1;
}