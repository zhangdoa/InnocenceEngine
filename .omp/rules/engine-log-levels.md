---
name: engine-log-levels
description: "Engine Log() levels: failures always Error/Warning; Success/Info only at startup milestones; per-frame paths Verbose or silent."
condition: "Log\\s*\\(\\s*(Success|Info)"
scope: ["tool:edit(*.cpp)", "tool:edit(*.h)", "tool:edit(*.hpp)", "tool:edit(*.inl)", "tool:write(*.cpp)", "tool:write(*.h)", "tool:write(*.hpp)", "tool:write(*.inl)"]
---

| Level | Use |
|---|---|
| `Error` / `Warning` | every failure path (setup/init/parse/one-shot) — never a silent `return false` |
| `Success` / `Info` | non-repeating startup milestones only |
| `Verbose` | per-frame / per-pass; off by default |

`Log(Success|Info)` in a per-frame path (render, update, tick) is forbidden — demote to `Verbose`. Diagnostic logs are removed before commit.
