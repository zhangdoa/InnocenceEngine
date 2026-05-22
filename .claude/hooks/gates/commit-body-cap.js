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
    `[commit-gate] git commit blocked — body ${n} lines > cap ${BODY_LINE_CAP}.`,
    '',
    'Trailers (Key: Value tail lines) excluded from the count.',
    '',
  ].join('\n'))
  process.exit(2)
}

module.exports = { run, needsTranscript: false }
