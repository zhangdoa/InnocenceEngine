# Discipline: comment-discipline

## Rule

Default: zero comments. Comment only when:

- The WHY is non-obvious from any feasible refactor (hidden invariant, workaround for external bug, hardware quirk, paper-port citation).
- A reader of the surrounding code would otherwise stop and ask "why this and not the obvious thing?"

If a comment restates what the code says, delete it. If the code is unclear, refactor it.

## Banned in source

- Multi-line essays above accessors / paraphrases of the function signature.
- Inline task IDs or commit hashes (`// TASK-213 CL 3.5: ...`, `// per a86e6e93`).
- Design-history narration (`// Replaces CL 3's pool-iteration shape`, `// Was X before Y`).
- WHAT-the-code-does prose (`// Drain the queue, then check if empty, then log`).
- Per-CL design rationale — belongs in the commit message.
- Cross-reference footnotes (`// See also: TextureResourceService.h:42`). Document constraints once at the source of truth.

## Refactor instead of comment

- Rename the function/variable so the name carries the meaning.
- Extract a helper with a self-describing name.
- Promote a magic value to a named constant.
- Split an over-stuffed function so each piece has obvious intent.
- Use the type system — sentinel-via-type, narrow-by-type, deny-by-construction.
- Move the explanation to the right venue — commit message, design doc, backlog task. Not source.

## Acceptable

- One-line WHY at the surprising spot.
- Paper / spec citation: `// Heitz 2014 §4.2`.
- TODO with a concrete next step: `// TODO: handle wraparound — see <spec>`.
- File-level header where the file has a non-obvious organizing principle.

## Self-application — discipline files and agent manifests

Files under `.claude/disciplines/`, `.claude/agents/`, and other harness instruction files are operational rules for agents to act on. Same imperative register as code:

- Rules and conditions ("when X → do Y").
- Structured tables and bullets over prose paragraphs.
- No roleplay ("you are X", "the Producer's value is Y").
- No rationale paragraphs explaining why a rule exists, beyond what's needed to apply it.
- No incident narratives recoverable from `git log` / backlog.
- No meta-commentary ("this discipline exists because...", "this rule closes that gap").

Source code under `Source/`, `Scripts/`, `.claude/hooks/` follows the no-comment rule above.

## Cross-references

- `always/fundamentals.md` § Comments.
- `on-commit/commit-message-policy.md` — venue for design rationale.
- `on-implement/no-shadow-state.md` — don't duplicate state into comments.
