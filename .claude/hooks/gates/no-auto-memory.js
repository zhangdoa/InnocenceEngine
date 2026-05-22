// no-auto-memory: block Write/Edit/MultiEdit/NotebookEdit targeting Claude default auto-memory dir.

const os = require('os')
const path = require('path')

const WRITE_TOOLS = new Set(['Write', 'Edit', 'MultiEdit', 'NotebookEdit'])

function normalise(p) {
  if (typeof p !== 'string' || p.length === 0) return ''
  const abs = path.isAbsolute(p) ? p : path.resolve(p)
  return abs.replace(/\\/g, '/').toLowerCase()
}

// Claude Code's project slug rewrites every separator char (`:`, `\`, `/`) to `-` one-for-one,
// not collapsing runs — so `C:\GitRepo\InnocenceEngine` → `C--GitRepo-InnocenceEngine`.
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
    'Routing by content shape:',
    '  Dispatcher rule              → .claude/skills/dispatch-briefs/SKILL.md',
    '  Project-state snapshot       → .claude/state/<topic>.md',
    '  Task cross-session continuity→ ## Implementation Notes in .backlog/tasks/<task>.md',
    '  High-cost enforcement        → new gate under .claude/hooks/gates/<name>.js',
    '  Ephemeral context            → do not persist',
    '',
  ].join('\n'))
  process.exit(2)
}

module.exports = { run, autoMemoryDir, normalise }
