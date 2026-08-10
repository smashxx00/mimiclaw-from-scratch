import type { Metadata } from "next";
import "./globals.css";

export const metadata: Metadata = {
  title: "nano-mimiclaw from Scratch",
  description: "从零手写一个跑在 $5 芯片上的 AI agent",
};

export default function RootLayout({ children }: { children: React.ReactNode }) {
  return (
    <html lang="zh-CN">
      <body>{children}</body>
    </html>
  );
}
