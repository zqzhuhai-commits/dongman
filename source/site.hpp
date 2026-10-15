#pragma once
#include <map>
#include <string>
#include <vector>
struct Anime {
  std::string title, url, cover;
};
struct Episode {
  std::string title, url;
  std::string source = "1";
};
struct PlaySource {
  std::string id, title;
  std::vector<Episode> episodes;
};
struct Config {
  std::string base = "https://omofun.org.cn/", listing = "/",
              detail_pattern = "/(?:voddetail|detail|show|vod|p)/",
              episode_pattern = "/(?:vodplay|play|v)/", ca_file;
  void load(const std::string &path);
};
Config runtime_config(const std::string &optional_path,
                      const std::string &bundled_ca);
std::string absolute_url(const std::string &base, const std::string &path);
std::string html_decode(std::string s);
std::string encode_http_url(const std::string &url);
std::vector<Anime> parse_catalog(const std::string &html,
                                 const std::string &base, const Config &config);
std::vector<Episode> parse_episodes(const std::string &html,
                                    const std::string &base,
                                    const Config &config);
std::string parse_media(const std::string &html, const std::string &base);
std::vector<PlaySource> parse_sources(const std::string &html,
                                     const std::string &base, const Config &config);
void validate_media_prefix(const std::string &body, const std::string &url,
                           const std::string &content_type);
