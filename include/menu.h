#include "./common.h"
#ifndef MENU_H
#define MENU_H

/**
 * 获取用户选择的菜单选项
 * @param greet 菜单提示语
 * @param choices 可选项数组
 * @return 用户选择的字符
 */
int getchoice(char* greet, char* choices[]);

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

// /**
//  * 配置选择处理
//  * @return 配置结果状态码
//  */
// int choice_config();

/**
 * 自动选择协议进行文件下载（支持多线程）
 * @param url 下载URL
 * @param output_filename 输出文件名
 * @param download_dir 下载目录
 * @param use_multithread 是否使用多线程下载（1启用，0禁用）
 * @return 下载结果代码
 */
int download_file_auto(const char* url, const char* output_filename, const char* download_dir, int use_multithread, int thread_count);

#endif