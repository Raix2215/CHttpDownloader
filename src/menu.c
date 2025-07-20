#include "../include/common.h"
#include "../include/menu.h"
#include "../include/download.h"
#include "../include/https.h"
#include "../include/http.h"
#include "../include/parser.h"
#include "../include/multithread.h"
#include "../include/utils.h"
#include "../include/progress.h"

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


int getchoice(char* greet, char* choices[]) {
  int choosen = 0;
  int selected;

  do {
    char** option = choices;
    while (*option) {
      printf("%s \n", *option);
      option++;
    }
    printf("%s", greet);
    do {
      selected = getchar();
    } while (selected == '\n');

    option = choices;
    while (*option) {
      if (selected == *option[0]) {
        choosen = 1;
        break;
      }
      option++;
    }
    if (!choosen) {
      printf("%s无效的选项，请重新选择%s\n", RED, RESET);
    }
  } while (!choosen);

  return selected;
}

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
      printf("用法：%s --download, -d <URL> [输出文件名] [下载目录] [--multithread]\n", argv[0]);
      return -1;
    }
    const char* url = argv[2];
    const char* output_filename = "Downloaded_File";
    const char* download_dir = NULL;
    int use_multithread = 0;

    // 解析参数
    for (int i = 3; i < argc; i++) {
      if (strcmp(argv[i], "--multithread") == 0 || strcmp(argv[i], "-m") == 0) {
        use_multithread = 1;
        printf("%s✓ 启用多线程下载模式%s\n", GREEN, RESET);
      }
      else if (i == 3 && argv[i][0] != '-') {
        // 第一个非选项参数是输出文件名
        output_filename = argv[i];
      }
      else if (i == 4 && argv[i][0] != '-') {
        // 第二个非选项参数是下载目录
        download_dir = argv[i];
      }
    }

    if (argc == 3) {
      printf("%s警告: 未指定下载文件名及下载目录，使用默认名称：%s 和当前目录: %s %s\n",
        YELLOW, output_filename, ".", RESET);
    }
    if (argc == 4 && !use_multithread) {
      printf("%s警告: 未指定下载目录，使用当前工作目录: %s%s\n", YELLOW, ".", RESET);
    }
    if (argc >= 6) {
      printf("%s警告: 多余的参数被忽略%s\n", YELLOW, RESET);
    }

    int result = download_file_auto(url, output_filename, download_dir, use_multithread);
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
    printf("直接运行进入交互式菜单。\n");
    printf("命令行用法: %s [选项]\n", argv[0]);
    printf("选项:\n");
    printf("  --version, -v        显示版本信息\n");
    printf("  --help, -h           显示帮助信息\n");
    printf("  --download, -d <URL> [输出文件名] [下载目录] [--multithread|-m]  下载文件\n");
    printf("  --test, -t           运行测试\n");
    printf("  --multithread, -m    启用多线程下载（与 --download 配合使用）\n");
    printf("\n示例:\n");
    printf("  %s -d http://example.com/file.zip\n", argv[0]);
    printf("  %s -d http://example.com/file.zip myfile.zip\n", argv[0]);
    printf("  %s -d http://example.com/file.zip myfile.zip /tmp --multithread\n", argv[0]);
    printf("\n可能的错误代码如下：\n");
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
  char* multithread_choice = malloc(sizeof(char) * 10);

  if (!input_url || !output_filename || !download_dir || !multithread_choice) {
    fprintf(stderr, "%s错误: 内存分配失败%s\n", RED, RESET);
    free(input_url);
    free(output_filename);
    free(download_dir);
    free(multithread_choice);
    return DOWNLOAD_ERROR_MEMORY;
  }

  input_url[0] = '\0';
  output_filename[0] = '\0';
  download_dir[0] = '\0';
  multithread_choice[0] = '\0';

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
      free(multithread_choice);
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
    free(multithread_choice);
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
    free(multithread_choice);
    return DOWNLOAD_ERROR_MEMORY;
  }
  len = strlen(download_dir);
  if (len > 0 && download_dir[len - 1] == '\n') {
    download_dir[len - 1] = '\0';
  }

  // 询问是否使用多线程下载
  printf("%s%s是否使用多线程下载？(y/N): %s", BOLD, CYAN, RESET);
  if (fgets(multithread_choice, 10, stdin) == NULL) {
    fprintf(stderr, "%s错误: 读取输入失败%s\n", RED, RESET);
    free(input_url);
    free(output_filename);
    free(download_dir);
    free(multithread_choice);
    return DOWNLOAD_ERROR_MEMORY;
  }
  len = strlen(multithread_choice);
  if (len > 0 && multithread_choice[len - 1] == '\n') {
    multithread_choice[len - 1] = '\0';
  }

  // 判断是否使用多线程
  int use_multithread = 0;
  if (strcasecmp(multithread_choice, "y") == 0 || strcasecmp(multithread_choice, "yes") == 0) {
    use_multithread = 1;
    printf("%s✓ 将尝试使用多线程下载%s\n", GREEN, RESET);
  }
  else {
    printf("%s→ 将使用单线程下载%s\n", YELLOW, RESET);
  }

  // 开始下载
  const char* dir_param = (strlen(download_dir) == 0) ? NULL : download_dir;
  int result = download_file_auto(input_url, output_filename, dir_param, use_multithread);

  free(input_url);
  free(output_filename);
  free(download_dir);
  free(multithread_choice);

  if (result != DOWNLOAD_SUCCESS) {
    fprintf(stderr, "%s%s下载失败，错误代码: %d%s\n", RED, BOLD, result, RESET);
    return result;
  }
  return DOWNLOAD_SUCCESS;
}

