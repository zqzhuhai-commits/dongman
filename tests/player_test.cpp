// Exercise the same Player implementation used by the application.
#define main dongman_client_main
#include "../source/main.cpp"
#undef main
#include <cassert>

int main(int argc, char **argv) {
  assert(argc == 2);
  assert(SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO) == 0);
  SDL_Window *window = SDL_CreateWindow("test", 0, 0, 640, 360, 0);
  assert(window);
  SDL_Renderer *renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_SOFTWARE);
  assert(renderer);
  {
    Player player;
    Config config;
    player.open(renderer, argv[1], "", config);
    bool loaded = false, rendered = false;
    Uint32 start = SDL_GetTicks();
    std::vector<unsigned char> frame(640 * 360 * 4);
    while (SDL_GetTicks() - start < 10000 && !rendered) {
      auto *event = mpv_wait_event(player.handle, 0);
      if (event->event_id == MPV_EVENT_FILE_LOADED)
        loaded = true;
      if (event->event_id == MPV_EVENT_END_FILE)
        break;
      player.draw(renderer);
      assert(SDL_RenderReadPixels(renderer, nullptr, SDL_PIXELFORMAT_RGBA32,
                                  frame.data(), 640 * 4) == 0);
      // Generated fixture is solid red; reject a black or unrendered frame.
      size_t pixel = (180 * 640 + 320) * 4;
      rendered = loaded && frame[pixel] > 150 && frame[pixel + 1] < 80;
      SDL_RenderPresent(renderer);
      SDL_Delay(10);
    }
    assert(loaded && rendered);
    player.command("set", "pause", "yes");
    int paused = 0;
    assert(mpv_get_property(player.handle, "pause", MPV_FORMAT_FLAG, &paused) == 0);
    assert(paused);
    player.command("seek", "1", "absolute");
    player.command("set", "pause", "no");
    assert(mpv_get_property(player.handle, "pause", MPV_FORMAT_FLAG, &paused) == 0);
    assert(!paused);
    player.close();
    player.close(); // Closing an already stopped player is safe.
  }
  SDL_DestroyRenderer(renderer);
  SDL_DestroyWindow(window);
  SDL_Quit();
  std::cout << "Player decoded a video frame; pause, seek and stop passed\n";
}
