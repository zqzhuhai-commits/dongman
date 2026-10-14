#pragma once
#include <string>
#include <atomic>
#include "site.hpp"
extern std::atomic<bool> cancelling;
struct Response { std::string body, url; };
Response get(const std::string& url,const std::string& referer,const Config& config,size_t limit=8*1024*1024);
