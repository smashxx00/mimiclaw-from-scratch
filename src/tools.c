#include "tools.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include <curl/curl.h>

#include "memory.h"

#define SEARCH_RESULT_MAX 4000

/* ---- HTTP GET 辅助（web_search 用） ---- */

typedef struct {
    char *data;
    size_t len;
    size_t cap;
} http_buf_t;

static size_t http_write(char *ptr, size_t size, size_t nmemb, void *userdata) {
    http_buf_t *buf = (http_buf_t *)userdata;
    size_t n = size * nmemb;
    if (buf->len + n + 1 > buf->cap) {
        size_t newcap = buf->cap ? buf->cap * 2 : 4096;
        while (newcap < buf->len + n + 1) newcap *= 2;
        char *p = (char *)realloc(buf->data, newcap);
        if (!p) return 0;
        buf->data = p;
        buf->cap = newcap;
    }
    memcpy(buf->data + buf->len, ptr, n);
    buf->len += n;
    buf->data[buf->len] = '\0';
    return n;
}

static int http_get(const char *url, const char *header, http_buf_t *out) {
    memset(out, 0, sizeof(*out));
    CURL *curl = curl_easy_init();
    if (!curl) return -1;

    struct curl_slist *hdrs = NULL;
    if (header) hdrs = curl_slist_append(hdrs, header);

    curl_easy_setopt(curl, CURLOPT_URL, url);
    curl_easy_setopt(curl, CURLOPT_HTTPHEADER, hdrs);
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, http_write);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, out);
    curl_easy_setopt(curl, CURLOPT_TIMEOUT, 30L);

    CURLcode rc = curl_easy_perform(curl);
    long http_code = 0;
    curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &http_code);
    curl_easy_cleanup(curl);
    curl_slist_free_all(hdrs);
    if (rc != CURLE_OK || http_code >= 400) {
        free(out->data);
        memset(out, 0, sizeof(*out));
        return -1;
    }
    return 0;
}

/* ---- 三个内置工具 ---- */

char *tool_get_time(const char *args_json, void *userdata) {
    (void)args_json;
    (void)userdata;
    time_t now = time(NULL);
    struct tm tmv;
    localtime_r(&now, &tmv);
    char buf[64];
    strftime(buf, sizeof(buf), "%Y-%m-%d %H:%M:%S %Z", &tmv);
    return strdup(buf);
}

char *tool_web_search(const char *args_json, void *userdata) {
    const char *search_key = (const char *)userdata;
    cJSON *args = cJSON_Parse(args_json);
    const char *query = args
        ? cJSON_GetStringValue(cJSON_GetObjectItemCaseSensitive(args, "query"))
        : NULL;
    if (!query || !*query) {
        cJSON_Delete(args);
        return strdup("error: missing query");
    }
    if (!search_key || !*search_key) {
        cJSON_Delete(args);
        return strdup("error: MIMI_SEARCH_KEY not configured");
    }

    char *escaped = curl_easy_escape(NULL, query, 0);
    char url[1024];
    snprintf(url, sizeof(url),
             "https://api.search.brave.com/res/v1/web/search?q=%s&count=3", escaped);
    curl_free(escaped);
    cJSON_Delete(args);

    char header[512];
    snprintf(header, sizeof(header), "X-Subscription-Token: %s", search_key);

    http_buf_t buf = {0};
    if (http_get(url, header, &buf) != 0) {
        return strdup("error: search API request failed");
    }

    cJSON *json = cJSON_Parse(buf.data);
    free(buf.data);
    if (!json) return strdup("error: failed to parse search response");

    cJSON *results = cJSON_GetObjectItemCaseSensitive(
        cJSON_GetObjectItemCaseSensitive(json, "web"), "results");

    size_t cap = 2048;
    char *out = (char *)malloc(cap);
    out[0] = '\0';
    size_t used = 0;

    if (cJSON_IsArray(results)) {
        int n = cJSON_GetArraySize(results);
        for (int i = 0; i < n; i++) {
            cJSON *r = cJSON_GetArrayItem(results, i);
            const char *title = cJSON_GetStringValue(
                cJSON_GetObjectItemCaseSensitive(r, "title"));
            const char *link = cJSON_GetStringValue(
                cJSON_GetObjectItemCaseSensitive(r, "url"));
            const char *desc = cJSON_GetStringValue(
                cJSON_GetObjectItemCaseSensitive(r, "description"));
            int need = snprintf(NULL, 0, "%d. %s\n   %s\n   %s\n\n",
                                i + 1, title ? title : "", link ? link : "",
                                desc ? desc : "");
            if (used + (size_t)need + 1 > cap) {
                cap = used + (size_t)need + 1;
                char *p = (char *)realloc(out, cap);
                out = p;
            }
            used += (size_t)snprintf(out + used, cap - used, "%d. %s\n   %s\n   %s\n\n",
                                     i + 1, title ? title : "", link ? link : "",
                                     desc ? desc : "");
            if (used > SEARCH_RESULT_MAX) break;
        }
    }
    cJSON_Delete(json);

    if (!used) {
        free(out);
        return strdup("no results");
    }
    return out;
}

