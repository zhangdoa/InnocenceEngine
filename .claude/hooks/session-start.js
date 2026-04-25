#!/usr/bin/env node
/**
 * SessionStart hook: inject a directive instructing main-session Claude
 * that the first action this session must be the producer briefing,
 * regardless of what the user typed.
 *
 * Why SessionStart and not PreToolUse: PreToolUse fires *after* the model
 * has already chosen what to do this turn. A purely-textual reply or a
 * Read-only investigation never reaches PreToolUse, so the
 * `producer-brief` PreToolUse gate (commit d39a2a0c) only catches drift
 * once Claude reaches for Bash/Edit/Write. SessionStart fires before any
 * model turn and lets us seed `additionalContext` that the model sees on
 * its very first response — the earliest event surface that can still
 * observe the violation.
 *
 * Dispatcher only — gate logic lives in
 * `.claude/hooks/gates/session-start-brief.js`. Mirrors the dispatcher
 * style of `session-gate.js` and `commit-gate.js`.
 *
 * Fails OPEN on any internal error so a hook bug never bricks a session.
 */

const sessionStartBriefGate = require('./gates/session-start-brief')

let raw = ''
process.stdin.setEncoding('utf8')
process.stdin.on('data', d => { raw += d })
process.stdin.on('end', () => main().catch(failOpen))

async function main() {
  let input
  try { input = JSON.parse(raw) } catch { return process.exit(0) }
  if (input.hook_event_name !== 'SessionStart') return process.exit(0)

  const result = sessionStartBriefGate.run(input)
  if (result.stdout) process.stdout.write(result.stdout)
  process.exit(0)
}

function failOpen(err) {
  process.stderr.write(`[session-start] internal error — failing open: ${err?.message || err}\n`)
  process.exit(0)
}
