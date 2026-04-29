# Discipline: safety-observability

Applies to agents authoring C++ or HLSL. Runtime safety and observability rules: assertions on the hot path, guard clauses that log, RAII for resources, no magic numbers, no copy-paste. The operational form of `target-qualities.md` *fail loudly* for the languages where the compiler will not catch silent corruption.

## How

### Assertions (debug-time)

```cpp
assert(in_Position < m_ElementCount && "Out-of-boundary access.");
```

### Guard clauses must log

A silent `return false` hides bugs. Every early-out that rejects work must log why:

```cpp
if (gpuBuffer->m_ObjectStatus != ObjectStatus::Activated)
{
    Log(Warning, "WriteMappedMemory rejected for [", gpuBuffer->m_InstanceName,
        "]: ObjectStatus is ", static_cast<int>(gpuBuffer->m_ObjectStatus),
        ", expected Activated.");
    return false;
}
```

### Log restraint on hot success paths

The "guard clauses must log" rule is about failure paths. Success paths have the opposite hazard: a `Log(Success, ...)` placed inside a function that fires every frame, every tick, every input event, or every inner-loop element produces a flood that drowns the signal it was meant to provide.

**Rule.** Before adding a success-path log, the author of the call site asks "how often does this fire?" and answers in concrete terms (per frame, per slider step, per audio buffer, per scene load, per startup). Success-path logs are forbidden in any context that fires per frame, per tick, per input event, per inner-loop element, or at any other rate driven by a tight loop. Failure-path logs (`Warning`/`Error`) on the same paths remain mandatory — this rule only restricts success noise, not diagnostic noise.

**Heuristic.** If the call frequency is bounded by a render frame, an audio buffer fill, an input-poll cycle, a slider-drag step, or a per-element loop body, treat it as hot. If it fires once at startup, once per scene load, or once per explicit user command, it is cold and a success log is fine.

**Acceptable patterns at hot call sites.**

- Log nothing on success. The absence of a Warning/Error is the signal.
- Log only on state transitions, not on every successful repetition (e.g. "save target changed to X" once, not "saved to X" every keystroke).
- If a developer-time trace is genuinely useful, use `Verbose` and gate it behind a verbose-runtime switch — never `Success`/`Info` at top level.

### Null checks before use

```cpp
void Destroy(T* in_Ptr)
{
    if (!in_Ptr)
    {
        Log(Error, "Unable to destroy ", in_Ptr);
        return;
    }
    // ...
}
```

### RAII — always clean up in destructors

```cpp
~ObjectPool()
{
    if (m_HeapAddress)
    {
        g_Engine->Get<Memory>()->Deallocate(m_HeapAddress);
        m_HeapAddress = nullptr;
    }
}
```

### No magic numbers

Numeric literals with non-obvious meaning must use named constants. If a named constant is not feasible, a same-line comment must explain the value.

```hlsl
cmd.m_IndexFormat = DXGI_FORMAT_R32_UINT; // named constant in common.hlsl
// NOT: cmd.m_IndexFormat = 42;
```

```cpp
static constexpr uint32_t MaxCSMSplits = 4;
// NOT: for (int i = 0; i < 4; i++) { ... }
```

### No copy-paste — extract shared logic

When the same logic appears in two or more places, extract it into a shared function or include. Duplication signals a missing abstraction. In HLSL, shared helpers belong in `common/common.hlsl`. In C++, shared logic belongs in a service or utility.

## Recorded incident

**`TweakRegistry::SaveToFile`** (commit `ff28c0be`, `Source/ExampleProject/LogicClient/TweakRegistry.inl`) — log restraint on hot success paths. The function is called from `DrawWindowAutoSaving` whenever any registered ImGui slider changes value, i.e. every drag step on every tunable. The original implementation ended with `Log(Success, "TweakRegistry: saved to ", l_FullPath, ".");`, which spammed the log on every mouse-move while a slider was held. The two guard-clause `Log(Warning, ...)` calls above it were correct and stayed; the trailing `Log(Success, ...)` was the violation. Correct shape: drop the success log entirely (slider responsiveness is its own confirmation), or demote to `Verbose` if a save trace is wanted during development.

**TASK-140 → TASK-165** — per-pass-per-frame Verbose log spam from the GPU-timer path. The same hot-path pattern, surfaced in rendering instead of editor. Prompted the *log restraint* section above; also the precedent for `peer-review-required.md` (a fresh reviewer would have caught it).

## Cross-references

- `target-qualities.md` — *fail loudly* is the underlying quality bar; this discipline is its operational form for C++/HLSL where the compiler does not catch silent corruption.
- `cpp-style.md` — paired discipline; that one governs naming / organisation / engine abstractions, this one governs runtime behaviour.
- `shader-standards.md` — the *no magic numbers* and *no copy-paste* rules apply to HLSL too; shared helpers belong in `common/common.hlsl`.
- `comment-discipline.md` — *no magic numbers* allows a same-line comment when a named constant is not feasible; that comment still follows the present-state rule.
