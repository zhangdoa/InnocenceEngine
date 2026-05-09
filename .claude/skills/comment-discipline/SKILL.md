---
name: comment-discipline
description: Use when editing harness instruction files (.claude/skills/, .claude/agents/, subtree CLAUDE.md). Self-application register: imperative, structured, no roleplay, no rationale paragraphs, no incident narratives.
---

# Skill: comment-discipline

Generic comment policy ("default zero, comment only when WHY is non-obvious, refactor instead of comment") lives at user level. This skill covers project-specific anti-patterns and the **self-application register** for harness files.

## Banned in source

- Multi-line essays above accessors / paraphrases of the function signature.
- Inline task IDs or commit hashes (`// TASK-213 CL 3.5: ...`, `// per a86e6e93`).
- Design-history narration (`// Replaces CL 3's pool-iteration shape`, `// Was X before Y`).
- WHAT-the-code-does prose (`// Drain the queue, then check if empty, then log`).
- Per-CL design rationale — belongs in the commit message.
- Cross-reference footnotes (`// See also: TextureResourceService.h:42`). Document constraints once at the source of truth.

## Banned in commit messages and closure notes

- "now via X / used to be Y", "replaces / subsumes / retires", "migrated from", "deleted alongside".
- "first consumer in next commit", "added in `<sha>`", "before this fix", "fixed in TASK-NN".

## Self-application — harness files

Files under `.claude/skills/`, `.claude/agents/`, every subtree `CLAUDE.md`, and other harness instruction files are operational rules for agents to act on. Same imperative register as code:

- Rules and conditions ("when X → do Y").
- Structured tables and bullets over prose paragraphs.
- No roleplay ("you are X", "the Producer's value is Y").
- No rationale paragraphs explaining why a rule exists, beyond what's needed to apply it.
- No incident narratives recoverable from `git log` / backlog. No specific dates, timestamps, or session anecdotes.
- No `TASK-N` references, commit SHAs, or "Related work" footnotes. Tasks close, get re-numbered, or get deleted; the rule has to stand without them.
- No meta-commentary ("this skill exists because...", "this rule closes that gap").

Subtree CLAUDE.md is the venue for all subtree conventions. Same register as the skills: rules, escape hatches, inventory tables. Not narrative, not rationale, not history.

Source code under `Source/`, `Scripts/`, `.claude/hooks/` follows the no-comment rule from user-level CLAUDE.md.

## Cross-references

- `commit-message-policy` — venue for design rationale.
- User-level `~/.claude/CLAUDE.md` § Working principles — generic comment policy.
