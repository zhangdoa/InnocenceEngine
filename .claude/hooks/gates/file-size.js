const { execSync } = require('child_process')
const {
  FILE_SIZE_LIMIT, FILE_SIZE_EXT_RE, FILE_SIZE_EXCLUDE_RE,
  blobLineCount,
} = require('../lib/common')

function buildRenameMap(cwd) {
  const map = new Map()
  try {
    const out = execSync(`git diff --cached --find-renames --name-status`, { cwd, encoding: 'utf8' })
    for (const line of out.split('\n')) {
      const m = line.match(/^R\d+\t(.+?)\t(.+)$/)
      if (m) map.set(m[2], m[1])
    }
  } catch { /* fall through; empty map */ }
  return map
}

function findViolations(cwd, staged) {
  const renames = buildRenameMap(cwd)
  const violations = []
  for (const f of staged) {
    if (!FILE_SIZE_EXT_RE.test(f)) continue
    if (FILE_SIZE_EXCLUDE_RE.test(f)) continue
    const newLines = blobLineCount(cwd, `:${f}`)
    if (newLines <= FILE_SIZE_LIMIT) continue
    let oldLines = blobLineCount(cwd, `HEAD:${f}`)
    if (oldLines === 0 && renames.has(f)) {
      oldLines = blobLineCount(cwd, `HEAD:${renames.get(f)}`)
    }
    if (newLines <= oldLines) continue
    violations.push({ file: f, oldLines, newLines })
  }
  return violations
}

function run(ctx) {
  const violations = findViolations(ctx.cwd, ctx.staged)
  if (violations.length === 0) return { ok: true }
  return { ok: false, block: () => emit(violations) }
}

function emit(violations) {
  const list = violations.slice(0, 10).map(v => `  ${v.file}: ${v.oldLines} → ${v.newLines}`).join('\n')
  const more = violations.length > 10 ? `\n  …and ${violations.length - 10} more` : ''
  process.stderr.write([
    '',
    `[commit-gate] git commit blocked — file(s) > ${FILE_SIZE_LIMIT} lines AND growing.`,
    '',
    list + more,
    '',
    'Renames followed via git diff --find-renames; no-growth touches pass. Split per .claude/skills/file-splitting/SKILL.md.',
    '',
  ].join('\n'))
  process.exit(2)
}

module.exports = { run, needsTranscript: false }
