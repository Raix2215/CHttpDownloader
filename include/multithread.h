#include "./common.h"

#ifndef MULTITHREAD_H
#define MULTITHREAD_H

/**
 * 创建多线程下载器
 * @param url 下载URL
 * @param output_filename 输出文件名
 * @param download_dir 下载目录
 * @param thread_count 线程数量
 * @return 成功返回下载器指针，失败返回NULL
 */
MultiThreadDownloader* create_multithread_downloader(const char* url, const char* output_filename, const char* download_dir, int thread_count
);

/**
 * 检查服务器是否支持 Range 请求
 * @param url 下载URL
 * @param file_size 输出文件大小
 * @return 支持返回1，不支持返回0，错误返回-1
 */
int check_range_support(const char* url, long long* file_size);

/**
 * 开始多线程下载
 * @param downloader 下载器指针
 * @return 成功返回0，失败返回错误代码
 */
int start_multithread_download(MultiThreadDownloader* downloader);

/**
 * 停止多线程下载
 * @param downloader 下载器指针
 */
void stop_multithread_download(MultiThreadDownloader* downloader);

/**
 * 销毁多线程下载器
 * @param downloader 下载器指针
 */
void destroy_multithread_downloader(MultiThreadDownloader* downloader);

/**
 * 显示多线程下载进度
 * @param downloader 下载器指针
 */
void display_multithread_progress(MultiThreadDownloader* downloader);

/**
 * 发送 HEAD 请求检查服务器响应
 * @param url 下载URL
 * @param response_info 输出响应信息
 * @return 成功返回0，失败返回-1
 */
int send_head_request(const char* url, HttpResponseInfo* response_info);

/**
 * 发送测试 Range 请求来验证支持
 * @param url 下载URL
 * @return 支持返回1，不支持返回0
 */
int test_range_request(const char* url);

#endif