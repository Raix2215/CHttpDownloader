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

/**
 * 设置菜单选择处理
 */
int choice_settings();

/**
 * 文件下载入口函数
 * @param url 下载URL
 * @param output_filename 输出文件名
 * @param download_dir 下载目录
 * @param redirect_count 重定向计数
 * @return 下载结果状态码
 */
int download_file_auto(const char* url, const char* output_filename, const char* download_dir, int redirect_count);


/**
 * 检查并转换特殊的下载URL
 * @param original_url 原始URL
 * @param converted_url 转换后的URL
 * @param url_size 缓冲区大小
 * @return 1-进行了转换，0-无需转换
 */
int convert_special_download_url(const char* original_url, char* converted_url, size_t url_size);

#endif