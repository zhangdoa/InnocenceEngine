// Commit-body-cap gate — body (between subject blank and trailer block)
// capped at BODY_LINE_CAP. Targets the "long honest disclosure / verbose
// audit prose" pattern. Trailers (Key: Value at message tail) excluded
// from the count.

const BODY_LINE_CAP = 40
const TRAILER_RE = /^[A-Z][\w-]*: \S/

function countBodyLines(message) {
  const lines = (message || '').split('\n')
  if (lines.length <= 1) return 0
  let i = 1
  while (i < lines.length && lines[i].trim() === '') i++
  const bodyStart = i
  let j = lines.length - 1
  while (j >= bodyStart && lines[j].trim() === '') j--
  let trailerStart = j + 1
  while (trailerStart > bodyStart && TRAILER_RE.test(lines[trailerStart - 1])) trailerStart--
  let bodyEnd = trailerStart
  while (bodyEnd > bodyStart && lines[bodyEnd - 1].trim() === '') bodyEnd--
  return Math.max(0, bodyEnd - bodyStart)
}

function run(ctx) {
  const n = countBodyLines(ctx.messageText || '')
  if (n <= BODY_LINE_CAP) return { ok: true }
  return { ok: false, block: () => emit(n) }
}

function emit(n) {
  process.stderr.write([
    '',
    `[commit-gate] git commit blocked — body ${n} lines exceeds cap of ${BODY_LINE_CAP}.`,
    '',
    'Long bodies substitute for working code. Tighten or split:',
    '  • Drop "honest disclosure", "what was NOT verified" sections.',
    '  • Move algorithm rationale to a code comment at the decision site.',
    '  • Cite a backlog task ID for design history rather than embedding it.',
    '',
    'Trailers (Key: Value lines at message tail) are excluded from the count.',
    'No string-escape sentinel.',
    '',
  ].join('\n'))
  process.exit(2)
}

module.exports = { run, needsTranscript: false }
