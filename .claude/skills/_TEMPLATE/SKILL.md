---
name: <skill-name>
description: Use when <trigger condition — phase, file shape, or symptom>. <One sentence on what the skill does.>
---

# Skill: <skill-name>

> **Template.** Copy as `.claude/skills/<name>/SKILL.md`. Section presence scales to load-bearing complexity.
>
> **Style.** Imperative. Structured. No roleplay, no rationale paragraphs, no incident narratives recoverable from `git log`. Rules and conditions ("when X → do Y"). Tables and bullets over prose. Cite sibling skills by name; user-level skills are at `~/.claude/skills/<name>/SKILL.md`.

One- or two-sentence load-bearing rule. Imperative.

## When applicable

Skip-categories with one-line reason for each. Omit when universally always-on.

## Procedure

Concrete steps. Subsections allowed for multi-step procedures or orthogonal concerns. Code blocks where format is load-bearing.

## Anti-patterns

- **Bullet 1.** Behaviour to avoid + one-line reason.
- **Bullet 2.** Same shape.

Omit when no drift has been observed.

## Cross-references

- `<sibling-skill>` — one-line relationship.

---

## Authoring checklist (delete before commit)

- [ ] Frontmatter `name:` matches the directory name.
- [ ] Frontmatter `description:` starts with "Use when …" and names the trigger.
- [ ] H1 is `# Skill: <name>` matching frontmatter.
- [ ] Lead is 1-2 imperative sentences.
- [ ] No "this session", no bare-SHA anchors, no incident recap.
- [ ] No roleplay framing.
- [ ] No rationale paragraphs explaining why the rule exists, beyond what's needed to apply it.
- [ ] Cross-references cite skills by name.
- [ ] Length scales to complexity. Single-rule ≈ 10-30 lines; multi-procedure ≈ 50-90.
