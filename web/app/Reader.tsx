"use client";

import { memo, useCallback, useEffect, useMemo, useRef, useState } from "react";
import hljs from "highlight.js/lib/core";
import c from "highlight.js/lib/languages/c";
import json from "highlight.js/lib/languages/json";
import bash from "highlight.js/lib/languages/bash";
import { marked } from "marked";
import { lessons, repoFiles, sourceFiles } from "./lesson-data";

if (!hljs.getLanguage("c")) hljs.registerLanguage("c", c);
if (!hljs.getLanguage("json")) hljs.registerLanguage("json", json);
if (!hljs.getLanguage("bash")) hljs.registerLanguage("bash", bash);
if (!hljs.getLanguage("shell")) hljs.registerLanguage("shell", bash);

function escapeHtml(value: string): string {
  return value
    .replaceAll("&", "&amp;")
    .replaceAll("<", "&lt;")
    .replaceAll(">", "&gt;")
    .replaceAll('"', "&quot;");
}

function renderMarkdown(markdown: string): string {
  const withAnchors = markdown
    .replace(
      /<!--\s*checkpoint:\s*([a-z0-9-]+)\s*-->/g,
      '<div class="checkpoint-anchor" data-checkpoint="$1" aria-hidden="true"></div>',
    )
    .replace(
      /^\[图：(.+)\]$/gm,
      (_, caption: string) =>
        `<figure class="figure-placeholder"><div class="figure-mark"><span></span><span></span><span></span></div><figcaption>${escapeHtml(caption)}</figcaption></figure>`,
    );

  const renderer = new marked.Renderer();
  renderer.code = ({ text, lang }) => {
    const language = lang && hljs.getLanguage(lang) ? lang : "c";
    const highlighted = hljs.highlight(text, { language, ignoreIllegals: true }).value;
    return `<pre class="article-code"><code>${highlighted}</code></pre>`;
  };
  renderer.link = ({ href, title, tokens }) => {
    const text = renderer.parser.parseInline(tokens);
    const external = /^https?:\/\//.test(href);
    const attrs = external ? ' target="_blank" rel="noreferrer"' : "";
    return `<a href="${escapeHtml(href)}"${title ? ` title="${escapeHtml(title)}"` : ""}${attrs}>${text}</a>`;
  };
  return marked.parse(withAnchors, { gfm: true, renderer }) as string;
}

const ArticleBody = memo(function ArticleBody({ html }: { html: string }) {
  return <div className="article-body" dangerouslySetInnerHTML={{ __html: html }} />;
});

function rangeContains(ranges: Array<[number, number]>, line: number): boolean {
  return ranges.some(([start, end]) => line >= start && line <= end);
}

function highlightLine(line: string): string {
  if (!line) return "&nbsp;";
  return hljs.highlight(line, { language: "c", ignoreIllegals: true }).value;
}

