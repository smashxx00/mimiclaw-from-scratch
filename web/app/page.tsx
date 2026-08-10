"use client";

import { useState } from "react";
import Reader from "./Reader";
import TraceLab from "./TraceLab";

type Screen = "read" | "trace";

export default function Home() {
  const [screen, setScreen] = useState<Screen>(() => {
    if (typeof window === "undefined") return "read";
    return new URLSearchParams(window.location.search).get("screen") === "trace" ? "trace" : "read";
  });

  return (
    <div className="app">
      <header className="topbar">
        <div className="brand">
          <span className="brand-mark" aria-hidden="true" />
          <div>
            <h1>nano-mimiclaw from Scratch</h1>
            <p>把 MimiClaw 从零写一遍</p>
          </div>
        </div>
        <nav className="screen-nav">
          <button className={screen === "read" ? "active" : ""} onClick={() => setScreen("read")}>
            阅读
          </button>
          <button className={screen === "trace" ? "active" : ""} onClick={() => setScreen("trace")}>
            TraceLab
          </button>
          <a href="https://github.com/memovai/mimiclaw" target="_blank" rel="noreferrer">
            MimiClaw
          </a>
        </nav>
      </header>
      {screen === "read" ? <Reader /> : <TraceLab />}
    </div>
  );
}
