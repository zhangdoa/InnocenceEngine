#!/usr/bin/env node
// PreToolUse hook: session-scope gates that fire on every tool call.
// Fails open on internal error.

const GATES = [
  require('./gates/task-mgmt-brief'),
  require('./gates/skill-evidence'),
  require('./gates/agent-dispatch'),
  require('./gates/no-auto-memory'),
]

let raw = ''
process.stdin.setEncoding('utf8')
process.stdin.on('data', d => { raw += d })
process.stdin.on('end', () => main().catch(failOpen))

async function main() {
  let input
  try { input = JSON.parse(raw) } catch { return process.exit(0) }
  if (input.hook_event_name !== 'PreToolUse') return process.exit(0)

  for (const gate of GATES) {
    const result = gate.run(input)
    if (!result.ok) { result.block(); return }
  }
  process.exit(0)
}

function failOpen(err) {
  process.stderr.write(`[session-gate] internal error — failing open: ${err?.message || err}\n`)
  process.exit(0)
}
