---
name: prefer-ast-rewrites
description: "Prefer AST-based rewrites (ast_edit / ast_grep) over plain-text edit calls for any non-trivial source change. Plain text edit is only for trivial single-line string swaps with full file context already in scope."
condition: "\\b(edit\\b.*\\bto\\b|replace\\b.*\\bwith\\b|s/\\b|edit_file|search_and_replace)"
scope: "text"
---

For any non-trivial source modification, prefer `ast_edit` / `ast_grep` over
plain-text `edit` calls. AST rewrites are structural — they match AST nodes
(metavariables, parens, braces), not byte substrings, so they don't break
when formatting changes, comments shift, or whitespace re-arranges.

## When to use AST

- Any rename or replace that crosses a `(` `)` `,` `;` boundary where
  adjacent lines could have the same substring.
- A pattern that has `$VAR` or `$$$REST` shape — i.e. "this thing plus a
  bunch of args I don't care about".
- A repeated edit across multiple files for the same pattern.
- Any change where the pattern matches the **shape** of the code, not the
  bytes.

## When plain-text edit is fine

- Single literal-string swap, e.g. `"foo"` -> `"bar"`, where you have
  fresh read of the exact line and the body has no interpolation.
- Adding a member to a struct/class with a known fixed signature.
- Removing a line that you have literally displayed in the read result.

## Why

- AST rewrites can't accidentally eat neighbours that happen to share a
  substring. The plain-text `edit` tool does a literal byte replace; a
  pattern like `Log(Error, $X, " CreateOutputMergerTargets: ", $Y)` will
  match anywhere the leading bytes line up, including log lines in other
  functions.
- `ast_edit` reports the matched regions before applying. If the
  replacement set is wider than intended, the call fails — no silent
  patch-the-patch.
- AST patterns document intent: `$X` says "an expression here" rather
  than "these specific bytes", which is what we mean.

## Pitfalls

- AST `pat` is single-node. A multi-line block (e.g. whole function body)
  needs to be wrapped in a valid context (`class $_ { ... }`,
  `if ($X) { ... }`).
- `$NAME` captures one **whole AST node**, not a prefix-suffix. Match the
  whole node, not `"prefix$NAME"` style.
- If `ast_edit` reports overlapping replacements, refine the patterns so
  each match's region is disjoint. Do not split into separate calls that
  can race against each other's line numbers.
