#include "./common.h"
#ifndef NET_H
#define NET_H

/**
 * 建立TCP连接
 * @param ip_str IP地址字符串
 * @param port 端口号
 * @return 成功返回socket文件描述符，失败返回-1
 */
int create_tcp_connection(const char* ip_str, int port);

/**
 * 发送完整数据的封装函数
 * @param sockfd socket文件描述符
 * @param buffer 要发送的数据buffer
 * @param length 数据长度
 * @return 成功返回0，失败返回-1
 */
int send_full_data(int sockfd, const char* buffer, size_t length);



#endif