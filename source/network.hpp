#pragma once
#include "site.hpp"
#include <atomic>
#include <string>
extern std::atomic<bool> cancelling;
inline constexpr const char *HTTP_USER_AGENT =
    "Mozilla/5.0 (Windows NT 10.0; Win64; x64) AppleWebKit/537.36 "
    "(KHTML, like Gecko) Chrome/120.0.0.0 Safari/537.36";
struct Response {
  std::string body, url, content_type;
};
Response get(const std::string &url, const std::string &referer,
             const Config &config, size_t limit = 8 * 1024 * 1024);
Response get_prefix(const std::string &url, const std::string &referer,
                    const Config &config, size_t limit = 16384);
