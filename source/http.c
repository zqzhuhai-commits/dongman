#include "http.h"
#include "config.h"

#include <curl/curl.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

// curl 写回调: 累积到动态 buffer
typedef struct {
    char  *data;
    size_t size;
} CurlBuf;

static size_t write_cb(void *ptr, size_t size, size_t nmemb, void *userdata) {
    size_t total = size * nmemb;
    CurlBuf *buf = (CurlBuf *)userdata;

    char *new_data = realloc(buf->data, buf->size + total + 1);
    if (!new_data) return 0;

    buf->data = new_data;
    memcpy(buf->data + buf->size, ptr, total);
    buf->size += total;
    buf->data[buf->size] = '\0';
    return total;
}

char *http_get(const char *url) {
    CURL *curl = curl_easy_init();
    if (!curl) return NULL;

    CurlBuf buf = {0};
    curl_easy_setopt(curl, CURLOPT_URL, url);
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_cb);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, &buf);
    curl_easy_setopt(curl, CURLOPT_TIMEOUT, HTTP_TIMEOUT_SEC);
    curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L);
    curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 0L);
    curl_easy_setopt(curl, CURLOPT_SSL_VERIFYHOST, 0L);
    curl_easy_setopt(curl, CURLOPT_USERAGENT, "Mozilla/5.0 (Switch) Dongman/1.0");
    curl_easy_setopt(curl, CURLOPT_ACCEPT_ENCODING, "");  // 自动处理 gzip

    CURLcode res = curl_easy_perform(curl);
    curl_easy_cleanup(curl);

    if (res != CURLE_OK) {
        free(buf.data);
        return NULL;
    }
    return buf.data;  // 调用者 free
}

bool http_download_file(const char *url, const char *save_path) {
    CURL *curl = curl_easy_init();
    if (!curl) return false;

    FILE *fp = fopen(save_path, "wb");
    if (!fp) { curl_easy_cleanup(curl); return false; }

    curl_easy_setopt(curl, CURLOPT_URL, url);
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, NULL);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, fp);
    curl_easy_setopt(curl, CURLOPT_TIMEOUT, HTTP_TIMEOUT_SEC * 2);
    curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L);
    curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 0L);
    curl_easy_setopt(curl, CURLOPT_USERAGENT, "Mozilla/5.0 (Switch) Dongman/1.0");

    CURLcode res = curl_easy_perform(curl);
    fclose(fp);
    curl_easy_cleanup(curl);

    return (res == CURLE_OK);
}

void url_encode(const char *src, char *dst, size_t dst_size) {
    CURL *curl = curl_easy_init();
    if (!curl) { dst[0] = '\0'; return; }
    char *enc = curl_easy_escape(curl, src, 0);
    if (enc) {
        strncpy(dst, enc, dst_size - 1);
        dst[dst_size - 1] = '\0';
        curl_free(enc);
    } else {
        dst[0] = '\0';
    }
    curl_easy_cleanup(curl);
}
