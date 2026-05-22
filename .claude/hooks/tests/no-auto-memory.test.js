#!/usr/bin/env node

const path = require('path')
const gate = require('../gates/no-auto-memory')

let passed = 0
let failed = 0

function assert(cond, label) {
  if (cond) { console.log(`  PASS  ${label}`); passed++ }
  else      { console.log(`  FAIL  ${label}`); failed++ }
}

function group(name, fn) {
  console.log(`\n[${name}]`)
  fn()
}

function runGate(input) {
  const r = gate.run(input)
  if (r.ok) return { blocked: false, stderr: '' }
  const realExit = process.exit
  const realWrite = process.stderr.write.bind(process.stderr)
  let captured = ''
  let exitCode = null
  process.stderr.write = (s) => { captured += s; return true }
  process.exit = (c) => { exitCode = c; throw new Error('__gate_exit__') }
  try { r.block() } catch (e) { if (e.message !== '__gate_exit__') throw e }
  finally {
    process.stderr.write = realWrite
    process.exit = realExit
  }
  return { blocked: true, stderr: captured, exitCode }
}

group('autoMemoryDir — slug rewrites every separator one-for-one', () => {
  const out = gate.autoMemoryDir('C:\\GitRepo\\InnocenceEngine')
  assert(typeof out === 'string', 'returns a string')
  assert(out.includes('c--gitrepo-innocenceengine'),
    'slug preserves double-dash from `:\\`')
  assert(out.endsWith('/memory'), 'path ends in /memory')
})

group('autoMemoryDir — POSIX cwd', () => {
  const out = gate.autoMemoryDir('/home/user/proj')
  assert(typeof out === 'string', 'returns a string')
  assert(out.includes('-home-user-proj'), 'every separator rewritten')
})

group('gate.run — Write inside memory dir blocks', () => {
  const r = runGate({
    tool_name: 'Write',
    tool_input: {
      file_path: 'C:\\Users\\zhangdoa\\.claude\\projects\\C--GitRepo-InnocenceEngine\\memory\\foo.md',
      content: 'x',
    },
    cwd: 'C:\\GitRepo\\InnocenceEngine',
  })
  assert(r.blocked, 'Write target inside memory dir blocked')
  assert(r.exitCode === 2, 'exit code 2')
})

group('gate.run — Edit inside memory dir blocks', () => {
  const r = runGate({
    tool_name: 'Edit',
    tool_input: {
      file_path: 'C:\\Users\\zhangdoa\\.claude\\projects\\C--GitRepo-InnocenceEngine\\memory\\MEMORY.md',
      old_string: 'a',
      new_string: 'b',
    },
    cwd: 'C:\\GitRepo\\InnocenceEngine',
  })
  assert(r.blocked, 'Edit target inside memory dir blocked')
})

group('gate.run — MultiEdit inside memory dir blocks', () => {
  const r = runGate({
    tool_name: 'MultiEdit',
    tool_input: {
      file_path: 'C:\\Users\\zhangdoa\\.claude\\projects\\C--GitRepo-InnocenceEngine\\memory\\foo.md',
      edits: [],
    },
    cwd: 'C:\\GitRepo\\InnocenceEngine',
  })
  assert(r.blocked, 'MultiEdit target inside memory dir blocked')
})

group('gate.run — NotebookEdit inside memory dir blocks', () => {
  const r = runGate({
    tool_name: 'NotebookEdit',
    tool_input: {
      notebook_path: 'C:\\Users\\zhangdoa\\.claude\\projects\\C--GitRepo-InnocenceEngine\\memory\\nb.ipynb',
      cell_id: 'x',
    },
    cwd: 'C:\\GitRepo\\InnocenceEngine',
  })
  assert(r.blocked, 'NotebookEdit target inside memory dir blocked')
})

group('gate.run — Write OUTSIDE memory dir allowed', () => {
  const r = runGate({
    tool_name: 'Write',
    tool_input: {
      file_path: 'C:\\GitRepo\\InnocenceEngine\\.claude\\state\\project-direction.md',
      content: 'x',
    },
    cwd: 'C:\\GitRepo\\InnocenceEngine',
  })
  assert(!r.blocked, 'Write outside memory dir passes through')
})

group('gate.run — Read of memory file allowed (not a write)', () => {
  const r = runGate({
    tool_name: 'Read',
    tool_input: {
      file_path: 'C:\\Users\\zhangdoa\\.claude\\projects\\C--GitRepo-InnocenceEngine\\memory\\foo.md',
    },
    cwd: 'C:\\GitRepo\\InnocenceEngine',
  })
  assert(!r.blocked, 'Read tool ignored (gate scope is writes)')
})

group('gate.run — Bash command allowed (out of scope)', () => {
  const r = runGate({
    tool_name: 'Bash',
    tool_input: { command: 'echo hi > /tmp/foo' },
    cwd: 'C:\\GitRepo\\InnocenceEngine',
  })
  assert(!r.blocked, 'Bash not gated by no-auto-memory')
})

group('gate.run — missing tool_input handled gracefully', () => {
  const r = runGate({
    tool_name: 'Write',
    tool_input: undefined,
    cwd: 'C:\\GitRepo\\InnocenceEngine',
  })
  assert(!r.blocked, 'no file_path -> pass through (fail open on shape)')
})

group('block message — routes to each new venue', () => {
  const r = runGate({
    tool_name: 'Write',
    tool_input: {
      file_path: 'C:\\Users\\zhangdoa\\.claude\\projects\\C--GitRepo-InnocenceEngine\\memory\\foo.md',
      content: 'x',
    },
    cwd: 'C:\\GitRepo\\InnocenceEngine',
  })
  assert(r.blocked, 'blocked')
  assert(r.stderr.includes('skills/dispatch-briefs/SKILL.md'),
    'mentions dispatcher skill venue')
  assert(r.stderr.includes('skills/<topic>/SKILL.md'),
    'mentions universal skill venue')
  assert(r.stderr.includes('agents/<role>.md'),
    'mentions agent-manifest venue')
  assert(r.stderr.includes('state/<topic>.md'),
    'mentions project-state venue')
  assert(r.stderr.includes('Implementation Notes'),
    'mentions backlog Implementation Notes venue')
  assert(r.stderr.includes('hooks/gates/'),
    'mentions hook-gate venue')
  assert(r.stderr.includes('persistence-venue/SKILL.md'),
    'pointer to full skill')
  assert(r.stderr.toLowerCase().includes('disabled'),
    'states the rule plainly (auto-memory is disabled)')
})

console.log(`\n${passed} passed, ${failed} failed`)
process.exit(failed === 0 ? 0 : 1)
