// 从 ../docs 和 ../src 生成 app/content.generated.ts。
import { readFile, writeFile } from "node:fs/promises";
import { dirname, resolve } from "node:path";
import { fileURLToPath } from "node:url";

const here = dirname(fileURLToPath(import.meta.url));
const webRoot = resolve(here, "..");
const projectRoot = resolve(webRoot, "..");

const sourcePaths = [
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

const generatedPath = resolve(webRoot, "app/content.generated.ts");

try {
  const lessons = {
    outline: await readFile(resolve(projectRoot, "docs/mimiclaw-from-scratch.md"), "utf8"),
    chapter1: await readFile(resolve(projectRoot, "docs/ch01-modules.md"), "utf8"),
    chapter2: await readFile(resolve(projectRoot, "docs/ch02-loop.md"), "utf8"),
    chapter3: await readFile(resolve(projectRoot, "docs/ch03-run.md"), "utf8"),
  };

  const sourceFiles = Object.fromEntries(
    await Promise.all(
      sourcePaths.map(async (path) => [path, await readFile(resolve(projectRoot, path), "utf8")]),
    ),
  );

  const output =
    "// Generated from ../docs and ../src. Do not edit by hand.\n\n" +
    `export const lessonMarkdown = ${JSON.stringify(lessons, null, 2)} as const;\n\n` +
    `export const sourceFiles = ${JSON.stringify(sourceFiles, null, 2)} as const;\n`;

  await writeFile(generatedPath, output, "utf8");
  console.log("content.generated.ts 已生成");
} catch (error) {
  if (error?.code !== "ENOENT") throw error;
  await readFile(generatedPath, "utf8");
  console.log("教学源文件缺失，使用已提交的生成内容。");
}
