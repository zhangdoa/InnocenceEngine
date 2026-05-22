// session-start-brief: SessionStart hook — inject directive to dispatch task-mgmt first.
// Fires before any model turn (PreToolUse fires after — too late for a textual-only reply).

const fs = require('fs')
const { scanTranscriptForTaskMgmtBrief } = require('../lib/common')

const DIRECTIVE = [
  'SESSION-START DIRECTIVE — task-mgmt briefing required.',
  '',
  'First tool call this session MUST be:',
  '  Agent(subagent_type="task-mgmt", ...)',
  '',
  'Applies regardless of user input — even "continue" or "hi" routes through task-mgmt first.',
  'Do not Read, Glob, Grep, or reply textually before dispatching.',
  '',
  'Escape: CLAUDE_SKIP_PRODUCER=1 for resumed sessions.',
].join('\n')

function run(input) {
  if (process.env.CLAUDE_SKIP_PRODUCER === '1') return { stdout: '' }

  const xp = input.transcript_path
  if (xp && fs.existsSync(xp)) {
    const scan = scanTranscriptForTaskMgmtBrief(xp)
    if (scan && scan.taskMgmtSeen) return { stdout: '' }
  }

  const payload = {
    hookSpecificOutput: {
      hookEventName: 'SessionStart',
      additionalContext: DIRECTIVE,
    },
  }
  return { stdout: JSON.stringify(payload) }
}

module.exports = { run }
