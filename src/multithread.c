#include "../include/common.h"
#include "../include/multithread.h"
#include "../include/http.h"
#include "../include/https.h"
#include "../include/parser.h"
#include "../include/net.h"
#include "../include/utils.h"
#include "../include/progress.h"
#include "../include/menu.h"
// CLI颜色定义
static const char* BLUE = "\033[34m";
static const char* CYAN = "\033[36m";
static const char* YELLOW = "\033[33m";
static const char* RESET = "\033[0m";
static const char* BOLD = "\033[1m";
static const char* RED = "\033[31m";
static const char* GREEN = "\033[32m";
static const char* CLEAR_LINE = "\r\033[K";

// Init and Cleanup
// 创建多线程下载器
MultiThreadDownloader* create_multithread_downloader(const char* url, const char* output_filename, const char* download_dir, int thread_count) {
  

  // 参数验证
  if (!url || !output_filename || thread_count <= 0) {
    fprintf(stderr, "错误: 无效的参数\n");
    return NULL;
  }

  // 限制线程数量
  if (thread_count > MAX_THREADS) {
    printf("%s警告: 线程数量过多，限制为 %d%s\n", YELLOW, MAX_THREADS, RESET);
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

  printf("%s✓ 多线程下载器创建成功 (线程数: %d)%s\n", GREEN, thread_count, RESET);
  return downloader;
}

int initialize_multithread_download(MultiThreadDownloader* downloader) {
  

  if (!downloader) {
    return -1;
  }

  // 检查 Range 支持并获取文件大小
  long long file_size = 0;
  int range_support = check_range_support(downloader->url, &file_size);

  if (range_support < 0) {
    fprintf(stderr, "错误: 无法检查 Range 支持\n");
    return -1;
  }

  if (range_support == 0 || file_size <= MIN_SEGMENT_SIZE) {
    // CLI颜色定义
    const char* BLUE = "\033[34m";
    const char* CYAN = "\033[36m";
    const char* YELLOW = "\033[33m";
    const char* RESET = "\033[0m";
    const char* BOLD = "\033[1m";
    const char* RED = "\033[31m";
    const char* GREEN = "\033[32m";
    const char* CLEAR_LINE = "\r\033[K";
    printf("%s警告: 将使用单线程下载 (Range不支持或文件过小)%s\n", YELLOW, RESET);
    downloader->thread_count = 1;
    downloader->file_size = file_size > 0 ? file_size : -1;
    return 0; // 退化到单线程
  }

  downloader->file_size = file_size;

  // 分配内存
  downloader->segments = malloc(sizeof(FileSegment) * downloader->thread_count);
  downloader->threads = malloc(sizeof(ThreadDownloadParams) * downloader->thread_count);

  if (!downloader->segments || !downloader->threads) {
    fprintf(stderr, "错误: 内存分配失败\n");
    free(downloader->segments);
    free(downloader->threads);
    return -1;
  }

  // 计算文件分段
  int actual_threads = calculate_file_segments(file_size, downloader->thread_count, downloader->segments);
  if (actual_threads < 0) {
    fprintf(stderr, "错误: 文件分段计算失败\n");
    return -1;
  }

  downloader->thread_count = actual_threads;

  // 初始化线程参数
  for (int i = 0; i < downloader->thread_count; i++) {
    ThreadDownloadParams* thread = &downloader->threads[i];
    memset(thread, 0, sizeof(ThreadDownloadParams));

    thread->thread_id = i;
    thread->url = strdup(downloader->url);
    thread->segment = &downloader->segments[i];
    thread->should_stop = 0;
    thread->progress_mutex = &downloader->progress_mutex;

    // 生成临时文件名
    thread->temp_filename = malloc(256);
    snprintf(thread->temp_filename, 256, "%s.part%d",
      downloader->output_filename, i);
  }

  printf("%s✓ 多线程下载初始化完成\n%s", GREEN, RESET);
  return 1; // 表示使用多线程
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
  // printf("✓ 多线程下载器已销毁\n");
}





// Workers
// 下载线程Worker函数
void* thread_download_worker(void* arg) {
  ThreadDownloadParams* thread_params = (ThreadDownloadParams*)arg;

  // 使用带重试的下载函数
  int result = download_segment_with_retry(thread_params);

  // 返回结果
  pthread_exit((void*)(intptr_t)result);
}

// 进度条线程Worker函数
void* progress_display_worker(void* arg) {
  MultiThreadDownloader* downloader = (MultiThreadDownloader*)arg;

  while (!downloader->should_stop) {
    display_multithread_progress(downloader);
    usleep(50000); // 每0.05秒更新一次
  }

  // 最后显示一次完整进度
  display_multithread_progress(downloader);
  printf("\n");

  pthread_exit(NULL);
}







// !!MAIN ENTRANCE!!
int multithread_download(MultiThreadDownloader* downloader) {
  

  if (!downloader) {
    return -1;
  }

  // 首先初始化多线程下载
  int init_result = initialize_multithread_download(downloader);
  if (init_result < 0) {
    return -1;
  }

  if (init_result == 0) {
    // 退化到单线程下载
    printf("%s使用单线程下载模式...%s\n", YELLOW, RESET);
    return download_file_fallback_single_thread(downloader);
  }

  printf("\n%s%s=== 开始多线程下载 ===%s\n", BOLD, GREEN, RESET);
  downloader->start_time = time(NULL);
  downloader->should_stop = 0;
  downloader->completed_threads = 0;
  downloader->error_count = 0;

  // 创建并启动所有下载线程
  for (int i = 0; i < downloader->thread_count; i++) {
    ThreadDownloadParams* thread = &downloader->threads[i];

    if (pthread_create(&thread->pthread_id, NULL, thread_download_worker, thread) != 0) {
      fprintf(stderr, "错误: 创建线程 %d 失败\n", i);
      downloader->should_stop = 1; // 停止其他线程
      downloader->error_count++;
      continue;
    }
  }

  // 显示进度的线程
  pthread_t progress_thread;
  if (pthread_create(&progress_thread, NULL, progress_display_worker, downloader) != 0) {
    fprintf(stderr, "警告: 无法创建进度显示线程\n");
  }

  // 等待所有下载线程完成
  int total_errors = 0;
  for (int i = 0; i < downloader->thread_count; i++) {
    ThreadDownloadParams* thread = &downloader->threads[i];

    if (thread->pthread_id != 0) {
      void* thread_result;
      if (pthread_join(thread->pthread_id, &thread_result) == 0) {
        int result = (int)(intptr_t)thread_result;
        if (result != 0) {
          total_errors++;
        }
        else {
          downloader->completed_threads++;
        }
      }
    }
  }

  // 停止进度显示线程
  downloader->should_stop = 1;
  pthread_join(progress_thread, NULL);

  // 检查下载结果
  if (total_errors > 0) {
    fprintf(stderr, "\n%s错误: %d 个线程下载失败%s\n", RED, total_errors, RESET);
    cleanup_temp_files(downloader);
    return -1;
  }

  printf("\n%s%s=== 合并文件 ===%s\n", BOLD, CYAN, RESET);

  // 合并所有临时文件
  int merge_result = merge_temp_files(downloader);
  if (merge_result != 0) {
    fprintf(stderr, "%s错误: 文件合并失败%s\n", RED, RESET);
    cleanup_temp_files(downloader);
    return -1;
  }

  // 清理临时文件
  cleanup_temp_files(downloader);
  printf("%s%s✓ 多线程下载完成！%s\n", GREEN, BOLD, RESET);
  return 0;
}






// Inner utils:
int merge_temp_files(MultiThreadDownloader* downloader) {
  
  if (!downloader) {
    return -1;
  }

  // 构造完整输出路径
  char full_output_path[4096];
  if (downloader->download_dir && strlen(downloader->download_dir) > 0) {
    if (downloader->download_dir[strlen(downloader->download_dir) - 1] == '/') {
      snprintf(full_output_path, sizeof(full_output_path), "%s%s",
        downloader->download_dir, downloader->output_filename);
    }
    else {
      snprintf(full_output_path, sizeof(full_output_path), "%s/%s",
        downloader->download_dir, downloader->output_filename);
    }
  }
  else {
    strcpy(full_output_path, downloader->output_filename);
  }

  // 打开最终输出文件
  FILE* output_file = fopen(full_output_path, "wb");
  if (!output_file) {
    fprintf(stderr, "错误: 无法创建输出文件 %s: %s\n",
      full_output_path, strerror(errno));
    return -1;
  }

  printf("合并到: %s\n", full_output_path);

  // 按顺序合并所有段文件
  const size_t BUFFER_SIZE = 65536; // 64KB 缓冲区
  char* buffer = malloc(BUFFER_SIZE);
  if (!buffer) {
    fclose(output_file);
    fprintf(stderr, "错误: 内存分配失败\n");
    return -1;
  }

  long long total_merged = 0;

  for (int i = 0; i < downloader->thread_count; i++) {
    ThreadDownloadParams* thread = &downloader->threads[i];

    printf("合并段 %d: %s\n", i, thread->temp_filename);

    FILE* temp_file = fopen(thread->temp_filename, "rb");
    if (!temp_file) {
      fprintf(stderr, "错误: 无法打开临时文件 %s: %s\n",
        thread->temp_filename, strerror(errno));
      free(buffer);
      fclose(output_file);
      return -1;
    }

    // 验证文件大小
    fseek(temp_file, 0, SEEK_END);
    long temp_file_size = ftell(temp_file);
    fseek(temp_file, 0, SEEK_SET);

    long long expected_size = thread->segment->end_byte - thread->segment->start_byte + 1;
    if (temp_file_size != expected_size) {
      fprintf(stderr, "警告: 段 %d 大小不匹配 (实际: %ld, 期望: %lld)\n",
        i, temp_file_size, expected_size);
    }

    // 复制文件内容
    size_t bytes_read;
    long long segment_copied = 0;

    while ((bytes_read = fread(buffer, 1, BUFFER_SIZE, temp_file)) > 0) {
      if (fwrite(buffer, 1, bytes_read, output_file) != bytes_read) {
        fprintf(stderr, "错误: 写入输出文件失败\n");
        fclose(temp_file);
        free(buffer);
        fclose(output_file);
        return -1;
      }
      segment_copied += bytes_read;
      total_merged += bytes_read;
    }

    fclose(temp_file);
    printf("  段 %d 合并完成 (%lld 字节)\n", i, segment_copied);
  }

  free(buffer);
  fclose(output_file);

  printf("文件合并完成，总大小: %lld 字节\n", total_merged);

  // 验证合并后的文件大小
  if (downloader->file_size > 0 && total_merged != downloader->file_size) {
    fprintf(stderr, "警告: 合并后文件大小不匹配 (实际: %lld, 期望: %lld)\n",
      total_merged, downloader->file_size);
  }
  return 0;
}

void cleanup_temp_files(MultiThreadDownloader* downloader) {
  if (!downloader || !downloader->threads) {
    return;
  }

  printf("清理临时文件...\n");

  for (int i = 0; i < downloader->thread_count; i++) {
    ThreadDownloadParams* thread = &downloader->threads[i];

    if (thread->temp_filename) {
      if (unlink(thread->temp_filename) == 0) {
        printf("  已删除: %s\n", thread->temp_filename);
      }
      else {
        // 文件可能已经不存在
      }
    }
  }
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

    HttpsConnection* https_connection = create_https_connection(url_info.host, url_info.port);
    if (!https_connection) {
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
    if (ssl_send_data(https_connection, request, request_len) != 0) {
      close_https_connection(https_connection);
      cleanup_openssl();
      return -1;
    }

    // 接收响应
    HttpReadBuffer read_buffer = { 0 };
    int result = parse_https_response_headers(https_connection, response_info, &read_buffer);

    close_https_connection(https_connection);
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

  // printf("正在检查服务器 Range 支持...\n");
  HttpResponseInfo response_info = { 0 };

  // 发送 HEAD 请求
  if (send_head_request(url, &response_info) != 0) {
    fprintf(stderr, "错误: HEAD 请求失败\n");
    return -1;
  }

  // printf("HTTP 状态码: %d %s\n", response_info.status_code, response_info.status_message);
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
  // printf("文件大小: %lld 字节 (%.2f MB)\n", *file_size, *file_size / 1024.0 / 1024.0);
  // 检查 Accept-Ranges 头
  int range_support = 0;
  // 在 HttpResponseInfo 结构中应该有 accept_ranges 字段
  // 我们需要更新这个结构体
  if (strstr(response_info.accept_ranges, "bytes") != NULL) {
    range_support = 1;
    // printf("✓ 服务器支持 Range 请求 (Accept-Ranges: %s)\n", response_info.accept_ranges);
  }
  else if (strlen(response_info.accept_ranges) == 0) {
    // 如果没有 Accept-Ranges 头，尝试发送一个测试 Range 请求
    // printf("未找到 Accept-Ranges 头，发送测试 Range 请求...\n");
    range_support = test_range_request(url);
  }
  else {
    printf("%s✗ 服务器不支持 Range 请求 (Accept-Ranges: %s)%s\n", RED, response_info.accept_ranges, RESET);
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

  // printf("发送测试 Range 请求 (bytes=0-1023)...\n");

  if (url_info.protocol_type == PROTOCOL_HTTPS) {
#ifdef WITH_OPENSSL
    if (init_openssl() != 0) {
      return 0;
    }

    HttpsConnection* https_connection = create_https_connection(url_info.host, url_info.port);
    if (!https_connection) {
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
    if (ssl_send_data(https_connection, request, request_len) != 0) {
      close_https_connection(https_connection);
      cleanup_openssl();
      return 0;
    }

    // 接收响应头
    HttpResponseInfo response_info = { 0 };
    HttpReadBuffer read_buffer = { 0 };
    if (parse_https_response_headers(https_connection, &response_info, &read_buffer) == 0) {
      close_https_connection(https_connection);
      cleanup_openssl();

      // printf("测试 Range 请求状态码: %d\n", response_info.status_code);

      if (response_info.status_code == 206) {
        printf("%s✓ 服务器支持 Range 请求 (返回 206 Partial Content)%s\n", GREEN, RESET);
        return 1;
      }
      else if (response_info.status_code == 200) {
        printf("%s✗ 服务器忽略了 Range 请求 (返回完整文件)%s\n", RED, RESET);
        return 0;
      }
    }

    close_https_connection(https_connection);
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

int build_range_request(const URLInfo* url_info, FileSegment* segment, char* buffer, size_t buffer_size) {
  // 使用当前的start_byte，在断点续传时会自动调整
  int length = snprintf(buffer, buffer_size,
    "GET %s HTTP/1.1\r\n"
    "Host: %s\r\n"
    "User-Agent: Mozilla/5.0 (X11; Linux x86_64) AppleWebKit/537.36\r\n"
    "Accept: */*\r\n"
    "Range: bytes=%lld-%lld\r\n"
    "Connection: close\r\n"
    "\r\n",
    url_info->path, url_info->host, segment->start_byte, segment->end_byte);

  if (length >= (int)buffer_size) {
    return -1;
  }
  return length;
}






// Progress Display
void display_multithread_progress(MultiThreadDownloader* downloader) {
  if (!downloader) {
    return;
  }
  // 颜色定义
  const char* GREEN = "\033[32m";
  const char* BLUE = "\033[34m";
  const char* YELLOW = "\033[33m";
  const char* CYAN = "\033[36m";
  const char* RED = "\033[31m";
  const char* MAGENTA = "\033[35m";
  const char* WHITE = "\033[37m";
  const char* RESET = "\033[0m";
  const char* BOLD = "\033[1m";

  // 计算总下载进度
  pthread_mutex_lock(&downloader->progress_mutex);

  long long total_downloaded = 0;
  double total_speed = 0.0;
  int active_threads = 0;
  int completed_threads = 0;
  int error_threads = 0;

  char total_file_size_str[64];
  strcpy(total_file_size_str, format_file_size(downloader->file_size));

  for (int i = 0; i < downloader->thread_count; i++) {
    FileSegment* segment = &downloader->segments[i];
    ThreadDownloadParams* thread = &downloader->threads[i];

    total_downloaded += segment->downloaded_bytes;

    switch (segment->state) {
    case THREAD_STATE_DOWNLOADING:
      active_threads++;
      total_speed += thread->download_speed;
      break;
    case THREAD_STATE_COMPLETED:
      completed_threads++;
      break;
    case THREAD_STATE_ERROR:
      error_threads++;
      break;
    default:
      break;
    }
  }

  downloader->total_downloaded = total_downloaded;
  downloader->total_speed = total_speed;

  pthread_mutex_unlock(&downloader->progress_mutex);

  // 是否是第一次显示
  static int first_display = 1;
  static int lines_printed = 0;

  if (first_display) {
    printf("文件: %s%s%s\n", YELLOW, downloader->output_filename, RESET);
    printf("总大小: %s%s%s\n\n", BLUE, format_file_size(downloader->file_size), RESET);

    // 为每个线程预留一行，加上总进度行和空行
    lines_printed = downloader->thread_count + 2;
    first_display = 0;
  }
  else {
    // 非第一次显示，回到开始位置
    printf("\033[%dA", lines_printed);  // 向上移动 lines_printed 行
  }

  // 显示各个线程的进度条
  for (int i = 0; i < downloader->thread_count; i++) {
    FileSegment* segment = &downloader->segments[i];
    ThreadDownloadParams* thread = &downloader->threads[i];

    long long segment_size = segment->end_byte - segment->start_byte + 1;
    double segment_progress = 0.0;
    if (segment_size > 0) {
      segment_progress = (double)segment->downloaded_bytes * 100.0 / segment_size;
    }

    // 线程状态颜色
    const char* status_color = WHITE;
    const char* status_text = "等待";

    switch (segment->state) {
    case THREAD_STATE_IDLE:
      status_color = WHITE;
      status_text = "空闲";
      break;
    case THREAD_STATE_CONNECTING:
      status_color = YELLOW;
      status_text = "连接";
      break;
    case THREAD_STATE_DOWNLOADING:
      status_color = CYAN;
      status_text = "下载";
      break;
    case THREAD_STATE_COMPLETED:
      status_color = GREEN;
      status_text = "完成";
      break;
    case THREAD_STATE_ERROR:
      status_color = RED;
      status_text = "错误";
      break;
    }

    // 清除当前行并显示线程信息
    printf("\r\033[K%s线程 %d:%s [%s%s%s] ", BOLD, i, RESET, status_color, status_text, RESET);

    // 线程进度条
    const int THREAD_BAR_WIDTH = 20;
    int thread_filled = (int)(segment_progress * THREAD_BAR_WIDTH / 100.0);

    printf("%s[", status_color);
    for (int j = 0; j < THREAD_BAR_WIDTH; j++) {
      if (j < thread_filled) {
        printf("█");
      }
      else if (j == thread_filled && segment_progress < 100.0) {
        printf("▌");
      }
      else {
        printf("░");
      }
    }
    printf("]%s ", RESET);

    // 显示线程百分比和下载量
    printf("%s%6.2f%%%s ", YELLOW, segment_progress, RESET);
    printf("%s%s%s/%s%s%s ",
      BLUE, format_file_size(segment->downloaded_bytes), RESET,
      WHITE, format_file_size(segment_size), RESET);

    // 显示线程速度
    if (segment->state == THREAD_STATE_DOWNLOADING) {
      printf("%s%s/s%s", CYAN, format_file_size((long long)thread->download_speed), RESET);
    }
    else if (segment->state == THREAD_STATE_COMPLETED) {
      printf("%s✓%s", GREEN, RESET);
    }
    else if (segment->state == THREAD_STATE_ERROR) {
      if (strlen(segment->error_message) > 0) {
        printf("%s✗ %s%s", RED, segment->error_message, RESET);
      }
      else {
        printf("%s✗%s", RED, RESET);
      }
    }

    printf("\n");
  }

  printf("\n");

  // 总进度条
  double total_progress = 0.0;
  if (downloader->file_size > 0) {
    total_progress = (double)total_downloaded * 100.0 / downloader->file_size;
  }

  printf("\r\033[K%s%s总进度:%s ", BOLD, MAGENTA, RESET);

  const int TOTAL_BAR_WIDTH = 40;
  int total_filled = (int)(total_progress * TOTAL_BAR_WIDTH / 100.0);

  printf("%s[%s", MAGENTA, GREEN);
  for (int i = 0; i < TOTAL_BAR_WIDTH; i++) {
    if (i < total_filled) {
      printf("█");
    }
    else if (i == total_filled && total_progress < 100.0) {
      printf("▌");
    }
    else {
      printf("░");
    }
  }
  printf("%s]%s ", MAGENTA, RESET);

  // 总百分比
  if (total_progress >= 100.0) {
    printf("%s%s✓ 100.00%%%s ", BOLD, GREEN, RESET);
  }
  else {
    printf("%s%6.2f%%%s ", YELLOW, total_progress, RESET);
  }

  // 总下载量和总文件大小
  printf("%s%s%s/%s%s%s ",
    BLUE, format_file_size(total_downloaded), RESET,
    BOLD, total_file_size_str, RESET);

  // 总速度
  printf("%s%s/s%s ",
    CYAN, format_file_size((long long)total_speed), RESET);

  // 线程状态统计
  printf(" 线程: %s%d%s活动 %s%d%s完成 ",
    YELLOW, active_threads, RESET,
    GREEN, completed_threads, RESET);
  if (error_threads > 0) {
    printf(" %s%d%s错误", RED, error_threads, RESET);
  }

  // ETA
  if (downloader->file_size > 0 && total_speed > 0 && total_progress < 100.0) {
    long long remaining_bytes = downloader->file_size - total_downloaded;
    time_t eta_seconds = (time_t)(remaining_bytes / total_speed);

    if (eta_seconds > 0) {
      printf(" ETA: %s%s%s", YELLOW, format_time_duration(eta_seconds), RESET);
    }
  }
  else if (total_progress >= 100.0) {
    printf(" %s完成%s", GREEN, RESET);
  }

  printf("\n");
  fflush(stdout);
}

void stop_multithread_download(MultiThreadDownloader* downloader) {
  if (!downloader) {
    return;
  }
  // 设置停止标志
  downloader->should_stop = 1;
  // 设置所有线程的停止标志
  if (downloader->threads) {
    for (int i = 0; i < downloader->thread_count; i++) {
      downloader->threads[i].should_stop = 1;
    }

    // 等待所有线程结束
    for (int i = 0; i < downloader->thread_count; i++) {
      ThreadDownloadParams* thread = &downloader->threads[i];
      if (thread->pthread_id != 0) {
        void* result;
        int join_result = pthread_join(thread->pthread_id, &result);
        if (join_result == 0) {
          // 成功join，重置ID
          thread->pthread_id = 0;
        }
        else {
          // join失败，尝试cancel
          pthread_cancel(thread->pthread_id);
          pthread_join(thread->pthread_id, NULL); // 等待cancel完成
          thread->pthread_id = 0;
        }
      }
    }
  }
}






// Segment Download
int download_segment(ThreadDownloadParams* thread_params) {
  if (!thread_params || !thread_params->url || !thread_params->segment) {
    return -1;
  }

  FileSegment* segment = thread_params->segment;
  segment->state = THREAD_STATE_CONNECTING;
  thread_params->start_time = time(NULL);

  // 解析 URL
  URLInfo url_info = { 0 };
  if (parse_url(thread_params->url, &url_info) != 0) {
    snprintf(segment->error_message, sizeof(segment->error_message), "URL解析失败");
    segment->state = THREAD_STATE_ERROR;
    return -1;
  }

  // 域名解析
  char ip_str[INET_ADDRSTRLEN];
  if (url_info.host_type == DOMAIN) {
    if (resolve_hostname(url_info.host, ip_str, sizeof(ip_str)) != 0) {
      snprintf(segment->error_message, sizeof(segment->error_message), "域名解析失败");
      segment->state = THREAD_STATE_ERROR;
      return -1;
    }
  }
  else {
    strcpy(ip_str, url_info.host);
  }

  // 检查是否需要停止
  if (thread_params->should_stop) {
    segment->state = THREAD_STATE_ERROR;
    return -1;
  }

  // 打开临时文件 - 支持断点续传
  FILE* temp_file;
  if (segment->downloaded_bytes > 0) {
    // 断点续传模式，追加写入
    temp_file = fopen(thread_params->temp_filename, "ab");
  }
  else {
    // 新下载，覆盖写入
    temp_file = fopen(thread_params->temp_filename, "wb");
  }

  if (!temp_file) {
    snprintf(segment->error_message, sizeof(segment->error_message),
      "无法创建临时文件: %s", strerror(errno));
    segment->state = THREAD_STATE_ERROR;
    return -1;
  }

  int result = -1;

  if (url_info.protocol_type == PROTOCOL_HTTPS) {
#ifdef WITH_OPENSSL
    result = download_https_segment(&url_info, thread_params, temp_file);
#else
    snprintf(segment->error_message, sizeof(segment->error_message), "HTTPS支持未编译");
    segment->state = THREAD_STATE_ERROR;
#endif
  }
  else {
    result = download_http_segment(&url_info, thread_params, temp_file);
  }

  fclose(temp_file);

  if (result == 0) {
    segment->state = THREAD_STATE_COMPLETED;
  }
  else {
    segment->state = THREAD_STATE_ERROR;
    // 只在非重试模式下删除临时文件（保留断点续传文件）
    if (segment->downloaded_bytes == 0) {
      unlink(thread_params->temp_filename);
    }
  }

  return result;
}

int calculate_file_segments(long long file_size, int thread_count, FileSegment* segments) {
  

  if (!segments || file_size <= 0 || thread_count <= 0) {
    return -1;
  }

  // 如果文件太小，减少线程数量
  long long min_total_size = MIN_SEGMENT_SIZE * thread_count;
  if (file_size < min_total_size) {
    thread_count = (int)(file_size / MIN_SEGMENT_SIZE);
    if (thread_count < 1) {
      thread_count = 1;
    }
    printf("文件较小，调整线程数为: %d\n", thread_count);
  }

  // 计算每个段的大小
  long long segment_size = file_size / thread_count;
  long long remaining_bytes = file_size % thread_count;

  printf("%s文件分段策略:%s\n", BOLD, RESET);
  printf("%s总大小: %s%lld 字节（%lld MB）%s\n", BOLD, BLUE, file_size, file_size / (1024 * 1024), RESET);
  printf("%s线程数: %s%d%s\n", BOLD, BLUE, thread_count, RESET);
  printf("%s基础段大小: %s%lld 字节（%lld MB）%s\n", BOLD, BLUE, segment_size, segment_size / (1024 * 1024), RESET);

  // 分配文件段
  long long current_pos = 0;
  for (int i = 0; i < thread_count; i++) {
    segments[i].thread_id = i;
    segments[i].start_byte = current_pos;

    // 最后一个段包含所有剩余字节
    if (i == thread_count - 1) {
      segments[i].end_byte = file_size - 1;
    }
    else {
      segments[i].end_byte = current_pos + segment_size - 1;
      // 将剩余字节分配给前面的段
      if (remaining_bytes > 0) {
        segments[i].end_byte++;
        remaining_bytes--;
      }
    }

    segments[i].downloaded_bytes = 0;
    segments[i].state = THREAD_STATE_IDLE;
    segments[i].error_message[0] = '\0';

    long long actual_size = segments[i].end_byte - segments[i].start_byte + 1;
    printf("%s段 %d:%s %s%lld-%lld (%s)%s\n", BOLD, i, RESET, BLUE, segments[i].start_byte, segments[i].end_byte, format_file_size(actual_size), RESET);

    current_pos = segments[i].end_byte + 1;
  }

  return thread_count;
}

int download_file_fallback_single_thread(MultiThreadDownloader* downloader) {
  // printf("执行单线程下载...\n");

  // 构造完整输出路径
  char full_output_path[4096];
  if (downloader->download_dir && strlen(downloader->download_dir) > 0) {
    snprintf(full_output_path, sizeof(full_output_path), "%s/%s",
      downloader->download_dir, downloader->output_filename);
  }
  else {
    strcpy(full_output_path, downloader->output_filename);
  }

  // 使用现有的单线程下载函数
  return download_file_auto(downloader->url, downloader->output_filename,
    downloader->download_dir, 0, 1);
}

int download_segment_with_retry(ThreadDownloadParams* thread_params) {
  const int MAX_RETRIES = 5;
  const int RETRY_DELAY = 3; // 秒

  // 备份原始的段信息
  long long original_start_byte = thread_params->segment->start_byte;
  long long original_downloaded = thread_params->segment->downloaded_bytes;

  for (int retry = 0; retry < MAX_RETRIES; retry++) {
    // 检查是否有已下载的部分文件
    if (retry > 0) {
      // 检查临时文件是否存在并获取已下载大小
      FILE* temp_file = fopen(thread_params->temp_filename, "rb");
      if (temp_file) {
        fseek(temp_file, 0, SEEK_END);
        long existing_size = ftell(temp_file);
        fclose(temp_file);

        if (existing_size > 0) {
          // 更新段的起始位置为已下载的部分之后
          thread_params->segment->downloaded_bytes = existing_size;
          thread_params->segment->start_byte = original_start_byte + existing_size;

          printf("线程 %d: 断点续传从 %lld 字节开始 (已下载: %ld)\n",
            thread_params->thread_id, thread_params->segment->start_byte, existing_size);
        }
        else {
          // 重置到原始状态
          thread_params->segment->start_byte = original_start_byte;
          thread_params->segment->downloaded_bytes = 0;
        }
      }
      else {
        // 文件不存在，重置到原始状态
        thread_params->segment->start_byte = original_start_byte;
        thread_params->segment->downloaded_bytes = 0;
      }

      printf("线程 %d: 第 %d 次重试...\n", thread_params->thread_id, retry + 1);
      sleep(RETRY_DELAY);
    }

    int result = download_segment(thread_params);
    if (result == 0) {
      return 0; // 成功
    }

    // 如果不是最后一次重试，继续尝试
    if (retry < MAX_RETRIES - 1) {
      printf("线程 %d: 下载失败，准备重试...\n", thread_params->thread_id);
      // 重置线程状态
      thread_params->segment->state = THREAD_STATE_IDLE;
    }
  }

  printf("线程 %d: 重试 %d 次后仍然失败\n", thread_params->thread_id, MAX_RETRIES);
  return -1;
}

int download_http_segment(const URLInfo* url_info, ThreadDownloadParams* thread_params, FILE* temp_file) {
  FileSegment* segment = thread_params->segment;

  // 建立TCP连接
  char ip_str[INET_ADDRSTRLEN];
  if (url_info->host_type == DOMAIN) {
    if (resolve_hostname(url_info->host, ip_str, sizeof(ip_str)) != 0) {
      snprintf(segment->error_message, sizeof(segment->error_message), "域名解析失败");
      return -1;
    }
  }
  else {
    strcpy(ip_str, url_info->host);
  }

  int sockfd = create_tcp_connection(ip_str, url_info->port);
  if (sockfd < 0) {
    snprintf(segment->error_message, sizeof(segment->error_message), "TCP连接失败");
    return -1;
  }

  // 构建Range请求，支持断点续传
  char request[REQUEST_BUFFER];
  int request_len = snprintf(request, sizeof(request),
    "GET %s HTTP/1.1\r\n"
    "Host: %s\r\n"
    "User-Agent: Mozilla/5.0 (X11; Linux x86_64) AppleWebKit/537.36\r\n"
    "Accept: */*\r\n"
    "Range: bytes=%lld-%lld\r\n"
    "Connection: close\r\n"
    "\r\n",
    url_info->path, url_info->host, segment->start_byte, segment->end_byte);

  if (request_len >= REQUEST_BUFFER) {
    close(sockfd);
    snprintf(segment->error_message, sizeof(segment->error_message), "请求构建失败");
    return -1;
  }

  // 发送请求
  if (send(sockfd, request, request_len, 0) != request_len) {
    close(sockfd);
    snprintf(segment->error_message, sizeof(segment->error_message), "请求发送失败");
    return -1;
  }

  segment->state = THREAD_STATE_DOWNLOADING;

  // 解析响应头
  HttpResponseInfo response_info = { 0 };
  HttpReadBuffer read_buffer = { 0 };
  read_buffer.sockfd = sockfd;

  if (parse_http_response_headers(sockfd, &response_info, &read_buffer) != 0) {
    close(sockfd);
    snprintf(segment->error_message, sizeof(segment->error_message), "响应解析失败");
    return -1;
  }

  // 检查状态码
  if (response_info.status_code != 206 && response_info.status_code != 200) {
    close(sockfd);
    snprintf(segment->error_message, sizeof(segment->error_message),
      "HTTP错误: %d", response_info.status_code);
    return -1;
  }

  // 下载内容
  const size_t BUFFER_SIZE = 16384;
  char buffer[BUFFER_SIZE];
  long long expected_bytes = segment->end_byte - segment->start_byte + 1;
  long long current_downloaded = segment->downloaded_bytes;

  // 首先处理缓冲区中的剩余数据
  if (read_buffer.parse_position < read_buffer.data_length) {
    size_t remaining_data = read_buffer.data_length - read_buffer.parse_position;
    size_t bytes_to_write = remaining_data;

    if (current_downloaded + bytes_to_write > expected_bytes) {
      bytes_to_write = expected_bytes - current_downloaded;
    }

    if (fwrite(read_buffer.buffer + read_buffer.parse_position, 1, bytes_to_write, temp_file) != bytes_to_write) {
      close(sockfd);
      snprintf(segment->error_message, sizeof(segment->error_message), "文件写入失败");
      return -1;
    }

    current_downloaded += bytes_to_write;

    // 更新全局进度
    pthread_mutex_lock(thread_params->progress_mutex);
    segment->downloaded_bytes = current_downloaded;
    pthread_mutex_unlock(thread_params->progress_mutex);
  }

  // 继续下载剩余数据
  while (current_downloaded < expected_bytes && !thread_params->should_stop) {
    long long remaining = expected_bytes - current_downloaded;
    size_t bytes_to_read = (remaining < BUFFER_SIZE) ? (size_t)remaining : BUFFER_SIZE;

    ssize_t bytes_received = recv(sockfd, buffer, bytes_to_read, 0);

    if (bytes_received <= 0) {
      if (bytes_received == 0) {
        // 连接关闭，检查是否下载完成
        if (current_downloaded == expected_bytes) {
          break; // 正常完成
        }
      }
      close(sockfd);
      snprintf(segment->error_message, sizeof(segment->error_message),
        "网络接收失败 (已下载: %lld/%lld)", current_downloaded, expected_bytes);
      return -1;
    }

    if (fwrite(buffer, 1, bytes_received, temp_file) != (size_t)bytes_received) {
      close(sockfd);
      snprintf(segment->error_message, sizeof(segment->error_message), "文件写入失败");
      return -1;
    }

    current_downloaded += bytes_received;

    // 更新进度（使用互斥锁保护）
    pthread_mutex_lock(thread_params->progress_mutex);
    segment->downloaded_bytes = current_downloaded;
    pthread_mutex_unlock(thread_params->progress_mutex);

    // 计算下载速度
    time_t current_time = time(NULL);
    time_t elapsed = current_time - thread_params->start_time;
    if (elapsed > 0) {
      thread_params->download_speed = (double)current_downloaded / elapsed;
    }

    // 强制刷新文件缓冲区
    fflush(temp_file);
  }

  close(sockfd);

  // 检查下载是否完成
  if (current_downloaded != expected_bytes) {
    snprintf(segment->error_message, sizeof(segment->error_message),
      "下载不完整: %lld/%lld", current_downloaded, expected_bytes);
    return -1;
  }

  return 0;
}

int download_https_segment(const URLInfo* url_info, ThreadDownloadParams* thread_params, FILE* temp_file) {
  FileSegment* segment = thread_params->segment;

  // 初始化 OpenSSL
  if (init_openssl() != 0) {
    snprintf(segment->error_message, sizeof(segment->error_message), "OpenSSL初始化失败");
    return -1;
  }

  // 建立 HTTPS 连接
  HttpsConnection* https_connection = create_https_connection(url_info->host, url_info->port);
  if (!https_connection) {
    cleanup_openssl();
    snprintf(segment->error_message, sizeof(segment->error_message), "HTTPS连接失败");
    return -1;
  }

  // 构建 Range 请求 - 支持断点续传
  char request[REQUEST_BUFFER];
  int request_len = snprintf(request, sizeof(request),
    "GET %s HTTP/1.1\r\n"
    "Host: %s\r\n"
    "User-Agent: Mozilla/5.0 (X11; Linux x86_64) AppleWebKit/537.36\r\n"
    "Accept: */*\r\n"
    "Range: bytes=%lld-%lld\r\n"
    "Connection: close\r\n"
    "\r\n",
    url_info->path, url_info->host, segment->start_byte, segment->end_byte);

  if (request_len >= REQUEST_BUFFER) {
    close_https_connection(https_connection);
    cleanup_openssl();
    snprintf(segment->error_message, sizeof(segment->error_message), "请求构建失败");
    return -1;
  }

  // 发送请求
  if (ssl_send_data(https_connection, request, request_len) != 0) {
    close_https_connection(https_connection);
    cleanup_openssl();
    snprintf(segment->error_message, sizeof(segment->error_message), "HTTPS请求发送失败");
    return -1;
  }

  segment->state = THREAD_STATE_DOWNLOADING;

  // 解析响应头
  HttpResponseInfo response_info = { 0 };
  HttpReadBuffer read_buffer = { 0 };

  if (parse_https_response_headers(https_connection, &response_info, &read_buffer) != 0) {
    close_https_connection(https_connection);
    cleanup_openssl();
    snprintf(segment->error_message, sizeof(segment->error_message), "HTTPS响应解析失败");
    return -1;
  }

  // 检查状态码
  if (response_info.status_code != 206 && response_info.status_code != 200) {
    close_https_connection(https_connection);
    cleanup_openssl();
    snprintf(segment->error_message, sizeof(segment->error_message),
      "HTTPS错误: %d", response_info.status_code);
    return -1;
  }

  // 下载内容
  const size_t BUFFER_SIZE = 16384;
  char buffer[BUFFER_SIZE];
  long long expected_bytes = segment->end_byte - segment->start_byte + 1;
  long long current_downloaded = segment->downloaded_bytes;

  // 首先处理缓冲区中的剩余数据
  if (read_buffer.parse_position < read_buffer.data_length) {
    size_t remaining_data = read_buffer.data_length - read_buffer.parse_position;
    size_t bytes_to_write = remaining_data;

    if (current_downloaded + bytes_to_write > expected_bytes) {
      bytes_to_write = expected_bytes - current_downloaded;
    }

    if (fwrite(read_buffer.buffer + read_buffer.parse_position, 1, bytes_to_write, temp_file) != bytes_to_write) {
      close_https_connection(https_connection);
      cleanup_openssl();
      snprintf(segment->error_message, sizeof(segment->error_message), "文件写入失败");
      return -1;
    }

    current_downloaded += bytes_to_write;

    // 更新全局进度
    pthread_mutex_lock(thread_params->progress_mutex);
    segment->downloaded_bytes = current_downloaded;
    pthread_mutex_unlock(thread_params->progress_mutex);
  }

  // 继续下载剩余数据
  while (current_downloaded < expected_bytes && !thread_params->should_stop) {
    long long remaining = expected_bytes - current_downloaded;
    size_t bytes_to_read = (remaining < BUFFER_SIZE) ? (size_t)remaining : BUFFER_SIZE;

    ssize_t bytes_received = ssl_recv_data(https_connection, buffer, bytes_to_read);

    if (bytes_received <= 0) {
      if (bytes_received == 0) {
        // 连接关闭，检查是否下载完成
        if (current_downloaded == expected_bytes) {
          break; // 正常完成
        }
      }
      close_https_connection(https_connection);
      cleanup_openssl();
      snprintf(segment->error_message, sizeof(segment->error_message),
        "SSL接收失败 (已下载: %lld/%lld)", current_downloaded, expected_bytes);
      return -1;
    }

    if (fwrite(buffer, 1, bytes_received, temp_file) != (size_t)bytes_received) {
      close_https_connection(https_connection);
      cleanup_openssl();
      snprintf(segment->error_message, sizeof(segment->error_message), "文件写入失败");
      return -1;
    }

    current_downloaded += bytes_received;

    // 更新进度（使用互斥锁保护）
    pthread_mutex_lock(thread_params->progress_mutex);
    segment->downloaded_bytes = current_downloaded;
    pthread_mutex_unlock(thread_params->progress_mutex);

    // 计算下载速度
    time_t current_time = time(NULL);
    time_t elapsed = current_time - thread_params->start_time;
    if (elapsed > 0) {
      thread_params->download_speed = (double)current_downloaded / elapsed;
    }

    // 强制刷新文件缓冲区
    fflush(temp_file);
  }

  close_https_connection(https_connection);
  cleanup_openssl();

  // 检查下载是否完成
  if (current_downloaded != expected_bytes) {
    snprintf(segment->error_message, sizeof(segment->error_message),
      "下载不完整: %lld/%lld", current_downloaded, expected_bytes);
    return -1;
  }

  return 0;
}