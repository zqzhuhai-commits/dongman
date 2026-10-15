#include "comic.hpp"
#include <cassert>
#include <cstdio>
#include <iostream>
int main(int argc, char **argv) {
  if (argc == 2) {
    ComicApi api;
    api.load(argv[1]);
    auto list = api.catalog("all");
    assert(list.size() == 2 && list[0].id == "series-1");
    assert(api.detail(list[0].id).title == "Test comic");
    auto chapters = api.chapters(list[0].id);
    assert(chapters.size() == 101);
    assert(chapters.front().number == "1");
    assert(chapters.back().number == "101");
    assert(api.image(chapters[0].id, 1) == "image-fixture");
    bool bad = false;
    try {
      api.detail("../escape");
    } catch (const std::exception &) {
      bad = true;
    }
    assert(bad);
    std::cout
        << "Comic protocol, pagination and numeric chapter ordering passed\n";
    return 0;
  }
  Json j = {
      {"content",
       Json::array(
           {{{"id", "b"}, {"number", "2.5"}, {"media", {{"pagesCount", 3}}}},
            {{"id", "empty"}, {"media", {{"pagesCount", 0}}}}})}};
  assert(parse_chapters(j).size() == 1);
  std::string path =
      "/tmp/dongman-shelf-test-" + std::to_string(std::time(nullptr)) + ".json";
  std::remove(path.c_str());
  {
    Shelf shelf(path);
    Series s{"one", "Comic", "Author", ""};
    shelf.toggle(s);
    assert(shelf.favorite("one"));
    shelf.progress(s, {"chapter", "1", 10}, 99);
  }
  {
    Shelf shelf(path);
    assert(shelf.favorite("one"));
    assert(shelf.list(false).size() == 1);
    assert(shelf.data["history"]["one"]["page"] == 10);
    shelf.toggle({"one", "Comic", "Author", ""});
    assert(!shelf.favorite("one"));
  }
  std::remove(path.c_str());
  std::cout << "Comic parsing and persisted favorites/resume passed\n";
}
