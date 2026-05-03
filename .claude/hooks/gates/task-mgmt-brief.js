// task-mgmt-brief gate — blocks substantive tool use until the
// task-mgmt subagent has run at least once this session.
//
// Source of truth: the session transcript. We read
// `input.transcript_path` and look for `Agent(subagent_type=task-mgmt)`.
//
// Allowed without a briefing:
//   • Agent(subagent_type=task-mgmt)  — the call that satisfies the gate
//   • Read, Glob, Grep                 — main-session can route the request
//   • ToolSearch                       — deferred-tool discovery is passive
//   • Skill, ScheduleWakeup            — meta-tools the harness needs
//
// Everything else is blocked until task-mgmt has run.
//
// Subagent transcripts contain no real user prompts (only the synthetic
// prompt the parent passed). Detected via the `isRealUserPrompt` helper.
// Subagent transcripts fail open — the gate governs main-session bootstrap.
//
// Escape: CLAUDE_SKIP_PRODUCER=1 when resuming a session whose briefing
// already happened. Env var name preserved for shell-history compatibility.
//
// Fails OPEN on any internal error.

const fs = require('fs')
const { isTaskMgmtAgentCall, scanTranscriptForTaskMgmtBrief } = require('../lib/common')

const ALLOWED_TOOLS = new Set([
  'Read', 'Glob', 'Grep', 'ToolSearch',
  'Skill', 'ScheduleWakeup',
])

function run(input) {
  if (process.env.CLAUDE_SKIP_PRODUCER === '1') return { ok: true }

  const toolName = input.tool_name
  if (ALLOWED_TOOLS.has(toolName)) return { ok: true }
  if (isTaskMgmtAgentCall(toolName, input.tool_input)) return { ok: true }

  const xp = input.transcript_path
  if (!xp || !fs.existsSync(xp)) return { ok: true }

  const scan = scanTranscriptForTaskMgmtBrief(xp)
  if (!scan) return { ok: true }
  if (!scan.hasRealUserPrompt) return { ok: true }
  if (scan.taskMgmtSeen) return { ok: true }

  return { ok: false, block: () => emit(toolName) }
}

function emit(toolName) {
  process.stderr.write([
    '',
    `[session-gate] ${toolName} blocked — task-mgmt briefing not yet completed this session.`,
    '',
    'Per CLAUDE.md § "Session start": the first action of every new session must be',
    'invoking the `task-mgmt` subagent. It reads in-progress tasks, recent commits,',
    'and continuity notes, then briefs the user on state and likely priorities.',
    'No substantive work begins before the briefing + user direction.',
    '',
    'To satisfy this gate:',
    '  Agent(subagent_type="task-mgmt", ...)',
    '',
    'Read / Glob / Grep / ToolSearch remain available for routing while the briefing',
    'is pending. Once task-mgmt returns, this gate stays satisfied for the session.',
    '',
    'Escape: set CLAUDE_SKIP_PRODUCER=1 to suppress on a resumed session whose',
    'briefing already happened.',
    '',
  ].join('\n'))
  process.exit(2)
}

module.exports = { run }
