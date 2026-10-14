#ifndef PARSER_H
#define PARSER_H

#include <stddef.h>
#include <stdbool.h>

// ============================================================
//  数据结构
// ============================================================

// 单集信息
typedef struct {
    int  episode_num;       // 集数
    char title[64];         // 显示标题 "第01集"
    char play_url[MAX_URL_LEN]; // 播放页相对路径 /v/70172-1-1/
} Episode;

// 一条线路(播放源)
typedef struct {
    char name[32];          // "线路六"
    int  line_id;           // 线路编号 1-4
    int  ep_count;
    Episode episodes[MAX_EPISODES];
} PlayLine;

// 番剧信息
typedef struct {
    int  id;
    char title[MAX_TITLE_LEN];
    char poster[MAX_POSTER_URL];  // 封面图 URL
    char rating[8];               // "4.0"
    char status[32];              // "更新至第01集"
    char category[16];            // "日漫"
    char desc[MAX_DESC_LEN];
    int  line_count;
    PlayLine lines[8];            // 最多8条线路
} AnimeInfo;

// 番剧列表项(卡片)
typedef struct {
    int  id;
    char title[MAX_TITLE_LEN];
    char poster[MAX_POSTER_URL];
    char rating[8];
    char status[32];
} AnimeItem;

// ============================================================
//  解析函数
// ============================================================

// 解析首页/分类页的番剧列表
// items: 输出数组
// max_items: 数组容量
// 返回实际解析到的数量
int parse_anime_list(const char *html, AnimeItem *items, int max_items);

// 解析详情页
// anime: 输出结构体
// 返回 true 成功
bool parse_detail(const char *html, int anime_id, AnimeInfo *anime);

// 从播放页 HTML 提取 m3u8 视频地址
// m3u8_url: 输出 buffer
// 返回 true 成功
bool parse_play_url(const char *html, char *m3u8_url, size_t url_size);

#endif // PARSER_H
