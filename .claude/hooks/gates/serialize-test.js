// Serialize-test gate — when JSONWrapper / AssetService / SceneService
// is staged, a `Main.exe -serialize_test ...` run must have happened
// this turn. Catches round-trip regressions that only surface on a
// real scene save-then-compare. SKIP_SENTINEL escapes.

const {
  SERIALIZER_CODE_PATH, SKIP_SENTINEL, firstArray,
} = require('../lib/common')

const SERIALIZE_TEST_RE = /Main\.exe\b[^|&;]*-serialize_test\b/

function didSerializeTestRun(transcript, lastUserIdx) {
  for (let i = lastUserIdx + 1; i < transcript.length; i++) {
    const m = transcript[i]
    const blocks = firstArray(m.message?.content, m.content)
    for (const b of blocks) {
      if (b?.type === 'tool_use' && b?.name === 'Bash') {
        if (SERIALIZE_TEST_RE.test(b.input?.command || '')) return true
      }
    }
  }
  return false
}

function run(ctx) {
  if (ctx.messageText.includes(SKIP_SENTINEL)) return { ok: true }
  const serializerStaged = ctx.staged.some(f => SERIALIZER_CODE_PATH.test(f))
  if (!serializerStaged) return { ok: true }
  if (didSerializeTestRun(ctx.transcript, ctx.lastUserIdx)) return { ok: true }
  return { ok: false, block: () => emit(ctx.staged) }
}

function emit(staged) {
  const filesList = staged.length
    ? staged.filter(f => SERIALIZER_CODE_PATH.test(f)).slice(0, 10).map(f => '  ' + f).join('\n')
    : '  (no serializer code detected — bug?)'
  process.stderr.write([
    '',
    '[commit-gate] git commit blocked — serializer code staged but no serialize-test ran.',
    '',
    'Serializer-facing staged paths:',
    filesList,
    '',
    'Run the serialize-determinism test in this turn before committing:',
    '  Main.exe -mode 0 -renderer 0 -loglevel 0 -offscreen -serialize_test ExampleProject/Scenes/UnitTest.InnoScene',
    '',
    `Escape hatch: include ${SKIP_SENTINEL} if this change genuinely cannot`,
    'be validated by a serialize-test (e.g. a rename with no logic change).',
    '',
  ].join('\n'))
  process.exit(2)
}

module.exports = { run, needsTranscript: true }
