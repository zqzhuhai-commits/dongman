#include "comic.hpp"
#include <algorithm>
#include <cerrno>
#include <cmath>
#include <cstdio>
#include <ctime>
#include <fstream>
#include <set>
#include <stdexcept>
namespace {
std::string field(const Json &j, const char *name,
                  const std::string &fallback = "") {
  auto it = j.find(name);
  if (it == j.end() || it->is_null())
    return fallback;
  return it->is_string() ? it->get<std::string>() : it->dump();
}
std::string segment(const std::string &id) {
  if (id.empty() ||
      id.find_first_not_of(
          "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789-_") !=
          std::string::npos)
    throw std::runtime_error("漫画或章节 ID 无效");
  return id;
}
Json request_json(const ComicApi &api, const std::string &url,
                  const std::string *body = nullptr) {
  auto response = api_request(url, api.network, api.key, body);
  try {
    return Json::parse(response.body);
  } catch (const Json::exception &) {
    throw std::runtime_error("服务器没有返回有效的漫画数据，请稍后重试");
  }
}
double chapter_number(const std::string &s) {
  try {
    double n = std::stod(s);
    return std::isfinite(n) ? n : 0;
  } catch (...) {
    return 0;
  }
}
} // namespace
void ComicApi::load(const std::string &path) {
  std::ifstream in(path);
  if (!in)
    return;
  Json j;
  in >> j;
  base = j.value("base", base);
  gateway = j.value("gateway", gateway);
  key = j.value("api_key", "");
  for (auto *url : {&base, &gateway}) {
    while (!url->empty() && url->back() == '/')
      url->pop_back();
    if (url->rfind("https://", 0) != 0 &&
        url->rfind("http://127.0.0.1:", 0) != 0)
      throw std::runtime_error("漫画接口必须使用 HTTPS");
  }
  if (key.find_first_of("\r\n") != std::string::npos)
    throw std::runtime_error("接口配置无效");
}
Series series_from_json(const Json &j) {
  return {field(j, "id"), field(j, "name", field(j, "title", "未命名漫画")),
          field(j, "author"), field(j, "summary")};
}
Json series_json(const Series &s) {
  return {{"id", s.id},
          {"name", s.title},
          {"author", s.author},
          {"summary", s.summary}};
}
std::vector<Series> ComicApi::catalog(const std::string &channel) const {
  static const std::set<std::string> channels{"all", "new", "hot", "xiaoyuan",
                                              "maoxian"};
  if (!channels.count(channel))
    throw std::runtime_error("未知分类");
  std::string body = "{}";
  auto j =
      request_json(*this, gateway + "/api/v1/channel/omo-" + channel, &body);
  if (!j.contains("data") || !j["data"].contains("book_ids") ||
      !j["data"]["book_ids"].is_array())
    throw std::runtime_error("分类接口数据格式已改变");
  std::vector<Series> out;
  std::set<std::string> seen;
  for (const auto &v : j["data"]["book_ids"]) {
    if (!v.is_string())
      continue;
    auto id = v.get<std::string>();
    segment(id);
    if (seen.insert(id).second)
      out.push_back({id, "加载书名…", "", ""});
    if (out.size() >= 2000)
      break;
  }
  return out;
}
Series ComicApi::detail(const std::string &id) const {
  auto j = request_json(*this, base + "/api/v1/series/" + segment(id));
  auto s = series_from_json(j);
  if (j.contains("metadata") && j["metadata"].is_object()) {
    s.summary = j["metadata"].value("summary", s.summary);
  }
  if (s.author.empty() && j.contains("booksMetadata") &&
      j["booksMetadata"].is_object()) {
    auto authors = j["booksMetadata"].value("authors", Json::array());
    if (authors.is_array())
      for (const auto &author : authors) {
        if (!author.is_object())
          continue;
        auto name = author.value("name", "");
        if (!name.empty())
          s.author += (s.author.empty() ? "" : " / ") + name;
      }
  }
  s.id = id;
  return s;
}
std::vector<Chapter> parse_chapters(const Json &j) {
  if (!j.contains("content") || !j["content"].is_array())
    throw std::runtime_error("章节接口数据格式已改变");
  std::vector<Chapter> out;
  for (const auto &v : j["content"]) {
    int pages = 0;
    if (v.contains("media") && v["media"].is_object())
      pages = v["media"].value("pagesCount", 0);
    auto id = field(v, "id");
    segment(id);
    if (pages > 0 && pages <= 10000)
      out.push_back({id, field(v, "number"), pages});
  }
  return out;
}
std::vector<Chapter> ComicApi::chapters(const std::string &id) const {
  segment(id);
  Json condition = {
      {"condition",
       {{"allOf", Json::array({Json{
                      {"seriesId", {{"operator", "is"}, {"value", id}}}}})}}}};
  auto body = condition.dump();
  std::vector<Chapter> out;
  std::set<std::string> seen;
  for (int page = 0; page < 100; ++page) {
    auto j = request_json(
        *this,
        base + "/api/v1/books/list?page=" + std::to_string(page) + "&size=100",
        &body);
    auto batch = parse_chapters(j);
    size_t added = 0;
    for (auto &c : batch)
      if (seen.insert(c.id).second) {
        out.push_back(c);
        ++added;
      }
    if (j.value("last", false) || j["content"].size() < 100 || added == 0)
      break;
    if (page == 99)
      throw std::runtime_error("章节过多，请缩小范围");
  }
  std::stable_sort(out.begin(), out.end(),
                   [](const Chapter &a, const Chapter &b) {
                     return chapter_number(a.number) < chapter_number(b.number);
                   });
  return out;
}
std::string ComicApi::image(const std::string &id, int page) const {
  std::string url = base + (page ? "/api/v1/books/" : "/api/v1/series/") +
                    segment(id) +
                    (page ? "/pages/" + std::to_string(page) : "/thumbnail");
  auto r = api_request(url, network, key);
  if (r.content_type.find("image/") != 0)
    throw std::runtime_error("服务器没有返回图片，请重试或切换章节");
  return std::move(r.body);
}
Shelf::Shelf(std::string filename) : path(std::move(filename)) {
  // Recover the previous complete save if power was lost during replacement.
  for (const auto &candidate : {path, path + ".bak"}) {
    std::ifstream in(candidate);
    if (!in)
      continue;
    try {
      Json j;
      in >> j;
      if (!j.is_object() || !j.contains("favorites") ||
          !j["favorites"].is_object() || !j.contains("history") ||
          !j["history"].is_object())
        continue;
      data = std::move(j);
      break;
    } catch (const Json::exception &) {
    }
  }
}
void Shelf::save() const {
  const std::string tmp = path + ".tmp", backup = path + ".bak";
  {
    std::ofstream out(tmp, std::ios::trunc);
    out << data.dump(2);
    out.flush();
    if (!out)
      throw std::runtime_error("无法保存书架，请检查 SD 卡剩余空间");
  }
  // Switch fsdev rename does not replace an existing destination. Rotate the
  // old complete file first and retain it until the new file is installed.
  bool previous = static_cast<bool>(std::ifstream(path));
  if (previous) {
    if (std::remove(backup.c_str()) != 0 && errno != ENOENT)
      throw std::runtime_error("无法更新书架备份");
    if (std::rename(path.c_str(), backup.c_str()) != 0)
      throw std::runtime_error("无法备份书架文件");
  }
  if (std::rename(tmp.c_str(), path.c_str()) != 0) {
    if (previous)
      std::rename(backup.c_str(), path.c_str());
    throw std::runtime_error("无法更新书架文件");
  }
  std::remove(backup.c_str());
}

bool Shelf::favorite(const std::string &id) const {
  return data["favorites"].contains(id);
}
void Shelf::toggle(const Series &s) {
  Json before = data;
  if (favorite(s.id))
    data["favorites"].erase(s.id);
  else
    data["favorites"][s.id] = series_json(s);
  try {
    save();
  } catch (...) {
    data = std::move(before);
    throw;
  }
}
void Shelf::progress(const Series &s, const Chapter &c, int page) {
  auto j = series_json(s);
  j["chapter"] = c.id;
  j["page"] = std::clamp(page, 1, c.pages);
  j["time"] = std::time(nullptr);
  data["history"][s.id] = j;
  save();
}
std::vector<Series> Shelf::list(bool favorites) const {
  std::vector<Json> ordered;
  for (const auto &v : data[favorites ? "favorites" : "history"])
    if (v.is_object())
      ordered.push_back(v);
  if (!favorites)
    std::stable_sort(
        ordered.begin(), ordered.end(), [](const Json &a, const Json &b) {
          return a.value("time", int64_t{0}) > b.value("time", int64_t{0});
        });
  std::vector<Series> out;
  for (const auto &v : ordered)
    out.push_back(series_from_json(v));
  return out;
}
