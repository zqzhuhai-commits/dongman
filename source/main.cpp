#include "comic.hpp"
#include <SDL.h>
#include <SDL_image.h>
#include <SDL_ttf.h>
#include <algorithm>
#include <chrono>
#include <curl/curl.h>
#include <functional>
#include <future>
#include <iostream>
#include <map>
#include <memory>
#include <sys/stat.h>
#ifdef __SWITCH__
#include <switch.h>
#endif
namespace {
constexpr int W = 1280, H = 720;
std::string root() {
#ifdef __SWITCH__
  return "sdmc:/switch/dongman";
#else
  return "packaging";
#endif
}
std::string assets() {
#ifdef __SWITCH__
  return "romfs:/";
#else
  return "romfs/";
#endif
}
void box(SDL_Renderer *r, int x, int y, int w, int h, SDL_Color c) {
  SDL_SetRenderDrawColor(r, c.r, c.g, c.b, c.a);
  SDL_Rect p{x, y, w, h};
  SDL_RenderFillRect(r, &p);
}
void label(SDL_Renderer *r, TTF_Font *f, const std::string &s, int x, int y,
           int w = 1180, int h = 60, SDL_Color color = {231, 235, 246, 255}) {
  if (s.empty())
    return;
  SDL_Surface *p = TTF_RenderUTF8_Blended_Wrapped(f, s.c_str(), color, w);
  if (!p)
    return;
  auto t = SDL_CreateTextureFromSurface(r, p);
  SDL_Rect src{0, 0, p->w, std::min(p->h, h)}, dst{x, y, p->w, src.h};
  if (t) {
    SDL_RenderCopy(r, t, &src, &dst);
    SDL_DestroyTexture(t);
  }
  SDL_FreeSurface(p);
}
using Texture = std::unique_ptr<SDL_Texture, decltype(&SDL_DestroyTexture)>;
Texture texture(SDL_Renderer *r, const std::string &data) {
  SDL_Surface *s = IMG_Load_RW(
      SDL_RWFromConstMem(data.data(), static_cast<int>(data.size())), 1);
  if (!s)
    throw std::runtime_error("图片解码失败，请重试");
  if (s->w <= 0 || s->h <= 0) {
    SDL_FreeSurface(s);
    throw std::runtime_error("图片尺寸无效");
  }
  double scale = std::min({1.0, 2048.0 / s->w, 4096.0 / s->h});
  if (scale < 1) {
    auto small = SDL_CreateRGBSurfaceWithFormat(
        0, std::max(1, int(s->w * scale)), std::max(1, int(s->h * scale)), 32,
        SDL_PIXELFORMAT_RGBA32);
    if (!small) {
      SDL_FreeSurface(s);
      throw std::runtime_error("图片内存不足");
    }
    SDL_BlitScaled(s, nullptr, small, nullptr);
    SDL_FreeSurface(s);
    s = small;
  }
  Texture t(SDL_CreateTextureFromSurface(r, s), SDL_DestroyTexture);
  SDL_FreeSurface(s);
  if (!t)
    throw std::runtime_error("无法创建图片纹理");
  return t;
}
struct JobResult {
  std::vector<Series> catalog;
  std::vector<Chapter> chapters;
  Series series;
  std::string image, error;
  int kind = 0;
  unsigned generation = 0;
};
enum Key { None, Up, Down, Left, Right, A, B, X, Y, L, R, ZL, ZR, Plus, Minus };
Key keyboard(SDL_Keycode k) {
  switch (k) {
  case SDLK_UP:
    return Up;
  case SDLK_DOWN:
    return Down;
  case SDLK_LEFT:
    return Left;
  case SDLK_RIGHT:
    return Right;
  case SDLK_RETURN:
    return A;
  case SDLK_ESCAPE:
    return B;
  case SDLK_x:
    return X;
  case SDLK_y:
    return Y;
  case SDLK_q:
    return L;
  case SDLK_e:
    return R;
  case SDLK_z:
    return ZL;
  case SDLK_c:
    return ZR;
  case SDLK_F5:
    return Minus;
  case SDLK_F10:
    return Plus;
  default:
    return None;
  }
}
} // namespace
int main(int argc, char **argv) {
  std::string api_path = assets() + "client.json",
              state_path = root() + "/bookshelf.json",
              font_path = "/usr/share/fonts/truetype/dejavu/DejaVuSans.ttf",
              screenshot;
  int frames = 0;
  for (int i = 1; i < argc; ++i) {
    std::string a = argv[i];
    if (a == "--help") {
      std::cout
          << "dongman comic reader [--api client.json] [--state state.json] "
             "[--font font.ttf] [--frames N] [--screenshot path.bmp]\n";
      return 0;
    }
    if (i + 1 >= argc) {
      std::cerr << "Missing argument\n";
      return 1;
    }
    if (a == "--api")
      api_path = argv[++i];
    else if (a == "--state")
      state_path = argv[++i];
    else if (a == "--font")
      font_path = argv[++i];
    else if (a == "--screenshot")
      screenshot = argv[++i];
    else if (a == "--frames")
      frames = std::stoi(argv[++i]);
    else
      return 1;
  }
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
  SDL_Window *window = nullptr;
  SDL_Renderer *renderer = nullptr;
  TTF_Font *font = nullptr, *small = nullptr;
  std::future<JobResult> job;
  int exit_code = 0;
  try {
    curl_global_init(CURL_GLOBAL_DEFAULT);
    if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_GAMECONTROLLER) != 0 ||
        TTF_Init() != 0)
      throw std::runtime_error(SDL_GetError());
    IMG_Init(IMG_INIT_PNG | IMG_INIT_JPG | IMG_INIT_WEBP);
    window = SDL_CreateWindow("动漫共和国 · 漫画", SDL_WINDOWPOS_CENTERED,
                              SDL_WINDOWPOS_CENTERED, W, H, 0);
    if (!window)
      throw std::runtime_error(SDL_GetError());
    renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_SOFTWARE);
    if (!renderer)
      throw std::runtime_error(SDL_GetError());
    SDL_RenderSetLogicalSize(renderer, W, H);
