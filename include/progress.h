#include "./common.h"
#ifndef PROGRESS_H
#define PROGRESS_H

/**
 * 更新并显示下载进度
 * @param progress 进度跟踪结构体
 */
void update_download_progress(DownloadProgress* progress);

/**
 * 显示确定大小的进度条
 * @param percentage 完成百分比
 * @param downloaded 已下载字节数
 * @param total 总字节数
 * @param speed 下载速度
 * @param eta 估计剩余时间
 */
void print_progress_bar(double percentage, long long downloaded, long long total, double speed, time_t eta);

/**
 * 显示不确定大小的进度条（动画效果）
 * @param downloaded 已下载字节数
 * @param speed 下载速度
 * @param elapsed 已用时间
 */
void print_indeterminate_progress(long long downloaded, double speed, time_t elapsed);

/**
 * 显示简单的文本进度（无颜色）
 * @param downloaded 已下载字节数
 * @param total 总字节数
 * @param speed 下载速度
 */
void print_simple_progress(long long downloaded, long long total, double speed);

/**
 * 格式化时间间隔
 * @param seconds 秒数
 * @return 格式化后的字符串（静态缓冲区）
 */
const char* format_time_duration(time_t seconds);

/**
 * 清除进度条行
 */
void clear_progress_line();

/**
 * 打印下载摘要
 * @param progress 进度结构体
 * @param success 是否成功
 */
void print_download_summary(DownloadProgress* progress, int success);

#endif