#ifndef PATH_MAX
#define PATH_MAX 4096
#endif


#ifndef COMMON_H
#define COMMON_H

#define _GNU_SOURCE
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <fcntl.h>
#include <errno.h>
#include <sys/time.h>
#include <time.h>
#include <ctype.h>
#include <sys/stat.h>
#include <limits.h>

#define READ_BUFFER_SIZE 8192
#define REQUEST_BUFFER 8192
typedef enum {
  DOWNLOAD_SUCCESS = 0,
  DOWNLOAD_ERROR_URL_PARSE = -1,
  DOWNLOAD_ERROR_DNS_RESOLVE = -2,
  DOWNLOAD_ERROR_CONNECTION = -3,
  DOWNLOAD_ERROR_HTTP_REQUEST = -4,
  DOWNLOAD_ERROR_HTTP_RESPONSE = -5,
  DOWNLOAD_ERROR_FILE_OPEN = -6,
  DOWNLOAD_ERROR_FILE_WRITE = -7,
  DOWNLOAD_ERROR_NETWORK = -8
} DownloadResult;

typedef enum {
  STATUS_ACTION_CONTINUE,
  STATUS_ACTION_REDIRECT,
  STATUS_ACTION_ERROR,   
  STATUS_ACTION_RETRY    
} StatusAction;

typedef enum {
  PROTOCOL_HTTP,
  PROTOCOL_HTTPS,
  PROTOCOL_UNKNOWN,
  PROTOCOL_FTP,
  PROTOCOL_SFTP,
  PROTOCOL_FILE
} ProtocolType;

typedef enum {
  UNALLOWED = 0,
  IPV4 = 1,
  DOMAIN = 2,
  IPV6 = 3
} HostType;

typedef struct {
  char scheme[8];   //协议
  char host[512];   //主机
  int port;         //端口
  char path[2048];  //路由
  char query[2048];
  ProtocolType protocol_type;
  HostType host_type;
}URLInfo;

// http解析状态机状态枚举
typedef enum {
  HTTP_PARSE_STATUS_LINE,    // 解析状态行阶段
  HTTP_PARSE_HEADERS,        // 解析头部字段阶段
  HTTP_PARSE_BODY,           // 解析消息主体阶段
  HTTP_PARSE_COMPLETE,       // 解析完成
  HTTP_PARSE_ERROR           // 解析错误
} HttpParseState;

// http请求响应信息结构体（解析结果）
typedef struct {
  int status_code;                    // HTTP状态码
  char status_message[128];           // 状态描述信息
  long long content_length;           // 内容长度，-1为未知
  char content_type[128];             // 内容类型
  char server[128];                   // 服务器信息
  int chunked_encoding;               // 是否使用分块传输编码
  int connection_close;               // 服务器是否要求关闭连接
  char location[2048];                 // 重定向位置（对3xx）
  char cookies[2048];                  // Cookie信息
} HttpResponseInfo;

// http请求响应信息buffer
typedef struct {
  char buffer[READ_BUFFER_SIZE];      // 读取缓冲区
  size_t data_length;                 // 缓冲区中有效数据长度
  size_t parse_position;              // 当前解析位置
  int sockfd;                         // Socket文件描述符
} HttpReadBuffer;

// 下载进度条
typedef struct {
  FILE* output_file;              // 输出文件指针
  long long total_size;           // 文件总大小
  long long downloaded_size;      // 已下载大小
  time_t start_time;              // 下载开始时间
  time_t last_update_time;        // 上次更新时间
  double download_speed;          // 下载速度（B/秒）
} DownloadProgress;

#endif