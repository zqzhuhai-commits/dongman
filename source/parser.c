#include "parser.h"
#include "config.h"

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <ctype.h>

// ============================================================
//  内部工具函数
// ============================================================

// 在 html 中找 pattern, 返回指向匹配内容的指针, 未找到返回 NULL
static const char *find_str(const char *html, const char *pattern) {
    return strstr(html, pattern);
}

// 提取引号内的字符串: 从 start=" 开始, 到下一个 " 结束
// 写入 dst, 返回 end 指针
static const char *extract_quoted(const char *start, char *dst, size_t dst_size) {
    if (!start || *start != '"') return NULL;
    start++;
    size_t i = 0;
    while (*start && *start != '"' && i < dst_size - 1) {
        dst[i++] = *start++;
    }
    dst[i] = '\0';
    return start;  // 指向 closing "
}

// 提取 href="/p/123/" 中的数字 ID
static int extract_p_id(const char *href) {
    const char *p = strstr(href, "/p/");
    if (!p) return 0;
    p += 3;
    int id = 0;
    while (*p >= '0' && *p <= '9') {
        id = id * 10 + (*p - '0');
        p++;
    }
    return id;
}

// 解码 HTML 实体 (最小化: &amp; &lt; &gt; &quot; &#39;)
static void html_decode(char *s) {
    char *r = s, *w = s;
    while (*r) {
        if (*r == '&') {
            if (strncmp(r, "&amp;", 5) == 0)       { *w++ = '&'; r += 5; }
            else if (strncmp(r, "&lt;", 4) == 0)   { *w++ = '<'; r += 4; }
            else if (strncmp(r, "&gt;", 4) == 0)   { *w++ = '>'; r += 4; }
            else if (strncmp(r, "&quot;", 6) == 0) { *w++ = '"'; r += 6; }
            else if (strncmp(r, "&#39;", 5) == 0)  { *w++ = '\''; r += 5; }
            else if (strncmp(r, "&nbsp;", 6) == 0) { *w++ = ' '; r += 6; }
            else { *w++ = *r++; }
        } else {
            *w++ = *r++;
        }
    }
    *w = '\0';
}

// ============================================================
//  番剧列表解析
// ============================================================
int parse_anime_list(const char *html, AnimeItem *items, int max_items) {
    int count = 0;
    const char *p = html;

    while (count < max_items) {
        // 找下一个 <article class="u-movie">
        p = find_str(p, "<article class=\"u-movie\">");
        if (!p) break;

        // 找 title="..."
        const char *t = find_str(p, "title=\"");
        if (t) {
            t += 7;
            size_t i = 0;
            while (*t && *t != '"' && i < MAX_TITLE_LEN - 1) {
                items[count].title[i++] = *t++;
            }
            items[count].title[i] = '\0';
            html_decode(items[count].title);
        }

        // 找 href="/p/123/"
        const char *h = find_str(p, "href=\"/p/");
        if (h) {
            items[count].id = extract_p_id(h);
        }

        // 找 data-original="封面URL"
        const char *img = find_str(p, "data-original=\"");
        if (img) {
            img += 15;
            size_t i = 0;
            while (*img && *img != '"' && i < MAX_POSTER_URL - 1) {
                items[count].poster[i++] = *img++;
            }
            items[count].poster[i] = '\0';
        }

        // 找评分 <span>4.0分</span>
        const char *rate = find_str(p, "pingfen");
        if (rate) {
            rate = find_str(rate, "<span>");
            if (rate) {
                rate += 6;
                size_t i = 0;
                while (*rate && *rate != '<' && i < 7) {
                    items[count].rating[i++] = *rate++;
                }
                items[count].rating[i] = '\0';
            }
        }

        // 找状态
        const char *st = find_str(p, "zhuangtai");
        if (st) {
            st = find_str(st, "<span>");
            if (st) {
                st += 6;
                size_t i = 0;
                while (*st && *st != '<' && i < 31) {
                    items[count].status[i++] = *st++;
                }
                items[count].status[i] = '\0';
            }
        }

        count++;
        p += 20;  // 跳过当前 article 开始位置
    }

    return count;
}

