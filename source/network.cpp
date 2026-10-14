#include "network.hpp"
#include <curl/curl.h>
#include <stdexcept>
std::atomic<bool> cancelling{false};
namespace {
struct Buffer {std::string data;size_t limit;};
size_t write(char* data,size_t size,size_t count,void* p){auto& b=*static_cast<Buffer*>(p);size_t n=size*count;if(n>b.limit-b.data.size())return 0;try{b.data.append(data,n);}catch(...){return 0;}return n;}
int progress(void*,curl_off_t,curl_off_t,curl_off_t,curl_off_t){return cancelling?1:0;}
}
Response get(const std::string& url,const std::string& referer,const Config& c,size_t limit){
 if(cancelling)throw std::runtime_error("请求已取消");
 CURL* h=curl_easy_init();if(!h)throw std::runtime_error("无法初始化网络");Buffer b{{},limit};char error[CURL_ERROR_SIZE]={};
 curl_easy_setopt(h,CURLOPT_URL,url.c_str());curl_easy_setopt(h,CURLOPT_USERAGENT,"Mozilla/5.0 AnimeRepublicSwitch/0.1");curl_easy_setopt(h,CURLOPT_REFERER,referer.c_str());
 curl_easy_setopt(h,CURLOPT_FOLLOWLOCATION,1L);curl_easy_setopt(h,CURLOPT_MAXREDIRS,5L);curl_easy_setopt(h,CURLOPT_CONNECTTIMEOUT,8L);curl_easy_setopt(h,CURLOPT_TIMEOUT,limit==2*1024*1024?5L:20L);curl_easy_setopt(h,CURLOPT_NOSIGNAL,1L);
 curl_easy_setopt(h,CURLOPT_PROTOCOLS_STR,"http,https");curl_easy_setopt(h,CURLOPT_REDIR_PROTOCOLS_STR,"http,https");curl_easy_setopt(h,CURLOPT_ACCEPT_ENCODING,"");
 curl_easy_setopt(h,CURLOPT_WRITEFUNCTION,write);curl_easy_setopt(h,CURLOPT_WRITEDATA,&b);curl_easy_setopt(h,CURLOPT_ERRORBUFFER,error);curl_easy_setopt(h,CURLOPT_NOPROGRESS,0L);curl_easy_setopt(h,CURLOPT_XFERINFOFUNCTION,progress);
 if(!c.ca_file.empty())curl_easy_setopt(h,CURLOPT_CAINFO,c.ca_file.c_str());
 CURLcode code=curl_easy_perform(h);long status=0;char* effective=nullptr;curl_easy_getinfo(h,CURLINFO_RESPONSE_CODE,&status);curl_easy_getinfo(h,CURLINFO_EFFECTIVE_URL,&effective);std::string final=effective?effective:url;curl_easy_cleanup(h);
 if(code!=CURLE_OK)throw std::runtime_error(std::string("网络失败：")+(error[0]?error:curl_easy_strerror(code)));
 if(status<200||status>=300)throw std::runtime_error("网站返回 HTTP "+std::to_string(status)+"："+final);
 return {std::move(b.data),std::move(final)};
}
