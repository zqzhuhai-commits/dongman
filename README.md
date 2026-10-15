# 动漫共和国 · Nintendo Switch

原生 Switch Homebrew 动漫客户端。源码提供封面网格、手柄选集及 libmpv 在线播放，并提供真正的 **NCA-based NSP** 打包流程。也保留 NRO 用于开发与调试。

## 当前交付状态

- 已恢复完整客户端，替换仅显示 Welcome 的入口及占位播放器。包含异步 HTTP、封面网格、本页目录分屏浏览、分集与播放源区分、MP4/HLS 地址解析、播放/暂停、快进快退、中文系统字体和 TLS 验证。
- 新增：B 取消加载、X 重读配置并刷新；配置错误与播放命令错误可在界面中恢复。
- 已验证：解析器回归、配置加载、HTTP 重定向/错误/大小限制/取消、Linux 完整客户端编译。播放器测试使用自行生成的视频验证解码、画面渲染、暂停、跳转和停止。原生 NRO 由 GitHub Actions 单独构建验证。
- 2026-10-15 已读取 OMOFUN 真实首页、详情与播放页：识别 48 个目录项、一个详情页的 631 个分集链接（含多播放源）及媒体地址。默认规则已加入 `/p/` 与 `/v/`。指定样本的媒体服务器返回 “Site Unavailable”，**真实网站视频及 Switch 实机播放仍未验证成功**；`cn.ghgdm.com.cn` 返回 HTTP 400，保留为未验证配置。
- 播放器当前使用 CPU 解码和 640×360 软件渲染再缩放至屏幕，不承诺高清流畅播放。高码率视频可能卡顿，硬件解码是后续工作。
- 需要可以运行 Homebrew 的 Switch；普通官方系统不支持本应用。

## 获取构建产物

打开本仓库 **Actions → Build Anime Republic → 成功的任务 → dongman-switch** 下载。

推送及 PR 默认生成 `switch/dongman/dongman.nro` 和 SD 卡配置文件。要生成 **`dongman.nsp`**，在仓库 **Settings → Secrets and variables → Actions** 添加名为 `SWITCH_PROD_KEYS` 的 secret，值为你自己合法取得的密钥文件内容，然后手动运行工作流。只有手动运行会读取此 secret；缺少密钥时明确跳过 NSP。请勿在 Issue、聊天、源码或公开文件中提供密钥。

NSP 是独立程序，不是指向 SD 卡 NRO 的转发器。配置与 CA 文件仍需在 SD 卡中：

```
switch/dongman/config.ini
switch/dongman/cacert.pem
```

安装 NSP 后从 HOME 菜单启动；NRO 调试请使用完整应用模式的 Homebrew Menu。错误日志位于 `sdmc:/switch/dongman/error.log`。

## 网站配置

默认使用 `https://omofun.org.cn/`。将打包文件中的 `ghgdm.ini` 复制为 `config.ini` 可切换到 `http://cn.ghgdm.com.cn/`。

```ini
base_url=https://omofun.org.cn/
listing_path=/
detail_pattern=/(?:voddetail|detail|show|vod|p)/
episode_pattern=/(?:vodplay|play|v)/
ca_file=sdmc:/switch/dongman/cacert.pem
```

`detail_pattern` 和 `episode_pattern` 是 C++ ECMAScript 正则；匹配详情链接与分集链接。封面读取 `data-original`、`data-src`、`src`。支持 `<video src>`、`<source src>` 和常见 `player_xxx` 对象的直连 URL，包含明文、百分号编码及 Base64 编码。

不执行网页 JavaScript，不自动绕过登录、验证码、DRM 或付费访问控制。不支持需要 JS/专用解析服务才产生的播放地址。网站若使用这类播放器，需要增加该站的正规接口适配。不要随意替换成搜索到的同名站点。

目录目前仅浏览 `listing_path` 指向的页面中最多 200 项，不会自动翻到网站下一页。OMOFUN 可将此项改为 `/h/1/`、`/h/2/` 或 `/h/3/` 浏览对应分类；修改后在目录按 X 生效。要完成网站接入验证，还需可正常访问的媒体流及实机测试。

## 手柄操作

| 按键 | 功能 |
|---|---|
| 方向键 | 选择动漫或分集 |
| A | 确认；播放时暂停/继续 |
| B | 返回；加载时取消；播放时停止并返回分集 |
| X | 重读配置并重新加载目录 |
| ← / →（播放时） | 快退/快进 10 秒 |
| + | 退出 |

## 本地开发

安装 devkitPro 的 `switch-dev` 及下列 portlibs：

```sh
dkp-pacman -S switch-sdl2 switch-sdl2_image switch-sdl2_ttf switch-libmpv switch-curl switch-pkg-config
make -j2
```

`make` 默认生成 NRO、ELF、NACP。打包 NSP 还需 PATH 中的 `elf2nso`、`npdmtool` 和 [hacBrewPack](https://github.com/pplatoon/hacBrewPack)（已固定提交 `56698df456eac4d5076e85499fe045a90fba790b`）：

```sh
SWITCH_KEYS_FILE=/absolute/path/to/prod.keys HACBREWPACK=/absolute/path/to/hacbrewpack bash scripts/package-nsp.sh
```

结果为 `dist/dongman.nsp`。Title ID 为 `0100D06A6E000000`；安装前检查没有与已有应用冲突。

也可下载 Actions 的 `dongman-nsp-inputs`，解压到本地目录后打包，无需本地 Switch SDK：

```sh
NSP_INPUT_DIR=/absolute/path/to/nsp-inputs SWITCH_KEYS_FILE=/absolute/path/to/prod.keys HACBREWPACK=/absolute/path/to/hacbrewpack bash scripts/package-nsp.sh
```

此流程仅在本机使用密钥，输入包只含本项目的原生程序、控制信息和图标。

仅测试解析器：

```sh
bash scripts/test.sh
```

Linux 安装 SDL2、SDL2_image、SDL2_ttf、libmpv、libcurl 开发包、pkg-config 及 ffmpeg 后：

```sh
bash scripts/test-network.sh
bash scripts/test-player.sh
bash scripts/build-host.sh
cp packaging/config.ini /tmp/dongman.ini
# 将 /tmp/dongman.ini 的 ca_file 值清空，使用本机系统 CA；Switch 配置保持原样。
build/dongman-host --config /tmp/dongman.ini --font /path/to/chinese.ttf
```

桌面操作：方向键选择、Enter 确认/暂停、Esc 返回/取消、R 重读配置，关闭窗口退出。默认桌面字体不含中文，使用中文 TTF 才能正常显示中文。

## 源码说明

- `source/site.cpp`：与平台无关的解析器；其支持的 HTML 子集需按真实网站校准。
- `source/network.cpp`：HTTP(S)、限时、限大小与取消。
- `source/main.cpp`：Switch/SDL 界面与 libmpv 播放。
- `packaging/application.json`：独立应用 NPDM 配置。
- `scripts/package-nsp.sh`：NSO → NPDM → NCA → 可安装 NSP。
- `.github/workflows/build.yml`：主机编译验证、Switch 构建及可选 NSP 打包。

使用 GPL-3.0-or-later 发布本项目源码，链接的 libmpv/FFmpeg 构建含 GPL 组件。第三方工具、库及网站内容由各自权利人保有权利。只播放你有权访问的内容。
