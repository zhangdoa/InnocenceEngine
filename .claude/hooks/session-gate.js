#!/usr/bin/env node
/**
 * PreToolUse hook: gate substantive tool use until the producer subagent
 * has been invoked at least once in the current session.
 *
 * Dispatcher only — gate logic lives in `.claude/hooks/gates/producer-brief.js`.
 * That gate reads `input.transcript_path` directly (mirroring the precedent
 * at `commit-gate.js:62-78`) — there is no per-session state file.
 *
 * Fails OPEN on any internal error so a hook bug never bricks a session.
 */

const producerBriefGate = require('./gates/producer-brief')

let raw = ''
process.stdin.setEncoding('utf8')
process.stdin.on('data', d => { raw += d })
process.stdin.on('end', () => main().catch(failOpen))

async function main() {
  let input
  try { input = JSON.parse(raw) } catch { return process.exit(0) }
  if (input.hook_event_name !== 'PreToolUse') return process.exit(0)

  const result = producerBriefGate.run(input)
  if (!result.ok) { result.block(); return }
  process.exit(0)
}

function failOpen(err) {
  process.stderr.write(`[session-gate] internal error — failing open: ${err?.message || err}\n`)
  process.exit(0)
}
