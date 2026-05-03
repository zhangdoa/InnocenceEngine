# Discipline: comment-discipline

Code self-explains. Comments are a refactoring failure that didn't get refactored. Before writing one, the question is whether better naming, smaller functions, named constants, or split responsibilities would carry the same meaning. Usually they would.

This discipline exists because every agent in the roster has produced bloated comment blocks this session despite `fundamentals.md` already saying "default to none." A dedicated doc makes the failure shapes concrete enough to recognize.

## The rule

Default: zero comments on new code. A comment is justified only when:

- The WHY is non-obvious from any feasible refactor (a hidden invariant, a workaround for an external bug, a hardware quirk, a paper-port citation).
- A future reader, looking at the surrounding code, would otherwise stop and ask "why this and not the obvious thing?"

If the comment restates what the code already says, delete it. If the code is unclear and a comment would explain it, refactor the code instead.

## Anti-patterns observed in this codebase

- **Multi-line essay above an accessor.** Function name says `IsDeferredQueueEmpty`; comment doesn't need to say "Returns true if the deferred queue is empty." The function header is enough.
- **Restating the function name.** If the comment paraphrases the signature, delete it.
- **Citing task IDs or commit hashes inline** (`// TASK-213 CL 3.5: ...`, `// per a86e6e93`). Source code is not the audit trail. Backlog tasks rot, commit hashes turn opaque, future readers don't have the context. Put rationale in commit messages.
- **Documenting design history** (`// Replaces CL 3's pool-iteration shape`, `// Was X before Y`). Already banned by `fundamentals.md`. Repeating because it keeps recurring.
- **"WHAT the code does" prose** (`// Drain the queue, then check if empty, then log`). The reader can read the code; spell out only the WHY.
- **Per-CL design rationale.** Why a CL chose pattern A over B is a commit-message concern. Inline comments record the *current* state of the code, not the path that got there.
- **Footnote chains** (`// See also: TextureResourceService.h:42 caveat`). Cross-references inside source rot when files move. If the constraint is real, document it once at the source of truth, not at every consumer.

## Refactor instead of comment

When tempted to write a comment, try:

- **Rename** the function/variable so the name carries the meaning.
- **Extract** a helper with a self-describing name.
- **Promote a magic value** to a named constant.
- **Split** an over-stuffed function so each piece has obvious intent.
- **Use the type system** — sentinel-via-type, narrow-by-type, deny-by-construction.
- **Move the explanation to the right venue** — commit message, design doc, backlog task. Not source.

## Acceptable comments

- One-line WHY at the surprising spot. Not a paragraph.
- A paper/spec citation when porting an algorithm: `// Heitz 2014 §4.2`. Not "this implements the multibounce write site from Capsaicin gi1.comp:1948-1989 because..."
- A TODO with a concrete actionable next step: `// TODO: handle wraparound — see comment in <referenced spec>`.
- File-level header comments where the file itself has a non-obvious organizing principle (rare).

## Self-application

Discipline files (`.claude/disciplines/*.md`), design docs, and similar narrative-by-design files are exempt from this rule — they ARE prose. Source code under `Source/`, `Scripts/`, `.claude/hooks/` is not.

## Cross-references

- `fundamentals.md` § Comments — the original rule this doc concretizes.
- `commit-message-policy.md` — the right venue for design rationale.
- `no-shadow-state.md` — same shape (don't duplicate state into comments either).
