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
    int episode_num;
    char title[MAX_TITLE_LEN];
    char play_url[MAX_URL_LEN];
} Episode;

// 播放线路 (多线路分集)
typedef struct {
    int line_id;
    char name[MAX_TITLE_LEN];
    Episode episodes[MAX_EPISODES];
    int ep_count;
} PlayLine;

// 番剧详情
typedef struct {
    int id;
    char title[MAX_TITLE_LEN];
    char poster[MAX_POSTER_URL];
    char rating[8];
    char desc[MAX_DESC_LEN];
    PlayLine lines[8];       // 最多 8 条线路
    int line_count;
} AnimeInfo;

// 番剧列表项
typedef struct {
    int id;
    char title[MAX_TITLE_LEN];
    char poster[MAX_POSTER_URL];
    char rating[8];
    char status[32];
    char detail_url[MAX_URL_LEN];
} AnimeItem;

// ============================================================
//  解析函数
// ============================================================

// 解析首页/分类页 HTML，提取番剧列表
int parser_parse_list(const char *html, AnimeItem *out, int max_count);

// 解析详情页 HTML，提取分集列表
int parser_parse_detail(const char *html, AnimeInfo *anime, int anime_id);

// 从播放页 HTML 提取 m3u8 地址
bool parser_extract_m3u8(const char *html, char *out_url, int out_len);

#endif // PARSER_H
