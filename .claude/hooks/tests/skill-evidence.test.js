#!/usr/bin/env node
// Standalone tests for the skill-evidence gate (TASK-187).
//
// Runner: `node .claude/hooks/tests/skill-evidence.test.js`. Zero deps —
// same shape as no-auto-memory.test.js. Each test prints PASS/FAIL
// and the process exits non-zero on any failure.
//
// Layer 1 (parseAlwaysApplySkills round-trip) lives in
// skill-evidence-parse.test.js — auto-run here via require() so this
// remains the canonical entry point. Split was forced by the 300-line
// file-size gate after the task-mgmt + ci-build-impl extension.
//
// Layers covered here:
//   2. scanTranscriptForSkillUses — synthetic JSONL with Skill tool_uses.
//   3. gate.run — passive tools pass; non-existent / unresolvable
//      transcripts fail open; the load-bearing block path is exercised
//      by a synthesized parent + sub-agent transcript pair.

const fs = require('fs')
const os = require('os')
const path = require('path')

const {
  scanTranscriptForSkillUses,
  resolveActiveSubagentTranscript,
  mapAgentIdToSubagentType,
} = require('../lib/subagent-transcript')
const gate = require('../gates/skill-evidence')
const parseTests = require('./skill-evidence-parse.test')

let passed = 0
let failed = 0

function assert(cond, label) {
  if (cond) { console.log(`  PASS  ${label}`); passed++ }
  else      { console.log(`  FAIL  ${label}`); failed++ }
}
function group(name, fn) { console.log(`\n[${name}]`); fn() }

// Capture process.exit + stderr.write while the gate's block() runs.
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

const REPO_ROOT = path.resolve(__dirname, '..', '..', '..')

// Layer 1 tests run via the sibling file's require() side-effect above.

// ---------------------------------------------------------------------
// Layer 2: scanTranscriptForSkillUses
// ---------------------------------------------------------------------
group('scanTranscriptForSkillUses — finds Skill tool_uses', () => {
  const tmp = fs.mkdtempSync(path.join(os.tmpdir(), 'skill-evidence-'))
  const xp = path.join(tmp, 't.jsonl')
  const rows = [
    { type: 'user', message: { role: 'user', content: 'hi' } },
    { type: 'assistant', message: { role: 'assistant', content: [
      { type: 'tool_use', id: 'a', name: 'Skill', input: { skill: 'fundamentals' } },
    ] } },
    { type: 'assistant', message: { role: 'assistant', content: [
      { type: 'tool_use', id: 'b', name: 'Skill', input: { skill: 'comment-discipline' } },
      { type: 'tool_use', id: 'c', name: 'Read', input: { file_path: '/x' } },
    ] } },
  ]
  fs.writeFileSync(xp, rows.map(r => JSON.stringify(r)).join('\n'), 'utf8')
  const s = scanTranscriptForSkillUses(xp)
  assert(s instanceof Set, 'returns Set')
  assert(s.has('fundamentals') && s.has('comment-discipline'), 'collects both Skill names')
  assert(!s.has('Read'), 'ignores non-Skill tools')
})

group('scanTranscriptForSkillUses — missing file returns null', () => {
  const s = scanTranscriptForSkillUses('/nonexistent/no.jsonl')
  assert(s === null, 'fail-open trigger')
})

// ---------------------------------------------------------------------
// Layer 3: gate.run — passive + fail-open
// ---------------------------------------------------------------------
group('gate.run — passive tools always pass', () => {
  for (const t of ['Read', 'Glob', 'Grep', 'ToolSearch', 'Skill']) {
    const r = runGate({ tool_name: t, tool_input: {}, transcript_path: '/nope' })
    assert(!r.blocked, `${t} passes through`)
  }
})

group('gate.run — non-passive without transcript fails open', () => {
  const r = runGate({ tool_name: 'Write', tool_input: { file_path: 'x' }, transcript_path: '' })
  assert(!r.blocked, 'empty transcript_path → pass')
})

group('gate.run — non-passive with non-existent transcript fails open', () => {
  const r = runGate({ tool_name: 'Write', tool_input: { file_path: 'x' }, transcript_path: '/nope.jsonl' })
  assert(!r.blocked, 'missing transcript file → pass')
})

