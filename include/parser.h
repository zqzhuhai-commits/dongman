#ifndef PARSER_H
#define PARSER_H

#include <stddef.h>
#include <stdbool.h>
#include "config.h"

// ============================================================
//  数据结构
// ============================================================

typedef struct {
    int episode_num;
    char title[MAX_TITLE_LEN];
    char play_url[MAX_URL_LEN];
} Episode;

typedef struct {
    int line_id;
    char name[MAX_TITLE_LEN];
    Episode episodes[MAX_EPISODES];
    int ep_count;
} PlayLine;

typedef struct {
    int id;
    char title[MAX_TITLE_LEN];
    char poster[MAX_POSTER_URL];
    char rating[8];
    char desc[MAX_DESC_LEN];
    PlayLine lines[8];
    int line_count;
} AnimeInfo;

typedef struct {
    int id;
    char title[MAX_TITLE_LEN];
    char poster[MAX_POSTER_URL];
    char rating[8];
    char status[32];
    char detail_url[MAX_URL_LEN];
} AnimeItem;

// ============================================================
//  解析函数 (与 parser.c 实现一致)
// ============================================================

int parse_anime_list(const char *html, AnimeItem *items, int max_items);
bool parse_detail(const char *html, int anime_id, AnimeInfo *anime);
bool parse_play_url(const char *html, char *m3u8_url, size_t url_size);

#endif // PARSER_H
