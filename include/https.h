#include "./common.h"

#ifndef HTTPS_H
#define HTTPS_H

struct ssl_ctx_st;
struct ssl_st;



#ifdef WITH_OPENSSL
#include <openssl/ssl.h>
#include <openssl/err.h>
#include <openssl/x509.h>
#include <openssl/x509v3.h>


/**
 * 初始化 OpenSSL 库
 * @return 成功返回0，失败返回-1
 */
int init_openssl();

/**
 * 清理 OpenSSL 库
 */
void cleanup_openssl();

/**
 * 创建 SSL 上下文
 * @return 成功返回 SSL_CTX 指针，失败返回 NULL
 */
SSL_CTX* create_ssl_context();

/**
 * 创建 HTTPS 连接
 * @param hostname 服务器主机名
 * @param port 端口号
 * @return 成功返回 HttpsConnection 指针，失败返回 NULL
 */
HttpsConnection* create_https_connection(const char* hostname, int port);

/**
 * 关闭 HTTPS 连接
 * @param conn HTTPS 连接指针
 */
void close_https_connection(HttpsConnection* conn);

/**
 * SSL 数据发送函数
 * @param conn HTTPS 连接指针
 * @param buffer 发送缓冲区
 * @param length 数据长度
 * @return 成功返回0，失败返回-1
 */
int ssl_send_data(HttpsConnection* conn, const char* buffer, size_t length);

/**
 * SSL 数据接收函数
 * @param conn HTTPS 连接指针
 * @param buffer 接收缓冲区
 * @param length 缓冲区大小
 * @return 实际接收的字节数，错误返回-1
 */
ssize_t ssl_recv_data(HttpsConnection* conn, void* buffer, size_t length);

/**
 * 验证服务器证书
 * @param conn HTTPS 连接指针
 * @param hostname 期望的主机名
 */
void verify_certificate(HttpsConnection* conn, const char* hostname);


/**
 * HTTPS 文件下载函数
 * @param url 下载URL
 * @param output_filename 输出文件名（不包含路径）
 * @param download_dir 下载目录，如果为NULL或空字符串则使用当前工作目录
 * @param redirect_count 重定向计数（用于防止无限重定向）
 * @return 下载结果状态码
 */
int download_file_https(const char* url, const char* output_filename, const char* download_dir, int redirect_count);

/**
 * HTTPS行读取函数
 * @param conn HTTPS 连接
 * @param read_buf 读取缓冲区
 * @param line_buffer 行缓冲区
 * @param line_buffer_size 行缓冲区大小
 * @return 成功返回行长度，失败返回-1，连接关闭返回0
 */
int ssl_read_line(HttpsConnection* conn, HttpReadBuffer* read_buf, char* line_buffer, size_t line_buffer_size);


/**
 * 解析 HTTPS 响应头
 * @param conn HTTPS 连接
 * @param response_info 响应信息结构体
 * @param remaining_buffer 剩余数据缓冲区
 * @return 成功返回0，失败返回-1
 */
int parse_https_response_headers(HttpsConnection* conn, HttpResponseInfo* response_info, HttpReadBuffer* remaining_buffer);

/**
 * 下载已知长度的 HTTPS 内容
 * @param conn HTTPS 连接
 * @param output_file 输出文件指针
 * @param content_length 内容总长度
 * @param progress 进度跟踪结构体
 * @param remaining_buffer 剩余数据缓冲区
 * @return 成功返回0，失败返回-1
 */
int download_https_content_with_length(HttpsConnection* conn, FILE* output_file, long long content_length, DownloadProgress* progress, HttpReadBuffer* remaining_buffer);

/**
 * 下载未知长度的 HTTPS 内容（直到连接关闭）
 * @param conn HTTPS 连接
 * @param output_file 输出文件指针
 * @param progress 进度跟踪结构体
 * @param remaining_buffer 剩余数据缓冲区
 * @return 成功返回0，失败返回-1
 */
int download_https_content_until_close(HttpsConnection* conn, FILE* output_file, DownloadProgress* progress, HttpReadBuffer* remaining_buffer);



#else
int init_openssl();
void cleanup_openssl();
HttpsConnection* create_https_connection(const char* hostname, int port);
void close_https_connection(HttpsConnection* conn);
int ssl_send_data(HttpsConnection* conn, const char* buffer, size_t length);
ssize_t ssl_recv_data(HttpsConnection* conn, void* buffer, size_t length);
int download_file_https(const char* url, const char* output_filename, const char* download_dir, int redirect_count);
// printf("编译缺少OpenSSL，请进行安装\n");
#endif

#endif