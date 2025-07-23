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
int multithread_download(MultiThreadDownloader* downloader);

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

/**
 * 计算文件分段
 * @param file_size 文件总大小
 * @param thread_count 线程数量
 * @param segments 输出的分段数组
 * @return 实际使用的线程数量
 */
int calculate_file_segments(long long file_size, int thread_count, FileSegment* segments);

/**
 * 初始化多线程下载
 * @param downloader 下载器指针
 * @return 成功返回1(多线程)，退化返回0(单线程)，失败返回-1
 */
int initialize_multithread_download(MultiThreadDownloader* downloader);

/**
 * 单线程段下载函数
 * @param thread_params 线程参数
 * @return 成功返回0，失败返回-1
 */
int download_segment(ThreadDownloadParams* thread_params);

/**
 * HTTP 段下载
 * @param url_info URL信息
 * @param thread_params 线程参数
 * @param temp_file 临时文件指针
 * @return 成功返回0，失败返回-1
 */
int download_http_segment(const URLInfo* url_info, ThreadDownloadParams* thread_params, FILE* temp_file);

#ifdef WITH_OPENSSL
/**
 * HTTPS 段下载
 * @param url_info URL信息
 * @param thread_params 线程参数
 * @param temp_file 临时文件指针
 * @return 成功返回0，失败返回-1
 */
int download_https_segment(const URLInfo* url_info, ThreadDownloadParams* thread_params, FILE* temp_file);
#endif

/**
 * 构建带 Range 的 HTTP 请求
 * @param url_info URL信息
 * @param segment 文件段信息
 * @param buffer 输出缓冲区
 * @param buffer_size 缓冲区大小
 * @return 成功返回请求长度，失败返回-1
 */
int build_range_request(const URLInfo* url_info, FileSegment* segment, char* buffer, size_t buffer_size);

/**
 * 线程下载工作函数
 * @param arg 线程参数
 * @return 线程结果
 */
void* thread_download_worker(void* arg);

/**
 * 进度显示工作线程
 * @param arg 下载器指针
 * @return NULL
 */
void* progress_display_worker(void* arg);

/**
 * 单线程下载回退函数
 * @param downloader 下载器指针
 * @return 下载结果
 */
int download_file_fallback_single_thread(MultiThreadDownloader* downloader);

/**
 * 合并临时文件到最终文件
 * @param downloader 下载器指针
 * @return 成功返回0，失败返回-1
 */
int merge_temp_files(MultiThreadDownloader* downloader);

/**
 * 清理临时文件
 * @param downloader 下载器指针
 */
void cleanup_temp_files(MultiThreadDownloader* downloader);

#endif