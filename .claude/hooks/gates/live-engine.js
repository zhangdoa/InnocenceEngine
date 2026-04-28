// Live-engine gate — when editor-facing code is staged, require a
// Playwright spec that spawns the real engine, or a Main.exe /
// RenderTest / InteractiveTest run. Mock-only Playwright specs hide
// optimistic-vs-server-truth races. SKIP_SENTINEL escapes.

const path = require('path')
const fs = require('fs')
const {
  EDITOR_CODE_PATH, NON_PLAYWRIGHT_LIVE, PLAYWRIGHT_RE,
  SKIP_SENTINEL, firstArray,
} = require('../lib/common')

function didLiveEngineTestRun(transcript, lastUserIdx, cwd) {
  const editorDir = path.join(cwd, 'Source', 'Editor-Next')
  for (let i = lastUserIdx + 1; i < transcript.length; i++) {
    const m = transcript[i]
    const blocks = firstArray(m.message?.content, m.content)
    for (const b of blocks) {
      if (b?.type !== 'tool_use' || b?.name !== 'Bash') continue
      const c = b.input?.command || ''
      if (NON_PLAYWRIGHT_LIVE.test(c)) return true
      const pw = c.match(PLAYWRIGHT_RE)
      if (!pw) continue
      const args = (pw[1] || '').trim()
      if (!args) return true
      const files = args.split(/\s+/).filter(s => s && !s.startsWith('-'))
      if (files.length === 0) return true
      for (const f of files) {
        const abs = path.isAbsolute(f) ? f : path.join(editorDir, f)
        try {
          if (fs.readFileSync(abs, 'utf8').includes('--engine=Main')) return true
        } catch { /* unreadable — skip */ }
      }
    }
  }
  return false
}

function run(ctx) {
  if (ctx.messageText.includes(SKIP_SENTINEL)) return { ok: true }
  const editorCodeStaged = ctx.staged.some(f => EDITOR_CODE_PATH.test(f))
  if (!editorCodeStaged) return { ok: true }
  if (didLiveEngineTestRun(ctx.transcript, ctx.lastUserIdx, ctx.cwd)) return { ok: true }
  return { ok: false, block: () => emit(ctx.staged) }
}

function emit(staged) {
  const filesList = staged.length
    ? staged.filter(f => EDITOR_CODE_PATH.test(f)).slice(0, 10).map(f => '  ' + f).join('\n')
    : '  (no editor code detected — bug?)'
  process.stderr.write([
    '',
    '[commit-gate] git commit blocked — editor code staged but no live-engine test ran.',
    '',
    'Editor-facing staged paths:',
    filesList,
    '',
    'Mock-only Playwright specs (scene-vertical, inspector-rotation, theme-reactivity,',
    'ipc-contract, ux-audit) hide optimistic-vs-server-truth races. Run at least one of:',
    '  • npx playwright test                                   (full suite — includes live)',
    '  • npx playwright test tests/render-toggles.spec.js      (live engine)',
    '  • npx playwright test tests/render-target-debugger.spec.js',
    '  • npx playwright test tests/scene-load.spec.js',
    '  • npx playwright test tests/editor.spec.js',
    '  • npx playwright test tests/window-menu.spec.js',
    '  • Bin\\RelWithDebInfo\\Main.exe -total_frames N          (engine frame-run; see disciplines/perf-measurement-frame-budget.md for choosing N)',
    '  • Bin\\RelWithDebInfo\\RenderTest.exe -test <name>',
    '',
    `Escape hatch: include ${SKIP_SENTINEL} if this commit genuinely cannot`,
    'be validated end-to-end (e.g. a typo fix in a comment).',
    '',
  ].join('\n'))
  process.exit(2)
}

module.exports = { run, needsTranscript: true }
