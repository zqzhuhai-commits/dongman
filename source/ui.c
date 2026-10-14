#include "ui.h"
#include "config.h"

#include <SDL2/SDL.h>
#include <SDL2/SDL_image.h>
#include <switch.h>
#include <stdio.h>
#include <string.h>

// ============================================================
//  全局状态
// ============================================================
static SDL_Window   *g_window = NULL;
static SDL_Renderer *g_renderer = NULL;
static UIState       g_state = UI_STATE_LIST;

static AnimeItem  g_items[120];
static int        g_item_count = 0;
static int        g_cursor = 0;       // 当前选中的格子
static int        g_scroll_page = 0;

static AnimeInfo  g_detail;
static bool       g_detail_loaded = false;

static char       g_status_msg[128] = "加载中...";
static Uint32     g_status_timer = 0;

// 加载的封面纹理缓存
static SDL_Texture *g_poster_tex[GRID_COLS * GRID_ROWS];

// ============================================================
//  绘制辅助
// ============================================================
static void draw_text(int x, int y, const char *text, SDL_Color color) {
    // 简化: 用 SDL 自带的绘制方式 (实际项目用 TTF 字体)
    // 这里用矩形占位表示文字区域
    int w = strlen(text) * FONT_SIZE * 0.6;
    SDL_SetRenderDrawColor(g_renderer, color.r, color.g, color.b, color.a);
    // 实际项目应使用 SDL_ttf 渲染中文
}

static void draw_rect(int x, int y, int w, int h, Uint8 r, Uint8 g, Uint8 b) {
    SDL_SetRenderDrawColor(g_renderer, r, g, b, 255);
    SDL_Rect rc = {x, y, w, h};
    SDL_RenderFillRect(g_renderer, &rc);
}

// ============================================================
//  界面绘制
// ============================================================
static void draw_list(void) {
    // 背景
    draw_rect(0, 0, SCREEN_W, SCREEN_H, COLOR_BG >> 16 & 0xFF,
              COLOR_BG >> 8 & 0xFF, COLOR_BG & 0xFF);

    // 顶部标题栏
    draw_rect(0, 0, SCREEN_W, 50, COLOR_ACCENT >> 16 & 0xFF,
              COLOR_ACCENT >> 8 & 0xFF, COLOR_ACCENT & 0xFF);

    // 绘制网格
    int page_start = g_scroll_page * LIST_PAGE_SIZE;
    int visible = g_item_count - page_start;
    if (visible > LIST_PAGE_SIZE) visible = LIST_PAGE_SIZE;

    for (int i = 0; i < visible; i++) {
        int idx = page_start + i;
        int col = i % GRID_COLS;
        int row = i / GRID_COLS;
        int x = 30 + col * (POSTER_W + 20);
        int y = 70 + row * (POSTER_H + 50);

        // 选中高亮
        if (i == g_cursor) {
            draw_rect(x - 3, y - 3, POSTER_W + 6, POSTER_H + 6,
                      COLOR_SELECTED >> 16 & 0xFF, COLOR_SELECTED >> 8 & 0xFF,
                      COLOR_SELECTED & 0xFF);
        }

        // 封面占位
        draw_rect(x, y, POSTER_W, POSTER_H, 60, 60, 80);

        // 标题 (截断)
        // draw_text(x, y + POSTER_H + 5, g_items[idx].title, white);
    }

    // 底部状态栏
    if (SDL_GetTicks() - g_status_timer < 3000) {
        draw_rect(0, SCREEN_H - 40, SCREEN_W, 40, 0, 0, 0);
        // draw_text(20, SCREEN_H - 35, g_status_msg, white);
    }
}

