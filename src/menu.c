#include "../include/common.h"
#include "../include/menu.h"
#include "../include/download.h"

// CLI颜色定义
const char* BLUE = "\033[34m";
const char* CYAN = "\033[36m";
const char* YELLOW = "\033[33m";
const char* RESET = "\033[0m";
const char* BOLD = "\033[1m";
const char* RED = "\033[31m";
const char* GREEN = "\033[32m";
const char* CLEAR_LINE = "\r\033[K";

#define MAX_SIZE 2048
int cli_choice(int argc, char* argv[]) {
  if (strcmp(argv[1], "--version") == 0 || strcmp(argv[1], "-v") == 0) {
    printf("CHttpDownloader 版本: %s%s%s\n", VERSION, BLUE, RESET);
    return 0;
  }
  else if (strcmp(argv[1], "--settings") == 0 || strcmp(argv[1], "-s") == 0) {
    // open_settings_menu();
    return 0;
  }
  else if (strcmp(argv[1], "--download") == 0 || strcmp(argv[1], "-d") == 0) {
    if (argc < 3) {
      printf("%s错误: 请提供下载URL%s\n", RED, RESET);
      printf("用法：%s --download, -d <URL> <输出文件名> <完整下载目录>\n", argv[0]);
      return -1;
    }
    const char* url = argv[2];
    const char* output_filename = "Downloaded_File";
    const char* download_dir = NULL;
    if (argc == 3) {
      printf("%s警告: 未指定下载文件名及下载目录，使用默认名称：%s 和当前目录: %s %s\n", YELLOW, output_filename, ".", RESET);
    }
    if (argc == 4) {
      output_filename = argv[3];
      printf("%s警告: 未指定下载目录，使用当前工作目录: %s%s\n", YELLOW, ".", RESET);
    }
    if (argc == 5) {
      output_filename = argv[3];
      download_dir = argv[4];
    }
    if (argc >= 6) {
      printf("%s警告: 多余的参数被忽略%s\n", YELLOW, RESET);
    }
    int result = download_file_http(url, output_filename, download_dir, 0);
    if (result != DOWNLOAD_SUCCESS) {
      fprintf(stderr, "%s下载失败，错误代码: %d%s\n", RED, result, RESET);
      return result;
    }
    return 0;
  }
  else if (strcmp(argv[1], "--test") == 0 || strcmp(argv[1], "-t") == 0) {
    download_test();
    return 0;
  }
  else if (strcmp(argv[1], "--help") == 0 || strcmp(argv[1], "-h") == 0) {
    printf(
      "\n"
      "   ________  ____  __        ____                      __                __         \n"
      "  / ____/ / / / /_/ /_____  / __ \\____ _      ______  / /___  ____ _____/ /__  _____\n"
      " / /   / /_/ / __/ __/ __ \\/ / / / __ \\ | /| / / __ \\/ / __ \\/ __ \\/ __  / _ \\/ ___/\n"
      "/ /___/ __  / /_/ /_/ /_/ / /_/ / /_/ / |/ |/ / / / / / /_/ / /_/ / /_/ /  __/ /    \n"
      "\\____/_/ /_/\\__/\\__/ .___/_____/\\____/|__/|__/_/ /_/_/\\____/\\__,_/\\__,_/\\___/_/     \n"
      "                  /_/                                                               \n"
      "CHttpDownloader - 简易HTTP下载器\n"
    );
    printf("用法: %s [选项]\n", argv[0]);
    printf("选项:\n");
    printf("  --version, -v   显示版本信息\n");
    printf("  --help, -h      显示帮助信息\n");
    printf("  --settings, -s  打开设置菜单\n");
    printf("  --download, -d  下载文件\n");
    printf("  --test, -t      运行测试\n");
    printf("可能的错误代码如下：\n");
    printf("  %d: 下载成功\n", DOWNLOAD_SUCCESS);
    printf("  %d: URL解析错误\n", DOWNLOAD_ERROR_URL_PARSE);
    printf("  %d: DNS解析错误\n", DOWNLOAD_ERROR_DNS_RESOLVE);
    printf("  %d: 连接错误\n", DOWNLOAD_ERROR_CONNECTION);
    printf("  %d: HTTP请求错误\n", DOWNLOAD_ERROR_HTTP_REQUEST);
    printf("  %d: HTTP响应错误\n", DOWNLOAD_ERROR_HTTP_RESPONSE);
    printf("  %d: 文件打开错误\n", DOWNLOAD_ERROR_FILE_OPEN);
    printf("  %d: 文件写入错误\n", DOWNLOAD_ERROR_FILE_WRITE);
    printf("  %d: 网络错误\n", DOWNLOAD_ERROR_NETWORK);
    printf("  %d: 内存分配错误\n", DOWNLOAD_ERROR_MEMORY);
    return 0;
  }
}

