#pragma once
#include "network.hpp"
#include "vendor/json.hpp"
#include <string>
#include <vector>
using Json = nlohmann::json;
struct Series {
  std::string id, title, author, summary;
};
struct Chapter {
  std::string id, number;
  int pages = 0;
};
struct ComicApi {
  Config network;
  std::string base = "https://manhua.jmcomic.net.cn";
  std::string gateway = "https://manhua.senhuxi.com.cn";
  std::string key;
  void load(const std::string &path);
  std::vector<Series> catalog(const std::string &channel) const;
  Series detail(const std::string &id) const;
  std::vector<Chapter> chapters(const std::string &id) const;
  std::string image(const std::string &id, int page = 0) const;
};
std::vector<Chapter> parse_chapters(const Json &value);
Json series_json(const Series &s);
Series series_from_json(const Json &j);
class Shelf {
  std::string path;

public:
  Json data = {{"favorites", Json::object()}, {"history", Json::object()}};
  explicit Shelf(std::string filename);
  void save() const;
  bool favorite(const std::string &id) const;
  void toggle(const Series &s);
  void progress(const Series &s, const Chapter &chapter, int page);
  std::vector<Series> list(bool favorites) const;
};
