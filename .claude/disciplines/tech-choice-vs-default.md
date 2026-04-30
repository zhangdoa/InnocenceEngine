# Discipline: tech-choice-vs-default

Before any non-trivial implementation — algorithm, data structure, sync primitive, IPC mechanism, denoiser, sampling strategy, anything load-bearing — name three references and justify the pick against all three. Long-form rationale + recorded incident in `tech-choice-vs-default-extras.md`.

## The three references

- **(a) Training-corpus default** — the textbook / commonly-cited approach an LLM (or textbook-trained engineer) reaches for first because shadow-map literature, AO literature, lock literature, etc. dominates the training corpus. Identify it explicitly so it can be rejected explicitly.
- **(b) Current SOTA** — what the literature / industry would actually do today. Hardware ray-tracing for shadows. Gradient noise for procedural content. Async compute for unrelated GPU work. The technique a 2026 SIGGRAPH / GDC / HPG talk would advocate, not a 2008 talk.
- **(c) Recent project precedent** — what this codebase has shipped in adjacent commits / nearby subsystems. The "we just built X for problem Y; reuse the X infrastructure for problem Z" check.

## Pick rule

When (c) exists and is recent (cite file:line and commit), prefer (c). When (c) is absent, weigh (b) > (a) with rationale. Default to (a) only when (b) is genuinely unsuitable for this codebase, and state the case in writing — *"RT not available because no DXR support on target"* is fine; *"cube atlas because that's how the tutorial does it"* is not.

## Recording template

Before any non-trivial implementation dispatch — design phase, not after — record in the brief or task notes:

```
Tech choice: <picked technique>
- (a) default: <name>; rejected because <rationale>
- (b) SOTA:    <name>; <picked / not-picked, why>
- (c) project: <recent precedent or "none"; cite file:line if precedent>
- Pick:        <(a) | (b) | (c) | hybrid> because <one-sentence rationale>
```

If (c) exists and is recent, default pick is (c). If (c) is absent, default pick is (b) unless (b) is unsuitable for the project's constraints. Falling back to (a) requires the unsuitability to be in writing.

The reviewer (per `peer-review-required.md`) checks the tech-choice block against the diff: does the picked technique match the rationale, did the implementer skip (c) when it existed, is (a) being rationalised after the fact.

## Recorded incident

TASK-66 → TASK-138 → TASK-175 (cube-shadow vs RT, 2026-04-28). Default-from-corpus tech pick shipped, RT infra from TASK-138 was the right (c), 93.5 ms/frame phantom load. Full narrative in `tech-choice-vs-default-extras.md`.

## Long-form

`tech-choice-vs-default-extras.md` — full rationale, anti-pattern catalogue (defaulting-without-naming, skipping (c), "famous engine in year X", reflexive (a) re-use, etc.), full recorded incident.

## Cross-references

- `cite-prior-art.md` — paired: citation finds precedent; this one evaluates against alternatives.
- `paper-port.md` — when there *is* a paper + reference implementation.
- `peer-review-required.md` — reviewer checks the tech-choice block.
- `regression-fix-flow.md` — *understand why* step often surfaces a wrong-tech-pick.
- `owner-mode.md` — *small tweaks → wrong approach* is the operational form.