#ifdef __SWITCH__
    PlFontData fd{};
    if (R_FAILED(plInitialize(PlServiceType_User)) ||
        R_FAILED(
            plGetSharedFontByType(&fd, PlSharedFontType_ChineseSimplified)))
      throw std::runtime_error("无法读取系统中文字体");
    font = TTF_OpenFontRW(SDL_RWFromConstMem(fd.address, fd.size), 1, 26);
    small = TTF_OpenFontRW(SDL_RWFromConstMem(fd.address, fd.size), 1, 20);
#else
    font = TTF_OpenFont(font_path.c_str(), 26);
    small = TTF_OpenFont(font_path.c_str(), 20);
#endif
    if (!font || !small)
      throw std::runtime_error("无法加载字体");
    mkdir(root().c_str(), 0777);
    ComicApi api;
    api.network.ca_file = assets() + "cacert.pem";
    api.load(api_path);
    Shelf shelf(state_path);
    Texture logo(IMG_LoadTexture(renderer, (assets() + "logo.png").c_str()),
                 SDL_DestroyTexture),
        page_texture(nullptr, SDL_DestroyTexture);
    std::map<std::string, Texture> covers;
    std::map<std::string, Series> details;
    std::map<std::string, bool> attempted;
    std::vector<Series> books;
    std::vector<Chapter> chapters;
    Series current;
    const char *tabs[] = {"推荐", "新作", "热门", "校园",
                          "冒险", "收藏", "历史"};
    const char *channels[] = {"all", "new", "hot", "xiaoyuan", "maoxian"};
    int tab = 0, screen = 0, selected = 0, book_selected = 0, chapter = 0,
        page = 1, frame = 0;
    float zoom = 1, pan = 0, pan_x = 0;
    bool fit_width = false, chrome = true, running = true;
    unsigned generation = 0;
    std::string status = "正在加载漫画…";
    std::function<JobResult()> pending;
    auto schedule = [&](int kind, std::function<JobResult()> fn) {
      ++generation;
      unsigned g = generation;
      if (job.valid())
        cancelling = true;
      pending = [fn, kind, g] {
        JobResult r;
        try {
          r = fn();
        } catch (const std::exception &e) {
          r.error = e.what();
        }
        r.kind = kind;
        r.generation = g;
        return r;
      };
      if (kind != 4)
        status = "正在加载…";
    };
    auto browse = [&] {
      screen = 0;
      selected = 0;
      covers.clear();
      attempted.clear();
      if (tab >= 5) {
        ++generation;
        pending = {};
        if (job.valid())
          cancelling = true;
        books = shelf.list(tab == 5);
        status = books.empty() ? "书架还是空的。打开漫画后按 X 收藏。" : "";
      } else {
        books.clear();
        std::string channel = channels[tab];
        schedule(1, [api, channel] {
          JobResult r;
          r.catalog = api.catalog(channel);
          return r;
        });
      }
    };
    auto read = [&] {
      if (chapters.empty())
        return;
      page = std::clamp(page, 1, chapters[chapter].pages);
      zoom = 1;
      pan = 0;
      pan_x = 0;
      screen = 2;
      page_texture.reset();
      std::string id = chapters[chapter].id;
      int requested = page;
      schedule(3, [api, id, requested] {
        JobResult r;
        r.image = api.image(id, requested);
        return r;
      });
    };
    auto open = [&] {
      if (books.empty())
        return;
      book_selected = selected;
      current = books[selected];
      screen = 1;
      selected = 0;
      chapters.clear();
      auto s = current;
      schedule(2, [api, s] {
        JobResult r;
        r.series = api.detail(s.id);
        r.chapters = api.chapters(s.id);
        return r;
      });
    };
    browse();
    while (running) {
#ifdef __SWITCH__
      if (!appletMainLoop())
        break;
#endif
      if (job.valid() &&
          job.wait_for(std::chrono::seconds(0)) == std::future_status::ready) {
        auto result = job.get();
        cancelling = false;
        if (result.generation == generation) {
          if (!result.error.empty()) {
            if (result.kind != 4)
              status = result.error + "  按 Y 重试";
          } else if (result.kind == 1) {
            books = std::move(result.catalog);
            status = books.empty() ? "这个分类暂无漫画" : "";
          } else if (result.kind == 2) {
            current = result.series;
            details[current.id] = current;
            chapters = std::move(result.chapters);
            status = chapters.empty() ? "暂无可阅读章节" : "";
            if (shelf.data["history"].contains(current.id)) {
              const auto &h = shelf.data["history"][current.id];
              auto id = h.value("chapter", "");
              for (size_t i = 0; i < chapters.size(); ++i)
                if (chapters[i].id == id)
                  selected = static_cast<int>(i);
            }
          } else if (result.kind == 3) {
            try {
              page_texture = texture(renderer, result.image);
              status = "";
              shelf.progress(current, chapters[chapter], page);
            } catch (const std::exception &e) {
              status = e.what();
            }
          } else if (result.kind == 4) {
            details[result.series.id] = result.series;
            for (auto &s : books)
              if (s.id == result.series.id)
                s = result.series;
            if (!result.image.empty())
              try {
                covers.emplace(result.series.id,
                               texture(renderer, result.image));
              } catch (...) {
              }
          }
        }
      }
      if (!job.valid() && pending) {
        auto fn = std::move(pending);
        pending = {};
        cancelling = false;
        job = std::async(std::launch::async, std::move(fn));
      }
      std::vector<Key> keys;
      SDL_Event event;
      while (SDL_PollEvent(&event)) {
        if (event.type == SDL_QUIT)
          running = false;
        if (event.type == SDL_KEYDOWN && !event.key.repeat)
          keys.push_back(keyboard(event.key.keysym.sym));
        if (event.type == SDL_CONTROLLERBUTTONDOWN) {
          switch (event.cbutton.button) {
          case SDL_CONTROLLER_BUTTON_A:
            keys.push_back(A);
            break;
          case SDL_CONTROLLER_BUTTON_B:
            keys.push_back(B);
            break;
          case SDL_CONTROLLER_BUTTON_X:
            keys.push_back(X);
            break;
          case SDL_CONTROLLER_BUTTON_Y:
            keys.push_back(Y);
            break;
          case SDL_CONTROLLER_BUTTON_DPAD_UP:
            keys.push_back(Up);
            break;
          case SDL_CONTROLLER_BUTTON_DPAD_DOWN:
            keys.push_back(Down);
            break;
          case SDL_CONTROLLER_BUTTON_DPAD_LEFT:
            keys.push_back(Left);
            break;
          case SDL_CONTROLLER_BUTTON_DPAD_RIGHT:
            keys.push_back(Right);
            break;
          case SDL_CONTROLLER_BUTTON_LEFTSHOULDER:
            keys.push_back(L);
            break;
          case SDL_CONTROLLER_BUTTON_RIGHTSHOULDER:
            keys.push_back(R);
            break;
          default:
            break;
          }
        }
      }
#ifdef __SWITCH__
      padUpdate(&pad);
      u64 down = padGetButtonsDown(&pad);
      const std::pair<u64, Key> mapping[] = {
          {HidNpadButton_Up, Up},     {HidNpadButton_Down, Down},
          {HidNpadButton_Left, Left}, {HidNpadButton_Right, Right},
          {HidNpadButton_A, A},       {HidNpadButton_B, B},
          {HidNpadButton_X, X},       {HidNpadButton_Y, Y},
          {HidNpadButton_L, L},       {HidNpadButton_R, R},
          {HidNpadButton_ZL, ZL},     {HidNpadButton_ZR, ZR},
          {HidNpadButton_Plus, Plus}, {HidNpadButton_Minus, Minus}};
      for (auto &m : mapping)
        if (down & m.first)
          keys.push_back(m.second);
      if (screen == 2) {
        auto stick = padGetStickPos(&pad, 1);
        if (std::abs(stick.x) > 5000)
          pan_x += stick.x / 32767.0f * 18;
        if (std::abs(stick.y) > 5000)
          pan -= stick.y / 32767.0f * 18;
      }
#endif
      for (Key key : keys)
        try {
          if (key == Plus) {
            running = false;
            continue;
          }
          if (screen == 0) {
            if (key == L || key == R) {
              tab = (tab + (key == R ? 1 : 6)) % 7;
              browse();
              continue;
            }
            int count = static_cast<int>(books.size());
            if (count) {
              int old = selected;
              if (key == Left)
                selected = std::max(0, selected - 1);
              if (key == Right)
                selected = std::min(count - 1, selected + 1);
              if (key == Up)
                selected = std::max(0, selected - 4);
              if (key == Down)
                selected = std::min(count - 1, selected + 4);
              if (old / 8 != selected / 8) {
                covers.clear();
                attempted.clear();
              }
              if (key == A)
                open();
              if (key == X) {
                auto s = books[selected];
                if (details.count(s.id))
                  s = details[s.id];
                shelf.toggle(s);
                if (tab == 5)
                  browse();
              }
            }
            if (key == Y)
              browse();
          } else if (screen == 1) {
            if (key == B) {
              ++generation;
              pending = {};
              if (job.valid())
                cancelling = true;
              screen = 0;
              selected = book_selected;
              status = "";
              continue;
            }
            if (key == X)
              shelf.toggle(current);
            if (key == Y) {
              screen = 0;
              selected = book_selected;
              open();
              continue;
            }
            int count = static_cast<int>(chapters.size());
            if (count) {
              if (key == Up)
                selected = std::max(0, selected - 1);
              if (key == Down)
                selected = std::min(count - 1, selected + 1);
              if (key == Left)
                selected = std::max(0, selected - 9);
              if (key == Right)
                selected = std::min(count - 1, selected + 9);
              if (key == A) {
                chapter = selected;
                page = 1;
                const auto &h = shelf.data["history"];
                if (h.contains(current.id) &&
                    h[current.id].value("chapter", "") == chapters[chapter].id)
                  page = h[current.id].value("page", 1);
                read();
              }
            }
          } else {
            if (key == B || key == X) {
              ++generation;
              pending = {};
              if (job.valid())
                cancelling = true;
              screen = 1;
              selected = chapter;
              page_texture.reset();
              status = "";
              continue;
            }
            if (key == Left || key == Right) {
              int next = page + (key == Right ? 1 : -1);
              if (next < 1 && chapter > 0) {
                --chapter;
                page = chapters[chapter].pages;
                read();
              } else if (next > chapters[chapter].pages &&
                         chapter + 1 < static_cast<int>(chapters.size())) {
                ++chapter;
                page = 1;
                read();
              } else if (next >= 1 && next <= chapters[chapter].pages) {
                page = next;
                read();
              }
            }
            if (key == L && chapter > 0) {
              --chapter;
              page = 1;
              read();
            }
            if (key == R && chapter + 1 < static_cast<int>(chapters.size())) {
              ++chapter;
              page = 1;
              read();
            }
            if (key == ZL)
              zoom = std::max(1.0f, zoom - 0.25f);
            if (key == ZR)
              zoom = std::min(4.0f, zoom + 0.25f);
            if (key == Up)
              pan -= 90;
            if (key == Down)
              pan += 90;
            if (key == Y) {
              fit_width = !fit_width;
              pan = 0;
              zoom = 1;
            }
            if (key == A)
              chrome = !chrome;
            if (key == Minus)
              read();
          }
        } catch (const std::exception &e) {
          status = e.what();
        }
      if (screen == 0 && !job.valid() && !pending && !books.empty()) {
        int first = (selected / 8) * 8;
        for (int i = first;
             i < std::min(first + 8, static_cast<int>(books.size())); ++i) {
          auto s = books[i];
          if (attempted[s.id])
            continue;
          attempted[s.id] = true;
          schedule(4, [api, s] {
            JobResult r;
            r.series = api.detail(s.id);
            try {
              r.image = api.image(s.id);
            } catch (...) {
            }
            return r;
          });
          break;
        }
      }
      box(renderer, 0, 0, W, H, {14, 19, 32, 255});
      if (screen == 2) {
        box(renderer, 0, 0, W, H, {8, 10, 15, 255});
        if (page_texture) {
          int iw = 0, ih = 0;
          SDL_QueryTexture(page_texture.get(), nullptr, nullptr, &iw, &ih);
          float scale = (fit_width ? float(W) / iw
                                   : std::min(float(W) / iw, float(H) / ih)) *
                        zoom;
          int dw = int(iw * scale), dh = int(ih * scale);
          pan = std::clamp(pan, 0.0f, float(std::max(0, dh - H)));
          pan_x = std::clamp(pan_x, -float(std::max(0, dw - W)) / 2,
                             float(std::max(0, dw - W)) / 2);
          SDL_Rect dst{(W - dw) / 2 - int(pan_x),
                       dh <= H ? (H - dh) / 2 : -int(pan), dw, dh};
          SDL_RenderCopy(renderer, page_texture.get(), nullptr, &dst);
        }
        if (chrome) {
          box(renderer, 0, 0, W, 54, {20, 27, 43, 255});
          label(renderer, small,
                current.title + " · 第 " + chapters[chapter].number + " 话", 28,
                15, 980, 30);
          label(renderer, small,
                std::to_string(page) + " / " +
                    std::to_string(chapters[chapter].pages),
                1130, 15, 140, 30);
          box(renderer, 0, 664, W, 56, {20, 27, 43, 255});
          label(renderer, small,
                "← → 翻页   ↑ ↓ 移动   ZL/ZR 缩放   右摇杆 移动   Y 适宽   L/R "
                "换章   A 隐藏 "
                "  B 目录",
                22, 680, 1230, 30);
        }
      } else {
        box(renderer, 0, 0, W, 92, {23, 31, 49, 255});
        if (logo) {
          SDL_Rect d{24, 12, 64, 64};
          SDL_RenderCopy(renderer, logo.get(), nullptr, &d);
        }
        label(renderer, font, "动漫共和国", 108, 17, 350, 40);
        label(renderer, small, "COMIC READER  /  Switch", 110, 53, 420, 30,
              {135, 152, 179, 255});
        if (screen == 0) {
          for (int i = 0; i < 7; ++i) {
            int x = 480 + i * 109;
            if (i == tab)
              box(renderer, x, 22, 101, 46, {48, 112, 221, 255});
            label(renderer, small, tabs[i], x + 27, 34, 100, 30);
          }
          label(renderer, small,
                std::string(tabs[tab]) + "  /  " +
                    std::to_string(books.size()) + " 部漫画",
                32, 107, 700, 30, {146, 166, 199, 255});
          int first = (selected / 8) * 8;
          for (int i = first;
               i < std::min(first + 8, static_cast<int>(books.size())); ++i) {
            int n = i - first, x = 32 + (n % 4) * 312, y = 150 + (n / 4) * 236;
            bool active = i == selected;
            box(renderer, x - 3, y - 3, 292, 222,
                active ? SDL_Color{72, 137, 244, 255}
                       : SDL_Color{31, 42, 63, 255});
            box(renderer, x, y, 286, 216, {26, 35, 53, 255});
            auto &s = books[i];
            if (covers.count(s.id)) {
              SDL_Rect dst{x + 8, y + 8, 110, 164};
              SDL_RenderCopy(renderer, covers.at(s.id).get(), nullptr, &dst);
            } else {
              box(renderer, x + 8, y + 8, 110, 164, {39, 53, 78, 255});
              label(renderer, font, "漫画", x + 33, y + 69, 90, 40,
                    {114, 138, 179, 255});
            }
            label(renderer, font, s.title, x + 130, y + 15, 145, 105);
            label(renderer, small, s.author, x + 130, y + 130, 145, 45,
                  {150, 167, 192, 255});
            label(renderer, small,
                  shelf.favorite(s.id) ? "★ 已收藏" : "A 打开章节", x + 12,
                  y + 182, 265, 30,
                  active ? SDL_Color{210, 230, 255, 255}
                         : SDL_Color{129, 152, 183, 255});
          }
          label(renderer, small,
                "L/R 切换分类    方向键 选择    A 打开    X 收藏    Y 刷新    "
                "+ 退出",
                32, 674, 1100, 35);
          if (!books.empty())
            label(renderer, small,
                  std::to_string(selected / 8 + 1) + " / " +
                      std::to_string((books.size() + 7) / 8),
                  1150, 674, 120, 35);
        } else {
          label(renderer, font, current.title, 35, 110, 1180, 70);
          label(renderer, small,
                current.author + "   /   " + std::to_string(chapters.size()) +
                    " 章" + (shelf.favorite(current.id) ? "   ★ 已收藏" : ""),
                36, 182, 1180, 32, {149, 172, 207, 255});
          int first = selected / 9 * 9;
          for (int i = first;
               i < std::min(first + 9, static_cast<int>(chapters.size()));
               ++i) {
            int y = 230 + (i - first) * 43;
            bool active = i == selected;
            box(renderer, 32, y, 1216, 38,
                active ? SDL_Color{46, 104, 202, 255}
                       : SDL_Color{28, 38, 57, 255});
            auto &c = chapters[i];
            label(renderer, small, "第 " + c.number + " 话", 48, y + 7, 850,
                  28);
            label(renderer, small, std::to_string(c.pages) + " 页", 1100, y + 7,
                  120, 28);
          }
          label(renderer, small,
                "↑ ↓ 选章节    ← → 翻目录    A 阅读 / 续读    X 收藏    Y 重试 "
                "   B 返回",
                32, 674, 1220, 34);
        }
      }
      if (!status.empty()) {
        box(renderer, 20, 611, 1240, 49, {48, 42, 46, 255});
        label(renderer, small, status, 32, 619, 1212, 36, {255, 213, 157, 255});
      }
      SDL_RenderPresent(renderer);
      if (frames > 0 && ++frame >= frames)
        running = false;
      SDL_Delay(16);
    }
    if (!screenshot.empty()) {
      auto s =
          SDL_CreateRGBSurfaceWithFormat(0, W, H, 32, SDL_PIXELFORMAT_ARGB8888);
      if (s) {
        SDL_RenderReadPixels(renderer, nullptr, s->format->format, s->pixels,
                             s->pitch);
        SDL_SaveBMP(s, screenshot.c_str());
        SDL_FreeSurface(s);
      }
    }
  } catch (const std::exception &e) {
    std::cerr << e.what() << "\n";
    exit_code = 1;
  }
  cancelling = true;
  if (job.valid())
    job.wait();
  if (font)
    TTF_CloseFont(font);
  if (small)
    TTF_CloseFont(small);
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