static void draw_detail(void) {
    draw_rect(0, 0, SCREEN_W, SCREEN_H, COLOR_BG >> 16 & 0xFF,
              COLOR_BG >> 8 & 0xFF, COLOR_BG & 0xFF);

    // 封面
    draw_rect(40, 70, POSTER_W, POSTER_H, 80, 80, 100);

    // 标题和信息
    // draw_text(200, 80, g_detail.title, white);
    // draw_text(200, 120, g_detail.rating, yellow);
    // draw_text(200, 150, g_detail.status, gray);

    // 分集列表
    int y = 320;
    for (int l = 0; l < g_detail.line_count; l++) {
        // draw_text(40, y, g_detail.lines[l].name, accent);
        y += 30;
        for (int e = 0; e < g_detail.lines[l].ep_count && e < 20; e++) {
            // draw_text(60 + (e % 8) * 120, y + (e / 8) * 35,
            //          g_detail.lines[l].episodes[e].title, white);
        }
        y += ((g_detail.lines[l].ep_count / 8) + 1) * 35 + 20;
    }
}

// ============================================================
//  输入处理
// ============================================================
static void handle_input(u64 kDown) {
    if (g_state == UI_STATE_LIST) {
        int col = g_cursor % GRID_COLS;
        int row = g_cursor / GRID_COLS;

        if (kDown & KEY_DRIGHT) { if (col < GRID_COLS - 1) g_cursor++; }
        if (kDown & KEY_DLEFT)  { if (col > 0) g_cursor--; }
        if (kDown & KEY_DDOWN)  { if (row < GRID_ROWS - 1) g_cursor += GRID_COLS; }
        if (kDown & KEY_DUP)    { if (row > 0) g_cursor -= GRID_COLS; }

        // A键: 进入详情
        if (kDown & KEY_A) {
            int idx = g_scroll_page * LIST_PAGE_SIZE + g_cursor;
            if (idx < g_item_count) {
                // TODO: 加载详情
                g_state = UI_STATE_DETAIL;
            }
        }

        // B键: 返回/退出
        if (kDown & KEY_B) {
            // 第一页按B退出
        }
    }
    else if (g_state == UI_STATE_DETAIL) {
        if (kDown & KEY_B) {
            g_state = UI_STATE_LIST;
        }
    }
}

// ============================================================
//  公开接口
// ============================================================
bool ui_init(void) {
    if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_JOYSTICK) < 0) return false;
    if (!(g_window = SDL_CreateWindow("Dongman", 0, 0, SCREEN_W, SCREEN_H, 0)))
        return false;
    if (!(g_renderer = SDL_CreateRenderer(g_window, -1,
                                         SDL_RENDERER_ACCELERATED)))
        return false;
    return true;
}

void ui_exit(void) {
    if (g_renderer) SDL_DestroyRenderer(g_renderer);
    if (g_window)   SDL_DestroyWindow(g_window);
    SDL_Quit();
}

void ui_set_anime_list(AnimeItem *items, int count) {
    g_item_count = count;
    g_cursor = 0;
    g_scroll_page = 0;
    memcpy(g_items, items, sizeof(AnimeItem) * (count < 120 ? count : 120));
    snprintf(g_status_msg, sizeof(g_status_msg), "加载完成: %d 部番剧", count);
    g_status_timer = SDL_GetTicks();
}

void ui_set_detail(const AnimeInfo *info) {
    memcpy(&g_detail, info, sizeof(AnimeInfo));
    g_detail_loaded = true;
}

void ui_show_message(const char *msg) {
    snprintf(g_status_msg, sizeof(g_status_msg), "%s", msg);
    g_status_timer = SDL_GetTicks();
}

void ui_run(void) {
    bool running = true;
    SDL_Event ev;

    while (running) {
        // 处理 libnx 输入
        u64 kDown = 0;
        // (实际项目用 padRead 获取手柄输入)

        while (SDL_PollEvent(&ev)) {
            if (ev.type == SDL_QUIT) running = false;
        }

        handle_input(kDown);

        // 绘制
        SDL_SetRenderDrawColor(g_renderer, 26, 26, 46, 255);
        SDL_RenderClear(g_renderer);

        if (g_state == UI_STATE_LIST) draw_list();
        else if (g_state == UI_STATE_DETAIL) draw_detail();

        SDL_RenderPresent(g_renderer);
        SDL_Delay(16);  // ~60fps
    }
}
