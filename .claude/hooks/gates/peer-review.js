// peer-review: commit message must contain Reviewed-By: or Review-Skipped: footer.

const REVIEW_RE = /^(Reviewed-By|Review-Skipped):\s*\S/m

function run(ctx) {
  if (REVIEW_RE.test(ctx.messageText)) return { ok: true }
  return { ok: false, block: emit }
}

function emit() {
  process.stderr.write([
    '',
    '[commit-gate] git commit blocked — peer-review footer missing.',
    '',
    'Add one of:',
    '  Reviewed-By: <reviewer-agent>       (one or more)',
    '  Review-Skipped: <reason>            (backlog-only / harness-internal / mechanical-rename / bootstrap)',
    '',
  ].join('\n'))
  process.exit(2)
}

module.exports = { run, needsTranscript: false, REVIEW_RE }
