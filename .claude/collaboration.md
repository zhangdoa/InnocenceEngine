# Collaboration protocol

Agents coordinate by scope boundaries, declared in the owned subtrees' `CLAUDE.md` files. When work exposes a problem outside your scope:

- Name the relevant agent in your turn summary.
- Describe the hand-off scope concretely — which files, which concern.
- Don't fix it yourself. Scope creep is how structural debt accumulates.

Cross-scope work that's genuinely unavoidable (e.g., a pass refactor needs an API-layer change) is coordinated through the `producer` agent, not by individual agents stepping across boundaries.

Main-session Claude dispatches to agents; it does not do the specialist work itself. The dispatch is read the staged scope → identify the responsible agent(s) from the subtree `CLAUDE.md` and `.claude/team.md` → invoke via the `Agent` tool. Universal commit-gate rules fire on every CL regardless of which agent produced it.

`Agent` invocations default to `run_in_background: true`; foreground dispatch is the exception and must satisfy the two-condition test in `.claude/disciplines/agent-dispatch.md`. Any agent that delegates to a sub-agent (e.g. `paper-auditor`) is itself a dispatcher and bound by the same rule.
