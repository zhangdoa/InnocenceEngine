// Data-generated gate — block any commit that tracks files under
// `Data/Generated/`, or that loosens `.gitignore` to permit such tracking.
//
// `Data/Generated/` is project-canonical derived runtime output; it is
// regenerated from sources in `Data/Engine/`, `Source/External/`, etc.
// Tracking files there poisons the working tree (binary churn, stale
// regenerations vs. tracked copy, "fresh clone boots" illusion that
// hides the real engine-path bug). The right fix to a fresh-clone boot
// failure is in the engine path or asset source location — never in
// `.gitignore`.
//
// Prior incident: TASK-133 / commit 652c7a29 added
// `Data/Generated/Fonts/FreeSans.otf` and carved `Generated/Fonts/` out
// of the `/Data/Generated/*` ignore mask. This gate exists to prevent
// that recurrence even if a future agent reaches the same wrong
// conclusion.

const { execSync } = require('child_process')

const DATA_GENERATED_PATH_RE = /^Data\/Generated\//

// Lines we accept as "the canonical ignore mask" for `Data/Generated/`.
// If any of these are deleted by the staged .gitignore diff, that's a
// violation. We accept the leading-slash and no-slash forms.
const PROTECTED_IGNORE_LINES = [
  '/Data/Generated/*',
  'Data/Generated/*',
]

// Match a staged-addition line that un-ignores something under
// `Data/Generated/` — e.g. `+!/Data/Generated/Foo/` or `+!Data/Generated/x`.
const UNIGNORE_ADDED_RE = /^\+!\/?Data\/Generated\//m

function findStagedDataGeneratedFiles(staged) {
  return staged.filter(f => DATA_GENERATED_PATH_RE.test(f))
}

function readGitignoreDiff(cwd) {
  try {
    return execSync(
      'git -c core.quotePath=false diff --cached -- .gitignore',
      { cwd, encoding: 'utf8', stdio: ['pipe', 'pipe', 'pipe'] }
    )
  } catch {
    return ''
  }
}

// Inspect the staged .gitignore diff for two violation shapes:
//   1. A deletion line (`-`) that removes a protected ignore mask.
//   2. An addition line (`+`) that un-ignores anything under Data/Generated/.
function findGitignoreViolations(diff) {
  if (!diff) return []
  const violations = []
  for (const rawLine of diff.split('\n')) {
    // Skip the diff header lines (`---`, `+++`) — they are not content.
    if (rawLine.startsWith('---') || rawLine.startsWith('+++')) continue
    if (rawLine.startsWith('-')) {
      const body = rawLine.slice(1).trim()
      for (const protect of PROTECTED_IGNORE_LINES) {
        if (body === protect) {
          violations.push({ kind: 'removed-ignore', line: rawLine })
          break
        }
      }
    } else if (rawLine.startsWith('+')) {
      // Re-test against the un-ignore shape; we want the original line
      // for the message, so test the raw line directly.
      if (UNIGNORE_ADDED_RE.test(rawLine)) {
        violations.push({ kind: 'added-unignore', line: rawLine })
      }
    }
  }
  return violations
}

function run(ctx) {
  const stagedFiles = findStagedDataGeneratedFiles(ctx.staged)
  const ignoreDiff = readGitignoreDiff(ctx.cwd)
  const ignoreViolations = findGitignoreViolations(ignoreDiff)
  if (stagedFiles.length === 0 && ignoreViolations.length === 0) {
    return { ok: true }
  }
  return { ok: false, block: () => emit(stagedFiles, ignoreViolations) }
}

function emit(stagedFiles, ignoreViolations) {
  const lines = [
    '',
    '[commit-gate] git commit blocked — staged change touches `Data/Generated/`.',
    '',
    'Rule: `Data/Generated/` is derived runtime output, never to be tracked.',
    'It is regenerated from `Data/Engine/`, `Source/External/`, and other',
    'tracked sources at engine boot or build time.',
    '',
  ]

  if (stagedFiles.length > 0) {
    lines.push('Staged file(s) under `Data/Generated/` (must not be tracked):')
    for (const f of stagedFiles.slice(0, 20)) lines.push(`  ${f}`)
    if (stagedFiles.length > 20) {
      lines.push(`  …and ${stagedFiles.length - 20} more`)
    }
    lines.push('')
  }

  if (ignoreViolations.length > 0) {
    lines.push('`.gitignore` change loosens the `Data/Generated/` mask:')
    for (const v of ignoreViolations.slice(0, 20)) {
      const kind = v.kind === 'removed-ignore'
        ? 'removes ignore mask'
        : 'adds un-ignore'
      lines.push(`  [${kind}] ${v.line}`)
    }
    if (ignoreViolations.length > 20) {
      lines.push(`  …and ${ignoreViolations.length - 20} more`)
    }
    lines.push('')
  }

  lines.push(
    'The right fix is in the engine path or asset source location, not the',
    'gitignore. If a fresh clone needs an asset, source it from a tracked',
    'location (e.g. `Data/Engine/`, `Source/External/`) or a setup-script',
    'download — never by un-ignoring derived output.',
    '',
    'Prior incident this gate prevents:',
    '  TASK-133 / commit 652c7a29 — `fix(build): TASK-133 deploy Shaders and',
    '  Data trees ...` tracked `Data/Generated/Fonts/FreeSans.otf` and carved',
    '  `Generated/Fonts/` out of the `/Data/Generated/*` ignore mask.',
    '',
    'No string-based escape. If a one-off file legitimately needs to ship,',
    'move it out of `Data/Generated/` to a tracked source location like',
    '`Data/Engine/` or `Source/External/` — that is the right fix.',
    '',
  )

  process.stderr.write(lines.join('\n'))
  process.exit(2)
}

module.exports = { run, needsTranscript: false }
