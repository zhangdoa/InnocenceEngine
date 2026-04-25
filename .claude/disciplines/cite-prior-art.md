# Discipline: cite-prior-art

Before introducing any mechanism, abstraction, pattern, algorithm, or convention, search for prior art and cite the source you're following:

- **Inside the project** — adjacent owned subtrees, sibling modules, existing helpers. Read line-by-line, not just at the file/structure level.
- **Official documentation** of any tool, SDK, library, or platform you're using.
- **Canonical reference implementations** for any paper, spec, or external standard you're translating.

If you genuinely find no precedent, justify the new pattern in writing before implementing. Trustworthy precedent beats first-principles design; surface mimicry is not citation.

Failure modes prevented: silo design (reinventing inside one subtree), shortcut design (speculation when a reference exists), pattern-matching at the wrong level (copying file layout but inventing the technique inside it).
