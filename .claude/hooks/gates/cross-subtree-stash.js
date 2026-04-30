// Cross-subtree-stash gate — block `git stash` calls that would sweep
// dirty files spanning two or more agent-owned subtrees.
//
// Failure mode this prevents (TASK-196 / 2026-04-28 incident):
//   Three agents ran in parallel under one shared worktree. Agent A hit
//   a build blocker and ran `git stash push -m "TASK-X build-blocker"`.
//   Because `git stash` operates worktree-wide (not on A's owned paths),
//   it captured Agent B's in-progress edits across seven unrelated
//   files. The stash label said TASK-X; the contents were 100% Agent
//   B's TASK-Y rendering work. Agent B's next file read returned pre-
//   edit content, they thought "linter is reverting my edits", and
//   returned blocked. Recovery required `git stash pop`.
//
// Detection rule — collision shape, not caller identity:
//   We can't reliably tell *which* agent issued the Bash call. We can
//   tell whether the call would *cause* a collision: enumerate dirty
//   files via `git status --porcelain`, resolve each to its owning
//   agent (lib/common.js OWNERSHIP_RULES, mirror of subtree CLAUDE.md
//   `Owned by` markers), and block if the owner-set has size > 1. The
//   block message names the owners and the paths in each bucket so
//   whichever agent is calling immediately sees what they would sweep.
//
// Subcommands gated:
//   `git stash`               (implicit push — worktree-wide)
//   `git stash push`          (worktree-wide unless `-- <paths>`)
//   `git stash save`          (deprecated alias for push)
//
// Subcommands NOT gated (do not sweep dirty work):
//   `git stash pop / list / show / drop / clear / branch / create / store`
//   `git stash push -- <paths>` — caller scoped to specific paths;
//                                  the gate's job is done by the path
//                                  filter itself.
//
// Out of scope (per TASK-196 AC #1, single CL / single concern):
//   `git checkout -- <path>`, `git reset --hard`, `git restore .`.
//   These have different semantics (e.g. checkout-with-pathspec is
//   fine when the path is owned; reset --hard is always worktree-wide
//   and destructive). A follow-up task should extend this gate or add
//   a sibling once the `git stash` form has stabilised.
//
// Sentinel: `[stash-cross-subtree-OK]` anywhere in the bash command
// (commonly as a trailing `# [stash-cross-subtree-OK]` comment) opts
// out for legitimate cross-subtree intentional stashes — e.g. a tree-
// wide cleanup the user explicitly approved. Discipline: rare; named
// in the block message so the agent has to type it deliberately.
//
// Fails OPEN on any internal error so a hook bug never bricks Bash
// dispatch — same posture as the other session-gate sub-gates.

const { resolveOwner, getDirtyFiles, parseGitStashCommand } = require('../lib/ownership')

const SENTINEL = '[stash-cross-subtree-OK]'

function run(input) {
  if (input.tool_name !== 'Bash') return { ok: true }
  const ti = input.tool_input || {}
  const cmd = typeof ti.command === 'string' ? ti.command : ''
  if (!cmd) return { ok: true }

  const parsed = parseGitStashCommand(cmd)
  if (!parsed) return { ok: true }
  if (parsed.hasPathFilter) return { ok: true }
  if (cmd.includes(SENTINEL)) return { ok: true }

  const cwd = input.cwd || process.cwd()
  const dirty = getDirtyFiles(cwd)
  if (dirty.length === 0) return { ok: true }

  const buckets = new Map()  // owner -> [paths]
  for (const path of dirty) {
    const owner = resolveOwner(path) || '<unowned>'
    if (!buckets.has(owner)) buckets.set(owner, [])
    buckets.get(owner).push(path)
  }
  if (buckets.size <= 1) return { ok: true }

  return { ok: false, block: () => emit(buckets) }
}

function emit(buckets) {
  const lines = []
  for (const [owner, paths] of buckets) {
    lines.push(`  ${owner}:`)
    for (const p of paths) lines.push(`    ${p}`)
  }
  process.stderr.write([
    '',
    '[session-gate] git stash blocked — dirty files cross subtree ownership.',
    '',
    'A worktree-wide `git stash` would sweep files owned by multiple agents:',
    '',
    lines.join('\n'),
    '',
    'This is the cross-agent collision TASK-196 was filed to prevent. The',
    'stash label would name your task but the contents would include another',
    "agent's in-progress work, breaking their next file read and producing a",
    'misleading audit trail.',
    '',
    'To proceed safely, scope the stash to your owned paths:',
    '  git stash push -m "<msg>" -- <path1> <path2> ...',
    '',
    `Escape hatch: include ${SENTINEL} in the command (typically as a`,
    'trailing `# [stash-cross-subtree-OK]` comment) ONLY if a tree-wide',
    'stash is genuinely the right move — e.g. a user-approved cleanup',
    'that legitimately spans subtrees. See .claude/disciplines/owner-mode.md.',
    '',
  ].join('\n'))
  process.exit(2)
}

module.exports = { run, SENTINEL }
