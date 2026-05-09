#!/usr/bin/env node
/**
 * PreToolUse hook: session-scope gates that fire on every tool call.
 *
 * Dispatcher only — each gate lives in `.claude/hooks/gates/<name>.js`.
 *
 * Gate order (first failure wins):
 *   1. task-mgmt-brief    — blocks substantive tool use until task-mgmt
 *                              subagent has been invoked at least once this
 *                              session. CLAUDE.md "Session start" rule.
 *   2. agent-dispatch        — blocks `Agent` calls dispatched foreground
 *                              without `[foreground-required]` in the prompt.
 *                              Enforces .claude/skills/agent-dispatch/SKILL.md.
 *   3. cross-subtree-stash   — blocks `git stash` Bash calls that would
 *                              sweep dirty files spanning multiple agent-
 *                              owned subtrees (TASK-196 / 2026-04-28
 *                              cross-agent collision incident).
 *   4. no-auto-memory        — blocks Write/Edit/MultiEdit/NotebookEdit
 *                              targeting the Claude default auto-memory
 *                              directory. The block message routes to the
 *                              new venues so a future Claude does not stall.
 *
 * Each gate exports `run(input)` returning `{ ok: true }` or
 * `{ ok: false, block: () => never-returns }`. The `block` callback
 * writes its own message to stderr and `process.exit(2)`s.
 *
 * Fails OPEN on any internal error so a hook bug never bricks a session.
 */

const GATES = [
  require('./gates/task-mgmt-brief'),
  require('./gates/agent-dispatch'),
  require('./gates/cross-subtree-stash'),
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
