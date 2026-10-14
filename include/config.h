#ifndef CONFIG_H
#define CONFIG_H

// ============================================================
//  站点配置 - omofun.org.cn (MacCMS)
// ============================================================
#define SITE_BASE_URL       "https://omofun.org.cn"

// API / 页面路径
#define PATH_HOME           "/"
#define PATH_CATEGORY       "/h/%d/"        // 分类: 1=日漫 2=国漫 3=美漫 20=剧场
#define PATH_DETAIL         "/p/%d/"        // 番剧详情页
#define PATH_PLAY           "/v/%d-%d-%d/" // 播放页: id-线路-集数
#define PATH_SEARCH         "/u/?wd="       // 搜索

// 分类ID
typedef enum {
    CAT_DAILY  = 0,   // 每日更新(首页)
    CAT_ANIME  = 1,   // 日漫
    CAT_MANHUA = 2,   // 国漫
    CAT_CARTOON= 3,   // 美漫
    CAT_MOVIE  = 20,  // 动漫剧场
} Category;

// ============================================================
//  UI 配置
// ============================================================
#define SCREEN_W            1280
#define SCREEN_H            720
#define FONT_SIZE           20
#define POSTER_W            140
#define POSTER_H            200
#define GRID_COLS           6
#define GRID_ROWS           4
#define LIST_PAGE_SIZE      (GRID_COLS * GRID_ROWS)

// 颜色 (RGBA8888)
#define COLOR_BG            0x1a1a2e
#define COLOR_ACCENT        0xe68700   // 网站主色橙
#define COLOR_TEXT          0xffffff
#define COLOR_TEXT_DIM      0xaaaaaa
#define COLOR_SELECTED      0xe94560

// ============================================================
//  网络配置
// ============================================================
#define HTTP_TIMEOUT_SEC    15
#define MAX_URL_LEN         512
#define MAX_TITLE_LEN       128
#define MAX_DESC_LEN        512
#define MAX_POSTER_URL      512
#define MAX_EPISODES        120
#define MAX_BUFFER_SIZE     (256 * 1024)  // 256KB HTML 缓冲

#endif // CONFIG_H
