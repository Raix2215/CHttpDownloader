#include "./common.h"
#ifndef UTILS_H
#define UTILS_H

/**
 * 格式化文件大小为人类可读格式
 * @param bytes 字节数
 * @return 格式化后的字符串（静态缓冲区）
 */
const char* format_file_size(long long bytes);



#endif