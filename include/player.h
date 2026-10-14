#ifndef PLAYER_H
#define PLAYER_H

// 播放器框架 - 基础接口
// 后续可接入 mpv/ffmpeg 实现真正的视频解码

// 初始化播放器
bool player_init(void);

// 播放 m3u8 视频流
// url: m3u8 地址
// 返回 true 成功启动播放
bool player_play(const char *m3u8_url);

// 停止播放
void player_stop(void);

// 退出播放器
void player_exit(void);

// 是否正在播放
bool player_is_playing(void);

#endif // PLAYER_H
