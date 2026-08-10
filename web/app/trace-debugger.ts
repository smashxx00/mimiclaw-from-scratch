import type { TraceCase, TraceStep } from "./trace-types";

export type DebugFrame = TraceStep & {
  variables: Record<string, unknown>;
};

export function buildDebugFrames(traceCase: TraceCase): DebugFrame[] {
  return traceCase.steps.map((step) => ({
    ...step,
    variables: {
      ...step.event,
      ...(step.context.messages ? { messages: step.context.messages } : {}),
    },
  }));
}
