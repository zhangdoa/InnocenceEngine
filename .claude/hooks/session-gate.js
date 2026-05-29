#!/usr/bin/env node
// PreToolUse hook: session-scope gates that fire on every tool call.
// Fails open on internal error.

const GATE_NAMES = ['task-mgmt-brief', 'skill-evidence', 'agent-dispatch', 'no-auto-memory']
const GATES = GATE_NAMES.map(n => require(`./gates/${n}`))

let raw = ''
process.stdin.setEncoding('utf8')
process.stdin.on('data', d => { raw += d })
process.stdin.on('end', () => main().catch(failOpen))

async function main() {
  let input
  try { input = JSON.parse(raw) } catch { return process.exit(0) }
  if (input.hook_event_name !== 'PreToolUse') return process.exit(0)

  for (let i = 0; i < GATES.length; i++) {
    let result
    try { result = GATES[i].run(input) }
    catch (err) { gateThrew('session-gate', GATE_NAMES[i], err); continue }
    if (!result.ok) { result.block(); return }
  }
  process.exit(0)
}

function gateThrew(dispatcher, name, err) {
  process.stderr.write(`[${dispatcher}] gate ${name} threw — skipping that gate only: ${err?.message || err}\n`)
}

function failOpen(err) {
  process.stderr.write(`[session-gate] internal error — failing open: ${err?.message || err}\n`)
  process.exit(0)
}
