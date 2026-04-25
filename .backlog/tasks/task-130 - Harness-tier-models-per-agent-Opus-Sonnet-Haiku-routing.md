---
id: TASK-130
title: 'Harness: tier models per agent (Opus / Sonnet / Haiku routing)'
status: To Do
assignee: []
created_date: '2026-04-25 13:00'
labels:
  - harness
  - agent-config
  - cost
dependencies: []
priority: medium
---

## Description

<!-- SECTION:DESCRIPTION:BEGIN -->
Research and recommendation pass — body of this task IS the report. Origin: 2026-04-25 session. Question: should main session and sub-agents run on a single tier (Opus 4.7 today) or be tiered to Sonnet / Haiku where the role does not need frontier reasoning? Goal is cost reduction without quality regression on the work the harness actually produces.

### 1. Feasibility

The agent-manifest format already supports this directly. Verified:

- All 11 manifests under `.claude/agents/` declare `model:` frontmatter; today every one is `model: inherit`.
- Anthropic docs (`code.claude.com/docs/en/sub-agents`) confirm the key accepts `opus`, `sonnet`, `haiku`, `inherit`, or a full model ID. The example in their own docs flips a `code-reviewer` agent between `inherit` and `sonnet`.
- Anthropic's built-in subagents already do this: `Explore` runs on Haiku, `claude-code-guide` on Haiku, `statusline-setup` on Sonnet, `Plan` and `general-purpose` inherit. This is the precedent.

Switch is mechanical: replace `inherit` with the chosen tier in each manifest's frontmatter. No hook changes, no schema migration, no code changes.

### 2. Routing taxonomy

Confirmed pricing (from `platform.claude.com/docs/en/about-claude/models/overview`, as of session date):

| Tier | Model | Input $ / MTok | Output $ / MTok | Ratio vs Opus 4.7 |
|---|---|---|---|---|
| Opus | claude-opus-4-7 | 5 | 25 | 1.0× |
| Sonnet | claude-sonnet-4-6 | 3 | 15 | 0.6× |
| Haiku | claude-haiku-4-5 | 1 | 5 | 0.2× |

Proposed per-agent routing:

| Agent | Tier | Why |
|---|---|---|
| rendering-researcher | Opus | Paper-port reasoning, shader algorithm derivation, multi-source synthesis (paper + reference impl + our code). Frontier reasoning load. |
| paper-auditor | Opus | Cross-checks our impl against paper math; false negatives here let drift land in `master`. Cheap insurance on a low-volume agent. |
| software-architect | Opus | Decisions about where responsibilities live and how to evolve schemas have long blast radius; one wrong call costs more than the per-token delta. |
| low-level-expert | Opus | Threading / memory / lifetime work — silent regressions are hard to catch; the foundation is called by everyone. |
| graphics-api-expert | Opus | DX12/Vulkan validation-layer diagnosis is heavy reasoning under sparse signals (validation messages, RenderDoc captures). |
| editor-tooling-expert | Sonnet | Vue/Electron/TS/Playwright is well-trodden territory in training data; problems are usually localized. Promote to Opus only on hard IPC-contract evolution. |
| ci-build-expert | Sonnet | CMake / scripts / build glue. Recipes more than reasoning. |
| platform-expert | Sonnet | OS/HID glue is well-documented; problems usually surface as compile/runtime errors with clear messages. |
| test-expert | Sonnet | Test-strategy decisions are mostly pattern-matching against known tiers (unit/integration/Playwright); the example project work is mechanical. |
| ai-expert | Sonnet | Hook authoring, manifest edits, discipline-fragment writing. Largely structural; tight feedback loop via the gates themselves. (Self-note: this very report would be authored on Sonnet under this scheme; quality would have to hold.) |
| producer | Sonnet | Backlog ops, briefings, dispatch plans. Routine state-summary work. Promote to Opus only for retrospectives that require deep cross-agent synthesis — those can come back to main session. |

