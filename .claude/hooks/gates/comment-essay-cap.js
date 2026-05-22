// comment-essay-cap: block staged code adding > N contiguous lines of `//` comments.
// Targets the essay-comment / file-header-essay pattern.

const { execSync } = require('child_process')

const ESSAY_CAP = 5
const CODE_EXT_RE = /\.(cpp|hpp|h|c|cc|cxx|inl|hlsl|hlsli|comp|frag|vert|js|mjs|ts|ps1|sh)$/i
const EXCLUDE_RE = /(^|\/)(ThirdParty|External|node_modules|Generated|dist)\//

function findEssayCommentRuns(cwd, staged) {
  const violations = []
  for (const f of staged) {
    if (!CODE_EXT_RE.test(f)) continue
    if (EXCLUDE_RE.test(f)) continue
    let diff = ''
    try {
      const escaped = f.replace(/"/g, '\\"')
      diff = execSync(`git -c core.quotePath=false diff --cached --no-color -- "${escaped}"`,
        { cwd, encoding: 'utf8' })
    } catch { continue }
    let runLen = 0
    let runStart = -1
    let lineNo = 0
    for (const line of diff.split('\n')) {
      const hunk = line.match(/^@@ -\d+(?:,\d+)? \+(\d+)/)
      if (hunk) { lineNo = parseInt(hunk[1], 10); runLen = 0; continue }
      if (line.startsWith('+++') || line.startsWith('---')) continue
      if (line.startsWith('+')) {
        if (/^\+\s*\/\//.test(line)) {
          if (runLen === 0) runStart = lineNo
          runLen++
          if (runLen > ESSAY_CAP) {
            violations.push({ file: f, line: runStart, length: runLen })
            break
          }
        } else {
          runLen = 0
        }
        lineNo++
      } else if (line.startsWith(' ')) {
        runLen = 0
        lineNo++
      }
    }
  }
  return violations
}

function run(ctx) {
  const violations = findEssayCommentRuns(ctx.cwd, ctx.staged)
  if (violations.length === 0) return { ok: true }
  return { ok: false, block: () => emit(violations) }
}

function emit(violations) {
  const list = violations.slice(0, 10).map(v => `  ${v.file}:${v.line}  (${v.length}-line // run)`).join('\n')
  process.stderr.write([
    '',
    `[commit-gate] git commit blocked — essay-comment run > ${ESSAY_CAP} contiguous lines.`,
    '',
    list,
    '',
    'WHY comments are fine when non-obvious. Multi-line explanatory essays at the top of a file',
    'or above a function are noise — the code names itself. Replace with a one-line summary or delete.',
    '',
  ].join('\n'))
  process.exit(2)
}

module.exports = { run, needsTranscript: false }
