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
 *   1. file-size        — universal soft ratchet on code/script files.
 *   2. paper-port       — closing a paper-port task needs a fresh alignment artifact.
 *   3. attribution      — commit message needs the AI-authorship header.
 *                         MUST live in this phase so transcript-fail-open
 *                         cannot bypass it (see CL fixing 55cf6a72 gap).
 *
 * Phase 2 (transcript-dependent; skipped if transcript unreadable):
 *   4. test-run         — staged code needs integration-test evidence.
 *   5. live-engine      — editor code needs a real-engine / Playwright run.
 *   6. serialize-test   — serializer code needs a serialize-determinism run.
 *
 * Each gate declares `needsTranscript: boolean` on its module exports;
 * the dispatcher partitions by that flag.
 */

const fs = require('fs')
const { execSync } = require('child_process')
const {
  collectCommitMessageText, detectClosingTasks,
  isRealUserPrompt,
} = require('./lib/common')

// Order matters: gates run top-to-bottom, first failure wins. The
// dispatcher partitions this list by `needsTranscript` at runtime so the
// transcript-independent gates run BEFORE the transcript fetch — that
// way a missing/unreadable transcript can never bypass attribution.
const GATES = [
  require('./gates/file-size'),       // needsTranscript: false
  require('./gates/paper-port'),      // needsTranscript: false
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
  const messageText = collectCommitMessageText(cmd, cwd)

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
  const xp = input.transcript_path
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
