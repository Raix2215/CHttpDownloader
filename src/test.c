#include "../include/common.h"
#include "../include/parser.h"
#include "../include/download.h"

void url_parse_test() {
  const char* test_urls[] = {
    "http://example.com/path/to/resource",
    "https://sub.example.com:8080/index.html?user=123&lang=en",
    "http://192.168.1.1:80",
    "https://example.com",
    "example.com/path?query=1", // 无协议
    "http://user:pass@example.com", // 包含用户信息（会被忽略）
    "https://example.com:443/api/data?id=123",
    "josdjfisdijiknwikrnikenikvjikrghejortrvwntybevrnvginretingvegivuynetriugvujeirugjmvkurcg4oirtgnertgoihmjfjhgiu.com",
    NULL
  };
  for (int i = 0; test_urls[i]; i++) {
    printf("Parsing: %s\n", test_urls[i]);
    URLInfo* info = malloc(sizeof(URLInfo));
    memset(info, 0, sizeof(info));
    int res = parse_url(test_urls[i], info);
    if (!res) {
      printf("Protocol: %d\n", info->protocol_type);
      printf("Host: %s\n", info->host);
      printf("Port: %d\n", info->port);
      printf("Path: %s\n", info->path);
      printf("Query: %s\n", info->query ? info->query : "(null)");
      printf("HostType: %d\n", info->host_type);
      printf("------------------------------------\n");
    }
    else {
      printf("Failed to parse URL\n\n");
    }
    free(info);
  }
}


void download_test() {
  const char* test_urls[] = {
    // "http://example.com/path/to/resource",
    "http://192.168.2.135/",
    "http://baidu.com/index.html",
    "http://www.scu.edu.cn/",
    "http://www.google.com/index.html",
    "http://hkg.download.datapacket.com/100mb.bin",
    // "http://202.115.47.141/login",
    // "192.168.10.1:8080",
    // "josdjfisdijiknwikrnikenikvjikrghejortrvwntybevrnvginretingvegivuynetriugvujeirugjmvkurcg4oirtgnertgoihmjfjhgiu.com",
    // "192.168.255.123",
    NULL
  };
  for (int i = 0; test_urls[i]; i++) {
    // printf("Parsing: %s\n", test_urls[i]);
    URLInfo* info = malloc(sizeof(URLInfo));
    memset(info, 0, sizeof(URLInfo));
    int res = parse_url(test_urls[i], info);
    char filename[50];
    sprintf(filename, "TEST_DOWNLOAD_FILE_OUTPUT%d", i);
    download_file_http(test_urls[i], filename, NULL, 0);
    printf("----------------------------下载完成----------------------------\n");
    free(info);
  }
}