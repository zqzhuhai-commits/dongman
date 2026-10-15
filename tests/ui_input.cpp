// Inject real SDL keyboard events into the desktop build for end-to-end QA.
#include <SDL.h>
#include <chrono>
#include <dlfcn.h>
extern "C" int SDL_PollEvent(SDL_Event *event) {
  using Clock = std::chrono::steady_clock;
  static auto start = Clock::now();
  static size_t next = 0;
  static const struct {
    int ms;
    SDL_Keycode key;
  } script[] = {{1200, SDLK_RETURN}, {2200, SDLK_x},      {2600, SDLK_RETURN},
                {3900, SDLK_RIGHT},  {4900, SDLK_y},      {5300, SDLK_c},
                {5800, SDLK_ESCAPE}, {6300, SDLK_RETURN}, {7600, SDLK_F10}};
  auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(
                     Clock::now() - start)
                     .count();
  if (event && next < sizeof(script) / sizeof(script[0]) &&
      elapsed >= script[next].ms) {
    SDL_zero(*event);
    event->type = SDL_KEYDOWN;
    event->key.keysym.sym = script[next++].key;
    return 1;
  }
  static auto original =
      reinterpret_cast<int (*)(SDL_Event *)>(dlsym(RTLD_NEXT, "SDL_PollEvent"));
  return original(event);
}
