// File-size gate — soft ratchet on code/script files.
// For each staged file matching FILE_SIZE_EXT_RE (and not excluded by
// FILE_SIZE_EXCLUDE_RE), block if `new_lines > FILE_SIZE_LIMIT` AND
// `new_lines > old_lines`. Files already over the limit keep working;
// they just can't grow. [skip-size-gate] in the commit message escapes.

const {
  FILE_SIZE_LIMIT, FILE_SIZE_EXT_RE, FILE_SIZE_EXCLUDE_RE,
  SKIP_SIZE_SENTINEL, blobLineCount,
} = require('../lib/common')

function findViolations(cwd, staged) {
  const violations = []
  for (const f of staged) {
    if (!FILE_SIZE_EXT_RE.test(f)) continue
    if (FILE_SIZE_EXCLUDE_RE.test(f)) continue
    const newLines = blobLineCount(cwd, `:${f}`)
    if (newLines <= FILE_SIZE_LIMIT) continue
    const oldLines = blobLineCount(cwd, `HEAD:${f}`)
    if (newLines > oldLines) violations.push({ file: f, oldLines, newLines })
  }
  return violations
}

function run(ctx) {
  if (ctx.messageText.includes(SKIP_SIZE_SENTINEL)) return { ok: true }
  const violations = findViolations(ctx.cwd, ctx.staged)
  if (violations.length === 0) return { ok: true }
  return { ok: false, block: () => emit(violations) }
}

function emit(violations) {
  const list = violations.slice(0, 10).map(v =>
    `  ${v.file}: ${v.oldLines} → ${v.newLines} (+${v.newLines - v.oldLines})`
  ).join('\n')
  const more = violations.length > 10
    ? `\n  …and ${violations.length - 10} more` : ''
  process.stderr.write([
    '',
    `[commit-gate] git commit blocked — code/script file(s) grew past the ${FILE_SIZE_LIMIT}-line soft ratchet.`,
    '',
    'Files over limit that grew in this CL:',
    list + more,
    '',
    'A growing oversized file usually means the responsibility belongs in a',
    'separate translation unit. Common responses:',
    '  • Split into multiple files (#include-based for shaders; new .cpp/.h for C++).',
    '  • Extract helper functions or pass objects into a common header.',
    '  • If the addition itself is small but the file is already way over,',
    '    shrink the file first (delete dead code, inline one-shot utilities, etc).',
    '',
    `Already-oversized files are grandfathered — as long as they don't GROW`,
    'the commit passes. The threshold only pressures files that are both',
    'over and getting larger.',
    '',
    `Escape hatch: include ${SKIP_SIZE_SENTINEL} in the commit message if`,
    'this is a legitimate one-off (e.g. auto-generated file, necessary migration).',
    '',
  ].join('\n'))
  process.exit(2)
}

module.exports = { run }
