#include "./include/common.h"
#include "./include/common.h"
#include "./include/download.h"
#include "./include/parser.h"
#include "./include/net.h"
#include "./include/http.h"
#include "./include/progress.h"
#include "./include/utils.h"
#include "./include/settings.h"
#include "./include/menu.h"
#include "./include/test.h"


int main(int argc, char* argv[]) {
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
			printf("\n");
			printf("请选择操作:\n");
			printf("1. 下载文件\n");
			// printf("2. 设置\n");
			printf("2. 退出\n");
			printf("%s%s请输入选项 (1-2): %s", BOLD, BLUE, RESET);

			int choice;
			int result = 0;
			scanf("%d", &choice);

			switch (choice) {
			case 1:
				result = choice_download();
				break;
				// case 2:
				// 	choice_settings();
				// 	break;
			case 2:
				printf("%s已退出程序。\n%s", GREEN, RESET);
				return 0;
			default:
				printf("%s无效选项，请重新输入。\n%s", YELLOW, RESET);
			}
		}
	}


}
