cmake_minimum_required(VERSION 3.10.0)
project(CHttpDownloader VERSION 0.1.0 LANGUAGES C)

set(CMAKE_C_STANDARD 11)

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
    src/settings.c

    main.c
)
add_executable(CHttpDownloader ${SOURCE_FILES})