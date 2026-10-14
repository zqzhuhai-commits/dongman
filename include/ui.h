#ifndef UI_H
#define UI_H

#include "parser.h"

// 界面状态
typedef enum {
    UI_STATE_LIST,       // 番剧列表(网格)
    UI_STATE_DETAIL,     // 番剧详情
    UI_STATE_PLAYER,     // 播放器
    UI_STATE_SEARCH,     // 搜索
} UIState;

// 初始化 SDL2 界面
bool ui_init(void);
void ui_exit(void);

// 主循环
void ui_run(void);

// 设置当前番剧列表数据
void ui_set_anime_list(AnimeItem *items, int count);

// 设置当前番剧详情
void ui_set_detail(const AnimeInfo *info);

// 显示状态消息
void ui_show_message(const char *msg);

#endif // UI_H