export default function Reader() {
  const [lessonIndex, setLessonIndex] = useState(0);
  const [active, setActive] = useState<string[]>([]);
  const [locked, setLocked] = useState(false);
  const [selectedFile, setSelectedFile] = useState<string>("src/agent.c");
  const articleRef = useRef<HTMLDivElement>(null);

  const lesson = lessons[lessonIndex];

  const reveal = useCallback((id: string) => {
    setActive((prev) => (prev.includes(id) ? prev : [...prev, id]));
  }, []);

  useEffect(() => {
    setActive([]);
    const first = lessons[lessonIndex].checkpoints[0];
    setSelectedFile(first?.files[0]?.file ?? "src/agent.c");
  }, [lessonIndex]);

  useEffect(() => {
    const root = articleRef.current;
    if (!root) return;
    const valid = new Set(lesson.checkpoints.map((cp) => cp.id));
    const observer = new IntersectionObserver(
      (entries) => {
        for (const entry of entries) {
          if (!entry.isIntersecting || locked) continue;
          const id = (entry.target as HTMLElement).dataset.checkpoint;
          if (!id || !valid.has(id)) continue;
          reveal(id);
          const cp = lesson.checkpoints.find((item) => item.id === id);
          if (cp?.files[0]) setSelectedFile(cp.files[0].file);
        }
      },
      { root, rootMargin: "-15% 0px -40% 0px", threshold: 0 },
    );
    root.querySelectorAll<HTMLElement>(".checkpoint-anchor").forEach((anchor) => observer.observe(anchor));
    return () => observer.disconnect();
  }, [lesson, locked, reveal]);

  const html = useMemo(() => renderMarkdown(lesson.markdown), [lesson]);
  const article = useMemo(() => <ArticleBody html={html} />, [html]);

  /* 每个文件当前已揭示的行范围（累计） */
  const revealed = useMemo(() => {
    const map = new Map<string, Array<[number, number]>>();
    for (const cp of lesson.checkpoints) {
      if (!active.includes(cp.id)) continue;
      for (const f of cp.files) {
        const list = map.get(f.file) ?? [];
        list.push(...f.ranges);
        map.set(f.file, list);
      }
    }
    return map;
  }, [lesson, active]);

  /* 最近激活的 checkpoint 新增的行，用于高亮 */
  const latestAdded = useMemo(() => {
    const map = new Map<string, Set<number>>();
    if (active.length === 0) return map;
    const lastId = active[active.length - 1];
    const cp = lesson.checkpoints.find((item) => item.id === lastId);
    if (!cp) return map;
    for (const f of cp.files) {
      const set = new Set<number>();
      for (const [start, end] of f.ranges) {
        for (let i = start; i <= end; i++) set.add(i);
      }
      map.set(f.file, set);
    }
    return map;
  }, [active, lesson]);

  const fileProgress = useMemo(() => {
    return repoFiles.map((file) => {
      const lines = (sourceFiles[file] ?? "").split("\n");
      const ranges = revealed.get(file) ?? [];
      let shown = 0;
      for (const [start, end] of ranges) {
        shown += Math.max(0, Math.min(end, lines.length) - start + 1);
      }
      return { file, shown, total: lines.length };
    });
  }, [revealed]);

  const codeLines = useMemo(() => (sourceFiles[selectedFile] ?? "").split("\n"), [selectedFile]);
  const ranges = revealed.get(selectedFile) ?? [];
  const added = latestAdded.get(selectedFile) ?? new Set<number>();

  return (
    <main className="reader">
      <div className="lesson-tabs">
        {lessons.map((item, index) => (
          <button
            key={item.id}
            className={index === lessonIndex ? "active" : ""}
            onClick={() => setLessonIndex(index)}
          >
            {item.title}
          </button>
        ))}
        <button className="lock" onClick={() => setLocked(!locked)}>
          {locked ? "解锁编辑器" : "锁定编辑器"}
        </button>
      </div>
      <div className="reader-panes">
        <div className="article-pane" ref={articleRef}>
          <h2 className="lesson-title">{lesson.title}</h2>
          <p className="lesson-desc">{lesson.description}</p>
          {article}
          <p className="article-end">读完了。去 TraceLab 打断点，或者回到仓库把代码跑起来。</p>
        </div>
        <div className="code-pane">
          <div className="code-tabs">
            {repoFiles.map((file) => (
              <button
                key={file}
                className={file === selectedFile ? "active" : ""}
                onClick={() => setSelectedFile(file)}
              >
                {file.replace("src/", "")}
              </button>
            ))}
          </div>
          <div className="file-progress">
            {fileProgress.map((item) => (
              <span
                key={item.file}
                className={item.file === selectedFile ? "active" : ""}
                title={`${item.file}：已揭示 ${item.shown}/${item.total} 行`}
              >
                {item.file.replace("src/", "")} {item.shown}/{item.total}
              </span>
            ))}
          </div>
          <div className="code-scroll">
            {ranges.length === 0 ? (
              <div className="code-empty">
                <p>这里会随阅读进度补全代码。</p>
                <p>读到对应章节时，相应文件的代码会逐段出现。</p>
              </div>
            ) : (
              <pre className="code-view">
                {codeLines.map((line, index) => {
                  const n = index + 1;
                  const visible = rangeContains(ranges, n);
                  const isAdded = added.has(n);
                  return (
                    <div
                      key={n}
                      className={`code-line${visible ? "" : " hidden"}${isAdded ? " added" : ""}`}
                      data-line={n}
                    >
                      <span className="ln">{n}</span>
                      <span
                        className="code-text"
                        dangerouslySetInnerHTML={{ __html: visible ? highlightLine(line) : "" }}
                      />
                    </div>
                  );
                })}
              </pre>
            )}
          </div>
        </div>
      </div>
    </main>
  );
}
