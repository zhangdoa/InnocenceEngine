// serialize-test: JSONWrapper / AssetService / SceneService staged → require
// Main.exe -serialize_test ... run this turn. Path-derived: only fires when
// SERIALIZER_CODE_PATH matches a staged file.

const {
  SERIALIZER_CODE_PATH, DOCS_ONLY_PATH, firstArray,
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
  const serializerStaged = ctx.staged.some(
    f => SERIALIZER_CODE_PATH.test(f) && !DOCS_ONLY_PATH.test(f))
  if (!serializerStaged) return { ok: true }
  if (didSerializeTestRun(ctx.transcript, ctx.lastUserIdx)) return { ok: true }
  return { ok: false, block: () => emit(ctx.staged) }
}

function emit(staged) {
  const filesList = staged.length
    ? staged.filter(f => SERIALIZER_CODE_PATH.test(f) && !DOCS_ONLY_PATH.test(f)).slice(0, 10).map(f => '  ' + f).join('\n')
    : '  (no serializer code detected — bug?)'
  process.stderr.write([
    '',
    '[commit-gate] git commit blocked — serializer code staged but no serialize-test ran.',
    '',
    'Serializer staged paths:',
    filesList,
    '',
    'Run: Main.exe -mode 0 -renderer 0 -loglevel 0 -offscreen -serialize_test ExampleProject/Scenes/UnitTest.InnoScene',
    '',
  ].join('\n'))
  process.exit(2)
}

module.exports = { run, needsTranscript: true }
