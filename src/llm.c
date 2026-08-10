#include "llm.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <curl/curl.h>

#include "config.h"

/* 响应体缓冲 */
typedef struct {
    char *data;
    size_t len;
    size_t cap;
} resp_buf_t;

static size_t write_cb(char *ptr, size_t size, size_t nmemb, void *userdata) {
    resp_buf_t *buf = (resp_buf_t *)userdata;
    size_t n = size * nmemb;

    if (buf->len + n + 1 > buf->cap) {
        size_t newcap = buf->cap ? buf->cap * 2 : 4096;
        while (newcap < buf->len + n + 1) newcap *= 2;
        if (newcap > MIMI_RESPONSE_MAX_LEN) return 0;  /* 防止失控 */
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

int llm_chat(const llm_config_t *cfg, const char *system, cJSON *messages,
             cJSON *tools, llm_response_t *out, char *err, size_t errlen) {
    memset(out, 0, sizeof(*out));
    if (errlen) err[0] = '\0';

    /* 组装请求体 */
    cJSON *body = cJSON_CreateObject();
    cJSON_AddStringToObject(body, "model",
                            (cfg->model && *cfg->model) ? cfg->model : MIMI_LLM_DEFAULT_MODEL);
    cJSON_AddNumberToObject(body, "max_tokens",
                            cfg->max_tokens > 0 ? cfg->max_tokens : MIMI_LLM_MAX_TOKENS);
    if (system && *system) cJSON_AddStringToObject(body, "system", system);
    if (tools) cJSON_AddItemToObject(body, "tools", cJSON_Duplicate(tools, 1));
    cJSON_AddItemToObject(body, "messages", cJSON_Duplicate(messages, 1));

    char *payload = cJSON_PrintUnformatted(body);
    cJSON_Delete(body);
    if (!payload) {
        snprintf(err, errlen, "json encode failed");
        return -1;
    }

    const char *url = (cfg->base_url && *cfg->base_url) ? cfg->base_url : MIMI_LLM_API_URL;

    struct curl_slist *hdrs = NULL;
    hdrs = curl_slist_append(hdrs, "content-type: application/json");
    if (cfg->api_key && *cfg->api_key) {
        char auth[512];
        snprintf(auth, sizeof(auth), "x-api-key: %s", cfg->api_key);
        hdrs = curl_slist_append(hdrs, auth);
        char ver[128];
        snprintf(ver, sizeof(ver), "anthropic-version: %s", MIMI_LLM_API_VERSION);
        hdrs = curl_slist_append(hdrs, ver);
    }

    resp_buf_t rbuf = {0};
    CURL *curl = curl_easy_init();
    if (!curl) {
        curl_slist_free_all(hdrs);
        free(payload);
        snprintf(err, errlen, "curl init failed");
        return -1;
    }

    curl_easy_setopt(curl, CURLOPT_URL, url);
    curl_easy_setopt(curl, CURLOPT_POST, 1L);
    curl_easy_setopt(curl, CURLOPT_POSTFIELDS, payload);
    curl_easy_setopt(curl, CURLOPT_HTTPHEADER, hdrs);
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_cb);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, &rbuf);
    curl_easy_setopt(curl, CURLOPT_TIMEOUT, 60L);

    CURLcode rc = curl_easy_perform(curl);
    long http_code = 0;
    curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &http_code);
    curl_easy_cleanup(curl);
    curl_slist_free_all(hdrs);
    free(payload);

    if (rc != CURLE_OK) {
        snprintf(err, errlen, "network error: %s", curl_easy_strerror(rc));
        free(rbuf.data);
        return -1;
    }
    if (http_code >= 400) {
        const char *body_preview = rbuf.data ? rbuf.data : "(no body)";
        snprintf(err, errlen, "API %ld: %.500s", http_code, body_preview);
        free(rbuf.data);
        return -1;
    }
    if (!rbuf.data) {
        snprintf(err, errlen, "API returned empty body");
        return -1;
    }

    /* 解析响应 */
    cJSON *json = cJSON_Parse(rbuf.data);
    free(rbuf.data);
    if (!json) {
        snprintf(err, errlen, "failed to parse API response");
        return -1;
    }

    const char *stop = cJSON_GetStringValue(
        cJSON_GetObjectItemCaseSensitive(json, "stop_reason"));
    snprintf(out->stop_reason, sizeof(out->stop_reason), "%s", stop ? stop : "end_turn");

    cJSON *content = cJSON_GetObjectItemCaseSensitive(json, "content");
    if (cJSON_IsArray(content)) {
        int n = cJSON_GetArraySize(content);
        out->blocks = (block_t *)calloc((size_t)n, sizeof(block_t));
        for (int i = 0; i < n; i++) {
            cJSON *b = cJSON_GetArrayItem(content, i);
            const char *type = cJSON_GetStringValue(
                cJSON_GetObjectItemCaseSensitive(b, "type"));
            if (!type) continue;

            if (strcmp(type, "text") == 0) {
                const char *text = cJSON_GetStringValue(
                    cJSON_GetObjectItemCaseSensitive(b, "text"));
                if (!text) continue;
                block_t *blk = &out->blocks[out->block_count++];
                blk->kind = BLOCK_TEXT;
                blk->text = strdup(text);
                size_t old = out->text ? strlen(out->text) : 0;
                char *p = (char *)realloc(out->text, old + strlen(text) + 1);
                out->text = p;
                memcpy(p + old, text, strlen(text) + 1);
            } else if (strcmp(type, "tool_use") == 0) {
                block_t *blk = &out->blocks[out->block_count++];
                blk->kind = BLOCK_TOOL_USE;
                const char *id = cJSON_GetStringValue(
                    cJSON_GetObjectItemCaseSensitive(b, "id"));
                const char *name = cJSON_GetStringValue(
                    cJSON_GetObjectItemCaseSensitive(b, "name"));
                cJSON *input = cJSON_GetObjectItemCaseSensitive(b, "input");
                char *printed = input ? cJSON_PrintUnformatted(input) : NULL;
                blk->tool_use_id = strdup(id ? id : "");
                blk->tool_name = strdup(name ? name : "");
                blk->tool_input = printed ? printed : strdup("{}");
            }
        }
    }

    cJSON_Delete(json);
    return 0;
}

