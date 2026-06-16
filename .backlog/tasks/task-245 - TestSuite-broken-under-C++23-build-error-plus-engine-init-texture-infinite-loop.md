---
id: TASK-245
title: >-
  TestSuite broken under C++23: build error + engine-init texture infinite-loop
status: To Do
assignee:
  - code-impl
created_date: '2026-06-16'
labels:
  - testing
  - c++23
  - bug
  - infra
dependencies:
  - TASK-240
priority: medium
---

## Description

Surfaced while verifying TASK-243. The `TestSuite` target (`Source/TestSuite/`) is
non-functional after the C++23 migration (TASK-240 / `51c83b30`). It was never
recompiled during that migration (stale-mtime incremental-build skip), so two
breaks went unnoticed.

### Issue 1 — does not compile under C++23

`Source/TestSuite/TestSuite.cpp:11` passes a string literal to `Engine::Setup`'s
`char*` 3rd parameter:

```
error C2664: 'bool Inno::Engine::Setup(void*,void*,char*,...)':
cannot convert argument 3 from 'const char [9]' to 'char *'
```

C++23 removed the deprecated string-literal → `char*` conversion. Minimal local fix
(convention-respecting — `Setup` takes `char*`, Main passes `lpCmdLine`):

```cpp
char l_cmdline[] = "headless";
if (!l_pEngine->Setup(nullptr, nullptr, l_cmdline, nullptr, nullptr))
```

(Cleaner alternative: change `Engine::Setup` arg 3 to `const char*` and update all
callers — wider blast radius.)

### Issue 2 — engine-init infinite-loop (the real blocker)

After Issue 1 is patched and TestSuite links, running `TestSuite.exe unit` with the
default (non-offscreen) config infinite-loops during `TextureResourceService::
InitializeComponents` (Template Assets init), spewing ~3M lines / 410MB in 90s:

```
[Error] DX12Helper::GetTextureMipLevels Invalid texture dimensions (Width:0,Height:0,Depth:0)
[Error] DX12TextureResourceService::InitializeImpl [Object Name: ] Failed to create default heap buffer
[Warning] TextureResourceService::InitializeComponents entity N no longer has TextureComponent, using stored pointer
```

It cycles entities 1–4 forever. Two coupled defects:
1. **Unbounded retry** — failed texture inits are re-enqueued without a cap
   (same class as TASK-242a's RenderPassResourceService fix, not applied here).
2. **Dangling component** — "entity N no longer has TextureComponent, using stored
   pointer" → a removed `TextureComponent` is reprocessed via a stale pointer, and
   the 0-dim placeholder fails `GetTextureMipLevels` every iteration.

Unit tests (containers, RenderGraph serializer) never run because engine
`Initialize()` never returns.

### How to reproduce

```
cmake --build Build --target TestSuite --config RelWithDebInfo   # Issue 1 here
# after patching TestSuite.cpp:
cd Bin && RelWithDebInfo\TestSuite.exe unit                      # Issue 2 hangs
```

## Acceptance Criteria

- [ ] #1 TestSuite compiles under C++23.
- [ ] #2 `TestSuite.exe unit` runs to completion and prints TEST RESULTS without an
      init loop.
- [ ] #3 Texture-init retry is bounded (cap + dead-letter) and the dangling-component
      reprocessing is fixed.
- [ ] #4 Unit suite green; `Failed: 0`.

## Definition of Done

- [ ] #1 Code compiles
- [ ] #2 `TestSuite.exe unit` exits cleanly with a readable TEST RESULTS summary
- [ ] #3 Root cause of the dangling TextureComponent documented