// ---------------------------------------------------------------------
// Layer 3b: gate.run — load-bearing block + satisfaction
// ---------------------------------------------------------------------
//
// Synthesize a parent transcript with an Agent(subagent_type=harness-impl)
// tool_use + tool_result naming a fake agentId; plant a sub-agent JSONL
// in the conventional sub-dir whose tail tool_use matches the in-flight
// Write call. With no Skill rows in the sub-agent transcript, the gate
// must block. After appending the five required Skill rows, the same
// gate call must pass.
function setupFakeSession() {
  const tmp = fs.mkdtempSync(path.join(os.tmpdir(), 'skill-evidence-session-'))
  const sessionId = 'fake-session-1'
  const parentXp = path.join(tmp, sessionId + '.jsonl')
  const subDir = path.join(tmp, sessionId, 'subagents')
  fs.mkdirSync(subDir, { recursive: true })
  const agentId = 'deadbeefcafef00d0'
  const subXp = path.join(subDir, `agent-${agentId}.jsonl`)
  const toolUseId = 'toolu_fake_agent_1'
  // Parent rows: the Agent tool_use, then the tool_result naming agentId.
  const parentRows = [
    { type: 'user', message: { role: 'user', content: 'main-session real prompt' } },
    { type: 'assistant', message: { role: 'assistant', content: [
      { type: 'tool_use', id: toolUseId, name: 'Agent',
        input: { subagent_type: 'harness-impl', prompt: 'do thing', description: 'd' } },
    ] } },
    { type: 'user', message: { role: 'user', content: [
      { type: 'tool_result', tool_use_id: toolUseId, content: [
        { type: 'text', text: 'work output...' },
        { type: 'text', text: `agentId: ${agentId} (use SendMessage with to: '${agentId}' to continue)` },
      ] },
    ] } },
  ]
  fs.writeFileSync(parentXp, parentRows.map(r => JSON.stringify(r)).join('\n'), 'utf8')
  return { tmp, parentXp, subXp, agentId }
}

function appendSubRow(subXp, row) {
  const existing = fs.existsSync(subXp) ? fs.readFileSync(subXp, 'utf8') : ''
  const prefix = existing.length === 0 || existing.endsWith('\n') ? '' : '\n'
  fs.appendFileSync(subXp, prefix + JSON.stringify(row) + '\n', 'utf8')
}

group('agentId → subagent_type resolution', () => {
  const { parentXp, agentId } = setupFakeSession()
  const t = mapAgentIdToSubagentType(parentXp, agentId)
  assert(t === 'harness-impl', `mapped ${t} === harness-impl`)
})

group('resolveActiveSubagentTranscript — matches by tool_use name+input', () => {
  const { parentXp, subXp } = setupFakeSession()
  const toolInput = { file_path: 'foo.md', content: 'x' }
  appendSubRow(subXp, { type: 'assistant', message: { role: 'assistant', content: [
    { type: 'tool_use', id: 'u1', name: 'Write', input: toolInput },
  ] } })
  const matcher = (b) => b?.name === 'Write' &&
    JSON.stringify(b.input || {}) === JSON.stringify(toolInput)
  const resolved = resolveActiveSubagentTranscript(parentXp, matcher)
  assert(resolved === subXp, `resolved to sub-agent file (${resolved})`)
})

group('gate.run — load-bearing block path (harness-impl, no Skill calls)', () => {
  const { parentXp, subXp } = setupFakeSession()
  const toolInput = { file_path: 'foo.md', content: 'x' }
  // Sub-agent tail = the in-flight Write tool_use (mirrors what Claude Code
  // appends at PreToolUse time before invoking the hook).
  appendSubRow(subXp, { type: 'assistant', message: { role: 'assistant', content: [
    { type: 'tool_use', id: 'u1', name: 'Write', input: toolInput },
  ] } })
  const r = runGate({ tool_name: 'Write', tool_input: toolInput, transcript_path: parentXp })
  assert(r.blocked, 'blocked')
  assert(r.exitCode === 2, 'exit code 2')
  assert(r.stderr.includes('harness-impl'), 'message names the subagent_type')
  assert(r.stderr.includes('fundamentals'), 'message lists missing fundamentals')
  assert(r.stderr.includes('comment-discipline'), 'message lists missing comment-discipline')
  assert(r.stderr.includes('persistence-venue'), 'message lists missing persistence-venue')
  assert(r.stderr.includes('Skill(skill='), 'message names the remedy form')
  assert(r.stderr.includes('harness-impl.md'), 'message names the manifest path')
})