char *tool_memory_write(const char *args_json, void *userdata) {
    const char *data_dir = (const char *)userdata;
    cJSON *args = cJSON_Parse(args_json);
    const char *content = args
        ? cJSON_GetStringValue(cJSON_GetObjectItemCaseSensitive(args, "content"))
        : NULL;
    if (!content) {
        cJSON_Delete(args);
        return strdup("error: missing content");
    }

    char *dup = strdup(content);   /* 先复制，args 删除后仍可用 */
    char err[256] = "";
    int rc = memory_save(data_dir, content, err, sizeof(err));
    cJSON_Delete(args);
    if (rc != 0) {
        size_t len = strlen(err) + 8;
        char *out = (char *)malloc(len);
        snprintf(out, len, "error: %s", err);
        free(dup);
        return out;
    }

    size_t len = strlen(dup) + 48;
    char *out = (char *)malloc(len);
    snprintf(out, len, "memory updated (%zu chars): %s", strlen(dup), dup);
    free(dup);
    return out;
}

/* ---- 注册表 ---- */

void tool_registry_init(tool_registry_t *reg) {
    memset(reg, 0, sizeof(*reg));
}

int tool_registry_add(tool_registry_t *reg, const char *name, const char *description,
                      const char *parameters_json, tool_exec_fn execute, void *userdata) {
    if (reg->count >= TOOL_REGISTRY_MAX) return -1;
    tool_t *t = &reg->items[reg->count++];
    t->name = name;
    t->description = description;
    t->parameters_json = parameters_json;
    t->execute = execute;
    t->userdata = userdata;
    return 0;
}

const tool_t *tool_registry_find(const tool_registry_t *reg, const char *name) {
    for (int i = 0; i < reg->count; i++) {
        if (strcmp(reg->items[i].name, name) == 0) return &reg->items[i];
    }
    return NULL;
}

cJSON *tool_registry_to_json(const tool_registry_t *reg) {
    cJSON *tools = cJSON_CreateArray();
    for (int i = 0; i < reg->count; i++) {
        const tool_t *t = &reg->items[i];
        cJSON *item = cJSON_CreateObject();
        cJSON_AddStringToObject(item, "name", t->name);
        cJSON_AddStringToObject(item, "description", t->description);
        cJSON *schema = cJSON_Parse(t->parameters_json);
        cJSON_AddItemToObject(item, "input_schema", schema ? schema : cJSON_CreateObject());
        cJSON_AddItemToArray(tools, item);
    }
    return tools;
}

void tools_builtin(tool_registry_t *reg, const char *data_dir, const char *search_key) {
    tool_registry_init(reg);
    tool_registry_add(reg,
        "get_time",
        "获取当前本地时间。参数：无。",
        "{\"type\":\"object\",\"properties\":{},\"required\":[]}",
        tool_get_time, (void *)data_dir);
    tool_registry_add(reg,
        "web_search",
        "搜索网络获取最新信息。参数：query（搜索关键词）。",
        "{\"type\":\"object\",\"properties\":{\"query\":{\"type\":\"string\"}},\"required\":[\"query\"]}",
        tool_web_search, (void *)search_key);
    tool_registry_add(reg,
        "memory_write",
        "把一条长期记忆写入 MEMORY.md，覆盖式更新。参数：content（记忆内容）。",
        "{\"type\":\"object\",\"properties\":{\"content\":{\"type\":\"string\"}},\"required\":[\"content\"]}",
        tool_memory_write, (void *)data_dir);
}
