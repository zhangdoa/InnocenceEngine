---
name: no-language-speculation
description: "Verify language/framework ABI claims (C++/HLSL/GPU layout) by searching in this session via web_search / context7. No byte-offset or register-slot claims without a fresh check."
condition: "(stride|sizeof|offset|align|pack|alignas|alignof)|cbuffer.*\\b(register|slot|t\\d+|b\\d+|u\\d+|s\\d+)\\b|(C\\+\\+|HLSL|cbuffer|GPU|DXIL|D3D).*?\\b(64B|56B|32B|16B|128B|byte offset)\\b"
scope: "text,tool:edit(*.hlsl),tool:write(*.hlsl)"
---

Verify ABI-level claims before asserting. C++ struct layout, HLSL `StructuredBuffer<T>` stride, std140 packing, DXIL register conventions, D3D12 root signature rules look like confident facts but compound into broken fixes when wrong.

## Practice

- **Search this turn.** Before asserting any ABI fact, call `web_search` (Microsoft docs, HLSL spec, D3D12 spec, GitHub issues) or `context7` (libraries). The check happens in the current session; do not assume prior knowledge or pre-baked URLs are still current.
- If `web_search` / `context7` is unreachable this turn, mark the claim `[INFERENCE]` and do not act on it.
- One speculation error cascades; verify once per turn, assert once per turn.
- Do not paste links into the codebase to "document" the claim — the codebase is not a spec archive, the rule is to search when needed, not to memorize URLs.
