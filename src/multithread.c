#include "../include/common.h"
#include "../include/multithread.h"
#include "../include/http.h"
#include "../include/https.h"
#include "../include/parser.h"
#include "../include/net.h"
#include "../include/utils.h"
#include "../include/progress.h"

// 最大线程数限制
#define MAX_THREADS 16
#define MIN_SEGMENT_SIZE (1024 * 1024) // 最小段大小：1MB

//创建多线程下载器
MultiThreadDownloader* create_multithread_downloader(const char* url, const char* output_filename, const char* download_dir, int thread_count) {

  // 参数验证
  if (!url || !output_filename || thread_count <= 0) {
    fprintf(stderr, "错误: 无效的参数\n");
    return NULL;
  }

  // 限制线程数量
  if (thread_count > MAX_THREADS) {
    printf("警告: 线程数量过多，限制为 %d\n", MAX_THREADS);
    thread_count = MAX_THREADS;
  }

  // 分配内存
  MultiThreadDownloader* downloader = malloc(sizeof(MultiThreadDownloader));
  if (!downloader) {
    fprintf(stderr, "错误: 内存分配失败\n");
    return NULL;
  }

  // 初始化基本信息
  memset(downloader, 0, sizeof(MultiThreadDownloader));
  downloader->url = strdup(url);
  downloader->output_filename = strdup(output_filename);
  downloader->download_dir = download_dir ? strdup(download_dir) : NULL;
  downloader->thread_count = thread_count;
  downloader->file_size = -1;
  downloader->should_stop = 0;

  // 初始化互斥锁
  if (pthread_mutex_init(&downloader->progress_mutex, NULL) != 0 ||
    pthread_mutex_init(&downloader->file_mutex, NULL) != 0) {
    fprintf(stderr, "错误: 互斥锁初始化失败\n");
    free(downloader->url);
    free(downloader->output_filename);
    free(downloader->download_dir);
    free(downloader);
    return NULL;
  }

  printf("✓ 多线程下载器创建成功 (线程数: %d)\n", thread_count);
  return downloader;
}

// 销毁多线程下载器
void destroy_multithread_downloader(MultiThreadDownloader* downloader) {
  if (!downloader) return;

  // 确保所有线程已停止
  stop_multithread_download(downloader);

  // 清理内存
  free(downloader->url);
  free(downloader->output_filename);
  free(downloader->download_dir);
  free(downloader->segments);
  free(downloader->threads);

  // 销毁互斥锁
  pthread_mutex_destroy(&downloader->progress_mutex);
  pthread_mutex_destroy(&downloader->file_mutex);

  free(downloader);
  printf("✓ 多线程下载器已销毁\n");
}

int start_multithread_download(MultiThreadDownloader* downloader) {
}

void stop_multithread_download(MultiThreadDownloader* downloader) {
}

