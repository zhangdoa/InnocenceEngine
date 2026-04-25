// Session-start producer-brief gate — primary enforcement of
// `CLAUDE.md` (root § "Session start"): the first action of every new
// session must be invoking the `producer` subagent.
//
// SessionStart fires before any model turn. We emit
// `hookSpecificOutput.additionalContext` (per the SessionStart JSON
// output contract) so the model reads a system-reminder on its very
// first response, *before* it has chosen what to do this turn. The
// existing PreToolUse `producer-brief` gate stays as belt-and-suspenders
// for the case where this hook is bypassed or misconfigured.
//
// Idempotency on session resume: settings.json wires this hook to the
// `startup|clear` matchers — `resume` and `compact` skip the directive
// because the prior briefing is already in transcript history. We also
// scan the transcript before injecting (using the same helper the
// PreToolUse gate uses) so a producer call that happened earlier this
// session is never re-prompted, even if a non-`resume` source somehow
// surfaces a populated transcript.
//
// Subagent transcripts contain only the synthetic prompt the parent
// passed (`hasRealUserPrompt === false`); SessionStart should never
// fire inside a subagent, but if it ever does we fail open — subagents
// are not "the session" this gate governs.
//
// Escape hatch: CLAUDE_SKIP_PRODUCER=1 — same env var the PreToolUse
// gate honors. One escape hatch, one mental model.
//
// Fails OPEN on any internal error: returns `{ stdout: '' }` and lets
// the dispatcher exit 0 so a hook bug never bricks a session.

const fs = require('fs')
const { scanTranscriptForProducerBrief } = require('../lib/common')

const DIRECTIVE = [
  'SESSION-START DIRECTIVE — producer briefing required.',
  '',
  'Per CLAUDE.md (root § "Session start"), the first action of every new',
  'session must be invoking the `producer` subagent. The producer reads',
  'in-progress tasks, recent commits, and continuity notes, then briefs',
  'the user on state and likely priorities. No substantive work begins',
  'before the briefing + user direction.',
  '',
  'Your first tool call this session MUST be:',
  '  Agent(subagent_type="producer", ...)',
  '',
  'This applies regardless of what the user typed — even a continuation',
  'prompt like "continue" or a casual "hi" routes through the producer',
  'first. Do not Read, Glob, Grep, or reply textually before dispatching.',
  '',
  'If the briefing already happened this session and the harness somehow',
  're-fired this directive, set CLAUDE_SKIP_PRODUCER=1 in the environment',
  'to suppress it on the next start.',
].join('\n')

function run(input) {
  if (process.env.CLAUDE_SKIP_PRODUCER === '1') return { stdout: '' }

  // Idempotency: if a prior producer dispatch already shows in the
  // transcript, do not re-inject. `startup|clear` matchers should keep
  // transcripts empty here, but the check costs ~one fs.readFileSync
  // and removes a class of "noise on every resume" failure modes.
  const xp = input.transcript_path
  if (xp && fs.existsSync(xp)) {
    const scan = scanTranscriptForProducerBrief(xp)
    if (scan && scan.producerSeen) return { stdout: '' }
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
