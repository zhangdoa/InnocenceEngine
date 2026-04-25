// Producer-brief gate — blocks substantive tool use until the producer
// subagent has been invoked at least once this session.
//
// Rationale: `CLAUDE.md` says "first action of every new session: invoke
// the producer agent". That's prose. Prose drifts. This gate makes the
// rule load-bearing at the harness level.
//
// Source of truth for "has the producer run this session?" is the
// session transcript that Claude Code already maintains. We read
// `input.transcript_path` and look for an `Agent(subagent_type=producer)`
// tool_use entry. This mirrors how `commit-gate.js` (lines 62–78) treats
// the transcript as the canonical record of session events — no invented
// state directory, no SessionStart/PostToolUse plumbing.
//
// Allowed without a briefing:
//   • Agent(subagent_type=producer)            — the call that satisfies the gate.
//   • Read                                     — so main-session Claude can consult
//                                                 backlog / memory / CLAUDE.md
//                                                 while routing the request.
//   • Glob, Grep                               — same reason.
//   • ToolSearch                               — deferred-tool discovery is passive.
//   • Skill, ScheduleWakeup                    — meta-tools the harness needs.
//
// Everything else (Edit, Write, Bash, MultiEdit, other Agent subagents, MCP)
// is blocked with an actionable message until the producer has run.
//
// Subagent behaviour: a subagent's `transcript_path` points at its own
// isolated transcript under `<session-id>/subagents/agent-*.jsonl`, which
// contains only the synthetic prompt the parent passed and the subagent's
// own tool calls — no real user prompts. We detect this with the same
// `isRealUserPrompt` helper commit-gate uses to find "the last real user
// prompt" (lib/common.js, used at commit-gate.js:75). A transcript with
// zero real user prompts is treated as a subagent transcript and the gate
// fails open: subagents are not "the session" the gate governs.
//
// Escape hatch: set env var CLAUDE_SKIP_PRODUCER=1 when resuming a
// session where the briefing already happened (no commit-message
// sentinel applies — no commit is involved).
//
// Fails OPEN on any internal error so a hook bug never bricks the session.

const fs = require('fs')
const { isRealUserPrompt } = require('../lib/common')

const ALLOWED_TOOLS = new Set([
  'Read', 'Glob', 'Grep', 'ToolSearch',
  // These meta-tools are required for the Skill infrastructure and
  // status-line to function; blocking them has no user-visible benefit.
  'Skill', 'ScheduleWakeup',
])

function isProducerAgentCall(toolName, toolInput) {
  // Current Claude Code serializes the Agent tool as `name: "Agent"`.
  // Older / future variants may use `"Task"`; accept either defensively.
  if (toolName !== 'Agent' && toolName !== 'Task') return false
  const sub = toolInput?.subagent_type || ''
  return sub === 'producer'
}

// Scan the transcript for any prior Agent(subagent_type=producer) tool_use.
// Returns { producerSeen: bool, hasRealUserPrompt: bool }.
function scanTranscript(xpPath) {
  const out = { producerSeen: false, hasRealUserPrompt: false }
  let raw
  try { raw = fs.readFileSync(xpPath, 'utf8') } catch { return null }
  for (const line of raw.split('\n')) {
    if (!line) continue
    let m
    try { m = JSON.parse(line) } catch { continue }
    const role = m.type || m.role || m.message?.role
    if (role === 'user' && isRealUserPrompt(m.message?.content ?? m.content)) {
      out.hasRealUserPrompt = true
    }
    // Tool-use entries live as content blocks on assistant messages.
    // Path: m.message.content[i] = { type: 'tool_use', name, input, ... }
    const content = m.message?.content
    if (!Array.isArray(content)) continue
    for (const block of content) {
      if (block?.type !== 'tool_use') continue
      if (isProducerAgentCall(block.name, block.input)) {
        out.producerSeen = true
      }
    }
    if (out.producerSeen && out.hasRealUserPrompt) break
  }
  return out
}

function run(input) {
  if (process.env.CLAUDE_SKIP_PRODUCER === '1') return { ok: true }

  const toolName = input.tool_name
  if (ALLOWED_TOOLS.has(toolName)) return { ok: true }
  if (isProducerAgentCall(toolName, input.tool_input)) return { ok: true }

  const xp = input.transcript_path
  if (!xp || !fs.existsSync(xp)) return { ok: true }  // fail open

  const scan = scanTranscript(xp)
  if (!scan) return { ok: true }  // I/O error — fail open

  // Subagent transcripts have no real user prompts (only the synthetic
  // prompt the parent passed). The gate governs main-session bootstrap,
  // not subagent tool use — fail open so subagents work.
  if (!scan.hasRealUserPrompt) return { ok: true }

  if (scan.producerSeen) return { ok: true }

  return { ok: false, block: () => emit(toolName) }
}

function emit(toolName) {
  process.stderr.write([
    '',
    `[session-gate] ${toolName} blocked — producer briefing not yet completed this session.`,
    '',
    'Per CLAUDE.md (root § "Session start"), the first action of every new session',
    'must be invoking the `producer` subagent. The producer reads in-progress tasks,',
    'recent commits, and continuity notes, then briefs the user on state and likely',
    'priorities. No substantive work begins before the briefing + user direction.',
    '',
    'To satisfy this gate, issue:',
    '  Agent(subagent_type="producer", ...)',
    '',
    'Read/Glob/Grep/ToolSearch remain available so you can route the request while',
    'the briefing is pending. Once the producer subagent returns, this gate stays',
    'satisfied for the remainder of the session.',
    '',
    'Escape hatch: set CLAUDE_SKIP_PRODUCER=1 in the environment when resuming a',
    'session whose briefing already happened (e.g. a continuation after a crash).',
    '',
  ].join('\n'))
  process.exit(2)
}

module.exports = { run }