// ============================================================
//  详情页解析
// ============================================================
bool parse_detail(const char *html, int anime_id, AnimeInfo *anime) {
    memset(anime, 0, sizeof(AnimeInfo));
    anime->id = anime_id;

    // 标题: <h1 class="article-title"><a ...>xxx在线观看</a>
    const char *t = find_str(html, "article-title");
    if (t) {
        t = find_str(t, "<a");
        if (t) {
            t = find_str(t, ">");
            if (t) {
                t++;
                size_t i = 0;
                while (*t && *t != '<' && i < MAX_TITLE_LEN - 1) {
                    anime->title[i++] = *t++;
                }
                anime->title[i] = '\0';
                // 去掉 "在线观看" 后缀
                char *suffix = strstr(anime->title, "在线观看");
                if (suffix) *suffix = '\0';
                html_decode(anime->title);
            }
        }
    }

    // 封面: <div class="video_img"><img src="...">
    const char *img = find_str(html, "video_img");
    if (img) {
        img = find_str(img, "src=\"");
        if (img) {
            img += 5;
            size_t i = 0;
            while (*img && *img != '"' && i < MAX_POSTER_URL - 1) {
                anime->poster[i++] = *img++;
            }
            anime->poster[i] = '\0';
        }
    }

    // 评分: <strong>评分:</strong>4.0
    const char *rate = find_str(html, "评分:");
    if (rate) {
        rate += 4;
        while (*rate == ' ' || *rate == '<') rate++;
        size_t i = 0;
        while (*rate && *rate != '<' && *rate != '\n' && i < 7) {
            anime->rating[i++] = *rate++;
        }
        anime->rating[i] = '\0';
    }

    // 剧情简介: <p class="jianjie"><span>...</span>
    const char *desc = find_str(html, "jianjie");
    if (desc) {
        desc = find_str(desc, "<span>");
        if (desc) {
            desc += 6;
            size_t i = 0;
            while (*desc && *desc != '<' && i < MAX_DESC_LEN - 1) {
                anime->desc[i++] = *desc++;
            }
            anime->desc[i] = '\0';
            html_decode(anime->desc);
        }
    }

    // 分集: 找所有 <h4 class="ctitle"><strong>线路X在线播放:</strong></h4>
    // 后面跟 <div id="video_list_li" class="video_list_li">
    const char *p = html;
    int line_idx = 0;

    while ((p = find_str(p, "ctitle")) != NULL && line_idx < 8) {
        // 提取线路名
        const char *name_start = find_str(p, "<strong>");
        if (!name_start) { p += 6; continue; }
        name_start += 8;
        size_t ni = 0;
        while (*name_start && *name_start != '<' && ni < 31) {
            anime->lines[line_idx].name[ni++] = *name_start++;
        }
        anime->lines[line_idx].name[ni] = '\0';

        // 跳到这个线路对应的 video_list_li
        p = find_str(p, "video_list_li");
        if (!p) break;
        p = find_str(p, ">");
        if (p) p++;

        // 在这个 div 内找所有 <a href="/v/...">
        int ep_idx = 0;
        const char *ep_start = p;
        // 简单方式: 从当前位置向后扫描 <a href="/v/
        while (ep_idx < MAX_EPISODES) {
            const char *a = find_str(ep_start, "<a href=\"/v/");
            if (!a) break;

            // 解析 /v/70172-1-1/
            a += 9;  // 跳过 <a href="/v/
            int aid, lid, epid;
            if (sscanf(a, "%d-%d-%d", &aid, &lid, &epid) == 3) {
                anime->lines[line_idx].line_id = lid;
                anime->lines[line_idx].episodes[ep_idx].episode_num = epid;
                snprintf(anime->lines[line_idx].episodes[ep_idx].play_url,
                         MAX_URL_LEN, "/v/%d-%d-%d/", aid, lid, epid);

                // 提取集数标题
                const char *tt = find_str(a, ">");
                if (tt) {
                    tt++;
                    size_t ti = 0;
                    while (*tt && *tt != '<' && ti < 63) {
                        anime->lines[line_idx].episodes[ep_idx].title[ti++] = *tt++;
                    }
                    anime->lines[line_idx].episodes[ep_idx].title[ti] = '\0';
                }
                ep_idx++;
            }
            ep_start = a + 5;
        }

        anime->lines[line_idx].ep_count = ep_idx;
        if (ep_idx > 0) line_idx++;
        p = ep_start;
    }

    anime->line_count = line_idx;
    return true;
}

// ============================================================
//  播放页 m3u8 提取
// ============================================================
bool parse_play_url(const char *html, char *m3u8_url, size_t url_size) {
    // 找 var player_aaaa={ ... "url":"...m3u8..." ... }
    const char *p = find_str(html, "player_aaaa");
    if (!p) return false;

    // 找 "url":"
    p = find_str(p, "\"url\"");
    if (!p) return false;

    p = find_str(p, ":");
    if (!p) return false;
    p++;  // 跳过 :
    while (*p == ' ' || *p == '"') p++;

    // 提取 URL
    size_t i = 0;
    while (*p && *p != '"' && *p != ',' && *p != '}' && i < url_size - 1) {
        // 处理转义 \/
        if (*p == '\\' && *(p+1) == '/') {
            m3u8_url[i++] = '/';
            p += 2;
        } else {
            m3u8_url[i++] = *p++;
        }
    }
    m3u8_url[i] = '\0';

    // 验证是否是 m3u8
    return (strstr(m3u8_url, ".m3u8") != NULL);
}
