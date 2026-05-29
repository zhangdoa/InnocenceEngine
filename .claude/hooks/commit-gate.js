#!/usr/bin/env node
// PreToolUse hook: gate Bash `git commit`. Dispatcher only — per-gate logic in gates/.
// Fails open on internal error. Attribution placed in transcript-independent phase so
// transcript I/O failure cannot bypass it.

const fs = require('fs')
const { execSync } = require('child_process')
const {
  collectCommitMessageText, detectClosingTasks,
  isRealUserPrompt, resolveActiveTranscriptPath,
} = require('./lib/common')

const GATE_NAMES = [
  'data-generated', 'no-images', 'no-new-md', 'commit-body-cap', 'comment-essay-cap',
  'file-size', 'closure-staleness', 'peer-review', 'visual-review',
  'test-run', 'live-engine', 'serialize-test',   // needsTranscript: true
  'attribution',
]
const GATES = GATE_NAMES.map(n => require(`./gates/${n}`))

let raw = ''
process.stdin.setEncoding('utf8')
process.stdin.on('data', d => { raw += d })
process.stdin.on('end', () => main().catch(failOpen))

async function main() {
  let input
  try { input = JSON.parse(raw) } catch { return process.exit(0) }
  if (input.tool_name !== 'Bash') return process.exit(0)
  const cmd = input.tool_input?.command || ''
  const unquoted = cmd
    .replace(/"(?:\\.|[^"\\])*"/g, '""')
    .replace(/'(?:[^'])*'/g, "''")
  if (!/\bgit\s+commit\b/.test(unquoted)) return process.exit(0)

  const cwd = input.cwd || process.cwd()
  const { text: messageText, fileError } = collectCommitMessageText(cmd, cwd)
  if (fileError) { blockUnreadableMessageFile(fileError); return }

  let stagedRaw = ''
  try {
    stagedRaw = execSync('git -c core.quotePath=false diff --cached --name-only', { cwd, encoding: 'utf8' })
  } catch { /* no git */ }
  const staged = stagedRaw.split('\n').map(s => s.trim()).filter(Boolean)

  const closingTasks = detectClosingTasks(cwd, staged)

  const noTranscriptCtx = { cmd, cwd, messageText, staged, closingTasks }
  for (let i = 0; i < GATES.length; i++) {
    if (GATES[i].needsTranscript) continue
    let result
    try { result = GATES[i].run(noTranscriptCtx) }
    catch (err) { gateThrew(GATE_NAMES[i], err); continue }
    if (!result.ok) { result.block(); return }
  }

  const xp = resolveActiveTranscriptPath(input.transcript_path, cmd)
  if (!xp || !fs.existsSync(xp)) return failOpen(new Error('transcript not accessible (phase 2 skipped)'))
  let transcript
  try {
    transcript = []
    for (const line of fs.readFileSync(xp, 'utf8').split('\n').filter(Boolean)) {
      try { transcript.push(JSON.parse(line)) } catch { /* skip malformed */ }
    }
  } catch (err) {
    return failOpen(new Error(`transcript unreadable (phase 2 skipped): ${err.message}`))
  }
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
  for (let i = 0; i < GATES.length; i++) {
    if (!GATES[i].needsTranscript) continue
    let result
    try { result = GATES[i].run(ctx) }
    catch (err) { gateThrew(GATE_NAMES[i], err); continue }
    if (!result.ok) { result.block(); return }
  }

  process.exit(0)
}

function gateThrew(name, err) {
  process.stderr.write(`[commit-gate] gate ${name} threw — skipping that gate only: ${err?.message || err}\n`)
}

function failOpen(err) {
  process.stderr.write(`[commit-gate] internal error — failing open: ${err?.message || err}\n`)
  process.exit(0)
}

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
    'MSYS path /<letter>/... is auto-translated to <letter>:/... — both forms failed.',
    'Fix: project-relative path (e.g. -F Build/commit-message.txt) or native Windows absolute.',
    '',
  )
  process.stderr.write(lines.join('\n'))
  process.exit(2)
}
