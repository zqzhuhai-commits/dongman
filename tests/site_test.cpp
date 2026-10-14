#include "site.hpp"
#include <cassert>
#include <iostream>
int main(){
 Config c;const std::string base="https://example.org/";
 assert(absolute_url(base,"//cdn.org/a.jpg")=="https://cdn.org/a.jpg");
 assert(absolute_url("https://example.org/dir/a.html","../b.html")=="https://example.org/b.html");
 assert(absolute_url(base,"javascript:alert(1)").empty());
 assert(absolute_url(base,"/a?token=1&b=2")=="https://example.org/a?token=1&b=2");
 auto list=parse_catalog(R"(<a title='动漫 &amp; 世界' href='/voddetail/1.html'><img data-original='//cdn.org/a.jpg'></a><a href='/voddetail/1.html'><img src='/duplicate.jpg'></a><a href='/ad'><img src='/ad.jpg'></a>)",base,c);
 assert(list.size()==1&&list[0].title=="动漫 & 世界"&&list[0].cover=="https://cdn.org/a.jpg");
 auto episodes=parse_episodes(R"(<a href='/vodplay/1-1.html'><span>第1集</span></a><a href='/vodplay/1-2.html'>第2集</a><a href='/vodplay/1-1.html'>重复</a>)",base,c);assert(episodes.size()==2&&episodes[0].title=="第1集");
 assert(parse_media(R"(<video src='//cdn.org/v.mp4?token=1&amp;x=2'></video>)",base)=="https://cdn.org/v.mp4?token=1&x=2");
 assert(parse_media(R"(<script>var player_aaaa={"encrypt":0,"url":"https:\/\/cdn.org\/v.m3u8?token=1"};</script>)",base)=="https://cdn.org/v.m3u8?token=1");
 assert(parse_media(R"(var player_x={"encrypt":1,"url":"https%3A%2F%2Fcdn.org%2Fv.mp4"})",base)=="https://cdn.org/v.mp4");
 assert(parse_media(R"(var player_x={"encrypt":2,"url":"aHR0cHM6Ly9jZG4ub3JnL3YubXA0"})",base)=="https://cdn.org/v.mp4");
 bool failed=false;try{parse_media("<iframe src='/js-player'></iframe>",base);}catch(...){failed=true;}assert(failed);
 failed=false;try{parse_media(R"(var player_x={"url":"https://example.org/not-a-stream"})",base);}catch(...){failed=true;}assert(failed);
 assert(parse_catalog("404 Not Found",base,c).empty());
 std::cout<<"All parser tests passed\n";
}
