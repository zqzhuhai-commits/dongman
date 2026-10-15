# 动漫共和国 · Switch 漫画版

根据用户提供的 Android APK（`com.jumang.fundmly` 1.1.52）的漫画浏览与阅读功能，实现的原生 Nintendo Switch 客户端。使用用户提供的 LOGO。

- 推荐、新作、热门、校园、冒险分类，按需加载封面与书名。
- 章节目录、数字顺序排序和分页加载。
- 单页图片阅读、跨章节翻页、放大缩小、适宽和整页显示。
- 本地收藏、阅读历史和自动续读，保存在 SD 卡 `switch/dongman/bookshelf.json`。
- 中文系统字体，适配掌机与底座的 1280×720 界面。

当前上游接口有间歇性 502 响应。客户端显示错误并支持重试；构建成功不等于服务始终可用。尚需 Switch 真机验证。

## 操作

| 页面 | 操作 |
| --- | --- |
| 书架 | L/R 切分类，方向键选漫画，A 打开，X 收藏，Y 刷新 |
| 目录 | 上下选章节，左右翻目录，A 阅读或续读，X 收藏，Y 重试，B 返回 |
| 阅读 | 左右翻页，上下或右摇杆移动，ZL/ZR 缩放，Y 切换整页/适宽，L/R 换章，A 隐藏工具栏，B/X 回目录，− 重试 |
| 全局 | + 退出 |

桌面调试：方向键、Enter=A、Escape=B、X/Y，Q/E=L/R、Z/C=ZL/ZR、F5=−、F10=+。

## 安装包

独立 NSP 的 RomFS 内置证书、LOGO 和客户端接口配置。用户无需在 SD 卡额外放置 `config.ini` 或 `cacert.pem`。收藏和进度文件由应用自动创建。

源码不包含 APK 中的共享客户端接口密钥，也不包含任何 Switch 私有密钥。自建版本需在本地准备 `romfs/client.json`（已忽略）：

```json
{
  "base": "https://manhua.jmcomic.net.cn",
  "gateway": "https://manhua.senhuxi.com.cn",
  "api_key": "YOUR_AUTHORIZED_CLIENT_KEY"
}
```

APK 的分类接口使用 POST `/api/v1/channel/omo-{all,new,hot,xiaoyuan,maoxian}`；漫画详情为 GET `/api/v1/series/{id}`；章节列表为 POST `/api/v1/books/list`；图片为 GET `/api/v1/books/{id}/pages/{page}`，页码从 1 开始。请求使用 HTTPS 验证和 `X-API-Key`。附带该头时禁止自动跟随重定向，避免向其他主机发送密钥。

## 构建与验证

Switch：devkitPro devkitA64、libnx、switch-sdl2、switch-sdl2_image、switch-sdl2_ttf、switch-curl、switch-pkg-config，然后 `make -j2`。CI 使用固定摘要的 SDK 镜像，产出 NRO 与独立 NSP 的打包输入。

桌面：安装 SDL2、SDL2_image、SDL2_ttf、libcurl 开发包与 pkg-config，执行 `bash scripts/build-host.sh`。使用 `--font` 指定中文字体、`--api` 指定私有配置、`--state` 指定进度文件。

```sh
bash scripts/test.sh
bash scripts/test-network.sh
bash scripts/test-comic.sh
python3 tests/ui_test.py build/dongman-host
```

漫画测试使用本地 HTTP 服务验证分类请求、请求头、章节条件、超过 100 章的分页、数字排序、从第 1 页取图，以及收藏与阅读进度持久化。端到端界面测试通过真实 SDL 事件检查打开漫画、选章、翻页、缩放、返回和续读。旧视频解析测试仅覆盖保留的基础解析模块。

独立 NSP 打包：设置本地 `SWITCH_KEYS_FILE`、`HACBREWPACK`，执行 `bash scripts/package-nsp.sh`；也可以设置 `NSP_INPUT_DIR` 使用 CI 产出的输入。此步骤使用自备的 Switch 密钥，密钥不得提交仓库或上传到公开构建日志。

JSON 库：nlohmann/json 3.12.0，MIT 许可证见 `source/vendor/LICENSE.json`。
