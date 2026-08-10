import { lessonMarkdown, sourceFiles as generatedSourceFiles } from "./content.generated";

export const sourceFiles: Record<string, string> = { ...generatedSourceFiles };

export type Checkpoint = {
  id: string;
  label: string;
  files: Array<{ file: string; ranges: Array<[number, number]> }>;
};

export type Lesson = {
  id: string;
  title: string;
  description: string;
  markdown: string;
  checkpoints: Checkpoint[];
};

export const repoFiles = [
  "src/agent.c",
  "src/llm.c",
  "src/tools.c",
  "src/memory.c",
  "src/bus.c",
  "src/main.c",
  "src/agent.h",
  "src/llm.h",
  "src/tools.h",
  "src/memory.h",
  "src/bus.h",
  "src/config.h",
];

const chapter1: Lesson = {
  id: "chapter1",
  title: "第一章：六个文件，各管各的",
  description: "先看地图，记住每个模块做什么、不做什么。",
  markdown: lessonMarkdown.chapter1,
  checkpoints: [
    { id: "ch1-bus", label: "消息总线", files: [{ file: "src/bus.c", ranges: [[1, 39]] }] },
    { id: "ch1-llm", label: "LLM 代理", files: [{ file: "src/llm.c", ranges: [[1, 213]] }] },
    { id: "ch1-tools", label: "工具注册表", files: [{ file: "src/tools.c", ranges: [[1, 240]] }] },
    { id: "ch1-memory", label: "记忆与会话", files: [{ file: "src/memory.c", ranges: [[1, 162]] }] },
    { id: "ch1-agent", label: "Agent Loop", files: [{ file: "src/agent.c", ranges: [[1, 221]] }] },
    { id: "ch1-main", label: "拼装入口", files: [{ file: "src/main.c", ranges: [[1, 145]] }] },
    { id: "ch1-collab", label: "留下骨架", files: [] },
  ],
};

const chapter2: Lesson = {
  id: "chapter2",
  title: "第二章：从一个 while 循环开始",
  description: "跟着数据流走，需要什么就造什么。",
  markdown: lessonMarkdown.chapter2,
  checkpoints: [
    {
      id: "ch2-start",
      label: "循环骨架",
      files: [{ file: "src/agent.c", ranges: [[1, 13], [77, 94]] }],
    },
    {
      id: "ch2-bus",
      label: "消息总线",
      files: [{ file: "src/bus.c", ranges: [[1, 39]] }],
    },
    {
      id: "ch2-prompt",
      label: "系统提示词",
      files: [{ file: "src/agent.c", ranges: [[60, 73]] }],
    },
    {
      id: "ch2-llm",
      label: "LLM 代理",
      files: [{ file: "src/llm.c", ranges: [[1, 213]] }],
    },
    {
      id: "ch2-loop",
      label: "Agent Loop",
      files: [{ file: "src/agent.c", ranges: [[74, 76], [95, 142], [197, 199]] }],
    },
    {
      id: "ch2-tools",
      label: "工具注册表",
      files: [
        { file: "src/tools.c", ranges: [[1, 240]] },
        { file: "src/agent.c", ranges: [[143, 196]] },
      ],
    },
    {
      id: "ch2-memory",
      label: "记忆与会话",
      files: [
        { file: "src/memory.c", ranges: [[1, 162]] },
        { file: "src/agent.c", ranges: [[207, 211], [213, 221]] },
      ],
    },
    {
      id: "ch2-edge",
      label: "边界情况",
      files: [{ file: "src/agent.c", ranges: [[101, 115], [201, 206]] }],
    },
    {
      id: "ch2-main",
      label: "拼装入口",
      files: [
        { file: "src/main.c", ranges: [[1, 145]] },
        { file: "src/agent.c", ranges: [[14, 59]] },
      ],
    },
  ],
};

const chapter3: Lesson = {
  id: "chapter3",
  title: "第三章：跑起来看看",
  description: "构建、mock、看落盘、生成 trace。",
  markdown: lessonMarkdown.chapter3,
  checkpoints: [
    {
      id: "ch3-all",
      label: "全部源码",
      files: repoFiles.map((file) => ({
        file,
        ranges: [[1, (sourceFiles[file] ?? "").split("\n").length]],
      })),
    },
  ],
};

export const lessons: Lesson[] = [chapter1, chapter2, chapter3];
