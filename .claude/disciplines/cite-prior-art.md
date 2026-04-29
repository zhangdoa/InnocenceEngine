# Discipline: cite-prior-art

Before introducing any mechanism, abstraction, pattern, algorithm, or convention, search for prior art and cite the source you're following. Trustworthy precedent beats first-principles design; surface mimicry is not citation.

## Why

Failure modes prevented: silo design (reinventing inside one subtree), shortcut design (speculation when a reference exists), pattern-matching at the wrong level (copying file layout but inventing the technique inside it).

## How

Search in this order, and cite the source explicitly in the design notes or task brief:

- **Inside the project** — adjacent owned subtrees, sibling modules, existing helpers. Read line-by-line, not just at the file/structure level.
- **Official documentation** of any tool, SDK, library, or platform you're using.
- **Canonical reference implementations** for any paper, spec, or external standard you're translating.

If you genuinely find no precedent, justify the new pattern in writing before implementing.

## Cross-references

- `tech-choice-vs-default.md` — citation finds precedent; that discipline evaluates precedent against alternatives. The two are paired: cite first, then compare against (a) default / (b) SOTA / (c) recent-project. A citation that names only one of those three is half the work.
- `paper-port.md` — paper-port is the special case of citation when the precedent is a published algorithm with a reference implementation.
- `coding-principles.md` — *avoid premature abstraction* implies cite-before-invent; this discipline is the operationalisation.