int choice_download() {

  char* input_url = malloc(sizeof(char) * MAX_SIZE);
  char* output_filename = malloc(sizeof(char) * MAX_SIZE);
  char* download_dir = malloc(sizeof(char) * MAX_SIZE);

  if (!input_url || !output_filename || !download_dir) {
    fprintf(stderr, "%s错误: 内存分配失败%s\n", RED, RESET);
    free(input_url);
    free(output_filename);
    free(download_dir);
    return DOWNLOAD_ERROR_MEMORY;
  }

  input_url[0] = '\0';
  output_filename[0] = '\0';
  download_dir[0] = '\0';

  // 清除输入缓冲区
  getchar();
  // 获取URL输入
  while (strlen(input_url) == 0) {
    printf("%s%s请输入下载URL: %s", BOLD, BLUE, RESET);
    if (fgets(input_url, MAX_SIZE, stdin) == NULL) {
      fprintf(stderr, "%s错误: 读取输入失败%s\n", RED, RESET);
      free(input_url);
      free(output_filename);
      free(download_dir);
      return DOWNLOAD_ERROR_MEMORY;
    }
    size_t len = strlen(input_url);
    if (len > 0 && input_url[len - 1] == '\n') {
      input_url[len - 1] = '\0';
    }
    if (strlen(input_url) == 0) {
      printf("%s错误: URL不能为空，请重新输入%s\n", RED, RESET);
    }
  }

  // 获取输出文件名
  printf("%s%s请输入保存文件名（留空则使用默认名\"Downloaded_File\"）: %s", BOLD, BLUE, RESET);
  if (fgets(output_filename, MAX_SIZE, stdin) == NULL) {
    fprintf(stderr, "%s错误: 读取输入失败%s\n", RED, RESET);
    free(input_url);
    free(output_filename);
    free(download_dir);
    return DOWNLOAD_ERROR_MEMORY;
  }
  size_t len = strlen(output_filename);
  if (len > 0 && output_filename[len - 1] == '\n') {
    output_filename[len - 1] = '\0';
  }
  if (strlen(output_filename) == 0) {
    strcpy(output_filename, "Downloaded_File");
  }

  // 获取下载目录
  printf("%s%s请输入下载目录（留空则使用当前目录）: %s", BOLD, BLUE, RESET);
  if (fgets(download_dir, MAX_SIZE, stdin) == NULL) {
    fprintf(stderr, "%s错误: 读取输入失败%s\n", RED, RESET);
    free(input_url);
    free(output_filename);
    free(download_dir);
    return DOWNLOAD_ERROR_MEMORY;
  }
  len = strlen(download_dir);
  if (len > 0 && download_dir[len - 1] == '\n') {
    download_dir[len - 1] = '\0';
  }

  // 开始下载
  const char* dir_param = (strlen(download_dir) == 0) ? NULL : download_dir;
  int result = download_file_http(input_url, output_filename, dir_param, 0);

  free(input_url);
  free(output_filename);
  free(download_dir);

  if (result != DOWNLOAD_SUCCESS) {
    fprintf(stderr, "%s%s下载失败，错误代码: %d%s\n", RED, BOLD, result, RESET);
    return result;
  }
  return DOWNLOAD_SUCCESS;
}

int choice_settings() {

}