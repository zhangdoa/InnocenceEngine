// comment-essay-cap: block staged code comments that are too long OR carry backlog
// content. On added // lines: (1) > N contiguous = essay; (2) a tracker id / design
// pointer = ref. Comments carry the non-obvious WHY; tracker ids, doc sections, and
// phase/increment narration live in the tracker, not the source.

const { execSync } = require('child_process')

const ESSAY_CAP = 5
const CODE_EXT_RE = /\.(cpp|hpp|h|c|cc|cxx|inl|hlsl|hlsli|comp|frag|vert|js|mjs|ts|ps1|sh)$/i
const EXCLUDE_RE = /(^|\/)(ThirdParty|External|node_modules|Generated|dist)\//

// Backlog/design-doc references that do not belong in a source comment.
// Matches issue ids (FOO-123), design-doc section/decision pointers, and the
// migration jargon this gate exists to keep out of the code.
const REF_PATTERNS = [
  /\b(?:TASK|ISSUE|BUG|JIRA|TICKET|PR|MR)-\d+/i,
  /\bRFC\b/i,
  /§\s*\d/,
  /\bprimitive\s*#?\d/i,
  /\bbin-[ab]\b/i,
  /\bPhase[-\s]?\d/i,
]

function isComment(diffLine) { return /^\+\s*\/\//.test(diffLine) }
function refHit(diffLine) { return REF_PATTERNS.some(re => re.test(diffLine)) }

// Pure scan of one file's `git diff --cached` text. Returns { essay, refs }
// arrays of { file, line } so the caller can aggregate and the test can drive
// it without a git working tree.
function scanDiff(diff, file) {
  const essay = []
  const refs = []
  let runLen = 0
  let runStart = -1
  let reported = false
  let lineNo = 0
  for (const line of diff.split('\n')) {
    const hunk = line.match(/^@@ -\d+(?:,\d+)? \+(\d+)/)
    if (hunk) { lineNo = parseInt(hunk[1], 10); runLen = 0; reported = false; continue }
    if (line.startsWith('+++') || line.startsWith('---')) continue
    if (line.startsWith('+')) {
      if (isComment(line)) {
        if (refHit(line)) refs.push({ file, line: lineNo })
        if (runLen === 0) { runStart = lineNo; reported = false }
        runLen++
        if (runLen > ESSAY_CAP && !reported) { essay.push({ file, line: runStart, length: runLen }); reported = true }
      } else {
        runLen = 0
      }
      lineNo++
    } else if (line.startsWith(' ')) {
      runLen = 0
      lineNo++
    }
  }
  return { essay, refs }
}

function findViolations(cwd, staged) {
  const essay = []
  const refs = []
  for (const f of staged) {
    if (!CODE_EXT_RE.test(f)) continue
    if (EXCLUDE_RE.test(f)) continue
    let diff = ''
    try {
      const escaped = f.replace(/"/g, '\\"')
      diff = execSync(`git -c core.quotePath=false diff --cached --no-color -- "${escaped}"`,
        { cwd, encoding: 'utf8' })
    } catch { continue }
    const r = scanDiff(diff, f)
    essay.push(...r.essay)
    refs.push(...r.refs)
  }
  return { essay, refs }
}

function run(ctx) {
  const { essay, refs } = findViolations(ctx.cwd, ctx.staged)
  if (essay.length === 0 && refs.length === 0) return { ok: true }
  return { ok: false, block: () => emit(essay, refs) }
}

function emit(essay, refs) {
  const out = ['']
  if (essay.length) {
    out.push(`[commit-gate] git commit blocked — essay-comment run > ${ESSAY_CAP} contiguous lines.`, '')
    out.push(essay.slice(0, 10).map(v => `  ${v.file}:${v.line}  (${v.length}-line // run)`).join('\n'), '')
    out.push('WHY comments are fine when non-obvious. Multi-line explanatory essays at the top of a file',
      'or above a function are noise — the code names itself. Replace with a one-line summary or delete.', '')
  }
  if (refs.length) {
    out.push('[commit-gate] git commit blocked — comment references a tracker id / design doc.', '')
    out.push(refs.slice(0, 10).map(v => `  ${v.file}:${v.line}`).join('\n'), '')
    out.push('Task ids (TASK-123), RFC/design-doc sections, phase/increment/bin jargon do not belong',
      'in source comments — they live in the tracker. Keep the comment to the non-obvious WHY.', '')
  }
  process.stderr.write(out.join('\n'))
  process.exit(2)
}

module.exports = { run, needsTranscript: false, scanDiff, REF_PATTERNS }
