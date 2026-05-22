// data-generated: block tracking files under Data/Generated/ or loosening the .gitignore mask.
// Data/Generated/ is derived runtime output, never to be tracked.

const { execSync } = require('child_process')

const DATA_GENERATED_PATH_RE = /^Data\/Generated\//

const PROTECTED_IGNORE_LINES = [
  '/Data/Generated/*',
  'Data/Generated/*',
]

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

function findGitignoreViolations(diff) {
  if (!diff) return []
  const violations = []
  for (const rawLine of diff.split('\n')) {
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
    '[commit-gate] git commit blocked — Data/Generated/ change.',
    '',
  ]

  if (stagedFiles.length > 0) {
    lines.push('Staged files under Data/Generated/ (must not be tracked):')
    for (const f of stagedFiles.slice(0, 20)) lines.push(`  ${f}`)
    if (stagedFiles.length > 20) lines.push(`  …and ${stagedFiles.length - 20} more`)
    lines.push('')
  }

  if (ignoreViolations.length > 0) {
    lines.push('.gitignore change loosens the Data/Generated/ mask:')
    for (const v of ignoreViolations.slice(0, 20)) {
      const kind = v.kind === 'removed-ignore' ? 'removes ignore mask' : 'adds un-ignore'
      lines.push(`  [${kind}] ${v.line}`)
    }
    lines.push('')
  }

  lines.push(
    'Data/Generated/ is regenerated from tracked sources. The right fix to a fresh-clone',
    'boot failure is in the engine path or asset source location, not the gitignore.',
    '',
  )

  process.stderr.write(lines.join('\n'))
  process.exit(2)
}

module.exports = { run, needsTranscript: false }