group('gate.run — satisfied when all always-apply Skill calls present', () => {
  const { parentXp, subXp } = setupFakeSession()
  // Seed all five required skill invocations into the sub-agent transcript.
  for (const s of ['fundamentals', 'comment-discipline', 'backlog-workflow', 'workspace-hygiene', 'persistence-venue']) {
    appendSubRow(subXp, { type: 'assistant', message: { role: 'assistant', content: [
      { type: 'tool_use', id: 's_' + s, name: 'Skill', input: { skill: s } },
    ] } })
  }
  const toolInput = { file_path: 'foo.md', content: 'x' }
  appendSubRow(subXp, { type: 'assistant', message: { role: 'assistant', content: [
    { type: 'tool_use', id: 'u1', name: 'Write', input: toolInput },
  ] } })
  const r = runGate({ tool_name: 'Write', tool_input: toolInput, transcript_path: parentXp })
  assert(!r.blocked, 'passes once required Skill names are in the transcript')
})

group('gate.run — partial satisfaction still blocks, names only missing', () => {
  const { parentXp, subXp } = setupFakeSession()
  appendSubRow(subXp, { type: 'assistant', message: { role: 'assistant', content: [
    { type: 'tool_use', id: 's1', name: 'Skill', input: { skill: 'fundamentals' } },
    { type: 'tool_use', id: 's2', name: 'Skill', input: { skill: 'comment-discipline' } },
  ] } })
  const toolInput = { file_path: 'foo.md', content: 'x' }
  appendSubRow(subXp, { type: 'assistant', message: { role: 'assistant', content: [
    { type: 'tool_use', id: 'u1', name: 'Write', input: toolInput },
  ] } })
  const r = runGate({ tool_name: 'Write', tool_input: toolInput, transcript_path: parentXp })
  assert(r.blocked, 'still blocked')
  assert(!r.stderr.includes('`fundamentals`'),
    'does NOT name already-loaded fundamentals')
  assert(r.stderr.includes('backlog-workflow'),
    'names missing backlog-workflow')
})

group('gate.run — unknown subagent_type fails open', () => {
  // Synthesize a session whose Agent dispatch is `general-purpose` (not
  // in ENFORCED_AGENTS) — gate must let the Write through silently.
  const tmp = fs.mkdtempSync(path.join(os.tmpdir(), 'skill-evidence-unk-'))
  const sessionId = 'fake-session-2'
  const parentXp = path.join(tmp, sessionId + '.jsonl')
  const subDir = path.join(tmp, sessionId, 'subagents')
  fs.mkdirSync(subDir, { recursive: true })
  const agentId = 'unknown00000000aa'
  const subXp = path.join(subDir, `agent-${agentId}.jsonl`)
  const toolUseId = 'toolu_unk_1'
  const parentRows = [
    { type: 'assistant', message: { role: 'assistant', content: [
      { type: 'tool_use', id: toolUseId, name: 'Agent',
        input: { subagent_type: 'general-purpose', prompt: 'x', description: 'd' } },
    ] } },
    { type: 'user', message: { role: 'user', content: [
      { type: 'tool_result', tool_use_id: toolUseId, content: [
        { type: 'text', text: `agentId: ${agentId}` },
      ] },
    ] } },
  ]
  fs.writeFileSync(parentXp, parentRows.map(r => JSON.stringify(r)).join('\n'), 'utf8')
  const toolInput = { file_path: 'foo.md', content: 'x' }
  appendSubRow(subXp, { type: 'assistant', message: { role: 'assistant', content: [
    { type: 'tool_use', id: 'u1', name: 'Write', input: toolInput },
  ] } })
  const r = runGate({ tool_name: 'Write', tool_input: toolInput, transcript_path: parentXp })
  assert(!r.blocked, 'unenforced subagent_type → pass')
})

const parseResult = parseTests.run()
const totalPassed = passed + parseResult.passed
const totalFailed = failed + parseResult.failed
console.log(`\n${totalPassed} passed, ${totalFailed} failed`)
process.exit(totalFailed === 0 ? 0 : 1)
