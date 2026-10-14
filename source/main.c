// ============================================================
//  动漫共和国 - Switch Homebrew 动漫客户端
//  数据源: https://omofun.org.cn (MacCMS)
// ============================================================

#include <switch.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include "config.h"
#include "http.h"
#include "parser.h"
#include "ui.h"
#include "player.h"

int main(int argc, char *argv[]) {
    // ---- 初始化 Switch 系统服务 ----
    socketInitializeDefault();
    nxlinkStdio();  // 调试输出到 nxlink

    printf("=== 动漫共和国 Dongman v1.0 ===\n");
    printf("数据源: %s\n", SITE_BASE_URL);

    // ---- 初始化播放器 ----
    if (!player_init()) {
        printf("[错误] 播放器初始化失败\n");
    }

    // ---- 初始化 UI ----
    if (!ui_init()) {
        printf("[错误] UI 初始化失败\n");
        socketExit();
        return 1;
    }

    // ---- 加载首页番剧列表 ----
    ui_show_message("正在加载番剧列表...");
    printf("[main] 请求首页 %s%s ...\n", SITE_BASE_URL, PATH_HOME);

    char url[MAX_URL_LEN];
    snprintf(url, sizeof(url), "%s%s", SITE_BASE_URL, PATH_HOME);

    char *html = http_get(url);
    if (html) {
        static AnimeItem items[120];
        int count = parser_parse_list(html, items, 120);
        printf("[main] 解析到 %d 部番剧\n", count);
        ui_set_anime_list(items, count);
        free(html);
    } else {
        ui_show_message("网络请求失败,请检查WiFi连接");
        printf("[错误] 无法连接到 %s\n", SITE_BASE_URL);
    }

    // ---- 主循环 ----
    printf("[main] 进入主循环...\n");
    ui_run();

    // ---- 清理 ----
    player_exit();
    ui_exit();
    socketExit();

    printf("=== 退出动漫共和国 ===\n");
    return 0;
}
