---
id: TASK-56
title: Add unit tests for FixedSizeString after off-by-one truncation bug
status: Todo
assignee: []
created_date: '2026-04-17 17:30'
labels:
  - testing
  - reliability
  - foundational
dependencies: []
priority: high
---

## What happened

A single-line off-by-one in `FixedSizeString(const char*)` and
`operator=(const char*)` silently dropped the last character of every
C-string assignment. The bug likely existed since the string class was
introduced. It went undetected for years because:

1. **Self-consistency masked the truncation** — any code assigning then
   reading the same FixedSizeString got the same (shorter) value.
2. **Peer-consistency masked it further** — two FixedSizeStrings built
   from the same literal compared equal.
3. **Developers worked around it implicitly** — shader paths across 30+
   pass files were written as `"FooPass.hlsl/"` with a trailing slash
   that the truncation "cleaned up" to produce a valid path.
4. **Log output looked like a different kind of bug** — "MeshComponen"
   instead of "MeshComponent" in logs reads as a log-buffer issue, not
   a string-storage corruption.

It surfaced only when one side stored a FixedSizeString and the other
used the raw C-string: `MeshLUT[name]` (full C-string key) vs.
`MeshLUT.erase(l_asset.m_Name.c_str())` (truncated stored-name key).
Release's erase silently did nothing, leaving a stale LUT entry that
pointed to a slot with a bumped generation. The next Allocate hit the
cached-handle path and returned a stale handle; GetMeshAsset saw the
generation mismatch and returned nullptr. Scene-reload broken.

## Structural finding

**Unenforced invariant: "FixedSizeString mirrors the C-string it's
constructed from."** Nothing in the codebase verified this. No test,
no assertion, no type-level guarantee.

## Action

Add `FixedSizeStringTest.cpp` with at minimum these cases:

- Construction from `const char*` preserves all characters (regression
  for the off-by-one).
- `operator=(const char*)` preserves all characters.
- Copy-construction and copy-assignment preserve content.
- Round-trip: `FixedSizeString s = "hello"; assert(strcmp(s.c_str(), "hello") == 0);`
- Boundary: content of length exactly `S` is truncated to `S-1` chars +
  null terminator (not to `S-2` like the old bug).
- Boundary: content of length `S-1` fits exactly with null at position S-1.
- `operator==(const char*)` compares correctly after const-char construction.

Integrate with whatever test harness exists for the engine (look for
RenderTest pattern and mirror it — RenderTest is the canonical
single-executable regression tier).

## Broader follow-up

The workarounds this bug induced (e.g. trailing `/` in 30+ shader paths)
were removed in commits 40993b51 and 7cf2b543. Still worth grepping for
other patterns where code might be relying on implicit truncation — for
example, any place that builds a string ending with a character expected
to get dropped.
