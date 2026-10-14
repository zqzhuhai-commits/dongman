#pragma once
#include <map>
#include <string>
#include <vector>
struct Anime {
  std::string title, url, cover;
};
struct Episode {
  std::string title, url;
};
struct Config {
  std::string base = "http://cn.ghgdm.com.cn/", listing = "/",
              detail_pattern = "/(?:voddetail|detail|show)/|/vod/",
              episode_pattern = "/(?:vodplay|play)/", ca_file;
  void load(const std::string &path);
};
std::string absolute_url(const std::string &base, const std::string &path);
std::string html_decode(std::string s);
std::vector<Anime> parse_catalog(const std::string &html,
                                 const std::string &base, const Config &config);
std::vector<Episode> parse_episodes(const std::string &html,
                                    const std::string &base,
                                    const Config &config);
std::string parse_media(const std::string &html, const std::string &base);
