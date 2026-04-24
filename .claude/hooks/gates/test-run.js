// Test-run gate — staged code CLs need a qualifying integration test to
// have run since the last real user prompt. Docs-only CLs (matching
// DOCS_ONLY_PATH) skip the gate UNLESS a closing task is also staged
// (which is the closure-evidence rule — the test must back the claim).

const {
  QUALIFYING_TEST, DOCS_ONLY_PATH, SKIP_SENTINEL, firstArray,
} = require('../lib/common')

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
  if (ctx.messageText.includes(SKIP_SENTINEL)) return { ok: true }

  // Docs-only bypass. Doesn't apply if a task is flipping to Done (see
  // closure-evidence gate — closure claim must be test-backed).
  const allDocs = ctx.staged.length > 0 && ctx.staged.every(f => DOCS_ONLY_PATH.test(f))
  if (allDocs && ctx.closingTasks.length === 0) return { ok: true }

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
    '  • Bin\\RelWithDebInfo\\Main.exe -total_frames N',
    '  • Bin\\RelWithDebInfo\\Main.exe -total_frames N -reload_at_frame M',
    '  • Bin\\RelWithDebInfo\\RenderTest.exe -test <name>',
    '  • Bin\\RelWithDebInfo\\Main.exe -capture_frame N',
    '  • InteractiveTest.ps1',
    '  • npx playwright test tests/<spec>.spec.js',
    '',
    `Escape hatch: include ${SKIP_SENTINEL} if this commit legitimately`,
    'cannot be validated by a test (commit-message-only edit, hook fix, etc).',
    '',
  ].join('\n'))
  process.exit(2)
}

module.exports = { run }
