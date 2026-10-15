#include "network.hpp"
#include "site.hpp"
#include <SDL.h>
#include <SDL_image.h>
#include <SDL_ttf.h>
#include <algorithm>
#include <curl/curl.h>
#include <fstream>
#include <future>
#include <iostream>
#include <memory>
#include <mpv/client.h>
#include <mpv/render.h>
#include <stdexcept>
#include <sys/stat.h>
#ifdef __SWITCH__
#include <switch.h>
#endif
namespace {
const int W = 1280, H = 720;
struct Item {
  Anime anime;
  std::string image;
};
struct JobResult {
  int kind;
  std::vector<Item> items;
  std::vector<PlaySource> sources;
  std::string media, referer, error;
};
struct Player {
  mpv_handle *handle = nullptr;
  mpv_render_context *context = nullptr;
  SDL_Texture *texture = nullptr;
  std::vector<unsigned char> pixels;
  std::string error_detail;
  ~Player() { close(); }
  void close() {
    if (context)
      mpv_render_context_free(context);
    context = nullptr;
    if (handle)
      mpv_terminate_destroy(handle);
    handle = nullptr;
    if (texture)
      SDL_DestroyTexture(texture);
    texture = nullptr;
  }
  void command(const char *a, const char *b = nullptr,
               const char *c = nullptr) {
    const char *args[] = {a, b, c, nullptr};
    if (mpv_command(handle, args) < 0)
      throw std::runtime_error("播放器命令失败");
  }
  void open(SDL_Renderer *r, const std::string &url, const std::string &referer,
            const Config &config) {
    close();
    error_detail.clear();
    handle = mpv_create();
    if (!handle)
      throw std::runtime_error("无法初始化播放器");
    auto opt = [&](const char *k, const char *v) {
      int e = mpv_set_option_string(handle, k, v);
      if (e < 0)
        throw std::runtime_error(std::string(k) + ": " + mpv_error_string(e));
    };
    opt("vo", "libmpv");
    opt("ao", "sdl");
    opt("hwdec", "no");
    opt("cache", "yes");
    opt("demuxer-max-bytes", "33554432");
    opt("user-agent", HTTP_USER_AGENT);
    opt("referrer", referer.c_str());
    if (!config.ca_file.empty())
      opt("tls-ca-file", config.ca_file.c_str());
    opt("tls-verify", "yes");
    if (mpv_initialize(handle) < 0)
      throw std::runtime_error("播放器初始化失败");
    mpv_request_log_messages(handle, "warn");
    mpv_render_param init[] = {
        {MPV_RENDER_PARAM_API_TYPE, (void *)MPV_RENDER_API_TYPE_SW},
        {MPV_RENDER_PARAM_INVALID, nullptr}};
    int rc = mpv_render_context_create(&context, handle, init);
    if (rc < 0)
      throw std::runtime_error(std::string("软件渲染初始化失败：") +
                               mpv_error_string(rc));
    pixels.resize(640 * 360 * 4);
    texture = SDL_CreateTexture(r, SDL_PIXELFORMAT_RGBA32,
                                SDL_TEXTUREACCESS_STREAMING, 640, 360);
    if (!texture)
      throw std::runtime_error(SDL_GetError());
    command("loadfile", url.c_str());
  }
  void draw(SDL_Renderer *r) {
    int size[] = {640, 360};
    size_t stride = 640 * 4;
    mpv_render_param params[] = {{MPV_RENDER_PARAM_SW_SIZE, size},
                                 {MPV_RENDER_PARAM_SW_FORMAT, (void *)"rgba"},
                                 {MPV_RENDER_PARAM_SW_STRIDE, &stride},
                                 {MPV_RENDER_PARAM_SW_POINTER, pixels.data()},
                                 {MPV_RENDER_PARAM_INVALID, nullptr}};
    if (mpv_render_context_render(context, params) < 0)
      throw std::runtime_error("视频渲染失败");
    SDL_UpdateTexture(texture, nullptr, pixels.data(), 640 * 4);
    SDL_RenderCopy(r, texture, nullptr, nullptr);
  }
};
void rect(SDL_Renderer *r, int x, int y, int w, int h, SDL_Color color) {
  SDL_SetRenderDrawColor(r, color.r, color.g, color.b, color.a);
  SDL_Rect b{x, y, w, h};
  SDL_RenderFillRect(r, &b);
}
void text(SDL_Renderer *r, TTF_Font *font, const std::string &s, int x, int y,
          int width = 1180, int height = 140) {
  if (s.empty())
    return;
  SDL_Surface *surface = TTF_RenderUTF8_Blended_Wrapped(
      font, s.c_str(), {235, 239, 249, 255}, width);
  if (!surface)
    return;
  SDL_Texture *t = SDL_CreateTextureFromSurface(r, surface);
  SDL_Rect dst{x, y, surface->w, std::min(surface->h, height)};
  SDL_Rect src{0, 0, surface->w, dst.h};
  if (t)
    SDL_RenderCopy(r, t, &src, &dst);
  if (t)
    SDL_DestroyTexture(t);
  SDL_FreeSurface(surface);
}
std::string root() {
#ifdef __SWITCH__
  return "sdmc:/switch/dongman";
#else
  return "packaging";
#endif
}
std::string bundled_ca() {
#ifdef __SWITCH__
  return "romfs:/cacert.pem";
#else
  return "romfs/cacert.pem";
#endif
}
} // namespace
int main(int argc, char **argv) {
  std::string config_path = root() + "/config.ini";
#ifndef __SWITCH__
  std::string font_path = "/usr/share/fonts/truetype/dejavu/DejaVuSans.ttf";
  for (int n = 1; n < argc; ++n) {
    std::string arg = argv[n];
    if (arg == "--help") {
      std::cout << "dongman [--config config.ini] [--font font.ttf]\n";
      return 0;
    }
    if ((arg == "--config" || arg == "--font") && n + 1 < argc) {
      (arg == "--config" ? config_path : font_path) = argv[++n];
    } else if (n == 1 && arg.rfind("--", 0) != 0) {
      font_path = arg;
    } else {
      std::cerr << "无效参数：" << arg << "\n";
      return 1;
    }
  }
#else
  (void)argc;
  (void)argv;
#endif
#ifdef __SWITCH__
  socketInitializeDefault();
  if (R_FAILED(romfsInit())) {
    socketExit();
    return 1;
  }
  padConfigureInput(1, HidNpadStyleSet_NpadStandard);
  PadState pad;
  padInitializeDefault(&pad);
#endif
  int exit_code = 0;
  SDL_Window *window = nullptr;
  SDL_Renderer *renderer = nullptr;
  TTF_Font *font = nullptr;
  SDL_Texture *logo = nullptr;
  std::vector<SDL_Texture *> covers;
  std::future<JobResult> job;
  std::future<JobResult> cover_job;
  int requested_cover = 0;
  unsigned generation = 0, cover_generation = 0;
  try {
    if (curl_global_init(CURL_GLOBAL_DEFAULT) != CURLE_OK)
      throw std::runtime_error("curl init failed");
    if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO | SDL_INIT_GAMECONTROLLER) !=
        0)
      throw std::runtime_error(SDL_GetError());
    IMG_Init(IMG_INIT_PNG | IMG_INIT_JPG);
    if (TTF_Init() != 0)
      throw std::runtime_error(TTF_GetError());
    window = SDL_CreateWindow("动漫共和国", SDL_WINDOWPOS_CENTERED,
                              SDL_WINDOWPOS_CENTERED, W, H, 0);
    if (!window)
      throw std::runtime_error(SDL_GetError());
    renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_SOFTWARE);
    if (!renderer)
      throw std::runtime_error(SDL_GetError());
    SDL_RenderSetLogicalSize(renderer, W, H);
