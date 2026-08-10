export type TraceSource = {
  file: string;
  line: number;
};

export type TraceStep = {
  id: string;
  label: string;
  detail: string;
  kind: "input" | "request" | "response" | "tool" | "end" | "error";
  source: TraceSource;
  event: Record<string, unknown>;
  context: Record<string, unknown>;
};

export type TraceCase = {
  id: string;
  number: string;
  title: string;
  summary: string;
  prompt: string;
  outcome: string;
  steps: TraceStep[];
};
