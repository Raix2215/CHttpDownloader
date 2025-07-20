#include "./common.h"
#ifndef MENU_H
#define MENU_H

/**
 * 处理命令行选项
 * @param argc 参数个数
 * @param argv 参数数组
 * @return 0表示成功，非0表示错误代码
 */
int cli_choice(int argc, char* argv[]);

/**
 * 下载文件选择处理
 * @return 下载结果状态码
 */
int choice_download();

/**
 * 设置菜单选择处理
 */
int choice_settings();

#endif