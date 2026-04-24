# Team

| Agent | Domain (summary) | File |
|---|---|---|
| (project-owner) | Product direction, priorities, merge approval | — (the human user) |
| producer | Backlog, task tracking, cross-scope coordination | `.claude/agents/producer.md` |
| rendering-researcher | Rendering R&D, shader implementation | `.claude/agents/rendering-researcher.md` |
| graphics-api-expert | DX12 / Vulkan glue, GPU resource plumbing | `.claude/agents/graphics-api-expert.md` |
| software-architect | Service architecture, code organization, serialization (code-data coupling) | `.claude/agents/software-architect.md` |
| low-level-expert | Threading, memory, IO, math | `.claude/agents/low-level-expert.md` |
| platform-expert | OS / windowing / HID | `.claude/agents/platform-expert.md` |
| ci-build-expert | Git clone → build → scripts | `.claude/agents/ci-build-expert.md` |
| ai-expert | Harness: hooks, agents, skills, commit-gates | `.claude/agents/ai-expert.md` |
| test-expert | Test strategy, tiers, example project, game-logic | `.claude/agents/test-expert.md` |
| editor-tooling-expert | Source/Editor-Next (Vue / Electron / TypeScript / IPC) | `.claude/agents/editor-tooling-expert.md` |
| paper-auditor (subagent) | Fresh-context audit of paper-port implementations | `.claude/agents/paper-auditor.md` |

## Dispatch

Main-session Claude is a dispatcher. The pattern per turn:

1. Read the scope declared in the owned subtrees' `CLAUDE.md` and the staged-file paths.
2. Identify the responsible agent(s).
3. Invoke them via the `Agent` tool.
4. Relay results; enforce universal gates on any commit.

Work that genuinely crosses agent domains is coordinated through the `producer` agent, not by individual agents stepping outside their scope.

## Universal gates

Fundamentals that apply to every CL regardless of which agent produced it live in `.claude/hooks/commit-gate.js`: file size, attribution, closure evidence with test-run backing. Agent-specific disciplines (paper-port alignment, serialize-determinism round-trip, live-engine Playwright) live in the agent's own working agreement and are enforced through the artifacts the agent produces.
