#ifndef PARSER_H
#define PARSER_H

#include <stddef.h>
#include <stdbool.h>
#include "config.h"

// ============================================================
//  数据结构
// ============================================================

// 单集
typedef struct {
    int ep_num;
    char title[MAX_TITLE_LEN];
    char play_url[MAX_URL_LEN]; // 播放页相对路径 /v/70172-1-1/
} Episode;

// 番剧详情
typedef struct {
    int id;
    char title[MAX_TITLE_LEN];
    char cover[MAX_POSTER_URL];
    char desc[MAX_DESC_LEN];
    int total_eps;
    Episode episodes[MAX_EPISODES];
} AnimeInfo;

// 番剧列表项
typedef struct {
    int id;
    char title[MAX_TITLE_LEN];
    char cover[MAX_POSTER_URL];
    char score[8];
    char status[32];
    char detail_url[MAX_URL_LEN];
} AnimeItem;

// ============================================================
//  解析函数
// ============================================================

// 解析首页/分类页 HTML，提取番剧列表
int parser_parse_list(const char *html, AnimeItem *out, int max_count);

// 解析详情页 HTML，提取分集列表
int parser_parse_detail(const char *html, AnimeInfo *out);

// 从播放页 HTML 提取 m3u8 地址
bool parser_extract_m3u8(const char *html, char *out_url, int out_len);

#endif // PARSER_H
