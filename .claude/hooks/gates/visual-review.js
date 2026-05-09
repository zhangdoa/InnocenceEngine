// Visual-review gate — when the commit message body references capture
// paths under `Build/captures/`, the reviewer's independent visual
// inspection must be recorded as a `Reviewed-Visually:` footer (or an
// explicit `Review-Skipped-Visual:` opt-out per the discipline).
//
// Trigger: any line in the commit message body matches the capture-path
// regex below. The trigger is structural — capture paths in the body
// signal that the CL is making (or relying on) visual claims, which is
// exactly when the reviewer is the only agent positioned to verify with
// a frame. Implementer prose about visuals is informational, not
// load-bearing; the gate forces the audit artifact to land.
//
// No escape sentinel. The `Review-Skipped-Visual:` footer IS the opt-out
// (mirrors peer-review's `Review-Skipped:` shape — surfaced in the
// message, not behind a `[skip-...]` token that hides the reason).
//
// Loud-failure shape mirrors peer-review.js: same transcript-independent
// phase, same single block emission with the discipline link, same
// fail-open-on-internal-error posture.
//
// Discipline: .claude/skills/peer-review-required/SKILL.md §
// "Reviewer visual inspection" + § "Commit-message line".

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
    '[commit-gate] git commit blocked — visual-review artifact missing.',
    '',
    'The commit body references `Build/captures/` paths, signalling a',
    'rendering CL that makes or relies on visual claims. Per',
    '.claude/skills/peer-review-required/SKILL.md § "Reviewer visual',
    'inspection", the reviewer must independently `Read` the candidate',
    'captures and the audit must land in the commit message via:',
    '',
    '  Reviewed-Visually: <reviewer-agent> — <improvement|regression|uncertain|per-scene-mixed>',
    '',
    'Implementer prose describing the captures is informational only —',
    'it cannot substitute for the reviewer\'s independent inspection.',
    '',
    'If the visual-inspection mandate is non-applicable for this CL',
    '(test-infra producing captures without claiming visual quality, pure',
    'refactor with toggle-off bit-identical proof, etc.), opt out',
    'explicitly:',
    '',
    '  Review-Skipped-Visual: <reason>',
    '',
    'Add the line to the commit message (inline with -m, or in the file',
    'passed to -F / -c) and retry.',
    '',
  ].join('\n'))
  process.exit(2)
}

module.exports = { run, needsTranscript: false, CAPTURE_PATH_RE, REVIEW_VISUAL_RE }
