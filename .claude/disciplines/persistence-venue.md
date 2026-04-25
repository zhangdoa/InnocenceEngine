# Discipline: persistence-venue

Where to record a cross-session correction or fact, given that we have several venues with different reach and durability.

## Reach matrix

| Venue | Main-session Claude | Spawned subagents | Travels with repo | Visible in git history | Enforced |
|---|---|---|---|---|---|
| `~/.claude/projects/<slug>/memory/*.md` (auto-memory) | yes | **no** | no | no | no |
| Root `CLAUDE.md` | yes | yes (every agent reads it) | yes | yes | no |
| Subtree `CLAUDE.md` | yes when scope touches subtree | yes for agents owning that subtree | yes | yes | no |
| `.claude/disciplines/<name>.md` (universal list in root `CLAUDE.md`) | yes | yes (every agent) | yes | yes | no |
| `.claude/disciplines/<name>.md` (opted in by specific agent manifest) | yes | only that agent | yes | yes | no |
| `.claude/agents/<role>.md` manifest | yes | only that agent | yes | yes | no |
| `.claude/hooks/*` | n/a | n/a | yes | yes | **yes — blocks tool calls** |

The load-bearing fact: **auto-memory does not reach spawned subagents.** Subagents are seeded from their manifest plus the disciplines that manifest opts into, not from `~/.claude/projects/.../memory/`. Anything a subagent must know cannot live only in memory.

## Decision rule

When recording a correction, ask in order:

1. **Is it about the project (facts, framing, audience, conventions)?** Root `CLAUDE.md` if it applies everywhere, subtree `CLAUDE.md` if it applies only inside one tree.
2. **Is it a behavior with multiple steps, cases, or applicability conditions?** `.claude/disciplines/<name>.md`. Add it to the universal list in root `CLAUDE.md` if every agent needs it; otherwise opt the specific agent(s) in via their manifest.
3. **Does it need to be unforgeable — i.e., the cost of one slip is high enough that prose alone is insufficient?** `.claude/hooks/<gate>.js`, with a clear escape sentinel.
4. **Is it about how I (Claude) personally prefer to work, with no project or audience consequence?** Auto-memory is acceptable. This is rare. Most "feedback_*" entries actually fail this test — they describe behaviors every agent must adopt, so they belong in disciplines.

A useful sanity check: if a subagent could plausibly violate the rule while writing an Implementation Note or commit message, the rule belongs in the repo, not in memory.

## Anti-pattern

Hoarding `feedback_*.md` entries in auto-memory under the assumption that every agent will obey them. They will not — subagents do not see them. A long memory index is not a sign of good cross-session retention; it is a sign that corrections are accumulating in a venue with the smallest possible reach.

When auditing memory, prefer migration to deletion-and-rewrite: if a memory entry is still valid, move its content to the right repo venue and delete the memory file. If it is no longer valid, just delete it.

## When auto-memory is the right venue

- Truly personal preferences with no audience consequence (e.g., "user prefers concise answers" — though even that is better captured in the user-scope `~/.claude/CLAUDE.md`).
- Short-lived continuity notes that are project-state-of-the-world, not corrections (e.g., "branch X is ahead of origin pending push") — but Implementation Notes on backlog tasks are a better venue for project state, because the producer agent reads them at session start.
