# Discipline: <name>

> **Template guide.** This file is the canonical shape for every `.claude/disciplines/*.md`. Copy it when authoring a new discipline. Section presence scales to load-bearing complexity — small disciplines may collapse `## Why` / `## When` into the lead paragraph and omit `## Anti-patterns`. The order is fixed; depth is judgement.

> **Write for cold readers.** A discipline must be readable by a future agent with no session memory: no recollection of the conversation that produced the rule, no continuity with the incident that motivated it. State the rule, describe the failure mode in abstract terms, and explain how to apply. Do **not** anchor the rule to "this session", a specific commit SHA, or a one-off incident description that presupposes the reader was there. Stable, durable references (a TASK-ID with a stand-alone task file; a commit-gate name; a documented engine invariant) are fine because the reader can resolve them; "the user said X in this session" is not. If a rule cannot be stated without leaning on session context, it is not yet ready to be a discipline.

One- or two-sentence load-bearing rule. No heading. This is the line an agent should be able to recall from a single read of the file. Imperative mood. Cite the paired discipline inline if the rule has an obvious pair.

## Why

The failure mode this rule prevents. Brief — one short paragraph. Phrase in abstract terms (general behaviour and consequence), not as a recap of the originating session. Omit this section only when the rule is self-evident from the lead (e.g. naming conventions, formatting).

## When

Applicability scope. When does this discipline apply, when does it not. Skip-categories listed here, with the rationale for each skip. Omit when the rule is universally always-on (e.g. attribution).

## How

Concrete steps, format, or artifact requirements. The body of the discipline. Subsections allowed for multi-step procedures (`### Step 1`, `### Step 2`, etc.) or for orthogonal concerns (`### By purpose`, `### Worked example`). Code blocks live here.

## Anti-patterns

- **Bullet 1.** One-sentence rationale. The form is *behaviour to avoid* + *why it fails*. Examples drawn from real incidents land harder than abstract anti-patterns; cite TASK-NN where one exists.
- **Bullet 2.** Same shape.

Omit this section when the discipline has not yet seen drift — adding placeholder bullets dilutes the ones that exist for a reason.

## Cross-references

- `paired-discipline.md` — one-sentence relationship (paired / sibling / supersedes / superseded-by).
- `another-related.md` — same shape.

Always present, even if it lists "none" — the act of asking "what does this pair with" is part of the discipline.

---

## Authoring checklist (delete before committing a real discipline)

- [ ] H1 is `# Discipline: <name>` matching the filename.
- [ ] Lead paragraph is 1-2 sentences and imperative.
- [ ] Rule is stated in cold-readable form — no "this session", no bare-SHA anchors, no incident recap that presupposes the reader was there.
- [ ] `## Why` describes the failure mode in abstract terms, or is omitted because the rule is self-evident.
- [ ] `## When` lists skip-categories explicitly, or is omitted because the rule is universally always-on.
- [ ] `## How` has concrete steps; code blocks where the format is load-bearing.
- [ ] `## Anti-patterns` only included when drift has actually been observed; bullets describe the behaviour to avoid in general terms.
- [ ] `## Cross-references` lists every paired discipline and the relationship is bidirectional (the paired file lists this one back).
- [ ] Total length scaled to load-bearing complexity: a single-rule discipline is ~10-30 lines; a multi-procedure discipline is ~80-150 lines. Filling sections to hit a length target is itself an anti-pattern.
