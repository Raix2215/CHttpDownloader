#include "./common.h"
#ifndef DOWNLOAD_H
#define DOWNLOAD_H

/**
 * 带超时检测的数据接收函数
 * @param sockfd Socket文件描述符
 * @param buffer 接收缓冲区
 * @param length 期望接收的字节数
 * @param timeout_ms 超时时间（毫秒）
 * @return 实际接收的字节数，-1表示错误，0表示连接关闭
 */
ssize_t recv_data_with_timeout(int sockfd, void* buffer, size_t length, int timeout_ms);

/**
 * 接收指定长度的完整数据
 * @param sockfd Socket文件描述符
 * @param buffer 接收缓冲区
 * @param total_length 需要接收的总字节数
 * @param timeout_ms 单次操作超时时间
 * @return 成功返回0，失败返回-1
 */
int recv_full_data(int sockfd, void* buffer, size_t total_length, int timeout_ms);


/**
 * 下载已知长度的HTTP内容
 * @param sockfd Socket文件描述符
 * @param output_file 输出文件指针
 * @param content_length 内容总长度
 * @param progress 进度跟踪结构体
 * @param remaining_buffer 用于返回剩余缓冲区数据的结构体指针
 * @return 成功返回0，失败返回-1
 */
int download_content_with_length(int sockfd, FILE* output_file, long long content_length, DownloadProgress* progress, HttpReadBuffer* remaining_buffer);

/**
 * 下载未知长度内容（连接关闭指示结束）
 * @param sockfd socket文件描述符
 * @param output_file 输出文件指针
 * @param progress 进度跟踪结构体
 * @param remaining_buffer 用于返回剩余缓冲区数据的结构体指针
 * @return 成功返回0，失败返回-1
 */
int download_content_until_close(int sockfd, FILE* output_file, DownloadProgress* progress, HttpReadBuffer* remaining_buffer);


/**
 * HTTP文件下载函数
 * @param url 下载URL
 * @param output_filename 输出文件名（不包含路径）
 * @param download_dir 下载目录，如果为NULL或空字符串则使用当前工作目录
 * @param redirect_count 重定向计数（内部使用）
 * @return 下载结果状态码
 */
int download_file_http(const char* url, const char* output_filename, const char* download_dir, int redirect_count);

// TODO
/**
 * HTTPS文件下载函数
 * @param url 下载URL
 * @param output_filename 输出文件名（不包含路径）
 * @param download_dir 下载目录，如果为NULL或空字符串则使用当前工作目录
 * @param redirect_count 重定向计数（内部使用）
 * @return 下载结果状态码
 */
int download_file_https(const char* url, const char* output_filename, const char* download_dir, int redirect_count);

#endif