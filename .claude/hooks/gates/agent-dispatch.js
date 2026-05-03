// Agent-dispatch gate — block `Agent` tool calls that occupy the
// dispatcher's surface (foreground) without an explicit per-call
// justification.
//
// Rule (from .claude/disciplines/on-dispatch/agent-dispatch.md): the default for
// every `Agent` (and the older `Task`) dispatch is `run_in_background:
// true`. Foreground is the documented exception, justified per call.
// Prose alone has not held — this gate makes the rule load-bearing.
//
// Violation shape:
//   `tool_input.run_in_background` is anything other than `true`
//   (false, missing, null, etc.) AND the prompt does not include the
//   sentinel `[foreground-required]`.
//
// Why a sentinel rather than auto-rewrite: PreToolUse hooks can deny
// a call but cannot rewrite its arguments — the harness offers no
// approved facility for that. Even if it did, auto-rewriting would
// silently change dispatch semantics and erode the discipline's
// "justified per call" framing. A visible sentinel in the prompt
// surfaces the choice to both the dispatcher (it has to type it) and
// the user (it shows up in the tool-call view).
//
// The bypass sentinel lives in the `prompt` field, not `description`,
// because:
//   • The prompt is the artifact the user actually sees in transcripts.
//   • The prompt is the artifact the sub-agent reads, which keeps the
//     justification co-located with the work itself.
//   • `description` is short and structural; loading it with policy
//     metadata pollutes its purpose.
//
// Failure mode the gate prevents — the recurring incident the user
// flagged when filing this hook:
//   Main-session Claude dispatches an implementation/research sub-agent
//   foreground, then sits blocked while the sub-agent works. The user's
//   interactive surface is occupied for minutes; the prompt cache TTL
//   (5 min) burns; if the user wants to redirect they must Esc, which
//   costs attention. Background dispatch keeps main session free.
//
// Fails OPEN on any internal error so a hook bug never bricks dispatch.

const FOREGROUND_SENTINEL = '[foreground-required]'

// `Agent` is the current Claude Code serialization for the subagent
// dispatch tool; older / future variants may use `Task`. Accept either.
function isAgentDispatch(toolName) {
  return toolName === 'Agent' || toolName === 'Task'
}

function run(input) {
  if (!isAgentDispatch(input.tool_name)) return { ok: true }

  const ti = input.tool_input || {}

  // Background dispatches are always fine.
  if (ti.run_in_background === true) return { ok: true }

  // Foreground dispatch — require the per-call sentinel in the prompt.
  const prompt = typeof ti.prompt === 'string' ? ti.prompt : ''
  if (prompt.includes(FOREGROUND_SENTINEL)) return { ok: true }

  return { ok: false, block: () => emit(ti) }
}

function emit(ti) {
  const subagent = ti.subagent_type || '<unspecified>'
  const desc = ti.description || '<no description>'
  process.stderr.write([
    '',
    '[session-gate] Agent dispatch blocked — foreground call without justification.',
    '',
    `  subagent_type: ${subagent}`,
    `  description:   ${desc}`,
    `  run_in_background: ${ti.run_in_background === undefined ? '<unset>' : String(ti.run_in_background)}`,
    '',
    'Per .claude/disciplines/on-dispatch/agent-dispatch.md, every `Agent` dispatch defaults',
    'to `run_in_background: true`. Foreground dispatch is the exception, justified',
    'per call by the two-condition test:',
    '',
    '  1. The dispatcher\'s immediate next action depends on the sub-agent\'s',
    '     result, AND',
    '  2. There is no parallel work the dispatcher could be doing while the',
    '     sub-agent runs.',
    '',
    'If both hold (typically: a tightly-bounded research call whose result blocks',
    'the very next sentence), retry with `[foreground-required]` somewhere in the',
    '`prompt` field — that sentinel is the per-call justification, surfaced where',
    'the user can see it.',
    '',
    'Otherwise — the common case — retry with `run_in_background: true`. The',
    'dispatcher keeps its surface free; pick up the result on completion or via',
    'Monitor. Implementation, build, test-suite, and audit dispatches are all',
    'background-required by the discipline.',
    '',
  ].join('\n'))
  process.exit(2)
}

module.exports = { run, FOREGROUND_SENTINEL }
