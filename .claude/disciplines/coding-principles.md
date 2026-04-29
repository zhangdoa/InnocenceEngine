# Discipline: coding-principles

Universal — applies to every code-authoring agent regardless of language. The fundamentals every CL aims at, before role-specific style or domain disciplines apply.

## How

- **Functional programming** — avoid complex state machines; inputs generate outputs in a deterministic way, unless quantum-randomized.
- **Data-oriented** — everything should serve, observe, manipulate, and deliver data, rather than processing data for the sake of processing.
- **Design by contract** — enforce data in and out with pre- and post-conditions. Predict possibilities, don't react to realities.
- **Avoid premature abstraction** — be pragmatic when modeling; everything eventually comes from and goes to hardware. Don't OOP because you know `class`.
- **Avoid premature optimization** — define a clear performance baseline and measure; optimize only if it doesn't match. Don't write obscure algorithms just because you can.
- **Be explicit in code** — modern code is for humans to read first and machines to execute second; otherwise write in assembly.
- **Be explicit in domain-specific code** — not everyone is an expert in your domain, and sometimes even you aren't, a couple thousand commits ago or later.
- **Refactor proactively to save your time** — only leave code alone if all of the above are already satisfied.

## Cross-references

- `target-qualities.md` — concrete quality bar (orthogonality, explicit contracts, fail loudly, reload-safe) every CL is measured against; this discipline is the design-time form of that quality bar.
- `comment-discipline.md` — explicit-in-code applies first; comments are the fallback when naming alone cannot carry the meaning.
- `cite-prior-art.md` — *avoid premature abstraction* implies cite-before-invent; the citation discipline is the operationalisation.
