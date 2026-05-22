// visual-review: when commit body references Build/captures/, require Reviewed-Visually:
// or Review-Skipped-Visual: footer.

const CAPTURE_PATH_RE = /Build\/captures\//
const REVIEW_VISUAL_RE = /^(Reviewed-Visually|Review-Skipped-Visual):\s*\S/m

function run(ctx) {
  if (!CAPTURE_PATH_RE.test(ctx.messageText)) return { ok: true }
  if (REVIEW_VISUAL_RE.test(ctx.messageText)) return { ok: true }
  return { ok: false, block: emit }
}

function emit() {
  process.stderr.write([
    '',
    '[commit-gate] git commit blocked — visual-review footer missing.',
    '',
    'Commit body references Build/captures/. Add one of:',
    '  Reviewed-Visually: <reviewer-agent> — <improvement|regression|uncertain|per-scene-mixed>',
    '  Review-Skipped-Visual: <reason>',
    '',
  ].join('\n'))
  process.exit(2)
}

module.exports = { run, needsTranscript: false, CAPTURE_PATH_RE, REVIEW_VISUAL_RE }