void llm_response_free(llm_response_t *resp) {
    free(resp->text);
    for (int i = 0; i < resp->block_count; i++) {
        free(resp->blocks[i].text);
        free(resp->blocks[i].tool_use_id);
        free(resp->blocks[i].tool_name);
        free(resp->blocks[i].tool_input);
    }
    free(resp->blocks);
    memset(resp, 0, sizeof(*resp));
}

cJSON *msg_text(const char *role, const char *text) {
    cJSON *m = cJSON_CreateObject();
    cJSON_AddStringToObject(m, "role", role);
    cJSON_AddStringToObject(m, "content", text);
    return m;
}

cJSON *msg_blocks(const char *role, cJSON *blocks) {
    cJSON *m = cJSON_CreateObject();
    cJSON_AddStringToObject(m, "role", role);
    cJSON_AddItemToObject(m, "content", blocks);
    return m;
}

cJSON *block_text(const char *text) {
    cJSON *b = cJSON_CreateObject();
    cJSON_AddStringToObject(b, "type", "text");
    cJSON_AddStringToObject(b, "text", text);
    return b;
}

cJSON *block_tool_use(const char *id, const char *name, cJSON *input) {
    cJSON *b = cJSON_CreateObject();
    cJSON_AddStringToObject(b, "type", "tool_use");
    cJSON_AddStringToObject(b, "id", id);
    cJSON_AddStringToObject(b, "name", name);
    cJSON_AddItemToObject(b, "input", input);
    return b;
}

cJSON *block_tool_result(const char *tool_use_id, const char *content) {
    cJSON *b = cJSON_CreateObject();
    cJSON_AddStringToObject(b, "type", "tool_result");
    cJSON_AddStringToObject(b, "tool_use_id", tool_use_id);
    cJSON_AddStringToObject(b, "content", content);
    return b;
}
