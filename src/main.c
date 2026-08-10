#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>

#include "agent.h"
#include "bus.h"
#include "config.h"
#include "tools.h"

/* 拼装层。对应 MimiClaw 的 mimi.c + cli/serial_cli.c：
 * 造 Model、注册工具、把消息从总线送进 agent、把回复打印出来。 */

static const char *env_or(const char *name, const char *fallback) {
    const char *v = getenv(name);
    return v && *v ? v : fallback;
}

static int ensure_dir(const char *path) {
    char *copy = strdup(path);
    int rc = 0;
    for (char *p = copy + 1; *p; p++) {
        if (*p == '/') {
            *p = '\0';
            if (mkdir(copy, 0755) != 0 && errno != EEXIST) {
                rc = -1;
                break;
            }
            *p = '/';
        }
    }
    if (rc == 0 && mkdir(copy, 0755) != 0 && errno != EEXIST) rc = -1;
    free(copy);
    return rc;
}

static char *data_dir_path(void) {
    const char *custom = getenv("MIMI_DATA_DIR");
    if (custom && *custom) return strdup(custom);
    const char *home = getenv("HOME");
    if (!home || !*home) home = ".";
    size_t len = strlen(home) + 16;
    char *path = (char *)malloc(len);
    snprintf(path, len, "%s/.mimiclaw", home);
    return path;
}

/* UI 回调：main 是 AgentEvent 的消费者，agent 自己不碰屏幕 */
static int turn_printed_text = 0;   /* 本轮是否已流式输出过文本 */

static void ui_assistant_text(const char *text, void *userdata) {
    (void)userdata;
    turn_printed_text = 1;
    printf("%s", text);
    fflush(stdout);
}

static void ui_tool_call(const char *name, const char *args_json, void *userdata) {
    (void)userdata;
    printf("\n[tool: %s] %s\n", name, args_json);
    fflush(stdout);
}

static void ui_tool_result(const char *name, const char *result, void *userdata) {
    (void)userdata;
    printf("[result: %s] %s\n", name, result);
    fflush(stdout);
}

int main(void) {
    const char *api_key = getenv("MIMI_API_KEY");
    if (!api_key || !*api_key) {
        fprintf(stderr, "请设置 MIMI_API_KEY 环境变量（本地 mock 也要设，比如 test）\n");
        return 1;
    }

    char *data_dir = data_dir_path();
    ensure_dir(data_dir);

    llm_config_t llm = {
        .api_key = api_key,
        .model = env_or("MIMI_MODEL", MIMI_LLM_DEFAULT_MODEL),
        .base_url = env_or("MIMI_LLM_URL", MIMI_LLM_API_URL),
        .max_tokens = MIMI_LLM_MAX_TOKENS,
    };

    tool_registry_t tools;
    tools_builtin(&tools, data_dir, env_or("MIMI_SEARCH_KEY", ""));

    message_bus_t bus;
    bus_init(&bus);

    agent_env_t env;
    memset(&env, 0, sizeof(env));
    env.llm = llm;
    env.tools = &tools;
    env.data_dir = data_dir;
    env.ui.on_assistant_text = ui_assistant_text;
    env.ui.on_tool_call = ui_tool_call;
    env.ui.on_tool_result = ui_tool_result;

    const char *trace_path = getenv("MIMI_TRACE");
    if (trace_path && *trace_path) {
        env.trace = fopen(trace_path, "w");
        if (!env.trace) fprintf(stderr, "警告：无法打开 trace 文件 %s\n", trace_path);
    }

    printf("nano-mimiclaw: MimiClaw 教学版（纯 C，桌面可运行）\n");
    printf("数据目录: %s | 模型: %s\n", data_dir, llm.model);
    printf("输入 exit 退出。\n\n");

    char line[MIMI_MSG_MAX_LEN];
    while (1) {
        printf("mimi> ");
        fflush(stdout);
        if (!fgets(line, sizeof(line), stdin)) break;
        line[strcspn(line, "\n")] = '\0';
        if (strcmp(line, "exit") == 0 || strcmp(line, "quit") == 0) break;
        if (!*line) continue;

        /* 消息走一遍总线：入站 → agent → 出站。
         * 真机上是两个任务隔着队列解耦，这里同步调用，语义不变。 */
        char *content = strdup(line);
        if (!bus_push(&bus, "cli", "local", content)) {
            free(content);
            printf("[bus full]\n");
            continue;
        }

        mimi_msg_t msg;
        if (!bus_pop(&bus, &msg)) continue;

        turn_printed_text = 0;
        char *reply = NULL;
        char err[512] = "";
        if (agent_run(&env, &msg, &reply, err, sizeof(err)) != 0) {
            printf("\n[error] %s\n", err);
        } else {
            /* 模型文本已经通过 ui 回调流式打印，这里只兜底 */
            if (!turn_printed_text) printf("\n%s\n", reply ? reply : "(无回复)");
            else printf("\n");
        }
        free(reply);
        bus_msg_free(&msg);
    }

    if (env.trace) fclose(env.trace);
    free(data_dir);
    return 0;
}
