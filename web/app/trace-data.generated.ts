// Generated from real runs against test/mock_llm.py. Do not edit by hand.
// 重新生成：make trace

import type { TraceCase } from "./trace-types";

export const traceCases: TraceCase[] = [
  {
    "id": "plain",
    "number": "01",
    "title": "没有 tool_call",
    "summary": "模型只返回文本，ReAct 循环一轮结束。",
    "prompt": "回我一句纯文本。",
    "outcome": "一次 LLM 请求，end_turn 收尾。",
    "steps": [
      {
        "id": "s1",
        "label": "用户消息进入 Context",
        "detail": "main 把输入推入消息总线，agent 弹出后追加到 Context（session 历史 + 当前消息）。",
        "kind": "input",
        "source": {
          "file": "src/agent.c",
          "line": 91
        },
        "event": {
          "content": "回我一句纯文本。",
          "chat_id": "local"
        },
        "context": {
          "messages": [
            {
              "role": "user",
              "content": "回我一句纯文本。"
            }
          ]
        }
      },
      {
        "id": "s2",
        "label": "第 1 次请求 LLM",
        "detail": "把 Context 和工具定义 POST 给 Anthropic Messages API（非流式）。",
        "kind": "request",
        "source": {
          "file": "src/agent.c",
          "line": 101
        },
        "event": {
          "iteration": 1,
          "message_count": 1
        },
        "context": {}
      },
      {
        "id": "s3",
        "label": "第 1 次回复：end_turn",
        "detail": "解析 content blocks：text 与 tool_use。stop_reason 决定下一轮动作。",
        "kind": "response",
        "source": {
          "file": "src/agent.c",
          "line": 137
        },
        "event": {
          "iteration": 1,
          "stop_reason": "end_turn",
          "text": "这是一条纯文本回复，没有调用任何工具。"
        },
        "context": {}
      },
      {
        "id": "s4",
        "label": "本轮结束（1 次迭代）",
        "detail": "没有新的 tool_use，agent 把 user + 最终回复写入 session。",
        "kind": "end",
        "source": {
          "file": "src/agent.c",
          "line": 215
        },
        "event": {
          "reply": "这是一条纯文本回复，没有调用任何工具。",
          "iterations": 1
        },
        "context": {
          "messages": [
            {
              "role": "user",
              "content": "回我一句纯文本。"
            },
            {
              "role": "assistant",
              "content": [
                {
                  "type": "text",
                  "text": "这是一条纯文本回复，没有调用任何工具。"
                }
              ]
            }
          ]
        }
      }
    ]
  },
  {
    "id": "time",
    "number": "02",
    "title": "一次工具往返",
    "summary": "模型调用 get_time，结果放回 Context，再回答。",
    "prompt": "现在几点？",
    "outcome": "两轮 LLM 请求，一轮工具执行。",
    "steps": [
      {
        "id": "s1",
        "label": "用户消息进入 Context",
        "detail": "main 把输入推入消息总线，agent 弹出后追加到 Context（session 历史 + 当前消息）。",
        "kind": "input",
        "source": {
          "file": "src/agent.c",
          "line": 91
        },
        "event": {
          "content": "现在几点？",
          "chat_id": "local"
        },
        "context": {
          "messages": [
            {
              "role": "user",
              "content": "现在几点？"
            }
          ]
        }
      },
      {
        "id": "s2",
        "label": "第 1 次请求 LLM",
        "detail": "把 Context 和工具定义 POST 给 Anthropic Messages API（非流式）。",
        "kind": "request",
        "source": {
          "file": "src/agent.c",
          "line": 101
        },
        "event": {
          "iteration": 1,
          "message_count": 1
        },
        "context": {}
      },
      {
        "id": "s3",
        "label": "第 1 次回复：tool_use",
        "detail": "解析 content blocks：text 与 tool_use。stop_reason 决定下一轮动作。",
        "kind": "response",
        "source": {
          "file": "src/agent.c",
          "line": 137
        },
        "event": {
          "iteration": 1,
          "stop_reason": "tool_use",
          "text": "我来处理这个请求。"
        },
        "context": {}
      },
      {
        "id": "s4",
        "label": "模型调用 get_time",
        "detail": "从注册表找到工具，把参数 JSON 交给 execute。",
        "kind": "tool",
        "source": {
          "file": "src/agent.c",
          "line": 162
        },
        "event": {
          "name": "get_time",
          "args": "{}"
        },
        "context": {}
      },
      {
        "id": "s5",
        "label": "get_time 返回结果",
        "detail": "结果作为 tool_result block 放回 Context，进入下一轮。",
        "kind": "tool",
        "source": {
          "file": "src/agent.c",
          "line": 173
        },
        "event": {
          "name": "get_time",
          "result": "2026-08-10 09:55:54 CST"
        },
        "context": {}
      },
      {
        "id": "s6",
        "label": "第 2 次请求 LLM",
        "detail": "把 Context 和工具定义 POST 给 Anthropic Messages API（非流式）。",
        "kind": "request",
        "source": {
          "file": "src/agent.c",
          "line": 101
        },
        "event": {
          "iteration": 2,
          "message_count": 3
        },
        "context": {}
      },
      {
        "id": "s7",
        "label": "第 2 次回复：end_turn",
        "detail": "解析 content blocks：text 与 tool_use。stop_reason 决定下一轮动作。",
        "kind": "response",
        "source": {
          "file": "src/agent.c",
          "line": 137
        },
        "event": {
          "iteration": 2,
          "stop_reason": "end_turn",
          "text": "现在是 2026-08-10 14:30:00。"
        },
        "context": {}
      },
      {
        "id": "s8",
        "label": "本轮结束（2 次迭代）",
        "detail": "没有新的 tool_use，agent 把 user + 最终回复写入 session。",
        "kind": "end",
        "source": {
          "file": "src/agent.c",
          "line": 215
        },
        "event": {
          "reply": "现在是 2026-08-10 14:30:00。",
          "iterations": 2
        },
        "context": {
          "messages": [
            {
              "role": "user",
              "content": "现在几点？"
            },
            {
              "role": "assistant",
              "content": [
                {
                  "type": "text",
                  "text": "我来处理这个请求。"
                },
                {
                  "type": "tool_use",
                  "id": "toolu_mock_01",
                  "name": "get_time",
                  "input": {}
                }
              ]
            },
            {
              "role": "user",
              "content": [
                {
                  "type": "tool_result",
                  "tool_use_id": "toolu_mock_01",
                  "content": "2026-08-10 09:55:54 CST"
                }
              ]
            },
            {
              "role": "assistant",
              "content": [
                {
                  "type": "text",
                  "text": "现在是 2026-08-10 14:30:00。"
                }
              ]
            }
          ]
        }
      }
    ]
  },
  {
    "id": "memory",
    "number": "03",
    "title": "写入长期记忆",
    "summary": "模型调用 memory_write，把记忆写进 MEMORY.md。",
    "prompt": "记住：我喜欢的颜色是蓝色。",
    "outcome": "两轮 LLM 请求，MEMORY.md 落盘。",
    "steps": [
      {
        "id": "s1",
        "label": "用户消息进入 Context",
        "detail": "main 把输入推入消息总线，agent 弹出后追加到 Context（session 历史 + 当前消息）。",
        "kind": "input",
        "source": {
          "file": "src/agent.c",
          "line": 91
        },
        "event": {
          "content": "记住：我喜欢的颜色是蓝色。",
          "chat_id": "local"
        },
        "context": {
          "messages": [
            {
              "role": "user",
              "content": "记住：我喜欢的颜色是蓝色。"
            }
          ]
        }
      },
      {
        "id": "s2",
        "label": "第 1 次请求 LLM",
        "detail": "把 Context 和工具定义 POST 给 Anthropic Messages API（非流式）。",
        "kind": "request",
        "source": {
          "file": "src/agent.c",
          "line": 101
        },
        "event": {
          "iteration": 1,
          "message_count": 1
        },
        "context": {}
      },
      {
        "id": "s3",
        "label": "第 1 次回复：tool_use",
        "detail": "解析 content blocks：text 与 tool_use。stop_reason 决定下一轮动作。",
        "kind": "response",
        "source": {
          "file": "src/agent.c",
          "line": 137
        },
        "event": {
          "iteration": 1,
          "stop_reason": "tool_use",
          "text": "我来处理这个请求。"
        },
        "context": {}
      },
      {
        "id": "s4",
        "label": "模型调用 memory_write",
        "detail": "从注册表找到工具，把参数 JSON 交给 execute。",
        "kind": "tool",
        "source": {
          "file": "src/agent.c",
          "line": 162
        },
        "event": {
          "name": "memory_write",
          "args": "{\"content\":\"用户喜欢的颜色是蓝色。\"}"
        },
        "context": {}
      },
      {
        "id": "s5",
        "label": "memory_write 返回结果",
        "detail": "结果作为 tool_result block 放回 Context，进入下一轮。",
        "kind": "tool",
        "source": {
          "file": "src/agent.c",
          "line": 173
        },
        "event": {
          "name": "memory_write",
          "result": "memory updated (33 chars): 用户喜欢的颜色是蓝色。"
        },
        "context": {}
      },
      {
        "id": "s6",
        "label": "第 2 次请求 LLM",
        "detail": "把 Context 和工具定义 POST 给 Anthropic Messages API（非流式）。",
        "kind": "request",
        "source": {
          "file": "src/agent.c",
          "line": 101
        },
        "event": {
          "iteration": 2,
          "message_count": 3
        },
        "context": {}
      },
      {
        "id": "s7",
        "label": "第 2 次回复：end_turn",
        "detail": "解析 content blocks：text 与 tool_use。stop_reason 决定下一轮动作。",
        "kind": "response",
        "source": {
          "file": "src/agent.c",
          "line": 137
        },
        "event": {
          "iteration": 2,
          "stop_reason": "end_turn",
          "text": "记住了：你的喜好已经写入 MEMORY.md。"
        },
        "context": {}
      },
      {
        "id": "s8",
        "label": "本轮结束（2 次迭代）",
        "detail": "没有新的 tool_use，agent 把 user + 最终回复写入 session。",
        "kind": "end",
        "source": {
          "file": "src/agent.c",
          "line": 215
        },
        "event": {
          "reply": "记住了：你的喜好已经写入 MEMORY.md。",
          "iterations": 2
        },
        "context": {
          "messages": [
            {
              "role": "user",
              "content": "记住：我喜欢的颜色是蓝色。"
            },
            {
              "role": "assistant",
              "content": [
                {
                  "type": "text",
                  "text": "我来处理这个请求。"
                },
                {
                  "type": "tool_use",
                  "id": "toolu_mock_01",
                  "name": "memory_write",
                  "input": {
                    "content": "用户喜欢的颜色是蓝色。"
                  }
                }
              ]
            },
            {
              "role": "user",
              "content": [
                {
                  "type": "tool_result",
                  "tool_use_id": "toolu_mock_01",
                  "content": "memory updated (33 chars): 用户喜欢的颜色是蓝色。"
                }
              ]
            },
            {
              "role": "assistant",
              "content": [
                {
                  "type": "text",
                  "text": "记住了：你的喜好已经写入 MEMORY.md。"
                }
              ]
            }
          ]
        }
      }
    ]
  }
];
