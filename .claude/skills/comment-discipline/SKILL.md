---
name: comment-discipline
description: Use when editing harness instruction files in this project (.claude/skills/, .claude/agents/, subtree CLAUDE.md). Project-specific anti-patterns + the self-application register. Generic comment policy lives at user level.
---

# Skill: comment-discipline (project extension)

Generic comment policy (default zero, comment only when WHY is non-obvious, refactor instead of comment, register for AI-instruction files) lives in user-level `comment-discipline`. This skill carries project-specific anti-patterns.

## Banned in project source

- Multi-line essays above accessors / paraphrases of the function signature.
- Inline task IDs or commit hashes (`// TASK-213 CL 3.5: ...`, `// per a86e6e93`).
- Design-history narration (`// Replaces CL 3's pool-iteration shape`, `// Was X before Y`).
- WHAT-the-code-does prose (`// Drain the queue, then check if empty, then log`).
- Per-CL design rationale — belongs in the commit message.
- Cross-reference footnotes (`// See also: TextureResourceService.h:42`). Document constraints once at the source of truth.

## Banned in commit messages and closure notes

- "now via X / used to be Y", "replaces / subsumes / retires", "migrated from", "deleted alongside".
- "first consumer in next commit", "added in `<sha>`", "before this fix", "fixed in TASK-NN".

## Self-application register — harness files

Files under `.claude/skills/`, `.claude/agents/`, every subtree `CLAUDE.md`, and other harness instruction files apply the user-level register: imperative, structured, no roleplay, no rationale paragraphs, no incident narratives, no `TASK-N` / SHA references, no meta-commentary.

Subtree CLAUDE.md is the venue for all subtree conventions. Same register as the skills: rules, escape hatches, inventory tables. Not narrative, not rationale, not history.

Source code under `Source/`, `Scripts/`, `.claude/hooks/` follows the no-comment rule from user-level CLAUDE.md.

## Cross-references

- User-level `comment-discipline` — generic comment policy + AI-instruction-file register.
- `commit-message-policy` — venue for design rationale.
