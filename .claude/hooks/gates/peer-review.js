// Peer-review gate — the commit message must contain one of the
// peer-review artifact lines per .claude/disciplines/on-commit/peer-review-required.md.
//
//   Reviewed-By: <reviewer-agent>      (one or more — pass)
//   Review-Skipped: <reason>           (single skip — pass)
//
// Either form satisfies the gate. The artifact IS the audit trail —
// `git log` after the fact shows whether the CL was reviewed or
// explicitly skipped, mirroring the attribution gate's discipline.
//
// No escape sentinel. The skip path is `Review-Skipped:`, surfaced in
// the message itself; a separate `[skip-...]` token would dilute the
// artifact. The reasons enumerated in peer-review-required.md
// § "When required" are reviewer / dispatcher discipline; the gate
// enforces presence of *some* reason, not the truthfulness of one
// (a fake reason is a peer-review concern, not a hook concern).
//
// Loud-failure shape mirrors attribution.js: same transcript-independent
// phase, same single block emission with the discipline link, same
// no-bypass posture.

const REVIEW_RE = /^(Reviewed-By|Review-Skipped):\s*\S/m

function run(ctx) {
  if (REVIEW_RE.test(ctx.messageText)) return { ok: true }
  return { ok: false, block: emit }
}

function emit() {
  process.stderr.write([
    '',
    '[commit-gate] git commit blocked — peer-review artifact missing.',
    '',
    'Per .claude/disciplines/on-commit/peer-review-required.md, every commit must',
    'end with one of:',
    '  Reviewed-By: <reviewer-agent>       (one or more)',
    '  Review-Skipped: <reason>            (per "When required" categories)',
    '',
    'Skip categories: backlog-only, hook-internal, mechanical-rename,',
    'bootstrap. The dispatcher must be able to articulate why review was',
    'skipped — the gate does not validate the reason, but a fresh peer',
    'reviewer of THIS CL would.',
    '',
    'Add the line to the commit message (inline with -m, or in the file',
    'passed to -F / -c) and retry.',
    '',
  ].join('\n'))
  process.exit(2)
}

module.exports = { run, needsTranscript: false, REVIEW_RE }
