#include "../include/config.h"
#include "../include/common.h"

int set_config(Config* config) {
  // CLI颜色定义
  const char* BLUE = "\033[34m";
  const char* CYAN = "\033[36m";
  const char* YELLOW = "\033[33m";
  const char* RESET = "\033[0m";
  const char* BOLD = "\033[1m";
  const char* RED = "\033[31m";
  const char* GREEN = "\033[32m";
  const char* CLEAR_LINE = "\r\033[K";

  if (!config) {
    return -1;
  }
  printf("%s%s=== 设置配置 ===%s\n", BOLD, BLUE, RESET);
  printf("%s请输入默认线程数: %d%s\n", BOLD, config->default_threads, RESET);
  scanf("%d", &config->default_threads);
  if (config->default_threads <= 0 || config->default_threads > MAX_THREADS) {
    printf("%s错误: 线程数必须在1到%d之间%s\n", RED, MAX_THREADS, RESET);
    return -1;
  }

  printf("%s请输入默认协议 (1: HTTP, 2: HTTPS): %d%s\n", BOLD, config->default_protocol, RESET);
  int protocol_choice;
  scanf("%d", &protocol_choice);
  if (protocol_choice == 1) {
    config->default_protocol = PROTOCOL_HTTP;
  }
  else if (protocol_choice == 2) {
    config->default_protocol = PROTOCOL_HTTPS;
  }
  else {
    printf("%s错误: 无效的协议选择%s\n", RED, RESET);
    return -1;
  }

  printf("%s请输入默认完整下载目录: %s%s\n", BOLD, config->default_download_dir, RESET);
  scanf("%s", config->default_download_dir);
  if (strlen(config->default_download_dir) == 0 || strlen(config->default_download_dir) >= PATH_MAX) {
    printf("%s错误: 下载目录不能为空或过长%s\n", RED, RESET);
    return -1;
  }

  printf("%s是否默认启用多线程下载 (1: 是, 0: 否): %d%s\n", BOLD, config->enable_multithread, RESET);
  int enable_multithread_choice;
  scanf("%d", &enable_multithread_choice);
  if (enable_multithread_choice == 1) {
    config->enable_multithread = 1;
  }
  else if (enable_multithread_choice == 0) {
    config->enable_multithread = 0;
  }
  else {
    printf("%s错误: 无效的选择%s\n", RED, RESET);
    return -1;
  }


  // 打开配置文件
  int fd = open("CHttpDownloader_config.cfg", O_WRONLY);
  if (fd < 0) {
    return -1;
  }

  ssize_t bytes_written = write(fd, config, sizeof(Config));
  close(fd);

  if (bytes_written != sizeof(Config)) {
    return -1;
  }
  printf("%s配置已保存到文件:%sCHttpDownloader_config.cfg%s\n", GREEN, RESET, RESET);
  return 0;
}

int init_config(Config* config) {
  if (!config) {
    return -1;
  }

  config->default_threads = 4;
  config->default_protocol = PROTOCOL_HTTP;
  strncpy(config->default_download_dir, "", PATH_MAX);
  config->enable_multithread = 1;
  strncpy(config->default_download_filename, "downloaded_file", PATH_MAX);

  int fd = open("CHttpDownloader_config.cfg", O_CREAT | O_RDWR, 0644);
  if (fd < 0) {
    return -1;
  }

  // 写入默认配置
  write(fd, &config, sizeof(Config));
  close(fd);
  return 0; // 成功
}

int load_config(Config* config) {
  if (!config) {
    return -1;
  }

  int fd = open("CHttpDownloader_config.cfg", O_RDONLY);
  if (fd < 0) {
    init_config(config);
    fd = open("CHttpDownloader_config.cfg", O_RDONLY);
    if (fd < 0) {
      fprintf(stderr, "错误: 无法打开配置文件\n");
      return -1;
    }

    ssize_t bytes_read = read(fd, config, sizeof(Config));
    close(fd);

    if (bytes_read != sizeof(Config)) {
      return -1;
    }

    return 0;
  }
  ssize_t bytes_read = read(fd, config, sizeof(Config));
  close(fd);
  if (bytes_read != sizeof(Config)) {
    fprintf(stderr, "错误: 无法读取配置文件\n");
    return -1;
  }
  return 0; 
}