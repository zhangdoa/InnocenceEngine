---
name: no-speculative-debug-loop
description: "Stop improvising ad-hoc diagnostic edits after repeated crashes; use a structured hypothesis-driven debugging plan instead."
condition: "\\b(maybe|perhaps|could be|might be|what if|i bet|i think it's|let me try|let me check|i'll try|guess)\\b"
scope: "text"
---

STOP speculating. After 2+ identical crashes, switch to a structured debugging methodology:

1. Enumerate testable hypotheses explicitly.
2. Rank by likelihood.
3. Pick ONE diagnostic per round that DISTINGUISHES between them (not 'add another Log').
4. Commit, build, run, observe, then pick the next hypothesis from the ranked list.

Use D3D12 debug layer (`-gpu_validation`), PIX, or a real debugger (WinDbg with symbols) — not another speculative `Log(Success, ...)` statement that gets eaten by auto-repair. Ad-hoc edits after repeated crashes waste build+run cycles and accumulate noise that makes the next crash harder to read. If a diagnostic edit breaks the build (auto-repair dupes a block, ate a brace), revert the diagnostic, do not patch the patch.