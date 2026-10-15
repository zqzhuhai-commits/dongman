#include "network.hpp"
#include <cassert>
#include <curl/curl.h>
#include <iostream>
#include <stdexcept>

int main(int argc, char **argv) {
  assert(argc == 2);
  assert(curl_global_init(CURL_GLOBAL_DEFAULT) == CURLE_OK);
  const std::string base = argv[1];
  Config config;
  auto response = get(base + "/ok", base, config);
  assert(response.body == std::string("hello\0world", 11));
  response = get(base + "/redirect", base, config);
  assert(response.url == base + "/ok" && response.body.size() == 11);
  auto fails = [&](const std::string &path, size_t limit,
                   const std::string &message) {
    try { get(base + path, base, config, limit); }
    catch (const std::runtime_error &e) {
      assert(std::string(e.what()).find(message) != std::string::npos);
      return;
    }
    assert(false && "Expected network request to fail");
  };
  fails("/missing", 100, "HTTP 404");
  fails("/large", 16, "大小限制");
  auto prefix = get_prefix(base + "/large", base, config, 16);
  assert(prefix.body == std::string(16, 'x'));
  fails("/bad-redirect", 100, "网络失败");
  cancelling = true;
  fails("/ok", 100, "取消");
  cancelling = false;
  assert(get(base + "/ok", base, config).body.size() == 11);
  curl_global_cleanup();
  std::cout << "All network tests passed\n";
}
