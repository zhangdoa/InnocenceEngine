// SessionStart task-mgmt-brief gate — primary enforcement of CLAUDE.md
// § "Session start": the first action of every new session must be
// invoking the `task-mgmt` subagent.
//
// SessionStart fires before any model turn. Emits `additionalContext` so
// the model reads a system-reminder on its very first response, before
// it has chosen what to do this turn. The PreToolUse `task-mgmt-brief`
// gate stays as belt-and-suspenders.
//
// Idempotency on resume: settings.json wires this hook to `startup|clear`
// matchers — `resume` and `compact` skip. Transcript scan also catches
// any task-mgmt call earlier this session.
//
// Subagent transcripts have no real user prompts; SessionStart should
// never fire inside a subagent, but if it does we fail open.
//
// Escape: CLAUDE_SKIP_PRODUCER=1 (env-var name preserved for shell-history
// compatibility through the rename).
//
// Fails OPEN on internal error.

const fs = require('fs')
const { scanTranscriptForTaskMgmtBrief } = require('../lib/common')

const DIRECTIVE = [
  'SESSION-START DIRECTIVE — task-mgmt briefing required.',
  '',
  'Per CLAUDE.md § "Session start", the first action of every new session',
  'must be invoking the `task-mgmt` subagent. It reads in-progress tasks,',
  'recent commits, and continuity notes, then briefs the user on state and',
  'likely priorities. No substantive work begins before the briefing + user',
  'direction.',
  '',
  'Your first tool call this session MUST be:',
  '  Agent(subagent_type="task-mgmt", ...)',
  '',
  'This applies regardless of what the user typed — even a continuation',
  'prompt like "continue" or a casual "hi" routes through task-mgmt first.',
  'Do not Read, Glob, Grep, or reply textually before dispatching.',
  '',
  'If the briefing already happened this session and the harness somehow',
  're-fired this directive, set CLAUDE_SKIP_PRODUCER=1 in the environment.',
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
