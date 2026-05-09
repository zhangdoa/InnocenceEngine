// no-auto-memory gate — block any write tool whose target lives under
// the Claude default auto-memory directory for this project.
//
// Rationale:
//   Auto-memory entries do not reach spawned subagents (per
//   `.claude/skills/persistence-venue/SKILL.md`) and bypass the review
//   pressure that every other persistence venue carries. They also
//   accumulate stale advice — entries get written once, never re-read,
//   and decay against the live codebase. The project no longer uses
//   auto-memory; this gate makes the rule load-bearing instead of
//   prose-only.
//
// Block contract:
//   The block message is the routing surface for a future Claude that
//   trips it. It does not just refuse. It states which venue applies
//   for each content shape (dispatcher rule, project state, engine
//   fact, ephemeral context) so the next action is "write to <venue>",
//   not "ask the user what to do".
//
// Path resolution:
//   The auto-memory root is derived from `os.homedir()` and the
//   current `cwd` (the project slug Claude Code uses is the cwd with
//   path separators rewritten to `-` and the leading drive `C:\` →
//   `C--`). Both POSIX and Windows-style paths are normalised before
//   comparison so the gate fires on either form.
//
// Tools matched: Write, Edit, MultiEdit, NotebookEdit, and any future
// write-shaped tool whose `tool_input` carries a `file_path` /
// `notebook_path` field. Bash writes (`>`, `tee`, `cp`) are out of
// scope — those would require parsing arbitrary shell, and the agent
// would have to reach for shell deliberately, at which point the
// discipline can carry the rule.
//
// Fails OPEN on any internal error so a hook bug never bricks a
// session — same posture as the other session-gate sub-gates.

const os = require('os')
const path = require('path')

const WRITE_TOOLS = new Set(['Write', 'Edit', 'MultiEdit', 'NotebookEdit'])

function normalise(p) {
  if (typeof p !== 'string' || p.length === 0) return ''
  // Resolve to absolute and normalise separators for cross-platform compare.
  const abs = path.isAbsolute(p) ? p : path.resolve(p)
  return abs.replace(/\\/g, '/').toLowerCase()
}

// Compute the auto-memory directory for the project rooted at `cwd`.
// Claude Code's project slug rewrites every separator character (`:`,
// `\`, `/`) to `-` one-for-one, NOT collapsing runs — so
// `C:\GitRepo\InnocenceEngine` → `C--GitRepo-InnocenceEngine` (the `:\`
// becomes `--`, not `-`). The single-char regex preserves that.
function autoMemoryDir(cwd) {
  const home = os.homedir()
  if (!home || !cwd) return null
  const slug = cwd.replace(/[:\\/]/g, '-')
  return normalise(path.join(home, '.claude', 'projects', slug, 'memory'))
}

function extractTargetPath(toolName, toolInput) {
  if (!toolInput) return ''
  if (toolName === 'NotebookEdit') return toolInput.notebook_path || ''
  return toolInput.file_path || ''
}

function run(input) {
  if (!WRITE_TOOLS.has(input.tool_name)) return { ok: true }
  const target = extractTargetPath(input.tool_name, input.tool_input)
  if (!target) return { ok: true }
  const cwd = input.cwd || process.cwd()
  const memDir = autoMemoryDir(cwd)
  if (!memDir) return { ok: true }
  const normTarget = normalise(target)
  if (!normTarget.startsWith(memDir + '/') && normTarget !== memDir) return { ok: true }
  return { ok: false, block: () => emit(normTarget) }
}

function emit(target) {
  process.stderr.write([
    '',
    '[session-gate] auto-memory is disabled in this project.',
    '',
    `  attempted target: ${target}`,
    '',
    'Auto-memory entries are subagent-invisible and bypass the review pressure',
    'every other persistence venue carries. To persist this content, write to the',
    'venue that matches its shape:',
    '',
    '  Rule the dispatcher (main-session) must follow      →  .claude/skills/dispatch-briefs/SKILL.md',
    '  Rule every agent must follow                        →  .claude/skills/<topic>/SKILL.md',
    '                                                          (and add to the universal preamble in CLAUDE.md)',
    '  Rule one specific role must follow                  →  add to that agent\'s manifest .claude/agents/<role>.md',
    '  Project-state snapshot (direction, sync, invariants)→  .claude/state/<topic>.md',
    '  Cross-session continuity for an in-flight task      →  the task\'s ## Implementation Notes in .backlog/tasks/',
    '  Cost-of-one-slip-is-high enforcement                →  a new gate under .claude/hooks/gates/<name>.js',
    '  Ephemeral conversation context                      →  do not persist; let it scroll',
    '',
    'Full routing rules: .claude/skills/persistence-venue/SKILL.md',
    '',
    'If none of the above fits, the content is probably not worth persisting — let',
    'it scroll. Re-deriving from source on the next session is cheaper than carrying',
    'stale advice forward.',
    '',
  ].join('\n'))
  process.exit(2)
}

module.exports = { run, autoMemoryDir, normalise }
