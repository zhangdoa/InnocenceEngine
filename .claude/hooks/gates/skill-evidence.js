// skill-evidence gate — blocks a sub-agent's first side-effecting tool
// call until the sub-agent's transcript shows a `Skill` invocation for
// every always-apply skill listed in its manifest.
//
// Design: .backlog/decisions/TASK-187-enforcement-mechanism-2026-05-14.md
//
// Scope: SUB-AGENT transcripts only. Main-session compliance is governed
// by CLAUDE.md auto-load and `dispatch-briefs`; this gate exists because
// sub-agent system-reminders list skills by NAME, not by content, and
// nothing else forces the agent to call `Skill` before acting.
//
// Passive tools pass through. The Skill tool itself MUST pass — it is
// the means by which the gate is satisfied. Read/Glob/Grep/ToolSearch
// are needed for the agent to navigate before satisfying the gate.
//
// Non-passive tools (Write, Edit, MultiEdit, NotebookEdit, Bash, Agent)
// require the always-apply set to be present in the sub-agent's
// transcript by the time they fire.
//
// Fails OPEN on any I/O / parse / resolution failure: missing parent
// transcript, unresolvable sub-agent transcript, unrecognised manifest,
// unknown subagent_type. Per design "Trade-offs": the gate is for
// observable agent compliance, not infrastructure correctness — silent
// pass on broken plumbing is preferred to silent block.
//
// What is NOT fail-open: a recognised manifest with a known always-apply
// list and a missing Skill invocation. That is the load-bearing block.

const fs = require('fs')
const path = require('path')
const {
  parseAlwaysApplySkills,
  scanTranscriptForSkillUses,
  resolveActiveSubagentTranscript,
  mapAgentIdToSubagentType,
} = require('../lib/subagent-transcript')

// Read-only tools never trigger the evidence requirement — the agent
// must be able to read its way around before satisfying the gate, and
// Skill itself is how the gate is satisfied.
const PASSIVE_TOOLS = new Set([
  'Read', 'Glob', 'Grep', 'ToolSearch', 'Skill',
])

// MVP per design: enforce only the three highest-traffic impl stages.
// Other subagent_types fail open until their manifest's always-apply
// shape has been audited against the parse regex.
const ENFORCED_AGENTS = new Set([
  'code-impl', 'shader-impl', 'harness-impl',
])

const REPO_ROOT_FROM_HOOK = path.resolve(__dirname, '..', '..', '..')

function manifestPathFor(subagentType) {
  return path.join(REPO_ROOT_FROM_HOOK, '.claude', 'agents', `${subagentType}.md`)
}

// Match a candidate `tool_use` block from a sub-agent's transcript
// against the in-flight call. Tool name must match; inputs must match by
// strict JSON equality. Stable stringify isn't needed — Claude Code
// serialises tool_input deterministically into the JSONL, and the
// hook input mirrors that shape.
function makeToolUseMatcher(toolName, toolInput) {
  const wanted = JSON.stringify(toolInput || {})
  return (b) => b?.name === toolName && JSON.stringify(b.input || {}) === wanted
}

// Extract the agentId from a sub-agent transcript filename like
// `subagents/agent-a1b6cd91134a7d1e4.jsonl`.
function agentIdFromPath(p) {
  const base = path.basename(p, '.jsonl')
  return base.startsWith('agent-') ? base.slice('agent-'.length) : null
}

function run(input) {
  const toolName = input.tool_name
  if (PASSIVE_TOOLS.has(toolName)) return { ok: true }

  const parentXp = input.transcript_path
  if (!parentXp || !fs.existsSync(parentXp)) return { ok: true }

  // Locate the sub-agent transcript whose tail tool_use matches the
  // in-flight call. If it's the same file as parentXp, this is a main-
  // session call (or an unresolvable case) — fail open.
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
    `[session-gate] ${toolName} blocked — sub-agent (${subagentType}) has not loaded its always-apply skills.`,
    '',
    `  manifest: ${repoRelative}`,
    `  missing:  ${missing.map(s => '`' + s + '`').join(', ')}`,
    '',
    'Per the manifest\'s "Always-apply skills" line, every listed skill must be',
    'loaded into the sub-agent\'s context before any side-effecting tool call',
    '(Write/Edit/Bash/etc.). System-reminders list skills by NAME only; the body',
    'enters the context window when the `Skill` tool is invoked.',
    '',
    'Remedy: invoke `Skill` for each missing name before retrying:',
    ...missing.map(s => `  Skill(skill="${s}")`),
    '',
    'Read / Glob / Grep / ToolSearch / Skill remain available while the gate is',
    'pending. The gate stays satisfied for the rest of the sub-agent\'s lifetime',
    'once every required name appears as a `Skill` tool-use in its transcript.',
    '',
    'Mechanism: .backlog/decisions/TASK-187-enforcement-mechanism-2026-05-14.md.',
    '',
  ].join('\n'))
  process.exit(2)
}

module.exports = { run, PASSIVE_TOOLS, ENFORCED_AGENTS }
