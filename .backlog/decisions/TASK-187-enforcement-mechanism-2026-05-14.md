# TASK-187 — Sub-agent skill-compliance enforcement: mechanism choice

**Status**: design only (AC #1). Implementation deferred to a follow-up dispatch.
**Author stage**: harness-impl design pass, 2026-05-14.
**Scope**: pick the mechanism that turns "sub-agent should load skill X" into
"sub-agent has loaded skill X" with verifiable evidence. Skill content itself is
not in scope (TASK-186).

## Current-state walk

Terminology has drifted since the task was filed (2026-04-28). The current shape:

- Skills live at `.claude/skills/<name>/SKILL.md` and at user level
  `~/.claude/skills/<name>/SKILL.md`. The old `.claude/disciplines/` path no
  longer exists.
- `tech-choice-vs-default.md` is now `tech-choice` at user level.
- Agent manifests (`.claude/agents/*.md`) are short — they enumerate
  "always-apply skills" and "conditional skills" **by name**, not by content
  (see `code-impl.md:8`, `shader-impl.md:8`, `harness-impl.md:8`).
- Claude Code auto-loads skills **by description match** at session start and
  re-lists name + description in every system-reminder. The bodies (`SKILL.md`)
  are not in context until the agent explicitly calls the `Skill` tool or `Read`s
  the file.
- Two PreToolUse hook dispatchers are already wired in `.claude/settings.json`:
  `session-gate.js` (universal) and `commit-gate.js` (Bash-only). Sub-gates
  follow a `{ run, block }` shape (`.claude/hooks/session-gate.js:30`).
- One SessionStart hook exists (`session-start.js`) that injects
  `additionalContext` for the task-mgmt briefing directive — proof that the
  harness can push content into a session before the first tool call
  (`hooks/gates/session-start-brief.js:25`).

**The gap**: a sub-agent dispatched mid-session sees its always-apply skills
listed by name in its system-reminder. To get the *content*, the sub-agent must
choose to call `Skill` or `Read SKILL.md` first. Nothing forces it. An obedient
agent does it; a pattern-matching agent can skip it and still produce output
that looks compliant.

## Decision

**Picked mechanism**: a **PreToolUse skill-evidence gate** that sits in
`session-gate.js`, scoped to sub-agent transcripts only, that blocks any
non-passive tool call (Write / Edit / Bash with side effects) until the
sub-agent's transcript shows a `Skill` invocation for every skill listed in its
manifest's "Always-apply skills" line. The gate reads the same per-sub-agent
JSONL the commit-gate already resolves via `resolveActiveTranscriptPath`
(`hooks/lib/common.js:195`).

Passive tools (`Read`, `Glob`, `Grep`, `ToolSearch`, `Skill` itself) remain
free so the agent can satisfy the gate.

### Three references (per skill `tech-choice`)

- **(a) Training-corpus default**: inline the skill content directly into each
  agent manifest body (option 1 in the task description). The textbook
  LLM-tooling answer — "just put the rules in the system prompt." Rejected: it
  pins every dispatch to the union-of-all-skills (~3-15 kB depending on the
  stage), drifts the moment a skill is edited without a manifest re-sync, and
  loses the ability for `comment-discipline` / `safety-observability` etc. to
  evolve independently of agent definitions. The drift audit (AC #4) would have
  to be reinvented as a separate script.
- **(b) Current SOTA**: agent-internal acknowledgment / structured first-turn
  block (option 4). Equivalent to "the agent must paraphrase the skill before
  acting," analogous to GitHub Copilot Workspace's plan-first protocol and
  Anthropic's published "verify before acting" pattern in agent-loop demos.
  Rejected: brittle to format drift, costs a full turn, and validates
  *paraphrasing* not *loading*. An agent that hallucinates plausible
  paraphrases still passes.
- **(c) Recent project precedent**: `task-mgmt-brief` gate
  (`.claude/hooks/gates/task-mgmt-brief.js:32`) plus `peer-review` gate
  (`.claude/hooks/gates/peer-review.js:24`). Both establish the
  "transcript-derived evidence" pattern: the gate doesn't trust the agent's
  word — it scans the transcript for a specific tool-use signature and blocks
  until it appears. This is the project's load-bearing precedent for converting
  prose rules into mechanical enforcement.

**Pick: (c)**, extended to per-sub-agent transcripts and per-manifest skill
lists.

### Why (c) beats each alternative for this project

- **vs. (1) inline manifests**: zero drift risk (the source-of-truth `SKILL.md`
  is *read*, not *copied*). No token-budget cost on dispatches that don't
  reach for side-effecting tools (e.g. a research dispatch that only `Read`s
  and replies textually pays nothing).
- **vs. (2) auto-include directive expansion**: same drift advantage but
  without inventing a new templating mechanism. The harness already has
  transcript-scan helpers (`scanTranscriptForTaskMgmtBrief`,
  `resolveActiveTranscriptPath`); the new gate is a sibling of an existing
  pattern, not a new one.
- **vs. (3) dispatcher-side brief inspection**: catches dispatcher noise but
  not what the user actually flagged ("the sub-agent must comply"). The
  user's framing is agent-side, not dispatcher-side.
- **vs. (4) acknowledgment block**: validates *behaviour*
  (`Skill` tool was called) instead of *prose* (the agent claims it read).
  Cheap for an obedient agent (one extra tool call per skill), expensive
  for a pattern-matching one — exactly the asymmetry the user wants.
- **vs. (5) hybrid (1+3)**: (3) is the weak half; replacing it with (c)
  obsoletes the hybrid framing.

## Minimum-viable implementation outline

Files this design proposes — listed as shapes, not diffs:

| Where | New / edit | Shape |
|---|---|---|
| `.claude/hooks/gates/skill-evidence.js` | new | The sub-gate. Exports `{ run }` matching the `session-gate.js` contract. On non-passive tool calls inside a sub-agent transcript, parses the active agent manifest's "Always-apply skills" line, scans the sub-agent's JSONL for `Skill(skill="<name>")` tool-uses, blocks if any required skill is missing. Fail-open on any I/O error. |
| `.claude/hooks/lib/common.js` | edit | Add `parseAlwaysApplySkills(manifestPath)` — pulls names from the manifest's first body line via a fixed regex. Add `scanTranscriptForSkillUses(xpPath)` returning `Set<skillName>`. Both ~15 LoC; both shareable with a future drift-audit script. |
| `.claude/hooks/session-gate.js` | edit | Append `require('./gates/skill-evidence')` to the GATES array, ordered after `task-mgmt-brief` and before `no-auto-memory`. |
| `.claude/settings.json` | unchanged | The session-gate dispatcher is already wired. |
| `.claude/skills/peer-review-required/SKILL.md` | edit | One-line cross-reference to the new gate under § Cross-references. |
| `CLAUDE.md` (root) § "Harness enforcement" | edit | One bullet naming the new gate, mirroring the existing entries. |
| `.backlog/tasks/task-187*.md` § Implementation Notes | edit | Reference this decision doc. |

**Minimum-viable slice**: enforce on the three most-active impl stages first —
`code-impl`, `shader-impl`, `harness-impl`. Their manifests already declare
"Always-apply skills" in a uniform shape. `task-mgmt` and `ci-build-impl` slot
in once the parse regex is proven against the first three. The gate's
fail-open posture means an unknown manifest format → silent pass, never
silent block.

## Trade-offs

- **Cost**: one transcript-scan per non-passive tool call inside a sub-agent
  (cheap — JSONL parse, same path as `task-mgmt-brief` already pays). One
  extra `Skill` invocation per skill per sub-agent dispatch (negligible token
  cost; the skill bodies are already in the model's working set after the
  invocation, which is the whole point).
- **Coverage**: catches sub-agents that proceed to side-effecting work without
  loading their declared skills. Does **not** catch a sub-agent that calls
  `Skill` then ignores the content — that's an alignment problem, not a
  mechanical one, and is the natural ceiling for any non-LLM-judge mechanism.
  Conditional-skills enforcement (e.g. "paper-port label → load `paper-port`")
  is out of scope for v1; the always-apply set is the load-bearing one.
- **Drift risk**: low. The gate parses live manifests at PreToolUse time —
  there's no copy of skill content to go stale. If a manifest renames a skill,
  the next dispatch fails loudly (block message names the missing skill);
  if a `SKILL.md` is deleted but still listed in the manifest, the agent's
  `Skill` call fails first and the gate observes the absence. Either failure
  surfaces in the same dispatch, not a later one.

## Verification approach (AC #3)

A round-trip dispatch test, runnable manually, no new infrastructure:

1. Dispatch `harness-impl` with a brief that requires zero file writes but
   *does* require an Edit/Write at the end (e.g. "draft a one-line addition
   to `.claude/state/<existing-file>.md`").
2. Observe: before the first Write, the sub-agent's transcript must contain
   `Skill(skill="fundamentals")`, `Skill(skill="comment-discipline")`, etc.
   for every name on the manifest's always-apply line.
3. Negative case: a doctored sub-agent that skips the `Skill` calls and goes
   straight to Write hits the gate's block message naming the missing skill.

The cheapest "agent has the content available" proof is the `Skill` tool-use
itself — when the tool returns the skill body, the body is in the context
window by the runtime contract. No separate read-back / paraphrase step
needed; that's the (b)-SOTA approach we rejected for being brittle.

## Out of scope

- Main-session (dispatcher) skill compliance. CLAUDE.md is auto-loaded;
  dispatcher discipline is enforced by `dispatch-briefs` gate work, not this
  one.
- Conditional-skill enforcement (skills triggered by task labels / file
  shapes). Always-apply first; conditionals are a v2 once the always-apply
  evidence pattern is observed-stable.
- User-level skills referenced from project manifests (e.g. `safety-principles`
  on `code-impl`). The gate treats them identically — the `Skill` tool resolves
  user-level skills the same way — but the implementer should confirm the
  Claude Code skill-resolution behaviour matches the assumption when v1 lands.
- Skill *quality* / agent compliance with the loaded content. Mechanical
  presence is the ceiling here; correctness of behaviour belongs to peer review.
