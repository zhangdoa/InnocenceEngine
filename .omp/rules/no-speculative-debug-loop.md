---
name: no-speculative-debug-loop
description: "Stop improvising ad-hoc diagnostic edits after repeated crashes; use a structured hypothesis-driven debugging plan instead."
condition: "(another|one more)\\s+(Log|print|trace|breakpoint)|let me (just )?try (another|a different|something)|trial.and.error|throw .*see if|keep (trying|guessing)"
scope: "text"
---

After 2+ identical crashes, switch to a structured method:

1. Enumerate testable hypotheses explicitly.
2. Rank by likelihood.
3. Pick ONE diagnostic per round that DISTINGUISHES between them (not 'add another Log').
4. Build, run, observe, then pick the next hypothesis.

Use the D3D12 debug layer (`-gpu_validation`), PIX, or a real debugger — not another speculative `Log(Success, ...)`. If a diagnostic edit breaks the build, revert it; do not patch the patch.
