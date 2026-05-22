#!/usr/bin/env node

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

group('scanTranscriptForSkillUses — finds Skill tool_uses', () => {
  const tmp = fs.mkdtempSync(path.join(os.tmpdir(), 'skill-evidence-'))
  const xp = path.join(tmp, 't.jsonl')
  const rows = [
    { type: 'user', message: { role: 'user', content: 'hi' } },
    { type: 'assistant', message: { role: 'assistant', content: [
      { type: 'tool_use', id: 'a', name: 'Skill', input: { skill: 'backlog-workflow' } },
    ] } },
    { type: 'assistant', message: { role: 'assistant', content: [
      { type: 'tool_use', id: 'b', name: 'Skill', input: { skill: 'commit-message-policy' } },
      { type: 'tool_use', id: 'c', name: 'Read', input: { file_path: '/x' } },
    ] } },
  ]
  fs.writeFileSync(xp, rows.map(r => JSON.stringify(r)).join('\n'), 'utf8')
  const s = scanTranscriptForSkillUses(xp)
  assert(s instanceof Set, 'returns Set')
  assert(s.has('backlog-workflow') && s.has('commit-message-policy'), 'collects both Skill names')
  assert(!s.has('Read'), 'ignores non-Skill tools')
})

group('scanTranscriptForSkillUses — missing file returns null', () => {
  const s = scanTranscriptForSkillUses('/nonexistent/no.jsonl')
  assert(s === null, 'fail-open trigger')
})

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

function setupFakeSession() {
  const tmp = fs.mkdtempSync(path.join(os.tmpdir(), 'skill-evidence-session-'))
  const sessionId = 'fake-session-1'
  const parentXp = path.join(tmp, sessionId + '.jsonl')
  const subDir = path.join(tmp, sessionId, 'subagents')
  fs.mkdirSync(subDir, { recursive: true })
  const agentId = 'deadbeefcafef00d0'
  const subXp = path.join(subDir, `agent-${agentId}.jsonl`)
  const toolUseId = 'toolu_fake_agent_1'
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
  appendSubRow(subXp, { type: 'assistant', message: { role: 'assistant', content: [
    { type: 'tool_use', id: 'u1', name: 'Write', input: toolInput },
  ] } })
  const r = runGate({ tool_name: 'Write', tool_input: toolInput, transcript_path: parentXp })
  assert(r.blocked, 'blocked')
  assert(r.exitCode === 2, 'exit code 2')
  assert(r.stderr.includes('harness-impl'), 'message names the subagent_type')
  assert(r.stderr.includes('Skill(skill='), 'message names the remedy form')
  assert(r.stderr.includes('harness-impl.md'), 'message names the manifest path')
})

group('gate.run — satisfied when all always-apply Skill calls present', () => {
  const { parentXp, subXp } = setupFakeSession()
  for (const s of ['backlog-workflow', 'commit-message-policy', 'peer-review-required']) {
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
    { type: 'tool_use', id: 's1', name: 'Skill', input: { skill: 'backlog-workflow' } },
  ] } })
  const toolInput = { file_path: 'foo.md', content: 'x' }
  appendSubRow(subXp, { type: 'assistant', message: { role: 'assistant', content: [
    { type: 'tool_use', id: 'u1', name: 'Write', input: toolInput },
  ] } })
  const r = runGate({ tool_name: 'Write', tool_input: toolInput, transcript_path: parentXp })
  assert(r.blocked, 'still blocked')
  assert(!r.stderr.includes('`backlog-workflow`'),
    'does NOT name already-loaded backlog-workflow')
})

group('gate.run — unknown subagent_type fails open', () => {
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
