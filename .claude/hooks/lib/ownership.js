// Ownership + git-stash helpers for the cross-subtree-stash gate
// (TASK-196). Kept separate from lib/common.js so common.js stays under
// the 400-line file-size gate. These helpers are used by exactly one
// gate today; if a second gate adopts them, the placement is still
// fine — common.js is for cross-cutting infrastructure (regexes,
// transcript scanning, commit-message parsing), not domain helpers.
//
// What lives here:
//   OWNERSHIP_RULES       — path-prefix → agent table, mirror of every
//                           subtree's CLAUDE.md `Owned by` marker.
//   resolveOwner(path)    — apply the table to one repo-relative path;
//                           returns the agent name or `null` for paths
//                           we deliberately don't classify (the gate
//                           treats `null` as its own domain to stay
//                           conservative on unknown territory).
//   getDirtyFiles(cwd)    — `git status --porcelain -z` parser; returns
//                           the list of files a worktree-wide stash
//                           would sweep (modified, staged, untracked).
//   parseGitStashCommand  — recognise the destructive-stash subcommand
//                           shapes and the `-- <pathspec>` opt-out.

const { execSync } = require('child_process')

// Path → owning impl stage. Used by the cross-subtree-stash gate to
// detect when a stash sweeps paths outside the dispatch's stage. Most-
// specific rules first.
//
// Most C++ / TypeScript source maps to `code-impl`. Shaders to
// `shader-impl`. Build/CMake/Scripts to `ci-build-impl`. Harness and
// alignments to `harness-impl`. Backlog to `task-mgmt`.
const OWNERSHIP_RULES = [
  { match: p => p.startsWith('Source/Shaders/'),    owner: 'shader-impl' },
  { match: p => p.startsWith('Source/'),            owner: 'code-impl' },
  { match: p => p.startsWith('CMake/'),             owner: 'ci-build-impl' },
  { match: p => p.startsWith('Scripts/'),           owner: 'ci-build-impl' },
  { match: p => p.startsWith('.backlog/'),          owner: 'task-mgmt' },
  { match: p => p.startsWith('.claude/'),           owner: 'harness-impl' },
  { match: p => p.startsWith('.alignments/'),       owner: 'harness-impl' },
]

function resolveOwner(filePath) {
  const norm = filePath.replace(/\\/g, '/').replace(/^"+|"+$/g, '')
  for (const rule of OWNERSHIP_RULES) {
    if (rule.match(norm)) return rule.owner
  }
  return null
}

// Parse `git status --porcelain -z` into a list of repo-relative paths.
// Includes unstaged changes (column-2 letter), staged changes (column-1
// letter), and untracked files (`??`). Renames (`R `) emit the
// destination path; `-z` then sends the *old* name as the next NUL-
// separated chunk, which we skip.
//
// Returns [] on any git failure — the gate fails open in that case so
// a transient git error never bricks Bash dispatch.
function getDirtyFiles(cwd) {
  let raw
  try {
    raw = execSync('git status --porcelain -z', {
      cwd, encoding: 'utf8', stdio: ['pipe', 'pipe', 'pipe'],
    })
  } catch { return [] }
  const out = []
  const chunks = raw.split('\0').filter(Boolean)
  for (let i = 0; i < chunks.length; i++) {
    const entry = chunks[i]
    if (entry.length < 4) continue
    const xy = entry.slice(0, 2)
    const path = entry.slice(3)
    out.push(path)
    if (xy[0] === 'R' || xy[0] === 'C') i++ // skip the old-name chunk
  }
  return out
}

// Detect the worktree-disruptive `git stash` invocation we want to
// gate. Returns null for non-stash commands and for non-destructive
// subcommands (`pop`, `list`, `show`, `drop`, `clear`, `branch`,
// `apply`, `create`, `store`). Returns `{ subcommand, hasPathFilter }`
// for destructive forms.
//
// Destructive forms (worktree-wide unless `-- <paths>` is present):
//   `git stash`                           — implicit push
//   `git stash push [...]`
//   `git stash save [...]` (deprecated alias for push; collapsed)
//   `git stash -m "msg"` / `-u` / `-a` / etc. — leading flag implies push
//
// Flag soup is tolerated — the worktree-wide-vs-pathspec distinction is
// determined by the presence of `-- <path>`, not by the flag set.
function parseGitStashCommand(cmd) {
  const m = cmd.match(/(?:^|[\s;&|])git\s+stash(?:\s+(\S+))?(.*)$/)
  if (!m) return null
  const next = (m[1] || '').toLowerCase()
  const tail = m[2] || ''
  const destructiveSubs = new Set([
    'push', 'save',
    '-m', '--message',
    '-u', '--include-untracked',
    '-a', '--all',
    '-k', '--keep-index',
    '-p', '--patch',
    '-S', '--staged',
    '-q', '--quiet',
  ])
  let subcommand
  if (!next) {
    subcommand = 'push'
  } else if (next === 'push' || next === 'save') {
    subcommand = 'push'
  } else if (destructiveSubs.has(next)) {
    subcommand = 'push'
  } else {
    return null
  }
  const hasPathFilter = /\s--\s+\S/.test(' ' + next + tail)
  return { subcommand, hasPathFilter }
}

module.exports = {
  OWNERSHIP_RULES,
  resolveOwner,
  getDirtyFiles,
  parseGitStashCommand,
}
