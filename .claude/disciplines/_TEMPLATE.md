# Discipline: <name>

> **Template guide.** This file is the canonical shape for every `.claude/disciplines/*.md`. Copy it when authoring a new discipline. Section presence scales to load-bearing complexity — small disciplines may collapse `## Why` / `## When` into the lead paragraph and omit `## Anti-patterns` / `## Recorded incident`. The order is fixed; depth is judgement.

One- or two-sentence load-bearing rule. No heading. This is the line an agent should be able to recall from a single read of the file. Imperative mood. Cite the paired discipline inline if the rule has an obvious pair.

## Why

The failure mode this rule prevents. Brief — one short paragraph. Cite the triggering incident inline (TASK-NN, commit `<sha>`, date) when one exists; the longer narrative goes in `## Recorded incident` below. Omit this section only when the rule is self-evident from the lead (e.g. naming conventions, formatting).

## When

Applicability scope. When does this discipline apply, when does it not. Skip-categories listed here, with the rationale for each skip. Omit when the rule is universally always-on (e.g. attribution).

## How

Concrete steps, format, or artifact requirements. The body of the discipline. Subsections allowed for multi-step procedures (`### Step 1`, `### Step 2`, etc.) or for orthogonal concerns (`### By purpose`, `### Worked example`). Code blocks live here.

## Anti-patterns

- **Bullet 1.** One-sentence rationale. The form is *behaviour to avoid* + *why it fails*. Examples drawn from real incidents land harder than abstract anti-patterns; cite TASK-NN where one exists.
- **Bullet 2.** Same shape.

Omit this section when the discipline has not yet seen drift — adding placeholder bullets dilutes the ones that exist for a reason.

## Recorded incident

Longer narrative (1-2 paragraphs) of the triggering incident: what shipped, what broke, what the user said, what the discipline changes for the next equivalent decision. Cite task IDs, commit SHAs, dates. Omit when no specific incident triggered the discipline (e.g. coding fundamentals).

## Cross-references

- `paired-discipline.md` — one-sentence relationship (paired / sibling / supersedes / superseded-by).
- `another-related.md` — same shape.

Always present, even if it lists "none" — the act of asking "what does this pair with" is part of the discipline.

---

## Authoring checklist (delete before committing a real discipline)

- [ ] H1 is `# Discipline: <name>` matching the filename.
- [ ] Lead paragraph is 1-2 sentences and imperative.
- [ ] `## Why` cites a triggering incident with task ID(s), or is omitted because the rule is self-evident.
- [ ] `## When` lists skip-categories explicitly, or is omitted because the rule is universally always-on.
- [ ] `## How` has concrete steps; code blocks where the format is load-bearing.
- [ ] `## Anti-patterns` only included when drift has actually been observed; bullets cite incidents.
- [ ] `## Recorded incident` present when a single incident drove the discipline; omitted for fundamentals.
- [ ] `## Cross-references` lists every paired discipline and the relationship is bidirectional (the paired file lists this one back).
- [ ] Total length scaled to load-bearing complexity: a single-rule discipline is ~10-30 lines; a multi-procedure discipline is ~80-150 lines. Filling sections to hit a length target is itself an anti-pattern.
