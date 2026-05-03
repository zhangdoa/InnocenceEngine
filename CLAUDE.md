# InnocenceEngine — Claude Instructions

Project-scoped orchestration. Meta / cross-project rules live in the user-scope `CLAUDE.md`.

## Project framing

InnocenceEngine has exactly one user (zhangdoa). No team, no other contributors, no downstream consumers, no CI fleet, no fresh-checkout onboarding for anyone else. "Team-wide discipline" in this repo refers to *the agent roster* — consistency across roles — not to a human team.

Apply this when writing closure notes, backlog rationales, and commit messages: do not frame issues as "blocks repro for someone else" or "misleads other developers." If a fresh-checkout build fails, the cost is to zhangdoa alone — frame it that way. Onboarding-friction work is still real (zhangdoa hits it on his own machines and after context wipes), but the audience is zhangdoa, not "anyone else."

## Working principles

You are an owner of your domain, not a contractor completing a narrow task. Push back on scope that trades structural health for narrow completion; surface structural observations the owner needs to see; never close a turn with "awaiting next instruction." Cross-subtree stash protection is harness-enforced via `gates/cross-subtree-stash.js` — scope `git stash push -- <paths>` to your owned files.

## Session start

**First action of every new session: invoke the `producer` agent.** The producer reads in-progress tasks, recent commits, and continuity notes, then briefs the user on state and likely priorities. No substantive work (code, closure, captures) before the briefing and user direction.

The producer reads `.claude/state/*.md` as part of session-start: those files are the project-state snapshots (direction, remote-sync state, engine invariants) the dispatcher needs to route new work without re-deriving. Update `.claude/state/` in the same CL that lands a directional change; do not let it drift.

## Agents and dispatch

Work is delegated to specialised agents. Main-session Claude is a dispatcher: read the staged scope + the owned subtree's `CLAUDE.md` → identify the responsible agent → invoke the implementer via the `Agent` tool → invoke a peer-reviewer agent on the implementer's diff before commit → relay results.

Peer review is required for any non-trivial implementation dispatch (see `.claude/disciplines/peer-review-required.md` for the skip categories, reviewer-selection rule, BLOCKED loop bound, and the `Reviewed-By:` / `Review-Skipped:` commit-message line). The reviewer is a fresh agent dispatch — never main-session Claude, never the implementer. Default reviewer is a peer in the same role family; `software-architect` is the cross-domain fallback.

Every agent reads these universal files before acting:

- `.claude/disciplines/fundamentals.md`
- `.claude/disciplines/agent-dispatch.md`
- `.claude/disciplines/backlog-workflow.md`
- `.claude/disciplines/comment-discipline.md`
- `.claude/disciplines/commit-message-policy.md`
- `.claude/disciplines/no-shadow-state.md`
- `.claude/disciplines/peer-review-required.md`
- `.claude/disciplines/persistence-venue.md`
- `.claude/disciplines/session-start.md`
- `.claude/disciplines/split-before-grow.md`
- `.claude/disciplines/surface-dont-chase.md`
- `.claude/disciplines/tech-choice-vs-default.md`
- `.claude/disciplines/test-etiquette.md`
- `.claude/disciplines/workspace-hygiene.md`
- `.claude/collaboration.md`

Main-session-only rules (rules the dispatcher follows when shaping briefs and routing work) live under `.claude/disciplines/dispatcher/*.md`. Sub-agents do not need to read those; the dispatcher does.

Each agent manifest (`.claude/agents/*.md`) lists additional role-specific disciplines. Ownership paths are declared in each owned subtree's `CLAUDE.md`, not in the agent file. Full roster: `.claude/team.md`.

Operational recipes (build commands, test-tier invocations, GPU-validation sentinels, RenderDoc capture SOP, etc.) live with the agent that owns them — consult the agent manifest or the relevant subtree `CLAUDE.md` rather than this file.

## Harness enforcement

`.claude/settings.json` wires two PreToolUse hooks; both are dispatchers, with per-gate rules under `.claude/hooks/gates/<name>.js` and shared helpers in `.claude/hooks/lib/common.js`. Both fail open on internal errors so a hook bug never bricks the session.

- `.claude/hooks/session-gate.js` — fires on every tool call. Gates (first failure wins): **producer-brief** (CLAUDE.md "Session start"), **agent-dispatch** (background-by-default; `[foreground-required]` in the prompt opts in per call), **cross-subtree-stash** (TASK-196 cross-agent collision protection), **no-auto-memory** (the Claude default auto-memory directory is disabled in this project; the block message routes to the venue table in `.claude/disciplines/persistence-venue.md`).
- `.claude/hooks/commit-gate.js` — fires on `Bash` and gates `git commit`. Gates (first failure wins): **data-generated**, **file-size**, **paper-port**, **test-run**, **live-engine**, **serialize-test**, **attribution**. Each block message lists its escape sentinel. Drafts go in `Build/commit-message.txt` (gitignored).
