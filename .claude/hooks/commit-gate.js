#!/usr/bin/env node
/**
 * PreToolUse hook: gate Bash `git commit`.
 *
 * Dispatcher only — each gate lives in `.claude/hooks/gates/<name>.js`.
 * Shared helpers in `.claude/hooks/lib/common.js`.
 *
 * Fails OPEN on any internal error so a hook bug never bricks commits.
 * Critically, the attribution gate is intentionally placed in the
 * transcript-INDEPENDENT phase so a transcript I/O failure cannot bypass
 * it. Attribution is the only invariant the user has declared
 * non-negotiable; everything else is allowed to fail open.
 *
 * Gate order (first failure wins) — split by transcript dependence:
 *
 * Phase 1 (no transcript needed; runs even if transcript I/O fails):
 *   1. data-generated   — never track files under `Data/Generated/`, never
 *                         loosen the gitignore mask that protects it.
 *                         Placed first: a structural "must never happen"
 *                         rule that no later evidence can excuse.
 *   2. file-size        — universal soft ratchet on code/script files.
 *   3. paper-port       — closing a paper-port task needs a fresh alignment artifact.
 *   4. closure-staleness— code-bearing commits citing TASK-N must flip
 *                         the task in the same CL or use the
 *                         [task-stays-open] sentinel. Symmetric to
 *                         closure-evidence (test-run.js) on the upstream
 *                         side: catches "wrote code, didn't close" before
 *                         the same gap surfaces as "claimed Done, no test."
 *   5. peer-review      — commit message needs Reviewed-By: or
 *                         Review-Skipped: <reason>. Placed before
 *                         attribution because it is the richer claim:
 *                         if both are missing the user gets the more
 *                         informative error first.
 *   6. visual-review    — when the body references Build/captures/, the
 *                         reviewer's visual-inspection footer
 *                         (Reviewed-Visually: / Review-Skipped-Visual:)
 *                         must be present. Placed after peer-review
 *                         because it is the narrower trigger; if both
 *                         are missing on a rendering CL the user sees
 *                         the broader peer-review error first.
 *   7. attribution      — commit message needs the AI-authorship header.
 *                         MUST live in this phase so transcript-fail-open
 *                         cannot bypass it (see CL fixing 55cf6a72 gap).
 *
 * Phase 2 (transcript-dependent; skipped if transcript unreadable):
 *   7. test-run         — staged code needs integration-test evidence.
 *   8. live-engine      — editor code needs a real-engine / Playwright run.
 *   9. serialize-test   — serializer code needs a serialize-determinism run.
 *
 * Each gate declares `needsTranscript: boolean` on its module exports;
 * the dispatcher partitions by that flag.
 */

const fs = require('fs')
const { execSync } = require('child_process')
const {
  collectCommitMessageText, detectClosingTasks,
  isRealUserPrompt, resolveActiveTranscriptPath,
} = require('./lib/common')

// Order matters: gates run top-to-bottom, first failure wins. The
// dispatcher partitions this list by `needsTranscript` at runtime so the
// transcript-independent gates run BEFORE the transcript fetch — that
// way a missing/unreadable transcript can never bypass attribution.
const GATES = [
  require('./gates/data-generated'),  // needsTranscript: false (structural; first)
  require('./gates/no-images'),       // needsTranscript: false (structural; near-first)
  require('./gates/file-size'),       // needsTranscript: false
  require('./gates/closure-staleness'),// needsTranscript: false (symmetric to closure-evidence)
  require('./gates/peer-review'),     // needsTranscript: false (before attribution — richer claim first)
  require('./gates/visual-review'),   // needsTranscript: false (after peer-review — narrower trigger)
  require('./gates/test-run'),        // needsTranscript: true
  require('./gates/live-engine'),     // needsTranscript: true
  require('./gates/serialize-test'),  // needsTranscript: true
  require('./gates/attribution'),     // needsTranscript: false (last in phase 1)
]

let raw = ''
process.stdin.setEncoding('utf8')
process.stdin.on('data', d => { raw += d })
process.stdin.on('end', () => main().catch(failOpen))

