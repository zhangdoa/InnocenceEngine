# Discipline: persistence-venue

Where to record a cross-session correction or fact, given that we have several venues with different reach and durability. The load-bearing fact: **auto-memory does not reach spawned subagents.** Anything a subagent must know cannot live only in `~/.claude/projects/<slug>/memory/`.

## How

### Reach matrix

| Venue | Main-session Claude | Spawned subagents | Travels with repo | Visible in git history | Enforced |
|---|---|---|---|---|---|
| `~/.claude/projects/<slug>/memory/*.md` (auto-memory) | yes | **no** | no | no | no |
| Root `CLAUDE.md` | yes | yes (every agent reads it) | yes | yes | no |
| Subtree `CLAUDE.md` | yes when scope touches subtree | yes for agents owning that subtree | yes | yes | no |
| `.claude/disciplines/<name>.md` (universal list in root `CLAUDE.md`) | yes | yes (every agent) | yes | yes | no |
| `.claude/disciplines/<name>.md` (opted in by specific agent manifest) | yes | only that agent | yes | yes | no |
| `.claude/agents/<role>.md` manifest | yes | only that agent | yes | yes | no |
| `.claude/hooks/*` | n/a | n/a | yes | yes | **yes — blocks tool calls** |

### Decision rule

When recording a correction, ask in order:

1. **Is it about the project (facts, framing, audience, conventions)?** Root `CLAUDE.md` if it applies everywhere, subtree `CLAUDE.md` if it applies only inside one tree.
2. **Is it a behavior with multiple steps, cases, or applicability conditions?** `.claude/disciplines/<name>.md`. Add it to the universal list in root `CLAUDE.md` if every agent needs it; otherwise opt the specific agent(s) in via their manifest.
3. **Does it need to be unforgeable — i.e., the cost of one slip is high enough that prose alone is insufficient?** `.claude/hooks/<gate>.js`, with a clear escape sentinel.
4. **Is it about how I (Claude) personally prefer to work, with no project or audience consequence?** Auto-memory is acceptable. This is rare.

A useful sanity check: if a subagent could plausibly violate the rule while writing an Implementation Note or commit message, the rule belongs in the repo, not in memory.

### Authoring register when promoting into the harness

When a correction moves from auto-memory or user prose into a repo venue (root `CLAUDE.md`, subtree `CLAUDE.md`, `.claude/disciplines/*.md`, agent manifest, hook), **paraphrase into discipline-file register** — do not transcribe casual prose verbatim. The audience changes: memory entries are written for one Claude reading itself; harness content is read by every spawned subagent across roles. Match the tone of the existing universal disciplines (`coding-principles.md`, `cite-prior-art.md`, `tech-choice-vs-default.md`): operative rule first, structured sections, no first-person or session-specific narrative. Translate the substance; drop the framing.

### When auto-memory is the right venue

- Truly personal preferences with no audience consequence (e.g., "user prefers concise answers" — though even that is better captured in the user-scope `~/.claude/CLAUDE.md`).
- Short-lived continuity notes that are project-state-of-the-world, not corrections — but Implementation Notes on backlog tasks are a better venue for project state, because the producer agent reads them at session start.

## Anti-patterns

- **Hoarding `feedback_*.md` entries in auto-memory** under the assumption that every agent will obey them. They will not — subagents do not see them. A long memory index is not a sign of good cross-session retention; it is a sign that corrections are accumulating in a venue with the smallest possible reach.
- **Migrating by deletion-and-rewrite.** When auditing memory, prefer migration to the right repo venue: if a memory entry is still valid, move its content and delete the memory file; if no longer valid, just delete.

## Cross-references

- `workspace-hygiene.md` — *the backlog is the only AI-authored project-state medium* is the operational form of this discipline's "rule belongs in the repo, not memory" finding.
- `backlog-workflow.md` — Implementation Notes are the cross-session medium subagents *do* see; that discipline governs how they get written.