int choice_settings() {

}

int download_file_auto(const char* url, const char* output_filename, const char* download_dir, int use_multithread) {
  if (!url || !output_filename) {
    fprintf(stderr, "%s错误: URL 或输出文件名不能为空%s\n", RED, RESET);
    return DOWNLOAD_ERROR_URL_PARSE;
  }

  // 确定协议类型
  URLInfo url_info = { 0 };
  if (parse_url(url, &url_info) != 0) {
    fprintf(stderr, "%s错误: 无法解析URL%s\n", RED, RESET);
    return DOWNLOAD_ERROR_URL_PARSE;
  }

  // 如果启用多线程下载，尝试使用多线程下载器
  if (use_multithread) {

    // 默认使用4个线程
    int thread_count = 4;

    // 创建多线程下载器
    MultiThreadDownloader* downloader = create_multithread_downloader(
      url, output_filename, download_dir, thread_count);

    if (downloader) {
      // 开始多线程下载
      int multithread_result = start_multithread_download(downloader);

      // 清理资源
      destroy_multithread_downloader(downloader);

      if (multithread_result == 0) {
        printf("%s✓ 多线程下载成功完成%s\n", GREEN, RESET);
        return DOWNLOAD_SUCCESS;
      }
      else {
        printf("%s警告：多线程下载失败，将回退到单线程下载%s\n", YELLOW, RESET);
        // 继续执行单线程下载逻辑
      }
    }
    else {
      printf("%s警告：多线程下载器创建失败，将使用单线程下载%s\n", YELLOW, RESET);
      // 继续执行单线程下载逻辑
    }
  }

  // 单线程下载逻辑（原有逻辑）
  printf("%s=== 使用单线程下载 ===%s\n", BOLD, RESET);

  // 根据协议类型选择下载方法
  switch (url_info.protocol_type) {
  case PROTOCOL_HTTP:
    return download_file_http(url, output_filename, download_dir, 0);

  case PROTOCOL_HTTPS:
#ifdef WITH_OPENSSL
    return download_file_https(url, output_filename, download_dir, 0);
#else
    fprintf(stderr, "%s错误: 程序编译时未包含 HTTPS 支持%s\n", RED, RESET);
    return DOWNLOAD_ERROR_NETWORK;
#endif
  default:
    fprintf(stderr, "%s错误: 不支持的协议类型%s\n", RED, RESET);
    return DOWNLOAD_ERROR_URL_PARSE;
  }
}