#ifdef __SWITCH__
    logo = IMG_LoadTexture(renderer, "romfs:/logo.png");
#else
    logo = IMG_LoadTexture(renderer, "romfs/logo.png");
#endif
#ifdef __SWITCH__
    PlFontData font_data{};
    if (R_FAILED(plInitialize(PlServiceType_User)))
      throw std::runtime_error("无法初始化系统中文字体");
    if (R_FAILED(plGetSharedFontByType(&font_data,
                                       PlSharedFontType_ChineseSimplified)))
      throw std::runtime_error("无法读取系统中文字体");
    font = TTF_OpenFontRW(SDL_RWFromConstMem(font_data.address, font_data.size),
                          1, 23);
#else
    font = TTF_OpenFont(font_path.c_str(), 23);
#endif
    if (!font)
      throw std::runtime_error(TTF_GetError());
    mkdir(root().c_str(), 0777);
    Config config;
    std::vector<Item> catalog;
    std::vector<PlaySource> sources;
    std::vector<int> source_positions;
    int active_source = 0;
    Player player;
    bool running = true, busy = false, cancel_requested = false;
    int screen = 0, selected = 0, anime_index = 0, page = 0;
    std::string status = "正在连接网站…", episode_page;
    auto start = [&](int kind, std::string url) {
      cancelling = false;
      cancel_requested = false;
      busy = true;
      status = "正在加载…";
      job = std::async(std::launch::async, [kind, url, config]() {
        JobResult out{};
        out.kind = kind;
        try {
          auto response = get(url, config.base, config);
          if (kind == 0) {
            auto list = parse_catalog(response.body, response.url, config);
            if (list.empty())
              throw std::runtime_error("未识别到动漫封面。网站可能不提供目录，"
                                       "或需要修改 config.ini 规则。");
            for (auto &a : list)
              out.items.push_back({a, {}});
          } else if (kind == 1) {
            out.sources = parse_sources(response.body, response.url, config);
            if (out.sources.empty())
              throw std::runtime_error(
                  "未识别到分集。请使用真实详情页校准规则。");
          } else if (kind == 2) {
            auto media = parse_media(response.body, response.url);
            auto check = get_prefix(media, response.url, config);
            validate_media_prefix(check.body, check.url, check.content_type);
            out.media = check.url;
            out.referer = response.url;
          }
          else
            out.media = std::move(response.body);
        } catch (const std::exception &e) {
          out.error = e.what();
        }
        return out;
      });
    };
    auto release_covers = [&]() {
      for (auto *t : covers)
        if (t)
          SDL_DestroyTexture(t);
      covers.clear();
    };
    auto reload_catalog = [&]() {
      try {
        Config updated = runtime_config(config_path, bundled_ca());
        config = std::move(updated);
        start(0, absolute_url(config.base, config.listing));
      } catch (const std::exception &e) {
        status = std::string(e.what()) + " · 修改配置后按 X 重试";
      }
    };
    reload_catalog();
    int cover_index = 0;
    while (running) {
#ifdef __SWITCH__
      if (!appletMainLoop())
        break;
      padUpdate(&pad);
      u64 buttons = padGetButtonsDown(&pad);
      bool a = buttons & HidNpadButton_A, b = buttons & HidNpadButton_B,
           left = buttons & HidNpadButton_Left,
           right = buttons & HidNpadButton_Right,
           up = buttons & HidNpadButton_Up, down = buttons & HidNpadButton_Down,
           reload = buttons & HidNpadButton_X;
      bool previous_source = buttons & HidNpadButton_L,
           next_source = buttons & HidNpadButton_R;
      if (buttons & HidNpadButton_Plus)
        running = false;
#else
      bool a = false, b = false, left = false, right = false, up = false,
           down = false, reload = false;
      bool previous_source = false, next_source = false;
#endif
      SDL_Event event;
      while (SDL_PollEvent(&event)) {
        if (event.type == SDL_QUIT)
          running = false;
        if (event.type == SDL_KEYDOWN) {
          auto k = event.key.keysym.sym;
          a |= k == SDLK_RETURN;
          b |= k == SDLK_ESCAPE;
          left |= k == SDLK_LEFT;
          right |= k == SDLK_RIGHT;
          up |= k == SDLK_UP;
          down |= k == SDLK_DOWN;
          reload |= k == SDLK_r;
          previous_source |= k == SDLK_q;
          next_source |= k == SDLK_e;
        }
      }
      if (!running)
        break;
      if (busy && b) {
        cancel_requested = true;
        cancelling = true;
        status = "正在取消…";
        b = false;
      }
      if (busy &&
          job.wait_for(std::chrono::seconds(0)) == std::future_status::ready) {
        auto result = job.get();
        busy = false;
        if (cancel_requested) {
          cancelling = false;
          cancel_requested = false;
          status = "已取消加载";
        } else if (!result.error.empty()) {
          status = result.error;
          std::ofstream(root() + "/playback.log", std::ios::app) << status << '\n';
        } else if (result.kind == 0) {
          ++generation;
          release_covers();
          catalog = std::move(result.items);
          covers.resize(catalog.size(), nullptr);
          selected = 0;
          page = 0;
          cover_index = 0;
          screen = 0;
          status = "目录已加载";
        } else if (result.kind == 1) {
          sources = std::move(result.sources);
          source_positions.assign(sources.size(), 0);
          active_source = 0;
          screen = 1;
          status = "← → / L R 切换播放源    ↑ ↓ 选择分集    A 播放";
        } else if (result.kind == 2) {
          try {
            player.open(renderer, result.media, result.referer, config);
            screen = 2;
            status = "正在缓冲…";
          } catch (const std::exception &e) {
            player.close();
            status = e.what();
          }
        }
      }
      if (cover_job.valid() && cover_job.wait_for(std::chrono::seconds(0)) ==
                                   std::future_status::ready) {
        auto result = cover_job.get();
        if (cover_generation == generation &&
            requested_cover < (int)covers.size()) {
          if (result.error.empty()) {
            auto *rw =
                SDL_RWFromConstMem(result.media.data(), result.media.size());
            auto *surface = rw ? IMG_Load_RW(rw, 1) : nullptr;
            if (surface) {
              auto *scaled = SDL_CreateRGBSurfaceWithFormat(
                  0, 186, 195, 32, SDL_PIXELFORMAT_RGBA32);
              if (scaled) {
                SDL_BlitScaled(surface, nullptr, scaled, nullptr);
                covers[requested_cover] =
                    SDL_CreateTextureFromSurface(renderer, scaled);
                SDL_FreeSurface(scaled);
              }
              SDL_FreeSurface(surface);
            }
          }
          if (cover_index == requested_cover)
            ++cover_index;
        }
      }
      if (screen == 2) {
        try {
          if (b) {
            player.close();
            screen = 1;
            status = "已停止播放";
          } else {
            if (a)
              player.command("cycle", "pause");
            if (left)
              player.command("seek", "-10", "relative");
            if (right)
              player.command("seek", "10", "relative");
            for (;;) {
              auto *e = mpv_wait_event(player.handle, 0);
              if (e->event_id == MPV_EVENT_NONE)
                break;
              if (e->event_id == MPV_EVENT_LOG_MESSAGE) {
                auto *log = static_cast<mpv_event_log_message *>(e->data);
                player.error_detail = std::string(log->prefix) + ": " + log->text;
                std::replace(player.error_detail.begin(), player.error_detail.end(), '\n', ' ');
                if (player.error_detail.size() > 220) player.error_detail.resize(220);
                std::ofstream(root() + "/playback.log", std::ios::app)
                    << log->prefix << ": " << log->text;
              }
              if (e->event_id == MPV_EVENT_FILE_LOADED)
                status = "A 暂停/继续    ← → 快退/快进 10 秒    B 返回";
              if (e->event_id == MPV_EVENT_END_FILE) {
                auto *d = (mpv_event_end_file *)e->data;
                status = d->error < 0 ? std::string("播放失败，请切源：") +
                                            (player.error_detail.empty() ? mpv_error_string(d->error) : player.error_detail)
                                      : "播放结束";
                player.close();
                screen = 1;
                break;
              }
            }
          }
        } catch (const std::exception &e) {
          player.close();
          screen = 1;
          status = std::string("播放失败：") + e.what();
        }
      } else if (!busy) {
        if (screen == 0) {
          int count = catalog.size();
          if (count) {
            int previous = selected;
            if (left)
              selected = std::max(0, selected - 1);
            if (right)
              selected = std::min(count - 1, selected + 1);
            if (up)
              selected = std::max(0, selected - 6);
            if (down)
              selected = std::min(count - 1, selected + 6);
            page = selected / 12;
            if (previous / 12 != page)
              cover_index = page * 12;
            if (a) {
              anime_index = selected;
              start(1, catalog[selected].anime.url);
            }
          }
          if (reload && !busy)
            reload_catalog();
        } else {
          if (left || previous_source)
            active_source = std::max(0, active_source - 1);
          if (right || next_source)
            active_source = std::min((int)sources.size() - 1, active_source + 1);
          auto &episodes = sources[active_source].episodes;
          int &episode_selection = source_positions[active_source];
          int count = episodes.size();
          if (up)
            episode_selection = std::max(0, episode_selection - 1);
          if (down)
            episode_selection = std::min(count - 1, episode_selection + 1);
          if (b) {
            screen = 0;
            selected = anime_index;
            page = selected / 12;
            cover_index = page * 12;
          } else if (a && count) {
            episode_page = episodes[episode_selection].url;
            start(2, episode_page);
          }
        }
      }
      // Cover requests are bounded to the current page and never touch SDL from
      // the worker.
      if (!busy && screen == 0 && !catalog.empty()) {
        int end = std::min((int)catalog.size(), page * 12 + 12);
        while (cover_index < end && (covers[cover_index] ||
                                     catalog[cover_index].anime.cover.empty()))
          ++cover_index;
        if (cover_index < end && !cover_job.valid()) {
          requested_cover = cover_index;
          cover_generation = generation;
          auto url = catalog[cover_index].anime.cover;
          cover_job = std::async(std::launch::async, [url, config]() {
            JobResult r{};
            try {
              r.media = get(url, config.base, config, 2 * 1024 * 1024).body;
            } catch (const std::exception &e) {
              r.error = e.what();
            }
            return r;
          });
        }
      }
      rect(renderer, 0, 0, W, H, {15, 19, 32, 255});
      if (screen == 2) {
        try {
          player.draw(renderer);
        } catch (const std::exception &e) {
          player.close();
          screen = 1;
          status = std::string("播放失败：") + e.what();
        }
        rect(renderer, 0, 640, W, 80, {15, 19, 32, 255});
        text(renderer, font, status, 30, 658);
      } else {
        if (logo) {
          SDL_Rect logo_rect{32, 12, 48, 48};
          SDL_RenderCopy(renderer, logo, nullptr, &logo_rect);
        }
        text(renderer, font, "动漫共和国", logo ? 94 : 32, 20);
        text(renderer, font, "A 确认   B 返回   X 刷新   + 退出", 650, 22, 600);
        if (screen == 0) {
          for (int n = page * 12;
               n < std::min((int)catalog.size(), page * 12 + 12); ++n) {
            int local = n - page * 12, x = 32 + (local % 6) * 206,
                y = 80 + (local / 6) * 263;
            rect(renderer, x - 4, y - 4, 194, 251,
                 n == selected ? SDL_Color{120, 91, 250, 255}
                               : SDL_Color{30, 37, 55, 255});
            if (covers[n]) {
              SDL_Rect dst{x, y, 186, 195};
              SDL_RenderCopy(renderer, covers[n], nullptr, &dst);
            }
            text(renderer, font, catalog[n].anime.title, x + 5, y + 201, 176, 46);
          }
          text(renderer, font,
               "第 " + std::to_string(page + 1) + " 页 · " +
                   std::to_string(catalog.size()) + " 部",
               32, 612);
        } else {
          text(renderer, font, catalog[anime_index].anime.title, 32, 80);
          int first_tab = active_source / 6 * 6;
          for (int n = first_tab; n < std::min((int)sources.size(), first_tab + 6); ++n) {
            int x = 32 + (n - first_tab) * 204;
            rect(renderer, x, 122, 194, 44,
                 n == active_source ? SDL_Color{120, 91, 250, 255} : SDL_Color{30, 37, 55, 255});
            text(renderer, font, sources[n].title, x + 10, 129, 180, 30);
          }
          const auto &episodes = sources[active_source].episodes;
          int episode_selection = source_positions[active_source];
          int first = (episode_selection / 8) * 8;
          for (int n = first; n < std::min((int)episodes.size(), first + 8);
               ++n) {
            int y = 181 + (n - first) * 52;
            rect(renderer, 32, y, 1216, 44,
                 n == episode_selection ? SDL_Color{120, 91, 250, 255}
                               : SDL_Color{30, 37, 55, 255});
            text(renderer, font, episodes[n].title, 48, y + 8);
          }
          text(renderer, font, sources[active_source].title + " · " +
               std::to_string(episodes.size()) + " 集    L/R 切源", 32, 610);
        }
        text(renderer, font, status, 32, 650, 1216);
      }
      SDL_RenderPresent(renderer);
      SDL_Delay(16);
    }
    cancelling = true;
    if (job.valid())
      job.wait();
    if (cover_job.valid())
      cover_job.wait();
    release_covers();
  } catch (const std::exception &e) {
    std::ofstream log(root() + "/error.log");
    log << e.what() << "\n";
    std::cerr << e.what() << "\n";
    exit_code = 1;
    cancelling = true;
    if (job.valid())
      job.wait();
    if (cover_job.valid())
      cover_job.wait();
    for (auto *t : covers)
      if (t)
        SDL_DestroyTexture(t);
  }
  if (font)
    TTF_CloseFont(font);
  if (logo)
    SDL_DestroyTexture(logo);
  if (renderer)
    SDL_DestroyRenderer(renderer);
  if (window)
    SDL_DestroyWindow(window);
  TTF_Quit();
  IMG_Quit();
  SDL_Quit();
  curl_global_cleanup();
#ifdef __SWITCH__
  plExit();
  romfsExit();
  socketExit();
#endif
  return exit_code;
}
