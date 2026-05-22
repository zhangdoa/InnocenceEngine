// test-run: staged code requires a qualifying integration test since last real user prompt.
// Docs-only auto-skips unless a task is closing. Closure-Reason: footer re-applies docs bypass
// for obsolete/non-reproducible/superseded closures.

const {
  QUALIFYING_TEST, DOCS_ONLY_PATH, firstArray,
} = require('../lib/common')

// [ \t]* not \s* — \s includes \n, would let `Closure-Reason:\n\nReviewed-By:` satisfy.
const CLOSURE_REASON_RE = /^Closure-Reason:[ \t]*\S/m

function didQualifyingTestRun(transcript, lastUserIdx) {
  for (let i = lastUserIdx + 1; i < transcript.length; i++) {
    const m = transcript[i]
    const blocks = firstArray(m.message?.content, m.content)
    for (const b of blocks) {
      if (b?.type === 'tool_use' && b?.name === 'Bash') {
        const c = b.input?.command || ''
        if (QUALIFYING_TEST.test(c)) return true
      }
    }
  }
  return false
}

function run(ctx) {
  const allDocs = ctx.staged.length > 0 && ctx.staged.every(f => DOCS_ONLY_PATH.test(f))
  const closureExempt = ctx.closingTasks.length > 0 && CLOSURE_REASON_RE.test(ctx.messageText || '')
  if (allDocs && (ctx.closingTasks.length === 0 || closureExempt)) return { ok: true }

  if (didQualifyingTestRun(ctx.transcript, ctx.lastUserIdx)) return { ok: true }

  const closing = ctx.closingTasks.length > 0
  return { ok: false, block: () => emit(ctx.staged, closing) }
}

function emit(staged, closing) {
  const filesList = staged.length
    ? staged.slice(0, 10).map(f => '  ' + f).join('\n') +
      (staged.length > 10 ? `\n  …and ${staged.length - 10} more` : '')
    : '  (no staged files detected)'
  const header = closing
    ? '[commit-gate] git commit blocked — task closure without integration test.'
    : '[commit-gate] git commit blocked — no integration test run in this turn.'
  const closingNote = closing
    ? [
        '',
        'Closure exemption: add `Closure-Reason: <value>` footer for obsolete/non-reproducible/superseded closures.',
        '',
      ]
    : ['']
  process.stderr.write([
    '',
    header,
    '',
    closing ? 'Tasks flipping to Done:' : 'Staged files:',
    filesList,
    ...closingNote,
    'Run one of:',
    '  Bin/RelWithDebInfo/Main.exe -total_frames N',
    '  Bin/RelWithDebInfo/Main.exe -total_frames N -reload_at_frame M',
    '  Bin/RelWithDebInfo/RenderTest.exe -test <name>',
    '  Bin/RelWithDebInfo/Main.exe -capture_frame N',
    '  Bin/RelWithDebInfo/Main.exe -serialize_test <scene>',
    '  InteractiveTest.ps1',
    '  npx playwright test tests/<spec>.spec.js',
    '',
  ].join('\n'))
  process.exit(2)
}

module.exports = { run, needsTranscript: true }
