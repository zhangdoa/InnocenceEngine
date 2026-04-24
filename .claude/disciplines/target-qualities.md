# Discipline: target-qualities

Every CL aims at these, regardless of domain:

- **Orthogonality** — each module has one responsibility; changes in one place don't silently affect another. Services own operation domains, not component types — a `FooComponent` does not imply a `FooSystem`, and multiple services may operate on the same component type independently.
- **Explicit contracts** — preconditions, postconditions, and ownership enforced through types, assertions, invariants. Not assumed, not implicit.
- **Fail loudly** — invalid state produces an immediate, visible error at the violation point, not silent corruption frames later. Guard clauses that return `false` on invalid input without logging are a failure mode.
- **Reload-safe by default** — any resource or asset loadable more than once handles re-initialization without accumulating stale state. Test this with the scene-reload integration tier before claiming done.

A CL that ships a feature while degrading one of these is a regression even when it compiles and passes the tier-2 test.