async function main() {
  let input
  try { input = JSON.parse(raw) } catch { return process.exit(0) }
  if (input.tool_name !== 'Bash') return process.exit(0)
  const cmd = input.tool_input?.command || ''
  // Strip quoted literals so `echo "git commit"` inside a test script
  // doesn't false-trigger; real commits have the phrase outside quotes.
  const unquoted = cmd
    .replace(/"(?:\\.|[^"\\])*"/g, '""')
    .replace(/'(?:[^'])*'/g, "''")
  if (!/\bgit\s+commit\b/.test(unquoted)) return process.exit(0)

  const cwd = input.cwd || process.cwd()
  const { text: messageText, fileError } = collectCommitMessageText(cmd, cwd)
  if (fileError) { blockUnreadableMessageFile(fileError); return }

  // Staged set.
  let stagedRaw = ''
  try {
    stagedRaw = execSync('git -c core.quotePath=false diff --cached --name-only', { cwd, encoding: 'utf8' })
  } catch { /* no git */ }
  const staged = stagedRaw.split('\n').map(s => s.trim()).filter(Boolean)

  const closingTasks = detectClosingTasks(cwd, staged)

  // Phase 1: transcript-independent gates. Run BEFORE the transcript
  // fetch so an unreadable transcript can't bypass them. Attribution
  // sits at the tail of this phase — that's the load-bearing placement
  // (see header comment / CL fixing 55cf6a72 gap).
  const noTranscriptCtx = { cmd, cwd, messageText, staged, closingTasks }
  for (const gate of GATES) {
    if (gate.needsTranscript) continue
    const result = gate.run(noTranscriptCtx)
    if (!result.ok) { result.block(); return }
  }

  // Phase 2: transcript-dependent gates. If the transcript is missing or
  // unreadable, fail open ONLY for this phase — phase 1 already ran.
  //
  // Sidechain swap: when the in-flight `git commit` originates from a
  // sub-agent, Claude Code passes the parent session's transcript path
  // here. Scanning the parent finds none of the sub-agent's evidence.
  // `resolveActiveTranscriptPath` matches the in-flight command against
  // every sub-agent JSONL's last assistant Bash tool_use; on a unique
  // match it returns that JSONL, so the gate scans the transcript that
  // actually contains the work. Falls back to the parent on any miss.
  const xp = resolveActiveTranscriptPath(input.transcript_path, cmd)
  if (!xp || !fs.existsSync(xp)) return failOpen(new Error('transcript not accessible (phase 2 skipped; phase 1 already passed)'))
  let transcript
  try {
    transcript = []
    for (const line of fs.readFileSync(xp, 'utf8').split('\n').filter(Boolean)) {
      try { transcript.push(JSON.parse(line)) } catch { /* skip malformed */ }
    }
  } catch (err) {
    return failOpen(new Error(`transcript unreadable (phase 2 skipped): ${err.message}`))
  }
  // Find last real user prompt — bounds the gate's transcript-scan window.
  let lastUserIdx = -1
  for (let i = transcript.length - 1; i >= 0; i--) {
    const m = transcript[i]
    const t = m.type || m.role || m.message?.role
    if (t !== 'user') continue
    if (!isRealUserPrompt(m.message?.content ?? m.content)) continue
    lastUserIdx = i
    break
  }

  const ctx = { cmd, cwd, messageText, staged, closingTasks, transcript, lastUserIdx }
  for (const gate of GATES) {
    if (!gate.needsTranscript) continue
    const result = gate.run(ctx)
    if (!result.ok) { result.block(); return }
  }

  process.exit(0)
}

function failOpen(err) {
  process.stderr.write(`[commit-gate] internal error — failing open: ${err?.message || err}\n`)
  process.exit(0)
}

// Loud failure when -F / --file / -c / --template points at a path the
// gate could not read in any form (native or MSYS-translated). Silently
// falling through to an empty message would hide attribution and
// content-based gates' true verdict — feedback_silent_failures.md.
function blockUnreadableMessageFile(fileError) {
  const lines = [
    '',
    '[commit-gate] git commit blocked — message file unreadable.',
    '',
    `Argument: ${fileError.rawPath}`,
    'Tried:',
  ]
  for (const a of fileError.attempts) lines.push(`  - ${a.path}  (${a.code})`)
  lines.push(
    '',
    'If you are running from Git Bash with an MSYS-style absolute path',
    '(e.g. /c/GitRepo/...), the gate translates `/<letter>/...` to',
    '`<letter>:/...` automatically. The translated form also failed,',
    'which means the file genuinely does not exist or is unreadable.',
    '',
    'Fix: re-issue the commit with a project-relative path',
    '(e.g. `git commit -F Build/commit-message.txt`) or a native Windows',
    'absolute path the Node fs API accepts.',
    '',
  )
  process.stderr.write(lines.join('\n'))
  process.exit(2)
}
