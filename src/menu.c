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
  else if (strcmp(argv[1], "--download") == 0 || strcmp(argv[1], "-d") == 0) {
    if (argc < 3) {
      printf("%s错误: 请提供下载URL%s\n", RED, RESET);
      printf("用法：%s --download, -d <URL> [输出文件名] [下载目录] [--multithread | -m] [下载线程数]\n", argv[0]);
      return -1;
    }
    const char* url = argv[2];
    const char* output_filename = NULL;
    const char* download_dir = NULL;
    int use_multithread = 0;
    int thread_count = 4;
    int next_is_thread_count = 0;

    // 解析参数
    for (int i = 3; i < argc; i++) {
      if (next_is_thread_count) {
        // 处理线程数参数
        thread_count = atoi(argv[i]);
        if (thread_count <= 0 || thread_count > MAX_THREADS) {
          printf("%s错误: 线程数必须在1到%d之间%s\n", RED, MAX_THREADS, RESET);
          return -1;
        }
        printf("%s设置线程数为: %d%s\n", BLUE, thread_count, RESET);
        next_is_thread_count = 0;
        continue;
      }

      if (strcmp(argv[i], "--multithread") == 0 || strcmp(argv[i], "-m") == 0) {
        use_multithread = 1;
        printf("%s✓ 启用多线程下载模式%s\n", GREEN, RESET);

        // 检查下一个参数是否是线程数
        if (i + 1 < argc && argv[i + 1][0] != '-' && isdigit(argv[i + 1][0])) {
          next_is_thread_count = 1;
        }
        else {
          printf("%s使用默认线程数: %d%s\n", BLUE, thread_count, RESET);
        }
      }
      else if (argv[i][0] != '-') {
        // 非选项参数，按顺序分配给 output_filename 和 download_dir
        if (output_filename == NULL) {
          output_filename = argv[i];
        }
        else if (download_dir == NULL) {
          download_dir = argv[i];
        }
        else {
          printf("%s警告: 忽略多余的参数 '%s'%s\n", YELLOW, argv[i], RESET);
        }
      }
      else {
        printf("%s警告: 未知选项 '%s' 被忽略%s\n", YELLOW, argv[i], RESET);
      }
    }

    // 设置默认值和给出相应警告
    if (output_filename == NULL) {
      output_filename = "Downloaded_File";
      if (download_dir == NULL) {
        printf("%s警告: 未指定文件名和下载目录，使用默认文件名 '%s' 和当前目录%s\n",
          YELLOW, output_filename, RESET);
      }
      else {
        printf("%s警告: 未指定文件名，使用默认文件名 '%s'%s\n",
          YELLOW, output_filename, RESET);
      }
    }
    else if (download_dir == NULL) {
      printf("%s警告: 未指定下载目录，使用当前目录%s\n", YELLOW, RESET);
    }

    // printf("%s下载URL: %s%s%s\n", BOLD, BLUE, url, RESET);
    // printf("%s输出文件名: %s%s%s\n", BOLD, BLUE,
    //   output_filename ? output_filename : "未指定", RESET);
    // printf("%s下载目录: %s%s%s\n", BOLD, BLUE,
    //   download_dir ? download_dir : "未指定", RESET);
    // printf("%s多线程下载: %s%s%s\n", BOLD, use_multithread ? GREEN : RED,
    //   use_multithread ? "启用" : "禁用", RESET);
    // printf("%s线程数: %s%d%s\n", BOLD, BLUE, thread_count, RESET);

    int result = download_file_auto(url, output_filename, download_dir, use_multithread, thread_count);
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
  // else if (strcmp(argv[1], "--config") == 0 || strcmp(argv[1], "-c") == 0) {
  //   return choice_config();
  // }
  else if (strcmp(argv[1], "--multithread") == 0 || strcmp(argv[1], "-m") == 0) {
    printf("%s错误: --multithread 选项需要与 --download 一起使用%s\n", RED, RESET);
    return -1;
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
    printf("  --download, -d <URL> [输出文件名] [下载目录] [--multithread|-m] [-线程数] 下载文件\n");
    printf("  --test, -t           运行测试\n");
    printf("  --config, -c       打开设置菜单\n");
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
  int thread_count = 4;
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
  if (strlen(multithread_choice) == 0) {
    strcpy(multithread_choice, "n"); // 默认不使用多线程
  }

  // 获取线程数
  if (strcasecmp(multithread_choice, "y") == 0 || strcasecmp(multithread_choice, "yes") == 0) {
    printf("%s%s请输入线程数 (默认: %d): %s", BOLD, CYAN, thread_count, RESET);
    char thread_input[10];
    if (fgets(thread_input, sizeof(thread_input), stdin) != NULL) {
      if (thread_input[0] != '\n') {
        int input_threads = atoi(thread_input);
        if (input_threads > 0 && input_threads <= MAX_THREADS) {
          thread_count = input_threads;
        }
        else {
          printf("%s警告: 无效的线程数，使用默认值 %d%s\n", YELLOW, thread_count, RESET);
        }
      }
    }
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
  int result = download_file_auto(input_url, output_filename, dir_param, use_multithread, thread_count);

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

// int choice_config() {
//   int config_choice = 0;
//   printf("请输入配置选项:(1.设定默认下载设置 2.初始化默认下载设置)\n");
//   scanf("%d", &config_choice);
//   if (config_choice == 1) {
//     Config config;
//     if (load_config(&config) != 0) {
//       fprintf(stderr, "%s错误: 无法加载配置文件%s\n", RED, RESET);
//       return -1;
//     }

//     printf("%s请输入默认线程数 (当前: %d): %s", BOLD, config.default_threads, RESET);
//     int threads;
//     scanf("%d", &threads);
//     if (threads <= 0 || threads > MAX_THREADS) {
//       printf("%s错误: 线程数必须在1到%d之间%s\n", RED, MAX_THREADS, RESET);
//       return -1;
//     }
//     config.default_threads = threads;

//     printf("%s请选择默认协议 (1: HTTP, 2: HTTPS): %s", BOLD, RESET);
//     int protocol_choice;
//     scanf("%d", &protocol_choice);
//     if (protocol_choice == 1) {
//       config.default_protocol = PROTOCOL_HTTP;
//     }
//     else if (protocol_choice == 2) {
//       config.default_protocol = PROTOCOL_HTTPS;
//     }
//     else {
//       printf("%s错误: 无效的协议选择%s\n", RED, RESET);
//       return -1;
//     }

//     printf("%s是否默认启用多线程下载 (1: 是, 0: 否) (当前: %d): %s", BOLD, config.enable_multithread, RESET);
//     int enable_multithread_choice;
//     scanf("%d", &enable_multithread_choice);
//     if (enable_multithread_choice == 1) {
//       config.enable_multithread = 1;
//     }
//     else if (enable_multithread_choice == 0) {
//       config.enable_multithread = 0;
//     }
//     else {
//       printf("%s错误: 无效的选择%s\n", RED, RESET);
//       return -1;
//     }

//     getchar();

//     printf("%s请输入默认下载目录 (当前: %s): %s", BOLD, config.default_download_dir, RESET);
//     char download_dir[PATH_MAX];
//     scanf("%s", download_dir);
//     if (strlen(download_dir) == 0 || strlen(download_dir) >= PATH_MAX)
//     {
//       printf("%s错误: 下载目录不能为空或过长%s\n", RED, RESET);
//       return -1;
//     }
//     strncpy(config.default_download_dir, download_dir, PATH_MAX);
//     config.default_download_dir[PATH_MAX - 1] = '\0'; // 确保字符串以'\0'结尾



//     printf("%s请输入默认下载文件名 (当前: %s): %s", BOLD, config.default_download_filename, RESET);
//     char download_filename[PATH_MAX];
//     scanf("%s", download_filename);
//     if (strlen(download_filename) == 0 || strlen(download_filename) >= PATH_MAX) {
//       printf("%s错误: 下载文件名不能为空或过长%s\n", RED, RESET);
//       return -1;
//     }
//     strncpy(config.default_download_filename, download_filename, PATH_MAX);
//     config.default_download_filename[PATH_MAX - 1] = '\0';

//     // 保存配置到文件
//     if (set_config(&config) != 0) {
//       fprintf(stderr, "%s错误: 无法保存配置%s\n", RED, RESET);
//       return -1;
//     }
//     printf("%s配置已保存到文件: CHttpDownloader_config.cfg%s\n", GREEN, RESET);
//     printf("%s默认设置已更新成功%s\n", GREEN, RESET);
//     printf("请重启程序以应用新的配置。\n");
//     return 0;

//   }
//   else if (config_choice == 2) {
//     Config config;
//     if (init_config(&config) != 0) {
//       fprintf(stderr, "%s错误: 无法初始化配置%s\n", RED, RESET);
//       return -1;
//     }
//     printf("%s默认设置已初始化成功%s\n", GREEN, RESET);
//     printf("请重启程序以应用新的配置。\n");
//     return 0;
//   }
//   else {
//     printf("%s无效的选项%s\n", RED, RESET);
//     return -1;
//   }
// }

int download_file_auto(const char* url, const char* output_filename, const char* download_dir, int use_multithread, int thread_count) {
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

    // 创建多线程下载器
    MultiThreadDownloader* downloader = create_multithread_downloader(
      url, output_filename, download_dir, thread_count);

    if (downloader) {
      // 开始多线程下载
      int multithread_result = multithread_download(downloader);

      // 清理资源
      destroy_multithread_downloader(downloader);

      if (multithread_result == 0) {
        printf("\n%s-------------------------下载已结束--------------------------%s\n\n", BOLD, RESET);
        return DOWNLOAD_SUCCESS;
      }
      else {
        printf("%s警告：多线程下载失败，将回退到单线程下载%s\n", YELLOW, RESET);
      }
    }
    else {
      printf("%s警告：多线程下载器创建失败，将使用单线程下载%s\n", YELLOW, RESET);
    }
  }

  // 单线程下载
  printf("%s%s=== 使用单线程下载 ===%s\n", GREEN, BOLD, RESET);
  printf("%s下载URL: %s%s%s\n", BOLD, BLUE, url, RESET);

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