No agent picked Haiku. Reason: every project agent is invoked with non-trivial codebase context (subtree CLAUDE.md, discipline fragments, source files) and produces structured artifacts (Implementation Notes, alignment files, manifest edits). Haiku's 200k context plus weaker reasoning makes it a poor fit even for "light" project agents. Haiku's natural home is the built-in `Explore` (already Haiku) and `claude-code-guide` (already Haiku) — Anthropic has placed those correctly without our intervention.

### 3. Main session tier

Recommendation: **leave main session on Opus for now; revisit after sub-agent tiering has been live for a representative period.**

The headline argument for flipping main is that it "just dispatches." The counter — which I find stronger — is that main session does the synthesis turns the user notices most: the producer-brief at session start (after the producer hands back), the CL review where multiple sub-agents' outputs collide (e.g. TASK-125 visual-comparison + decision turn), the cross-agent retrospective. These turns benefit from the most capable model because that is where bad routing or mis-synthesis becomes user-visible.

The cost argument also weakens once sub-agents are tiered: most of the token volume per session lives inside sub-agents (long file reads, long tool transcripts), not in main. Tiering sub-agents captures the bulk of savings; flipping main captures a smaller delta at higher synthesis-quality risk.

Fallback if cost is still material after sub-agent tiering: user manually `/model sonnet` on light sessions (status updates, backlog grooming), `/model opus` for active development. This is the cheapest knob and reversible per turn.

### 4. Risks

- **Prompt fitness**: sub-agent system prompts (manifests + discipline fragments) were written assuming Opus. Sonnet on these prompts may show up as: skipped sentinels in disciplines, weaker root-cause analysis, more "decent but distant" handoffs. The discipline-fragment style — terse, principle-stated-once — depends on the model inferring intent from a single sentence. This is exactly where Sonnet vs Opus diverges.
  - Mitigation: pilot one Sonnet-tiered agent first; compare output on a representative dispatch from the last week. Tighten the manifest with explicit examples or guardrails only if the pilot shows drift.

- **Silent drift**: a Sonnet sub-agent that does 90% of an Opus sub-agent's job will not look broken; it will produce slightly worse Implementation Notes or miss a sentinel once a week. This compounds.
  - Detection: spot-check by re-running last week's dispatch against the new tier and diffing the produced artifact. Build a small "regression dispatch" set — three or four dispatches representative of each agent's work — and re-run them after every tier change. This is not a CI item; it is a manual gate before adopting the change.

- **Cost telemetry**: we have no per-agent / per-tier token telemetry today. Without it, the cost-saving claim is theoretical. The Anthropic Console shows per-session totals but does not break down by sub-agent.
  - Suggestion: do not block tiering on building telemetry, but do record total session cost before-and-after on three "typical" sessions (a backlog-grooming session, a feature-implementation session, a debugging session) so the savings are observed rather than assumed.

- **Pricing dynamics**: Opus 4.7 is 1.67× Sonnet 4.6 on input, 1.67× on output. Older Opus tiers (4.1) were 5× Sonnet, which made tiering a no-brainer; today the gap is narrower. The win is real but smaller than it would have been a year ago. If the bottleneck on cost is total tokens (long contexts, long tool transcripts) rather than tier, tiering produces marginal savings — the larger lever is shorter prompts and fewer redundant reads.

- **Model availability / drift over time**: Anthropic deprecates older models (Sonnet 4 / Opus 4 retire 2026-06-15 per the docs). Pinning to current aliases (`sonnet` / `opus`) auto-tracks the latest in each tier; pinning to a specific snapshot ID would freeze quality but require periodic migration. Recommendation: use the alias form, accept that sub-agent quality will track Anthropic's improvements.

### 5. Migration path

Phased rollout, lowest-risk first:

1. **Phase 0 — baseline measurement (1 session)**: pick three representative dispatches from the last seven days (one rendering-researcher, one ai-expert, one producer). Re-run each dispatch with the same prompt against current Opus and capture the output as the reference artifact.

