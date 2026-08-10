#include "memory.h"

#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <time.h>
#include <unistd.h>

/* 递归创建 path 的父目录 */
static int ensure_parent(const char *path) {
    char *copy = strdup(path);
    char *slash = strrchr(copy, '/');
    if (!slash) {
        free(copy);
        return 0;  /* 没有父目录可创建 */
    }
    *slash = '\0';
    if (!*copy) {
        free(copy);
        return 0;
    }
    for (char *p = copy + 1; *p; p++) {
        if (*p == '/') {
            *p = '\0';
            mkdir(copy, 0755);
            *p = '/';
        }
    }
    int rc = mkdir(copy, 0755);
    free(copy);
    return (rc == 0 || errno == EEXIST) ? 0 : -1;
}

static char *memory_path(const char *data_dir) {
    size_t len = strlen(data_dir) + 16;
    char *path = (char *)malloc(len);
    snprintf(path, len, "%s/MEMORY.md", data_dir);
    return path;
}

char *memory_load(const char *data_dir) {
    char *path = memory_path(data_dir);
    FILE *f = fopen(path, "r");
    free(path);
    if (!f) return strdup("");

    char *buf = NULL;
    size_t cap = 0;
    size_t used = 0;
    char chunk[1024];
    size_t n;
    while ((n = fread(chunk, 1, sizeof(chunk), f)) > 0) {
        if (used + n + 1 > cap) {
            cap = cap ? cap * 2 : 4096;
            while (cap < used + n + 1) cap *= 2;
            buf = (char *)realloc(buf, cap);
        }
        memcpy(buf + used, chunk, n);
        used += n;
    }
    fclose(f);
    if (!buf) return strdup("");
    buf[used] = '\0';
    return buf;
}

int memory_save(const char *data_dir, const char *content, char *err, size_t errlen) {
    char *path = memory_path(data_dir);
    int rc = 0;
    if (ensure_parent(path) != 0) {
        snprintf(err, errlen, "cannot create directory for %s", path);
        rc = -1;
    } else {
        int fd = open(path, O_WRONLY | O_CREAT | O_TRUNC, 0644);
        if (fd < 0) {
            snprintf(err, errlen, "cannot open %s", path);
            rc = -1;
        } else {
            size_t n = strlen(content);
            if (write(fd, content, n) != (ssize_t)n) {
                snprintf(err, errlen, "write failed: %s", path);
                rc = -1;
            }
            close(fd);
        }
    }
    free(path);
    return rc;
}

char *session_path(const char *data_dir, const char *chat_id) {
    size_t len = strlen(data_dir) + strlen(chat_id) + 32;
    char *path = (char *)malloc(len);
    snprintf(path, len, "%s/sessions/%s.jsonl", data_dir, chat_id);
    return path;
}

cJSON *session_load(const char *data_dir, const char *chat_id, int max_msgs) {
    cJSON *messages = cJSON_CreateArray();
    char *path = session_path(data_dir, chat_id);
    FILE *f = fopen(path, "r");
    free(path);
    if (!f) return messages;

    char *line = NULL;
    size_t cap = 0;
    ssize_t n;
    while ((n = getline(&line, &cap, f)) >= 0) {
        cJSON *obj = cJSON_Parse(line);
        if (!obj || !cJSON_IsObject(obj) ||
            !cJSON_GetStringValue(cJSON_GetObjectItemCaseSensitive(obj, "role"))) {
            cJSON_Delete(obj);
            continue;  /* 容错：坏行跳过，不丢弃全部历史 */
        }
        cJSON_AddItemToArray(messages, obj);
    }
    free(line);
    fclose(f);

    /* 只保留最近 max_msgs 条 */
    int total = cJSON_GetArraySize(messages);
    int drop = total - max_msgs;
    if (drop > 0) {
        while (drop-- > 0) {
            cJSON_DeleteItemFromArray(messages, 0);
        }
    }
    return messages;
}

int session_append(const char *data_dir, const char *chat_id,
                   const char *role, const char *content) {
    char *path = session_path(data_dir, chat_id);
    int rc = 0;
    if (ensure_parent(path) != 0) {
        rc = -1;
    } else {
        int fd = open(path, O_WRONLY | O_CREAT | O_APPEND, 0644);
        if (fd < 0) {
            rc = -1;
        } else {
            cJSON *obj = cJSON_CreateObject();
            cJSON_AddStringToObject(obj, "role", role);
            cJSON_AddStringToObject(obj, "content", content);
            cJSON_AddNumberToObject(obj, "ts", (double)time(NULL));
            char *line = cJSON_PrintUnformatted(obj);
            if (line) {
                size_t len = strlen(line);
                if (write(fd, line, len) != (ssize_t)len ||
                    write(fd, "\n", 1) != 1) rc = -1;
                free(line);
            }
            cJSON_Delete(obj);
            close(fd);
        }
    }
    free(path);
    return rc;
}
