#include "./common.h"

#ifndef HTTP_H
#define HTTP_H

/**
 * 从Socket读取一行数据
 * @param read_buf 读取缓冲区结构体
 * @param line_buffer 输出行缓冲区
 * @param line_buffer_size 行缓冲区大小
 * @return 成功返回行长度，0表示连接关闭，-1表示错误
 */
int read_line_from_socket(HttpReadBuffer* read_buf, char* line_buffer, size_t line_buffer_size);

/**
 * 解析HTTP状态行
 * @param status_line 状态行字符串
 * @param response_info 响应信息结构体
 * @return 成功返回0，失败返回-1
 */
int parse_status_line(const char* status_line, HttpResponseInfo* response_info);

//根据状态码确定处理策略
StatusAction determine_status_action(int status_code);

/**
 * 解析单个HTTP头部字段
 * @param header_line 头部行字符串
 * @param response_info 响应信息结构体
 * @return 成功返回0，失败返回-1
 */
int parse_header_field(const char* header_line, HttpResponseInfo* response_info);

// 关键头部字段处理
int process_header_field(const char* name, const char* value, HttpResponseInfo* response_info);

/**
 * 解析HTTP响应头部
 * @param sockfd Socket文件描述符
 * @param response_info 用于存储解析结果的结构体
 * @param remaining_buffer 用于返回剩余缓冲区数据的结构体指针
 * @return 成功返回0，失败返回-1
 */
int parse_http_response_headers(int sockfd, HttpResponseInfo* response_info, HttpReadBuffer* remaining_buffer);


/**
 * 构建HTTP GET请求
 * @param host 主机名
 * @param path 请求路径
 * @param buffer 输出缓冲区
 * @param buffer_size 缓冲区大小
 * @return 成功返回请求长度，失败返回-1
 */
int build_http_get_request(const char* host, const char* path, char* buffer, size_t buffer_size);

#endif