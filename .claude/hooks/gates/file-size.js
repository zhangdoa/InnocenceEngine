// File-size gate — strict block on code/script files past the limit.
// For each staged file matching FILE_SIZE_EXT_RE (and not excluded by
// FILE_SIZE_EXCLUDE_RE), block if `new_lines > FILE_SIZE_LIMIT`.
// No grandfathering — touching an oversized file forces it under the limit
// in the same CL. Split per `disciplines/on-implement/file-splitting.md`.

const {
  FILE_SIZE_LIMIT, FILE_SIZE_EXT_RE, FILE_SIZE_EXCLUDE_RE,
  blobLineCount,
} = require('../lib/common')

function findViolations(cwd, staged) {
  const violations = []
  for (const f of staged) {
    if (!FILE_SIZE_EXT_RE.test(f)) continue
    if (FILE_SIZE_EXCLUDE_RE.test(f)) continue
    const newLines = blobLineCount(cwd, `:${f}`)
    if (newLines <= FILE_SIZE_LIMIT) continue
    const oldLines = blobLineCount(cwd, `HEAD:${f}`)
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
  const list = violations.slice(0, 10).map(v =>
    `  ${v.file}: ${v.oldLines} → ${v.newLines}`
  ).join('\n')
  const more = violations.length > 10
    ? `\n  …and ${violations.length - 10} more` : ''
  process.stderr.write([
    '',
    `[commit-gate] git commit blocked — file(s) over the ${FILE_SIZE_LIMIT}-line limit.`,
    '',
    'Files in this CL exceeding the limit:',
    list + more,
    '',
    'Touching a file makes you responsible for its size. Split per',
    '`.claude/disciplines/on-implement/file-splitting.md`:',
    '  • Same class, different responsibility cluster → Foo_SubsectionName.cpp.',
    '  • Separate concern → new class; original holds an instance.',
    '  • Free-function header → split by domain; umbrella header includes parts.',
    '',
    'No string-based escape exists. If a path legitimately requires exemption',
    '(third-party drop, generated output), add it to FILE_SIZE_EXCLUDE_RE in',
    '.claude/hooks/lib/common.js.',
    '',
  ].join('\n'))
  process.exit(2)
}

module.exports = { run, needsTranscript: false }
