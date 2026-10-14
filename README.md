# 动漫共和国 · Nintendo Switch

原生 Switch Homebrew 动漫客户端。源码提供封面网格、手柄选集及 libmpv 在线播放，并提供真正的 **NCA-based NSP** 打包流程。也保留 NRO 用于开发与调试。

## 当前交付状态

- 已实现：异步 HTTP 请求、封面读取、目录分页浏览、分集列表、MP4/HLS 直连 URL 解析、播放/暂停、10 秒快进快退、中文系统字体、错误显示、TLS 证书验证。
- 已验证：合成 HTML 样本的目录、分集、相对 URL、HTML 实体、播放器 URL 编码解析测试；Linux 完整客户端编译与 Switch 原生 NRO 编译。
- **未验证**：两个指定网站的真实页面结构、真实媒体流及 Switch 实机性能。2026-10-14 开发环境访问 `cn.ghgdm.com.cn` 返回 404/400，访问 `omofun.org.cn` 超时。配置规则是常见站点格式的初始适配，不能据此宣称这两个网站已经能播放。
- 播放器当前使用 CPU 解码和 640×360 软件渲染再缩放至屏幕，不承诺高清流畅播放。高码率视频可能卡顿，硬件解码是后续工作。
- 需要可以运行 Homebrew 的 Switch；普通官方系统不支持本应用。

## 获取构建产物

打开本仓库 **Actions → Build Anime Republic → 成功的任务 → dongman-switch** 下载。

默认生成 `switch/dongman/dongman.nro` 和 SD 卡配置文件。要生成 **`dongman.nsp`**，在仓库 **Settings → Secrets and variables → Actions** 添加名为 `SWITCH_PROD_KEYS` 的 secret，值为你自己合法取得的密钥文件内容，然后手动运行工作流。缺少密钥时会明确跳过 NSP，不会把 NRO 改名成 NSP。请勿在 Issue、聊天、源码或公开文件中提供密钥。

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
detail_pattern=/(?:voddetail|detail|show)/|/vod/
episode_pattern=/(?:vodplay|play)/
ca_file=sdmc:/switch/dongman/cacert.pem
```

`detail_pattern` 和 `episode_pattern` 是 C++ ECMAScript 正则；匹配详情链接与分集链接。封面读取 `data-original`、`data-src`、`src`。支持 `<video src>`、`<source src>` 和常见 `player_xxx` 对象的直连 URL，包含明文、百分号编码及 Base64 编码。

不执行网页 JavaScript，不自动绕过登录、验证码、DRM 或付费访问控制。不支持需要 JS/专用解析服务才产生的播放地址。网站若使用这类播放器，需要增加该站的正规接口适配。不要随意替换成搜索到的同名站点。

要完成指定网站的接入验证，请提供 **首页 HTML、动漫详情页 HTML 和分集播放页 HTML** 样本（或开发工具导出的相关请求，去除 Cookie、令牌等私密字段），并在真机验证至少一条正常媒体流。

## 手柄操作

| 按键 | 功能 |
|---|---|
| 方向键 | 选择动漫或分集 |
| A | 确认；播放时暂停/继续 |
| B | 返回；播放时停止并返回分集 |
| X | 重新加载目录 |
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

仅测试解析器：

```sh
bash scripts/test.sh
```

Linux 可安装 SDL2、SDL2_image、SDL2_ttf、libmpv、libcurl 开发包，用工作流中的 g++ 命令编译完整客户端。运行时第一个参数可传中文 TTF 路径；修改 `packaging/config.ini` 中的 `ca_file` 为本机 CA 路径。

## 源码说明

- `source/site.cpp`：与平台无关的解析器；其支持的 HTML 子集需按真实网站校准。
- `source/network.cpp`：HTTP(S)、限时、限大小与取消。
- `source/main.cpp`：Switch/SDL 界面与 libmpv 播放。
- `packaging/application.json`：独立应用 NPDM 配置。
- `scripts/package-nsp.sh`：NSO → NPDM → NCA → 可安装 NSP。
- `.github/workflows/build.yml`：主机编译验证、Switch 构建及可选 NSP 打包。

使用 GPL-3.0-or-later 发布本项目源码，链接的 libmpv/FFmpeg 构建含 GPL 组件。第三方工具、库及网站内容由各自权利人保有权利。只播放你有权访问的内容。