2. **Phase 1 — flip the structural / mechanical agents**: `ai-expert`, `producer`, `ci-build-expert`, `platform-expert`. These do the most pattern-matching against well-documented surfaces and the least frontier reasoning. Edit each manifest's frontmatter from `model: inherit` to `model: sonnet`. Run for one full session; compare outputs against Phase 0 baselines.

3. **Phase 2 — flip the higher-risk Sonnet candidates**: `editor-tooling-expert`, `test-expert`. These touch broader surfaces but on well-trodden tech. Same comparison protocol.

4. **Phase 3 — leave Opus agents on Opus**: `rendering-researcher`, `paper-auditor`, `software-architect`, `low-level-expert`, `graphics-api-expert`. No change.

5. **Per-agent rollback**: if a Sonnet-tiered agent produces visibly worse artifacts in Phase 1 or 2, flip its single manifest line back to `inherit` (or to `opus`). The change is one line per agent; reverting is trivial.

6. **No main-session change in this rollout.** Defer until at least three weeks of Phase 1 + 2 data exist.

### 6. Decision recommendation

**Pilot Phase 1 (4 manifests: ai-expert, producer, ci-build-expert, platform-expert) → Sonnet. Hold the rest.**

Reasoning:

- Feasibility is a one-line frontmatter edit per manifest; no harness work needed.
- The four Phase 1 agents are the lowest-stakes flips and produce artifacts whose quality the user reviews directly (manifest edits, backlog updates, build scripts, platform glue) — drift will surface fast.
- The high-stakes agents (rendering-researcher, paper-auditor, software-architect, low-level-expert, graphics-api-expert) stay on Opus until we have evidence the lower tier suffices, because the cost of a bad routing decision in those domains is paid in master-branch regressions, not in re-running a turn.
- Main session stays on Opus — its synthesis turns are where the user most directly experiences quality.
- The pricing gap (1.67×) makes Phase 1 worth doing but does not justify aggressive cuts; the dominant cost lever in this codebase is total token volume per session, which tiering only partially addresses.

Do not implement in this dispatch — this is decision input. Implementation is a separate dispatch once the user signs off on Phase 1.
<!-- SECTION:DESCRIPTION:END -->

## Acceptance Criteria
<!-- AC:BEGIN -->
- [ ] #1 User signs off on Phase 1 set (or proposes a different set / different tiers)
- [ ] #2 Phase 0 baseline artifacts captured for at least three representative dispatches
- [ ] #3 Phase 1 manifest edits applied (frontmatter `model:` line only) and committed under `[skip-test-gate]` (no engine code touched)
- [ ] #4 One full post-Phase-1 session run against the same dispatches; outputs diffed against Phase 0 baselines and either accepted or rolled back per agent
- [ ] #5 Decision recorded — keep, expand to Phase 2, or revert
<!-- AC:END -->

## Definition of Done
<!-- DOD:BEGIN -->
- [ ] #1 Code compiles — N/A; manifest-only change. State this in the closure summary rather than skipping the line.
- [ ] #2 Pre-existing integration tests covering the changed area were re-run against the change and green — N/A for manifest edits; the validation is the diff-against-baseline protocol in AC #4.
- [ ] #3 If no pre-existing integration test covers the change: a new integration test (NOT a mock-based unit test) was written and run — N/A; the diff-against-baseline run IS the validation. State this explicitly in closure.
- [ ] #4 Self-authored mock-based tests are not the sole validation — N/A; no tests authored.
- [ ] #5 User-observable outcome verified — the Phase 1 dispatch outputs themselves are the user-observable artifact.
- [ ] #6 Final summary lists what was NOT verified — honestly. Specifically: cost telemetry was not built, so the savings claim remains observed-not-measured; main-session tier was not evaluated empirically.
<!-- DOD:END -->
