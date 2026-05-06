// Test-run gate — staged code CLs need a qualifying integration test to
// have run since the last real user prompt. Docs-only CLs (matching
// DOCS_ONLY_PATH) skip the gate UNLESS a closing task is also staged
// (which is the closure-evidence rule — the test must back the claim).
//
// Closure-evidence exemption: a `Closure-Reason: <value>` commit-message
// footer suspends the closure-as-evidence-override on the docs-only path.
// Use for genuinely-obsolete / non-reproducible / superseded closures
// where running an integration test purely to satisfy the gate adds no
// signal. Code-bearing CLs still need a qualifying test — the exemption
// only re-applies the docs-only bypass.

const {
  QUALIFYING_TEST, DOCS_ONLY_PATH, firstArray,
} = require('../lib/common')

// Same-line value required ([ \t]* not \s*, because \s includes \n and
// would let `Closure-Reason:\n\nReviewed-By: …` satisfy the regex).
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
  // Docs-only bypass. Normally doesn't apply if a task is flipping to
  // Done (closure claim must be test-backed). The Closure-Reason: footer
  // re-applies the bypass for genuinely-obsolete / non-reproducible /
  // superseded closures.
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
  const rationale = closing
    ? [
        'Closing a task asserts the work is validated. The docs-only bypass does',
        'NOT apply to a completion claim — a closing CL must be backed by a test',
        'run in the current turn, same as a code CL.',
        '',
        'Exemption: add a `Closure-Reason: <value>` commit-message footer for',
        'genuinely-obsolete / non-reproducible / superseded closures where a test',
        'run adds no signal. With the footer present, the docs-only bypass applies',
        'again. Use sparingly — not for "the test was a pain to set up".',
        '',
      ]
    : []
  process.stderr.write([
    '',
    header,
    '',
    closing ? 'Task(s) flipping to status: Done this commit:' : 'Staged files:',
    filesList,
    '',
    ...rationale,
    'Run one of the following in this turn before committing:',
    '  • Bin\\RelWithDebInfo\\Main.exe -total_frames N    (see disciplines/on-bug/perf-measurement-frame-budget.md for choosing N)',
    '  • Bin\\RelWithDebInfo\\Main.exe -total_frames N -reload_at_frame M',
    '  • Bin\\RelWithDebInfo\\RenderTest.exe -test <name>',
    '  • Bin\\RelWithDebInfo\\Main.exe -capture_frame N',
    '  • InteractiveTest.ps1',
    '  • npx playwright test tests/<spec>.spec.js',
    '',
    'No string-based escape. The bypass is path-derived: if every staged file',
    'matches DOCS_ONLY_PATH (.backlog/, .claude/, *.md, etc.) and no closing',
    'task is staged, the gate auto-skips. If your change touches code, run a test.',
    '',
  ].join('\n'))
  process.exit(2)
}

module.exports = { run, needsTranscript: true }
