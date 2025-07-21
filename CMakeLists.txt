cmake_minimum_required(VERSION 3.10.0)
project(CHttpDownloader VERSION 0.1.0 LANGUAGES C)

set(CMAKE_C_STANDARD 11)

# 查找 OpenSSL 库
find_package(OpenSSL REQUIRED)
find_package(Threads REQUIRED)

# 如果找不到 OpenSSL，显示错误信息
if(NOT OpenSSL_FOUND)
    message(FATAL_ERROR "OpenSSL 库未找到。请安装 OpenSSL 开发包：\n"
                        "Ubuntu/Debian: sudo apt-get install libssl-dev\n"
                        "CentOS/RHEL: sudo yum install openssl-devel\n"
                        "Fedora: sudo dnf install openssl-devel")
endif()

include_directories(include)

set(SOURCE_FILES 
    src/download.c
    src/http.c
    src/parser.c
    src/utils.c
    src/progress.c
    src/net.c
    src/test.c
    src/menu.c
    src/config.c
    src/https.c
    src/multithread.c
    main.c
)

# 主程序
add_executable(CHttpDownloader ${SOURCE_FILES})

# 链接 OpenSSL 库
target_link_libraries(CHttpDownloader 
    OpenSSL::SSL 
    OpenSSL::Crypto
    Threads::Threads
)

# 添加编译定义以启用 HTTPS 支持
target_compile_definitions(CHttpDownloader PRIVATE WITH_OPENSSL=1)

# 显示找到的库信息
message(STATUS "OpenSSL 版本: ${OPENSSL_VERSION}")
message(STATUS "线程库: ${CMAKE_THREAD_LIBS_INIT}")