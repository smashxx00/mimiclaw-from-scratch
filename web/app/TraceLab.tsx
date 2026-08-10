"use client";

import { useEffect, useMemo, useRef, useState } from "react";
import hljs from "highlight.js/lib/core";
import c from "highlight.js/lib/languages/c";
import json from "highlight.js/lib/languages/json";
import { traceCases } from "./trace-data.generated";
import { sourceFiles } from "./lesson-data";
import { buildDebugFrames } from "./trace-debugger";

if (!hljs.getLanguage("c")) hljs.registerLanguage("c", c);
if (!hljs.getLanguage("json")) hljs.registerLanguage("json", json);

function highlightLine(line: string): string {
  if (!line) return "&nbsp;";
  return hljs.highlight(line, { language: "c", ignoreIllegals: true }).value;
}

function breakpointKey(file: string, line: number): string {
  return `${file}:${line}`;
}

export default function TraceLab() {
  const [caseIndex, setCaseIndex] = useState(0);
  const [frameIndex, setFrameIndex] = useState(0);
  const [breakpoints, setBreakpoints] = useState<ReadonlySet<string>>(() => new Set());
  const codeScrollRef = useRef<HTMLDivElement>(null);

  const traceCase = traceCases[caseIndex] ?? null;
  const frames = useMemo(() => (traceCase ? buildDebugFrames(traceCase) : []), [traceCase]);
  const frame = frames[frameIndex] ?? null;
  const file = frame?.source.file ?? "src/agent.c";
  const codeLines = useMemo(() => (sourceFiles[file] ?? "").split("\n"), [file]);
  const executableLines = useMemo(
    () => new Set(frames.filter((f) => f.source.file === file).map((f) => f.source.line)),
    [frames, file],
  );

  const selectCase = (index: number) => {
    setCaseIndex(index);
    setFrameIndex(0);
  };

  const goToFrame = (next: number) => {
    if (!frames.length) return;
    setFrameIndex(Math.max(0, Math.min(frames.length - 1, next)));
  };

  const continueToBreakpoint = () => {
    if (!frames.length || frameIndex >= frames.length - 1) return;
    const hit = frames.findIndex(
      (f, i) => i > frameIndex && breakpoints.has(breakpointKey(f.source.file, f.source.line)),
    );
    goToFrame(hit >= 0 ? hit : frames.length - 1);
  };

  const toggleBreakpoint = (line: number) => {
    if (!executableLines.has(line)) return;
    const key = breakpointKey(file, line);
    setBreakpoints((current) => {
      const next = new Set(current);
      if (next.has(key)) next.delete(key);
      else next.add(key);
      return next;
    });
  };

  useEffect(() => {
    const onKeyDown = (event: KeyboardEvent) => {
      const target = event.target as HTMLElement | null;
      if (target?.closest("input, textarea, select")) return;
      if (event.key === "F10") {
        event.preventDefault();
        goToFrame(frameIndex + 1);
      } else if (event.key === "F5") {
        event.preventDefault();
        continueToBreakpoint();
      } else if (!target?.closest("button, a") && event.key === "ArrowRight") {
        event.preventDefault();
        goToFrame(frameIndex + 1);
      } else if (!target?.closest("button, a") && event.key === "ArrowLeft") {
        event.preventDefault();
        goToFrame(frameIndex - 1);
      }
    };
    window.addEventListener("keydown", onKeyDown);
    return () => window.removeEventListener("keydown", onKeyDown);
  });

  useEffect(() => {
    const scroller = codeScrollRef.current;
    if (!scroller || !frame) return;
    const line = scroller.querySelector<HTMLElement>(`[data-trace-line="${frame.source.line}"]`);
    if (!line) return;
    scroller.scrollTo({
      top: Math.max(0, line.offsetTop - scroller.clientHeight * 0.35),
      behavior: "auto",
    });
  }, [frame]);

  if (!traceCase) {
    return <main className="trace"><p style={{ padding: 32 }}>trace 数据缺失，先运行 make trace 生成。</p></main>;
  }

  const messageCount = Array.isArray(frame?.context.messages)
    ? (frame!.context.messages as unknown[]).length
    : 0;

  return (
    <main className="trace">
      <div className="trace-cases">
        {traceCases.map((tc, index) => (
          <button
            key={tc.id}
            className={index === caseIndex ? "active" : ""}
            onClick={() => selectCase(index)}
          >
            <span className="tc-number">{tc.number}</span>
            <span>{tc.title}</span>
          </button>
        ))}
      </div>
      <div className="trace-panes">
        <div className="trace-steps">
          <h3>执行步骤</h3>
          <div className="steps-scroll">
            {frames.map((f, index) => (
              <button
                key={f.id}
                className={`step${index === frameIndex ? " active" : ""}`}
                onClick={() => goToFrame(index)}
              >
                <span className={`dot ${f.kind}`} />
                <span className="step-label">{f.label}</span>
                <span className="step-line">
                  {f.source.file.replace("src/", "")}:{f.source.line}
                </span>
              </button>
            ))}
          </div>
          <div className="trace-controls">
            <button onClick={() => goToFrame(0)}>重来</button>
            <button onClick={() => goToFrame(frameIndex + 1)}>单步 (F10)</button>
            <button onClick={continueToBreakpoint}>继续 (F5)</button>
          </div>
          <p className="trace-hint">点代码行号设断点，按 F5 跳到下一个断点，F10 单步。</p>
        </div>
        <div className="trace-code">
          <div className="code-head">
            <span>{file}</span>
            <span>
              {frameIndex + 1} / {frames.length}
            </span>
          </div>
          <div className="code-scroll" ref={codeScrollRef}>
            <pre className="code-view">
              {codeLines.map((line, index) => {
                const n = index + 1;
                const isCurrent = frame?.source.file === file && frame.source.line === n;
                const isExec = executableLines.has(n);
                const hasBp = breakpoints.has(breakpointKey(file, n));
                return (
                  <div
                    key={n}
                    className={`code-line trace-line${isCurrent ? " current" : ""}${isExec ? " exec" : ""}`}
                    data-trace-line={n}
                    onClick={() => toggleBreakpoint(n)}
                  >
                    <span className="ln">{n}</span>
                    <span className="bp-marker">{isExec ? (hasBp ? "●" : "○") : ""}</span>
                    <span className="code-text" dangerouslySetInnerHTML={{ __html: highlightLine(line) }} />
                  </div>
                );
              })}
            </pre>
          </div>
        </div>
        <div className="trace-detail">
          <h3>{frame?.label ?? ""}</h3>
          <p className="detail-text">{frame?.detail ?? ""}</p>
          <div className="detail-section">
            <h4>事件</h4>
            <pre>{JSON.stringify(frame?.event ?? {}, null, 2)}</pre>
          </div>
          <div className="detail-section">
            <h4>Context messages（{messageCount} 条）</h4>
            <pre>{JSON.stringify(frame?.context.messages ?? [], null, 2)}</pre>
          </div>
        </div>
      </div>
    </main>
  );
}
