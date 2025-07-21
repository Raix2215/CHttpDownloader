#include "./include/common.h"
#include "./include/common.h"
#include "./include/download.h"
#include "./include/parser.h"
#include "./include/net.h"
#include "./include/http.h"
#include "./include/progress.h"
#include "./include/utils.h"
#include "./include/config.h"
#include "./include/menu.h"
#include "./include/test.h"
#include "./include/multithread.h"

char* menu[] = {
	"d - 下载文件",
	"q - 退出",
	"t - 测试",
	"h - 帮助",
	// "c - 配置",
	NULL,
};

int main(int argc, char* argv[]) {
	Config config;
	load_config(&config);

	// CLI颜色定义
	const char* BLUE = "\033[34m";
	const char* CYAN = "\033[36m";
	const char* YELLOW = "\033[33m";
	const char* RESET = "\033[0m";
	const char* BOLD = "\033[1m";
	const char* RED = "\033[31m";
	const char* GREEN = "\033[32m";

	if (argc > 1) {
		return cli_choice(argc, argv);
	}
	else if (argc == 1) {
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
		while (1) {
			char* greet = (char*)malloc(100 * sizeof(char));
			snprintf(greet, 100, "%s%s请输入选项 > %s", BOLD, BLUE, RESET);

			int choice = 0;
			int result = 0;
			choice = getchoice(greet, menu);

			switch (choice) {
			case 'd':
				result = choice_download();
				break;
			case 'q':
				printf("%s已退出程序。\n%s", GREEN, RESET);
				free(greet);
				return 0;
			// case 'c':
			// 	result = choice_config();
			// 	break;
			case 't':
				download_test();
				break;
			case 'h': {
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
				break;
			}
			default:
				printf("%s无效选项，请重新输入。\n%s", YELLOW, RESET);
			}

			free(greet);
			return result;
		}
	}
}