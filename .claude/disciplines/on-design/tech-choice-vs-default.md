# Discipline: tech-choice-vs-default

Before any non-trivial implementation — algorithm, data structure, sync primitive, IPC mechanism, denoiser, sampling strategy — name three references and justify the pick against all three.

## The three references

- **(a) Training-corpus default** — the textbook / first-thing-LLMs-reach-for answer. Identify it explicitly so it can be rejected explicitly.
- **(b) Current SOTA** — what current literature / industry would actually do today.
- **(c) Recent project precedent** — capability the codebase shipped in adjacent commits. Cite file:line and commit.

When (c) exists and is recent → prefer (c). When (c) is absent → prefer (b) > (a). Falling back to (a) requires the unsuitability of (b) in writing.

## Recording template

```
Tech choice: <picked technique>
- (a) default: <name>; rejected because <rationale>
- (b) SOTA:    <name>; <picked / not-picked, why>
- (c) project: <recent precedent or "none"; cite file:line if precedent>
- Pick:        <(a) | (b) | (c) | hybrid> because <one-sentence rationale>
```

The reviewer (per `on-commit/peer-review-required.md`) checks the block against the diff.

## Cross-references

- `always/fundamentals.md` — paired with cite-before-invent.
- `on-paper-port/paper-port.md` — when there is a paper + reference implementation.
- `on-commit/peer-review-required.md` — reviewer checks the tech-choice block.
- `on-bug/regression-fix-flow.md` — *understand why* often surfaces a wrong-tech-pick.