void display_multithread_progress(MultiThreadDownloader* downloader) {
}
// 发送 HEAD 请求检查 Range 支持
int send_head_request(const char* url, HttpResponseInfo* response_info) {
  // 解析 URL
  URLInfo url_info = { 0 };
  if (parse_url(url, &url_info) != 0) {
    fprintf(stderr, "错误: 无法解析URL\n");
    return -1;
  }

  // 建立连接
  int sockfd = -1;
  char ip_str[INET_ADDRSTRLEN];

  // 域名解析
  if (url_info.host_type == DOMAIN) {
    if (resolve_hostname(url_info.host, ip_str, sizeof(ip_str)) != 0) {
      fprintf(stderr, "错误: 域名解析失败\n");
      return -1;
    }
  }
  else {
    strcpy(ip_str, url_info.host);
  }

  // 建立连接
  if (url_info.protocol_type == PROTOCOL_HTTPS) {
#ifdef WITH_OPENSSL
    // 对于 HTTPS，我们需要使用 SSL 连接
    if (init_openssl() != 0) {
      return -1;
    }

    HttpsConnection* conn = create_https_connection(url_info.host, url_info.port);
    if (!conn) {
      cleanup_openssl();
      return -1;
    }

    // 构建 HEAD 请求
    char request[REQUEST_BUFFER];
    int request_len = snprintf(request, sizeof(request),
      "HEAD %s HTTP/1.1\r\n"
      "Host: %s\r\n"
      "User-Agent: Mozilla/5.0 (X11; Linux x86_64) AppleWebKit/537.36\r\n"
      "Accept: */*\r\n"
      "Connection: close\r\n"
      "\r\n", url_info.path, url_info.host);

    // 发送请求
    if (ssl_send_data(conn, request, request_len) != 0) {
      close_https_connection(conn);
      cleanup_openssl();
      return -1;
    }

    // 接收响应
    HttpReadBuffer read_buffer = { 0 };
    int result = parse_https_response_headers(conn, response_info, &read_buffer);

    close_https_connection(conn);
    cleanup_openssl();
    return result;
#else
    fprintf(stderr, "错误: HTTPS 支持未编译\n");
    return -1;
#endif
  }
  else {
    // HTTP 连接
    sockfd = create_tcp_connection(ip_str, url_info.port);
    if (sockfd < 0) {
      return -1;
    }

    // 构建 HEAD 请求
    char request[REQUEST_BUFFER];
    int request_len = snprintf(request, sizeof(request),
      "HEAD %s HTTP/1.1\r\n"
      "Host: %s\r\n"
      "User-Agent: Mozilla/5.0 (X11; Linux x86_64) AppleWebKit/537.36\r\n"
      "Accept: */*\r\n"
      "Connection: close\r\n"
      "\r\n",
      url_info.path, url_info.host);

    // 发送请求
    if (send(sockfd, request, request_len, 0) != request_len) {
      close(sockfd);
      return -1;
    }

    // 接收响应
    HttpReadBuffer read_buffer = { 0 };
    read_buffer.sockfd = sockfd;
    int result = parse_http_response_headers(sockfd, response_info, &read_buffer);

    close(sockfd);
    return result;
  }
}

// 检查服务器是否支持 Range 请求
int check_range_support(const char* url, long long* file_size) {
  if (!url || !file_size) {
    fprintf(stderr, "错误: 无效的参数\n");
    return -1;
  }

  printf("正在检查服务器 Range 支持...\n");

  HttpResponseInfo response_info = { 0 };

  // 发送 HEAD 请求
  if (send_head_request(url, &response_info) != 0) {
    fprintf(stderr, "错误: HEAD 请求失败\n");
    return -1;
  }

  printf("HTTP 状态码: %d %s\n", response_info.status_code, response_info.status_message);

  // 检查状态码
  if (response_info.status_code != 200) {
    fprintf(stderr, "错误: 服务器返回非 200 状态码\n");
    return -1;
  }

  // 获取文件大小
  *file_size = response_info.content_length;
  if (*file_size <= 0) {
    printf("警告: 无法获取文件大小 (Content-Length: %lld)\n", *file_size);
    return 0; // 文件大小未知，不支持多线程
  }

  printf("文件大小: %lld 字节 (%.2f MB)\n", *file_size, *file_size / 1024.0 / 1024.0);

  // 检查 Accept-Ranges 头
  int range_support = 0;

  // 在 HttpResponseInfo 结构中应该有 accept_ranges 字段
  // 我们需要更新这个结构体
  if (strstr(response_info.accept_ranges, "bytes") != NULL) {
    range_support = 1;
    printf("✓ 服务器支持 Range 请求 (Accept-Ranges: %s)\n", response_info.accept_ranges);
  }
  else if (strlen(response_info.accept_ranges) == 0) {
    // 如果没有 Accept-Ranges 头，尝试发送一个测试 Range 请求
    printf("未找到 Accept-Ranges 头，发送测试 Range 请求...\n");
    range_support = test_range_request(url);
  }
  else {
    printf("✗ 服务器不支持 Range 请求 (Accept-Ranges: %s)\n", response_info.accept_ranges);
    range_support = 0;
  }

  return range_support;
}

