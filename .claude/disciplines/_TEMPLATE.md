# Discipline: <name>

> **Template.** Copy when authoring a new discipline. Drop into the matching `<stage>/` directory under `.claude/disciplines/`. Section presence scales to load-bearing complexity.
>
> **Style.** Imperative. Structured. No roleplay, no rationale paragraphs, no incident narratives recoverable from `git log`. Rules and conditions ("when X → do Y"). Tables and bullets over prose. Cite sibling disciplines by stage-relative path (`always/foo.md` from another stage, `foo.md` within the same stage).

One- or two-sentence load-bearing rule. No heading. Imperative.

## When applicable

Skip-categories with one-line reason for each. Omit when universally always-on.

## Procedure

Concrete steps. Subsections allowed for multi-step procedures or orthogonal concerns. Code blocks where format is load-bearing.

## Anti-patterns

- **Bullet 1.** Behaviour to avoid + one-line reason.
- **Bullet 2.** Same shape.

Omit when no drift has been observed.

## Cross-references

- `<stage>/<file>.md` — one-line relationship.

---

## Authoring checklist (delete before commit)

- [ ] H1 is `# Discipline: <name>` matching the filename.
- [ ] Lead is 1-2 imperative sentences.
- [ ] No "this session", no bare-SHA anchors, no incident recap.
- [ ] No roleplay framing.
- [ ] No rationale paragraphs explaining why the rule exists, beyond what's needed to apply it.
- [ ] Cross-references use stage-relative paths and are bidirectional.
- [ ] Length scales to complexity. Single-rule ≈ 10-30 lines; multi-procedure ≈ 50-90.
