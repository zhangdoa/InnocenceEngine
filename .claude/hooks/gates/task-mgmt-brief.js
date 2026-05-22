// task-mgmt-brief: blocks substantive tool use until task-mgmt subagent has run this session.
// Escape: CLAUDE_SKIP_PRODUCER=1 for resumed sessions.

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
    `[session-gate] ${toolName} blocked — task-mgmt briefing required.`,
    '',
    'First action of every new session: Agent(subagent_type="task-mgmt", ...)',
    'Escape: CLAUDE_SKIP_PRODUCER=1 for resumed sessions.',
    '',
  ].join('\n'))
  process.exit(2)
}

module.exports = { run }
