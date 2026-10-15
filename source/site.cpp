#include "site.hpp"
#include <algorithm>
#include <cctype>
#include <fstream>
#include <regex>
#include <set>
#include <sstream>
#include <stdexcept>
namespace {
using Attrs = std::map<std::string, std::string>;
Attrs attrs(const std::string &s) {
  Attrs out;
  static const std::regex re(
      R"re(([a-zA-Z_:][-a-zA-Z0-9_:]*)\s*=\s*(?:"([^"]*)"|'([^']*)'|([^\s>]+)))re");
  for (auto i = std::sregex_iterator(s.begin(), s.end(), re);
       i != std::sregex_iterator(); ++i) {
    auto k = (*i)[1].str();
    for (char &c : k)
      c = std::tolower((unsigned char)c);
    out[k] = html_decode((*i)[2].matched   ? (*i)[2].str()
                         : (*i)[3].matched ? (*i)[3].str()
                                           : (*i)[4].str());
  }
  return out;
}
std::string plain(std::string s) {
  s = std::regex_replace(s, std::regex("<[^>]*>"), "");
  s = html_decode(s);
  auto a = s.find_first_not_of(" \r\n\t");
  if (a == s.npos)
    return {};
  return s.substr(a, s.find_last_not_of(" \r\n\t") - a + 1);
}
std::string percent(const std::string &s) {
  std::string o;
  for (size_t i = 0; i < s.size(); ++i) {
    if (s[i] == '%' && i + 2 < s.size() &&
        std::isxdigit((unsigned char)s[i + 1]) &&
        std::isxdigit((unsigned char)s[i + 2])) {
      o += char(std::stoi(s.substr(i + 1, 2), nullptr, 16));
      i += 2;
    } else
      o += s[i];
  }
  return o;
}
std::string b64(const std::string &s) {
  const std::string table =
      "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
  std::string o;
  unsigned v = 0;
  int bits = -8;
  for (unsigned char c : s) {
    if (c == '=')
      break;
    auto n = table.find(c);
    if (n == table.npos) {
      if (std::isspace(c))
        continue;
      throw std::runtime_error("Invalid base64 player URL");
    }
    v = (v << 6) | (unsigned)n;
    bits += 6;
    if (bits >= 0) {
      o += char((v >> bits) & 255);
      bits -= 8;
    }
  }
  return o;
}
std::string unescape(const std::string &s) {
  std::string o;
  for (size_t i = 0; i < s.size(); ++i) {
    if (s[i] == '\\' && i + 1 < s.size()) {
      char c = s[++i];
      if (c == 'u' && i + 4 < s.size()) {
        auto h = s.substr(i + 1, 4);
        if (h == "0026" || h == "003d" || h == "002f") {
          o += char(std::stoi(h, nullptr, 16));
          i += 4;
        } else
          throw std::runtime_error("Unsupported Unicode escape in media URL");
      } else if (c == '/' || c == '\\' || c == '"')
        o += c;
      else
        throw std::runtime_error("Unsupported escape in media URL");
    } else
      o += s[i];
  }
  return o;
}
bool media(const std::string &u) {
  return std::regex_search(
      u, std::regex(R"(\.(?:m3u8|mp4|mkv|webm)(?:[?#]|$))", std::regex::icase));
}
} // namespace
void Config::load(const std::string &path) {
  std::ifstream in(path);
  if (!in)
    throw std::runtime_error("无法读取配置：" + path);
  Config next = *this;
  auto trim = [](std::string s) {
    auto first = s.find_first_not_of(" \t\r\n");
    return first == s.npos ? std::string{} :
           s.substr(first, s.find_last_not_of(" \t\r\n") - first + 1);
  };
  std::string s;
  while (std::getline(in, s)) {
    if (s.rfind("\xef\xbb\xbf", 0) == 0)
      s.erase(0, 3);
    s = trim(s);
    if (s.empty() || s[0] == '#')
      continue;
    auto p = s.find('=');
    if (p == s.npos)
      continue;
    auto k = trim(s.substr(0, p)), v = trim(s.substr(p + 1));
    if (k == "base_url")
      next.base = v;
    else if (k == "listing_path")
      next.listing = v;
    else if (k == "detail_pattern")
      next.detail_pattern = v;
    else if (k == "episode_pattern")
      next.episode_pattern = v;
    else if (k == "ca_file")
      next.ca_file = v;
  }
  if ((next.base.rfind("http://", 0) != 0 &&
       next.base.rfind("https://", 0) != 0) ||
      next.base.find_first_of("\r\n\t ") != next.base.npos ||
      absolute_url(next.base, "/").empty())
    throw std::runtime_error("base_url must be HTTP(S)");
  if (next.listing.empty() || next.detail_pattern.empty() ||
      next.episode_pattern.empty())
    throw std::runtime_error("目录路径与链接匹配规则不能为空");
  try {
    std::regex a(next.detail_pattern), b(next.episode_pattern);
  } catch (const std::regex_error &) {
    throw std::runtime_error("配置中的链接匹配正则表达式无效");
  }
  *this = std::move(next);
}
Config runtime_config(const std::string &optional_path,
                      const std::string &bundled_ca) {
  Config config; // Built-in site and parser defaults need no external INI.
  if (std::ifstream(optional_path).good())
    config.load(optional_path);
  // Old SD configurations may still point to a deleted external CA bundle.
  if (config.ca_file.empty() || !std::ifstream(config.ca_file).good()) {
    if (bundled_ca.empty() || !std::ifstream(bundled_ca).good())
      throw std::runtime_error("无法读取内置 CA 证书，请重新安装完整程序");
    config.ca_file = bundled_ca;
  }
  return config;
}
std::string html_decode(std::string s) {
  for (auto pair : {std::pair<const char *, const char *>("&amp;", "&"),
                    {"&quot;", "\""},
                    {"&#39;", "'"},
                    {"&apos;", "'"},
                    {"&lt;", "<"},
                    {"&gt;", ">"},
                    {"&nbsp;", " "}}) {
    size_t p = 0;
    while ((p = s.find(pair.first, p)) != s.npos) {
      s.replace(p, std::char_traits<char>::length(pair.first), pair.second);
      p += std::char_traits<char>::length(pair.second);
    }
  }
  return s;
}
std::string absolute_url(const std::string &base, const std::string &path) {
  if (path.empty())
    return {};
  if (path.rfind("http://", 0) == 0 || path.rfind("https://", 0) == 0)
    return path;
  auto scheme = base.find("://");
  if (scheme == base.npos)
    return {};
  auto end = base.find_first_of("/?#", scheme + 3);
  if (end == scheme + 3 || base.size() == scheme + 3)
    return {};
  if (path.rfind("//", 0) == 0)
    return base.substr(0, scheme) + ":" + path;
  if (path.find(':') != path.npos && path.find(':') < path.find_first_of("/?#"))
    return {};
  std::string origin = base.substr(0, end);
  if (path[0] == '#')
    return base.substr(0, base.find('#')) + path;
  if (path[0] == '?') {
    auto clean = base.substr(0, base.find_first_of("?#"));
    if (clean == origin)
      clean += '/';
    return clean + path;
  }
  std::string raw;
  if (path[0] == '/')
    raw = path;
  else {
    auto clean = base.substr(0, base.find_first_of("?#"));
    auto slash = clean.find_last_of('/');
    raw =
        (slash < scheme + 3 ? "/" : clean.substr(end, slash - end + 1)) + path;
  }
  auto q = raw.find_first_of("?#");
  auto suffix = q == raw.npos ? "" : raw.substr(q);
  raw = raw.substr(0, q);
  std::vector<std::string> parts;
  std::istringstream in(raw);
  std::string item;
  while (std::getline(in, item, '/')) {
    if (item == "..") {
      if (!parts.empty())
        parts.pop_back();
    } else if (!item.empty() && item != ".")
      parts.push_back(item);
  }
  std::string result = origin;
  for (auto &p : parts)
    result += "/" + p;
  if (parts.empty() || (!raw.empty() && (raw.back() == '/' ||
      raw == "." || raw == ".." ||
      (raw.size() >= 2 && raw.compare(raw.size() - 2, 2, "/.") == 0) ||
      (raw.size() >= 3 && raw.compare(raw.size() - 3, 3, "/..") == 0))))
    result += "/";
  return result + suffix;
}
std::vector<Anime> parse_catalog(const std::string &html,
                                 const std::string &base, const Config &cfg) {
  std::vector<Anime> out;
  std::set<std::string> seen;
  std::regex link(R"(<a\b([^>]*)>([\s\S]*?)</a\s*>)", std::regex::icase),
      img(R"(<img\b([^>]*)>)", std::regex::icase), pattern(cfg.detail_pattern);
  for (auto i = std::sregex_iterator(html.begin(), html.end(), link);
       i != std::sregex_iterator(); ++i) {
    auto a = attrs((*i)[1]);
    auto url = absolute_url(base, a["href"]);
    std::smatch m;
    auto body = (*i)[2].str();
    if (url.empty() || !std::regex_search(url, pattern) ||
        !std::regex_search(body, m, img) || !seen.insert(url).second)
      continue;
    auto b = attrs(m[1]);
    auto title = a["title"];
    if (title.empty())
      title = b["alt"];
    if (title.empty())
      title = plain(body);
    if (title.empty())
      title = "未命名动漫";
    auto cover = b["data-original"];
    if (cover.empty())
      cover = b["data-src"];
    if (cover.empty())
      cover = b["src"];
    out.push_back({title, url, absolute_url(base, cover)});
    if (out.size() >= 200)
      break;
  }
  return out;
}
std::vector<Episode> parse_episodes(const std::string &html,
                                    const std::string &base,
                                    const Config &cfg) {
  std::vector<Episode> out;
  std::set<std::string> seen;
  std::regex re(R"(<a\b([^>]*)>([\s\S]*?)</a\s*>)", std::regex::icase),
      pattern(cfg.episode_pattern), source(R"(/(?:v|vodplay|play)/[0-9]+-([0-9]+)-[0-9]+)");
  for (auto i = std::sregex_iterator(html.begin(), html.end(), re);
       i != std::sregex_iterator(); ++i) {
    auto a = attrs((*i)[1]);
    auto u = absolute_url(base, a["href"]);
    if (u.empty() || !std::regex_search(u, pattern) || !seen.insert(u).second)
      continue;
    auto title = plain((*i)[2]);
    if (title.empty())
      title = a["title"];
    std::smatch line;
    if (!title.empty())
      out.push_back({title, u, std::regex_search(u, line, source) ? line[1].str() : "1"});
    if (out.size() >= 2000)
      break;
  }
  return out;
}
std::vector<PlaySource> parse_sources(const std::string &html,
                                     const std::string &base, const Config &cfg) {
  std::vector<PlaySource> sources;
  for (auto &episode : parse_episodes(html, base, cfg)) {
    auto group = std::find_if(sources.begin(), sources.end(), [&](const PlaySource &s) {
      return s.id == episode.source;
    });
    if (group == sources.end()) {
      sources.push_back({episode.source, "播放源 " + episode.source, {}});
      group = sources.end() - 1;
    }
    group->episodes.push_back(std::move(episode));
  }
  return sources;
}
std::string encode_http_url(const std::string &url) {
  const char *hex = "0123456789ABCDEF";
  std::string out;
  for (unsigned char c : url) {
    if (c <= 32 || c >= 127 || std::string("\"<>\\^`{|}").find(c) != std::string::npos) {
      out += '%'; out += hex[c >> 4]; out += hex[c & 15];
    } else out += char(c);
  }
  return out;
}
void validate_media_prefix(const std::string &body, const std::string &url,
                           const std::string &content_type) {
  auto start = body.find_first_not_of(" \r\n\t");
  if (body.compare(0, 3, "\xef\xbb\xbf") == 0)
    start = body.find_first_not_of(" \r\n\t", 3);
  auto prefix = start == body.npos ? std::string{} : body.substr(start, 64);
  std::string lower = prefix, mime = content_type;
  for (char &c : lower) c = std::tolower((unsigned char)c);
  for (char &c : mime) c = std::tolower((unsigned char)c);
  if (prefix.empty())
    throw std::runtime_error("视频服务器返回空内容，请切换播放源");
  if (mime.find("text/html") != mime.npos || lower.rfind("<html", 0) == 0 ||
      lower.rfind("<!doctype", 0) == 0)
    throw std::runtime_error("视频服务器返回错误网页，请切换播放源");
  if (std::regex_search(url, std::regex(R"(\.m3u8(?:[?#]|$))", std::regex::icase)) &&
      prefix.rfind("#EXTM3U", 0) != 0)
    throw std::runtime_error("视频服务器未返回有效 HLS 播放列表，请切换播放源");
}
std::string parse_media(const std::string &html, const std::string &base) {
  std::regex tag(R"(<(?:video|source)\b([^>]*)>)", std::regex::icase);
  for (auto i = std::sregex_iterator(html.begin(), html.end(), tag);
       i != std::sregex_iterator(); ++i) {
    auto a = attrs((*i)[1]);
    auto u = absolute_url(base, a["src"]);
    if (!u.empty())
      return encode_http_url(u);
  }
  std::smatch p;
  std::regex player(R"((?:var\s+)?player_[A-Za-z0-9_]+\s*=\s*(\{[\s\S]*?\}))");
  if (std::regex_search(html, p, player)) {
    auto json = p[1].str();
    std::smatch m, e;
    std::regex url(R"re("url"\s*:\s*"((?:\\.|[^"\\])*)")re"),
        enc(R"re("encrypt"\s*:\s*"?([012])"?)re");
    if (std::regex_search(json, m, url)) {
      auto u = unescape(m[1]);
      int encoding = std::regex_search(json, e, enc) ? std::stoi(e[1]) : 0;
      if (encoding == 1)
        u = percent(u);
      if (encoding == 2)
        u = percent(b64(u));
      u = absolute_url(base, u);
      if (media(u))
        return encode_http_url(u);
    }
  }
  throw std::runtime_error("未找到直连媒体：可能需要 "
                           "JavaScript、鉴权或专用解析器。请提供播放页样本。");
}
