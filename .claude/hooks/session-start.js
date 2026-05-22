#!/usr/bin/env node
// SessionStart hook: dispatcher. Gate logic in gates/session-start-brief.js.
// Fails open on internal error.

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
