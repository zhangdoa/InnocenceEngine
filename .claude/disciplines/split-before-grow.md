# Discipline: split-before-grow

The universal file-size gate blocks commits that grow a file past the project's line-count limit. When adding to a file approaching the limit, the default move is to split — extract into a common header, a helper module, or a new unit — not to request the skip sentinel.

The skip sentinel exists for genuinely atomic additions where splitting would create more coupling than it resolves. If you use it, the commit message must explain why.

Existing files already over the limit are grandfathered but still subject to the gate on growth. When editing such a file, include a short judgement in the turn summary on whether the file should be split as part of the CL you're in, or filed as a follow-up refactor task.
