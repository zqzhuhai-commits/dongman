#ifndef HTTP_H
#define HTTP_H

#include <stddef.h>
#include <stdbool.h>

// HTTP GET 请求，返回动态分配的字符串(调用者需 free)
// 失败返回 NULL
char *http_get(const char *url);

// 下载文件到本地路径
bool http_download_file(const char *url, const char *save_path);

// URL 编码
void url_encode(const char *src, char *dst, size_t dst_size);

#endif // HTTP_H
