// skill-evidence: block a sub-agent's first side-effecting tool call until its transcript
// shows a Skill invocation for every always-apply skill listed in its manifest.

const fs = require('fs')
const path = require('path')
const {
  parseAlwaysApplySkills,
  scanTranscriptForSkillUses,
  resolveActiveSubagentTranscript,
  mapAgentIdToSubagentType,
} = require('../lib/subagent-transcript')

// Skill itself must pass — it is how the gate is satisfied.
// Read/Glob/Grep/ToolSearch are needed for navigation before satisfying.
const PASSIVE_TOOLS = new Set(['Read', 'Glob', 'Grep', 'ToolSearch', 'Skill'])

const ENFORCED_AGENTS = new Set([
  'code-impl', 'shader-impl', 'harness-impl',
  'task-mgmt', 'ci-build-impl',
])

const REPO_ROOT_FROM_HOOK = path.resolve(__dirname, '..', '..', '..')

function manifestPathFor(subagentType) {
  return path.join(REPO_ROOT_FROM_HOOK, '.claude', 'agents', `${subagentType}.md`)
}

function makeToolUseMatcher(toolName, toolInput) {
  const wanted = JSON.stringify(toolInput || {})
  return (b) => b?.name === toolName && JSON.stringify(b.input || {}) === wanted
}

function agentIdFromPath(p) {
  const base = path.basename(p, '.jsonl')
  return base.startsWith('agent-') ? base.slice('agent-'.length) : null
}

function run(input) {
  const toolName = input.tool_name
  if (PASSIVE_TOOLS.has(toolName)) return { ok: true }

  const parentXp = input.transcript_path
  if (!parentXp || !fs.existsSync(parentXp)) return { ok: true }

  const matcher = makeToolUseMatcher(toolName, input.tool_input)
  const subXp = resolveActiveSubagentTranscript(parentXp, matcher)
  if (subXp === parentXp) return { ok: true }

  const agentId = agentIdFromPath(subXp)
  const subagentType = mapAgentIdToSubagentType(parentXp, agentId)
  if (!subagentType) return { ok: true }
  if (!ENFORCED_AGENTS.has(subagentType)) return { ok: true }

  const manifestPath = manifestPathFor(subagentType)
  const required = parseAlwaysApplySkills(manifestPath)
  if (!required) return { ok: true }

  const loaded = scanTranscriptForSkillUses(subXp)
  if (loaded === null) return { ok: true }

  const missing = required.filter(s => !loaded.has(s))
  if (missing.length === 0) return { ok: true }

  return { ok: false, block: () => emit(toolName, subagentType, manifestPath, missing) }
}

function emit(toolName, subagentType, manifestPath, missing) {
  const repoRelative = path.relative(REPO_ROOT_FROM_HOOK, manifestPath).replace(/\\/g, '/')
  process.stderr.write([
    '',
    `[session-gate] ${toolName} blocked — sub-agent (${subagentType}) missing always-apply skills.`,
    '',
    `  manifest: ${repoRelative}`,
    `  missing:  ${missing.map(s => '`' + s + '`').join(', ')}`,
    '',
    'Invoke Skill for each missing name before retrying:',
    ...missing.map(s => `  Skill(skill="${s}")`),
    '',
  ].join('\n'))
  process.exit(2)
}

module.exports = { run, PASSIVE_TOOLS, ENFORCED_AGENTS }
