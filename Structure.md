# CHttpDownloader 项目结构总结

## 项目概述

**项目名称**: CHttpDownloader  
**版本**: 1.3  
**描述**: 基于C语言开发的轻量级HTTP/HTTPS文件下载器，支持多线程下载、断点续传、进度显示等功能

## 技术栈

- **编程语言**: C (C11标准)
- **构建系统**: CMake (最低版本3.10.0)
- **依赖库**: 
  - OpenSSL (HTTPS支持)
  - pthread (多线程支持)
- **平台支持**: Linux/Unix系统

## 目录结构

```
CHttpDownloader/
├── CMakeLists.txt             # CMake构建配置文件
├── main.c                     # 程序入口
│
├── include/                   # 头文件目录
│   ├── common.h               # 公共定义和数据结构
│   ├── config.h               # 配置管理（未使用）
│   ├── download.h             # 下载核心功能
│   ├── http.h                 # HTTP协议处理
│   ├── https.h                # HTTPS协议处理
│   ├── menu.h                 # 用户界面和菜单
│   ├── multithread.h          # 多线程下载
│   ├── net.h                  # TCP连接
│   ├── parser.h               # URL解析和域名解析
│   ├── progress.h             # 下载进度显示
│   ├── test.h                 # 测试功能
│   └── utils.h                # 工具函数
│
├── src/                       # 源代码目录
│   ├── config.c               # 配置管理实现（未使用）
│   ├── download.c             # 文件下载核心功能实现
│   ├── http.c                 # HTTP协议处理实现
│   ├── https.c                # HTTPS协议处理实现
│   ├── menu.c                 # 用户界面和菜单实现
│   ├── multithread.c          # 多线程下载实现
│   ├── net.c                  # TCP连接实现
│   ├── parser.c               # URL解析和域名解析实现
│   ├── progress.c             # 下载进度显示实现
│   ├── test.c                 # 测试功能实现
│   └── utils.c                # 工具函数实现
│
├── build/                     # 构建输出目录
│   ├── CHttpDownloader        # 可执行文件
│   ├── CMakeCache.txt         # CMake缓存
│   ├── Makefile               # 生成的Makefile
│   ├── compile_commands.json  # 编译命令数据库
│   └── CMakeFiles/            # CMake生成的文件
│
└── download/                  # 测试使用的下载目录
    └── [下载的文件]            
```

## 核心模块架构

### 1. 数据结构层 (common.h)
- **URLInfo**: URL解析结果结构体
- **HttpResponseInfo**: HTTP响应信息结构体
- **DownloadProgress**: 下载进度跟踪结构体
- **MultiThreadDownloader**: 多线程下载器管理结构体
- **ThreadDownloadParams**: 单线程参数结构体
- **FileSegment**: 文件分段信息结构体

### 2. 网络通信层
- **net.c/net.h**: TCP连接建立、数据发送
- **parser.c/parser.h**: URL解析、域名解析
- **http.c/http.h**: HTTP协议解析和请求构建
- **https.c/https.h**: HTTPS/SSL加密通信

### 3. 下载引擎层
- **download.c/download.h**: 单线程下载核心逻辑
- **multithread.c/multithread.h**: 多线程下载管理

### 4. 用户界面层
- **menu.c/menu.h**: 交互式菜单和命令行参数处理
- **progress.c/progress.h**: 进度条显示和下载统计

### 5. 工具支持层
- **utils.c/utils.h**: 文件大小格式化、时间处理等工具函数
- **config.c/config.h**: 配置文件管理
- **test.c/test.h**: 功能测试

## 主要功能特性

### 1. 协议支持
- **HTTP/1.1**: 完整的HTTP下载支持
- **HTTPS**: 基于OpenSSL的安全下载
- **重定向处理**: 自动处理3xx重定向响应

### 2. 多线程下载
- **Range请求支持检测**: 自动检测服务器是否支持断点续传
- **智能分段**: 根据文件大小和线程数自动计算下载分段
- **线程池管理**: 支持1-16个并发下载线程
- **单线程回退**: 当多线程失败时自动降级到单线程模式

### 3. 下载管理
- **进度显示**: 实时显示下载进度、速度、预计剩余时间
- **文件合并**: 多线程下载完成后自动合并临时文件
- **错误处理**: 完善的错误处理和恢复机制
- **超时控制**: 网络连接和数据传输超时保护

### 4. 用户界面
- **交互式菜单**: 友好的终端交互界面
- **命令行支持**: 支持直接命令行参数调用
- **彩色输出**: 支持终端颜色显示增强用户体验

## 编译和构建

### 依赖安装
```bash
# Ubuntu/Debian
sudo apt-get install libssl-dev cmake build-essential

# CentOS/RHEL
sudo yum install openssl-devel cmake gcc

# Fedora
sudo dnf install openssl-devel cmake gcc
```

### 构建步骤
```bash
mkdir build && cd build
cmake ..
make
```

### 可执行文件
生成的可执行文件为 `build/CHttpDownloader`

## 使用方式

### 1. 交互式模式
```bash
./CHttpDownloader
```

### 2. 命令行模式
```bash
# 基本下载
./CHttpDownloader -d http://example.com/file.zip

# 指定文件名和目录
./CHttpDownloader -d http://example.com/file.zip myfile.zip /download/path

# 启用多线程下载
./CHttpDownloader -d http://example.com/file.zip --multithread

# 指定线程数
./CHttpDownloader -d http://example.com/file.zip -m -4
```

## 错误代码

- **0**: 下载成功
- **-1**: URL解析错误
- **-2**: DNS解析失败
- **-3**: 网络连接失败
- **-4**: HTTP请求错误
- **-5**: HTTP响应错误
- **-6**: 文件操作错误
- **-7**: 文件写入错误
- **-8**: 网络传输错误
- **-9**: 内存分配错误

## 项目特色

1. **模块化设计**: 清晰的分层架构，便于维护和扩展
2. **健壮性**: 完善的错误处理和异常恢复机制
3. **高性能**: 多线程并发下载，充分利用网络带宽
4. **跨平台**: 基于标准C库和POSIX接口，易于移植
5. **用户友好**: 直观的进度显示和交互界面
6. **安全性**: 完整的HTTPS支持和证书验证

## 待优化项

1. 断点续传功能完善
2. 更多协议支持(FTP, SFTP)
3. 配置文件系统完善
4. Windows平台适配
5. GUI界面开发
6. 下载队列管理

## 代码统计

- **总文件数**: 24个C/H文件
- **总函数数**: 69个函数声明
- **代码行数**: 约5000+行C代码
- **模块数**: 11个功能模块
