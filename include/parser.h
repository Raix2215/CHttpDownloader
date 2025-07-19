#include "./common.h"
#ifndef PARSER_H
#define PARSER_H

/**
 * 域名解析函数
 * @param hostname 输入的域名字符串
 * @param ip_str 输出的IP地址字符串缓冲区
 * @param ip_str_len IP地址字符串缓冲区长度
 * @return 成功返回0，失败返回-1
 */
int resolve_hostname(const char* hostname, char* ip_str, size_t ip_str_len);


/**
 * 通用url解析函数
 * @param url 输入的下载url字符串
 * @param url_info  输出的url信息结构体
 * @return 成功返回0，失败返回-1
 */
int parse_url(const char* url, URLInfo* url_info);

/**
 * 判断是否为有效域名或IPv4地址
 * @param str host地址
 * @return 0 - 无效, 1 - IPv4地址, 2 - 域名
 */
int is_domain_or_ipv4(const char* str);

#endif