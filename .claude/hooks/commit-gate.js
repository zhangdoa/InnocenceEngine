#!/usr/bin/env node
/**
 * PreToolUse hook: gate Bash `git commit`.
 *
 * Dispatcher only — each gate lives in `.claude/hooks/gates/<name>.js`.
 * Shared helpers in `.claude/hooks/lib/common.js`.
 *
 * Fails OPEN on any internal error so a hook bug never bricks commits.
 *
 * Gate order (first failure wins):
 *   1. file-size        — universal soft ratchet on code/script files.
 *   2. paper-port       — closing a paper-port task needs a fresh alignment artifact.
 *   3. test-run         — staged code needs integration-test evidence (closure claim too).
 *   4. live-engine      — editor code needs a real-engine / Playwright run.
 *   5. serialize-test   — serializer code needs a serialize-determinism run.
 *   6. attribution      — commit message needs the AI-authorship header.
 */

const fs = require('fs')
const { execSync } = require('child_process')
const {
  collectCommitMessageText, detectClosingTasks,
  isRealUserPrompt,
} = require('./lib/common')

const GATES = [
  require('./gates/file-size'),
  require('./gates/paper-port'),
  require('./gates/test-run'),
  require('./gates/live-engine'),
  require('./gates/serialize-test'),
  require('./gates/attribution'),
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

  // Transcript — parse once, share across gates.
  const xp = input.transcript_path
  if (!xp || !fs.existsSync(xp)) return failOpen(new Error('transcript not accessible'))
  const transcript = []
  for (const line of fs.readFileSync(xp, 'utf8').split('\n').filter(Boolean)) {
    try { transcript.push(JSON.parse(line)) } catch { /* skip */ }
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

  const closingTasks = detectClosingTasks(cwd, staged)

  const ctx = { cmd, cwd, messageText, staged, closingTasks, transcript, lastUserIdx }

  for (const gate of GATES) {
    const result = gate.run(ctx)
    if (!result.ok) { result.block(); return }
  }

  process.exit(0)
}

function failOpen(err) {
  process.stderr.write(`[commit-gate] internal error — failing open: ${err?.message || err}\n`)
  process.exit(0)
}
