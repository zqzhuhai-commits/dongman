#include "site.hpp"
#include <cassert>
#include <iostream>
#include <fstream>
#include <unistd.h>
int main() {
  Config c;
  const std::string base = "https://example.org/";
  assert(absolute_url(base, "//cdn.org/a.jpg") == "https://cdn.org/a.jpg");
  assert(absolute_url("https://example.org/dir/a.html", "../b.html") ==
         "https://example.org/b.html");
  assert(absolute_url(base, "javascript:alert(1)").empty());
  assert(absolute_url(base, "/a?token=1&b=2") ==
         "https://example.org/a?token=1&b=2");
  assert(absolute_url("https://example.org/h/1/?page=1", "?page=2") ==
         "https://example.org/h/1/?page=2");
  assert(absolute_url("https://example.org/a?x=1#old", "#new") ==
         "https://example.org/a?x=1#new");
  assert(absolute_url("https://example.org?x=1", "?page=2") ==
         "https://example.org/?page=2");
  assert(absolute_url(base, "a/..") == base);
  assert(absolute_url(base, "a/.") == base + "a/");
  auto list = parse_catalog(
      R"(<a title='动漫 &amp; 世界' href='/voddetail/1.html'><img data-original='//cdn.org/a.jpg'></a><a href='/voddetail/1.html'><img src='/duplicate.jpg'></a><a href='/ad'><img src='/ad.jpg'></a>)",
      base, c);
  assert(list.size() == 1 && list[0].title == "动漫 & 世界" &&
         list[0].cover == "https://cdn.org/a.jpg");
  auto actual_shape = parse_catalog(
      R"(<article class="u-movie"><a title="示例番剧" href="/p/63232/" target="_blank"><img data-original="https://cdn.org/poster.webp" class="thumb"></a></article>)",
      c.base, c);
  assert(actual_shape.size() == 1 &&
         actual_shape[0].url == "https://omofun.org.cn/p/63232/");
  auto episodes = parse_episodes(
      R"(<a href='/vodplay/1-1.html'><span>第1集</span></a><a href='/vodplay/1-2.html'>第2集</a><a href='/vodplay/1-1.html'>重复</a>)",
      base, c);
  assert(episodes.size() == 2 && episodes[0].title == "第1集");
  episodes = parse_episodes(
      R"html(<section class="tab"><a href="/v/63232-1-1/" onclick="play(this)">第01集</a></section><section class="tab"><a href="/v/63232-2-1/">第01集</a></section>)html",
      c.base, c);
  assert(episodes.size() == 2 &&
         episodes[0].title == "播放源 1 · 第01集" &&
         episodes[1].title == "播放源 2 · 第01集");
  assert(parse_media(R"(<video src='//cdn.org/v.mp4?token=1&amp;x=2'></video>)",
                     base) == "https://cdn.org/v.mp4?token=1&x=2");
  assert(
      parse_media(
          R"(<script>var player_aaaa={"encrypt":0,"url":"https:\/\/cdn.org\/v.m3u8?token=1"};</script>)",
          base) == "https://cdn.org/v.m3u8?token=1");
  assert(
      parse_media(
          R"(var player_x={"encrypt":1,"url":"https%3A%2F%2Fcdn.org%2Fv.mp4"})",
          base) == "https://cdn.org/v.mp4");
  assert(
      parse_media(
          R"(var player_x={"encrypt":2,"url":"aHR0cHM6Ly9jZG4ub3JnL3YubXA0"})",
          base) == "https://cdn.org/v.mp4");
  assert(parse_media("var player_aaaa={\n\"encrypt\":0,\n"
                     "\"url\":\"https://cdn.org/v.m3u8\"\n}", base) ==
         "https://cdn.org/v.m3u8");
  bool failed = false;
  try {
    parse_media("<iframe src='/js-player'></iframe>", base);
  } catch (...) {
    failed = true;
  }
  assert(failed);
  failed = false;
  try {
    parse_media(R"(var player_x={"url":"https://example.org/not-a-stream"})",
                base);
  } catch (...) {
    failed = true;
  }
  assert(failed);
  assert(parse_catalog("404 Not Found", base, c).empty());
  char path[] = "/tmp/dongman-config-XXXXXX";
  int fd = mkstemp(path);
  assert(fd >= 0);
  close(fd);
  {
    std::ofstream out(path);
    out << "\xef\xbb\xbf  # comment\r\n base_url = https://example.org/ \r\n"
           " listing_path = /h/1/?page=1 \r\n ca_file = \r\n";
  }
  c.load(path);
  assert(c.base == base && c.listing == "/h/1/?page=1" && c.ca_file.empty());
  {
    std::ofstream out(path);
    out << "base_url=https://changed.org/\ndetail_pattern=[\n";
  }
  failed = false;
  try { c.load(path); } catch (const std::exception &) { failed = true; }
  assert(failed && c.base == base); // Invalid reload must preserve prior config.
  unlink(path);
  failed = false;
  try { c.load(path); } catch (const std::exception &) { failed = true; }
  assert(failed);
  // No SD files: defaults and the application-owned CA must be sufficient.
  auto internal = runtime_config(path, "romfs/cacert.pem");
  assert(internal.base == "https://omofun.org.cn/" &&
         internal.ca_file == "romfs/cacert.pem");
  {
    std::ofstream out(path);
    out << "base_url=https://example.org/\n"
           "ca_file=sdmc:/switch/dongman/cacert.pem\n";
  }
  auto legacy = runtime_config(path, "romfs/cacert.pem");
  assert(legacy.base == base && legacy.ca_file == "romfs/cacert.pem");
  {
    std::ofstream out(path);
    out << "ca_file=romfs/cacert.pem\n";
  }
  assert(runtime_config(path, "/missing-bundle").ca_file == "romfs/cacert.pem");
  unlink(path);
  failed = false;
  try { runtime_config(path, "/missing-bundle"); }
  catch (const std::exception &) { failed = true; }
  assert(failed); // Never solve a missing CA by disabling TLS verification.
  std::cout << "All parser tests passed\n";
}
