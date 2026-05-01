# Discipline: persistence-venue

Where to record a cross-session correction or fact, given several venues with different reach. The load-bearing fact: **auto-memory does not reach spawned subagents.** Anything a subagent must know cannot live only in `~/.claude/projects/<slug>/memory/`.

## Reach matrix

| Venue | Main-session | Spawned subagents | Travels with repo | Enforced |
|---|---|---|---|---|
| `~/.claude/projects/<slug>/memory/*.md` | yes | **no** | no | no |
| Root / subtree `CLAUDE.md` | yes | yes | yes | no |
| `.claude/disciplines/<name>.md` | yes | yes (universal) or per-manifest opt-in | yes | no |
| `.claude/hooks/*` | n/a | n/a | yes | **yes** |

## Decision rule

1. **Project framing / facts / conventions?** Root `CLAUDE.md` (universal) or subtree `CLAUDE.md` (scoped).
2. **Behaviour with steps, cases, applicability?** `.claude/disciplines/<name>.md`. Add to root universal preamble if every agent needs it; otherwise opt the specific agent in via their manifest.
3. **Cost of one slip is high?** `.claude/hooks/<gate>.js` with a clear escape sentinel.
4. **Personal preference, no project consequence?** Auto-memory. Rare.

When promoting from auto-memory or user prose into a repo venue, **paraphrase into discipline-file register** — operative rule first, structured sections, no first-person or session-specific narrative. Match the existing universal disciplines' tone. Translate the substance; drop the framing.

## Cross-references

- `workspace-hygiene.md` — *the backlog is the only AI-authored project-state medium* is the operational form of this discipline's "rule belongs in the repo, not memory" finding.
- `backlog-workflow.md` — Implementation Notes are the cross-session medium subagents *do* see; that discipline governs how they get written.