// 发送测试 Range 请求来验证支持
int test_range_request(const char* url) {
  // 解析 URL
  URLInfo url_info = { 0 };
  if (parse_url(url, &url_info) != 0) {
    return 0;
  }

  int sockfd = -1;
  char ip_str[INET_ADDRSTRLEN];

  // 域名解析
  if (url_info.host_type == DOMAIN) {
    if (resolve_hostname(url_info.host, ip_str, sizeof(ip_str)) != 0) {
      return 0;
    }
  }
  else {
    strcpy(ip_str, url_info.host);
  }

  printf("发送测试 Range 请求 (bytes=0-1023)...\n");

  if (url_info.protocol_type == PROTOCOL_HTTPS) {
#ifdef WITH_OPENSSL
    if (init_openssl() != 0) {
      return 0;
    }

    HttpsConnection* conn = create_https_connection(url_info.host, url_info.port);
    if (!conn) {
      cleanup_openssl();
      return 0;
    }

    // 构建带 Range 的 GET 请求
    char request[REQUEST_BUFFER];
    int request_len = snprintf(request, sizeof(request),
      "GET %s HTTP/1.1\r\n"
      "Host: %s\r\n"
      "User-Agent: Mozilla/5.0 (X11; Linux x86_64) AppleWebKit/537.36\r\n"
      "Range: bytes=0-1023\r\n"
      "Connection: close\r\n"
      "\r\n",
      url_info.path, url_info.host);

    // 发送请求
    if (ssl_send_data(conn, request, request_len) != 0) {
      close_https_connection(conn);
      cleanup_openssl();
      return 0;
    }

    // 接收响应头
    HttpResponseInfo response_info = { 0 };
    HttpReadBuffer read_buffer = { 0 };
    if (parse_https_response_headers(conn, &response_info, &read_buffer) == 0) {
      close_https_connection(conn);
      cleanup_openssl();

      printf("测试 Range 请求状态码: %d\n", response_info.status_code);

      if (response_info.status_code == 206) {
        printf("✓ 服务器支持 Range 请求 (返回 206 Partial Content)\n");
        return 1;
      }
      else if (response_info.status_code == 200) {
        printf("✗ 服务器忽略了 Range 请求 (返回完整文件)\n");
        return 0;
      }
    }

    close_https_connection(conn);
    cleanup_openssl();
#endif
  }
  else {
    // HTTP 测试
    sockfd = create_tcp_connection(ip_str, url_info.port);
    if (sockfd < 0) {
      return 0;
    }

    // 构建带 Range 的 GET 请求
    char request[REQUEST_BUFFER];
    int request_len = snprintf(request, sizeof(request),
      "GET %s HTTP/1.1\r\n"
      "Host: %s\r\n"
      "User-Agent: Mozilla/5.0 (X11; Linux x86_64) AppleWebKit/537.36\r\n"
      "Range: bytes=0-1023\r\n"
      "Connection: close\r\n"
      "\r\n",
      url_info.path, url_info.host);

    // 发送请求
    if (send(sockfd, request, request_len, 0) != request_len) {
      close(sockfd);
      return 0;
    }

    // 接收响应头
    HttpResponseInfo response_info = { 0 };
    HttpReadBuffer read_buffer = { 0 };
    read_buffer.sockfd = sockfd;

    if (parse_http_response_headers(sockfd, &response_info, &read_buffer) == 0) {
      close(sockfd);

      printf("测试 Range 请求状态码: %d\n", response_info.status_code);

      if (response_info.status_code == 206) {
        printf("✓ 服务器支持 Range 请求 (返回 206 Partial Content)\n");
        return 1;
      }
      else if (response_info.status_code == 200) {
        printf("✗ 服务器忽略了 Range 请求 (返回完整文件)\n");
        return 0;
      }
    }

    close(sockfd);
  }

  return 0;
}