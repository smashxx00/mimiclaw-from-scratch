import assert from "node:assert/strict";
import { spawn } from "node:child_process";
import { test } from "node:test";
import { join } from "node:path";

const ROOT = new URL("..", import.meta.url).pathname;
const PORT = 18923;
const BASE = `http://127.0.0.1:${PORT}`;
const NEXT_BIN = join(ROOT, "node_modules", ".bin", "next");

async function waitFor(fn, timeoutMs) {
  const deadline = Date.now() + timeoutMs;
  while (Date.now() < deadline) {
    try {
      if (await fn()) return;
    } catch {
      /* 继续重试 */
    }
    await new Promise((resolve) => setTimeout(resolve, 500));
  }
  throw new Error("等待服务器超时");
}

test("首页渲染文章、模块与 TraceLab", async () => {
  const server = spawn(NEXT_BIN, ["start", "-p", String(PORT)], {
    cwd: ROOT,
    stdio: "ignore",
  });
  try {
    await waitFor(() => fetch(`${BASE}/`).then((r) => r.status === 200), 30000);
    const html = await (await fetch(`${BASE}/`)).text();
    assert.match(html, /nano-mimiclaw/);
    assert.match(html, /第二章：从一个 while 循环开始/);
    assert.match(html, /TraceLab/);
    assert.match(html, /src\/agent\.c/);
  } finally {
    server.kill("SIGTERM");
  }
});
