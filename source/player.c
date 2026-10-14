#include "player.h"
#include "config.h"

#include <stdio.h>
#include <string.h>

// ============================================================
//  基础播放器框架
//  当前为占位实现, 后续可接入:
//    - ffmpeg/libav 软解
//    - Switch NVDEC 硬件解码
//    - 或调用外部 mpv homebrew
// ============================================================

static bool g_playing = false;
static char g_current_url[MAX_URL_LEN] = {0};

bool player_init(void) {
    g_playing = false;
    g_current_url[0] = '\0';
    return true;
}

bool player_play(const char *m3u8_url) {
    if (!m3u8_url || !strstr(m3u8_url, ".m3u8")) {
        printf("[player] 无效的 m3u8 URL: %s\n", m3u8_url ? m3u8_url : "(null)");
        return false;
    }

    strncpy(g_current_url, m3u8_url, MAX_URL_LEN - 1);
    g_playing = true;

    printf("[player] 开始播放: %s\n", g_current_url);
    printf("[player] TODO: 接入 ffmpeg/NVDEC 实现实际解码播放\n");
    printf("[player] 当前为框架占位, 仅记录播放地址\n");

    // ============================================================
    //  后续接入点:
    //  1. 创建 HTTP 客户端下载 m3u8
    //  2. 解析 m3u8, 获取 ts 分片列表
    //  3. 用 ffmpeg/libav 解码视频流
    //  4. 用 SDL2 渲染帧 + 音频输出
    //  5. 支持进度拖动、暂停、音量控制
    // ============================================================

    return true;
}

void player_stop(void) {
    if (g_playing) {
        printf("[player] 停止播放\n");
        g_playing = false;
        g_current_url[0] = '\0';
    }
}

void player_exit(void) {
    player_stop();
}

bool player_is_playing(void) {
    return g_playing;
}
