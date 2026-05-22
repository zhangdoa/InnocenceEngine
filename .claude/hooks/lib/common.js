// Shared constants and helpers for commit-gate + session-gate dispatchers.

const fs = require('fs')
const { execSync } = require('child_process')

const ATTRIBUTION_RE = /^(Code-AI-Generated-By|Message-AI-Generated-By):\s*\S/m

const QUALIFYING_TEST = new RegExp([
  String.raw`npx\s+playwright\s+test`,
  String.raw`Main\.exe\b[^|&;]*-(total_frames|reload_at_frame|bake|capture_frame)\b`,
  String.raw`RenderTest\.exe\b[^|&;]*-test\b`,
  String.raw`InteractiveTest\.ps1`,
  String.raw`Main\.exe\b[^|&;]*-serialize_test\b`,
].join('|'))

const NON_PLAYWRIGHT_LIVE = new RegExp([
  String.raw`Main\.exe\b[^|&;]*-(total_frames|reload_at_frame|bake|capture_frame)\b`,
  String.raw`RenderTest\.exe\b[^|&;]*-test\b`,
  String.raw`InteractiveTest\.ps1`,
].join('|'))

const PLAYWRIGHT_RE = /npx\s+playwright\s+test(?:\b|$)([^|&;\n]*)/

// `-U0` upstream keeps context lines out so an unchanged neighbouring `status:` doesn't match.
const STATUS_DONE_ADDED_RE = /^\+status:\s*Done\b/mi

const DOCS_ONLY_PATH = /^\.backlog\/|\.md$|^\.claude\/|\.gitignore$/

const EDITOR_CODE_PATH = /^Source\/(Editor-Next\/src\/|Engine\/Services\/EditorService\.)/

const SERIALIZER_CODE_PATH = /^Source\/Engine\/(ThirdParty\/JSONWrapper\/|Services\/(AssetService|SceneService)\.)/

const FILE_SIZE_LIMIT = 300
const FILE_SIZE_EXT_RE = /\.(cpp|hpp|h|c|cc|cxx|inl|hlsl|hlsli|comp|py|js|mjs|ts|ps1|sh|bash|zsh)$/i
const FILE_SIZE_EXCLUDE_RE = /(^|\/)(ThirdParty|External|node_modules|Generated|dist)\//

// Harness-generated pseudo-prompts masquerade as user messages — filter so
// "last real user prompt" lands where a human actually typed.
const PSEUDO_PROMPT_PREFIXES = [
  '<task-notification>',
  '<system-reminder>',
  '<command-message>',
  '<command-name>',
  '<local-command',
]

function isRealUserPrompt(content) {
  if (typeof content !== 'string') return false
  const trimmed = content.trimStart()
  for (const p of PSEUDO_PROMPT_PREFIXES) {
    if (trimmed.startsWith(p)) return false
  }
  return true
}

// `Agent` is the current serialization; older / future variants use `Task`. Accept either.
function isTaskMgmtAgentCall(toolName, toolInput) {
  if (toolName !== 'Agent' && toolName !== 'Task') return false
  return (toolInput?.subagent_type || '') === 'task-mgmt'
}

function scanTranscriptForTaskMgmtBrief(xpPath) {
  const out = { taskMgmtSeen: false, hasRealUserPrompt: false }
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
    const content = m.message?.content
    if (!Array.isArray(content)) continue
    for (const block of content) {
      if (block?.type !== 'tool_use') continue
      if (isTaskMgmtAgentCall(block.name, block.input)) {
        out.taskMgmtSeen = true
      }
    }
    if (out.taskMgmtSeen && out.hasRealUserPrompt) break
  }
  return out
}

function firstArray(...xs) {
  for (const x of xs) if (Array.isArray(x)) return x
  return []
}

function blobLineCount(cwd, spec) {
  const escaped = spec.replace(/"/g, '\\"')
  try {
    const content = execSync(`git -c core.quotePath=false show "${escaped}"`,
      { cwd, encoding: 'utf8', stdio: ['pipe', 'pipe', 'pipe'] })
    const parts = content.split('\n')
    if (parts.length > 0 && parts[parts.length - 1] === '') parts.pop()
    return parts.length
  } catch {
    return 0
  }
}

function readStagedTaskFrontmatter(cwd, file) {
  try {
    const escaped = file.replace(/"/g, '\\"')
    const content = execSync(
      `git -c core.quotePath=false show ":${escaped}"`,
      { cwd, encoding: 'utf8', stdio: ['pipe', 'pipe', 'pipe'] }
    )
    const fm = content.match(/^---\r?\n([\s\S]*?)\r?\n---/)
    if (!fm) return null
    const body = fm[1]
    const idMatch = body.match(/^id:\s*(\S+)/mi)
    const labels = []
    const labelsBlock = body.match(/^labels:\s*\r?\n((?:\s*-\s*.+\r?\n?)+)/mi)
    if (labelsBlock) {
      for (const line of labelsBlock[1].split(/\r?\n/)) {
        const m = line.match(/^\s*-\s*(.+?)\s*$/)
        if (m) labels.push(m[1].replace(/^['"]|['"]$/g, ''))
      }
    }
    return { id: idMatch ? idMatch[1] : null, labels }
  } catch { return null }
}

function detectClosingTasks(cwd, staged) {
  const backlogFiles = staged.filter(f => f.startsWith('.backlog/tasks/'))
  const closing = []
  for (const f of backlogFiles) {
    try {
      const escaped = f.replace(/"/g, '\\"')
      const diff = execSync(
        `git -c core.quotePath=false diff --cached -U0 -- "${escaped}"`,
        { cwd, encoding: 'utf8' }
      )
      if (STATUS_DONE_ADDED_RE.test(diff)) closing.push(f)
    } catch { /* per-file diff unavailable — skip */ }
  }
  return closing
}

// When `git commit` originates from a sub-agent sidechain, Claude Code passes the PARENT
// session's transcript_path. The transcript-dependent gates would then scan the wrong file.
// Match the in-flight command against every sub-agent JSONL's last assistant Bash tool_use;
// on a unique match swap that JSONL in. Falls back to parent on miss/ambiguity.
function resolveActiveTranscriptPath(xpFromHook, currentCmd) {
  if (!xpFromHook || !currentCmd) return xpFromHook
  const { resolveActiveSubagentTranscript } = require('./subagent-transcript')
  return resolveActiveSubagentTranscript(xpFromHook, (b) =>
    b?.type === 'tool_use' && b?.name === 'Bash' && (b.input?.command || '') === currentCmd
  )
}

// Handle Git Bash MSYS-style paths (/c/...) — Node fs reads them as drive-relative
// (C:\c\...) and ENOENTs. Translate /<letter>/... → <letter>:/... and retry.
function collectCommitMessageText(cmd, cwd) {
  const text0 = cmd
  const fileArg = cmd.match(/\s(?:-F|--file|-c|--template)\s+("[^"]+"|'[^']+'|\S+)/)
  if (!fileArg) return { text: text0, fileError: null }

  const path = require('path')
  const rawPath = fileArg[1].replace(/^['"]|['"]$/g, '')

  const candidates = []
  const native = path.isAbsolute(rawPath) ? rawPath : path.join(cwd, rawPath)
  candidates.push(native)
  const msysTranslated = rawPath.replace(/^\/([a-zA-Z])\//, '$1:/')
  if (msysTranslated !== rawPath) candidates.push(msysTranslated)

  const attempts = []
  for (const candidate of candidates) {
    try {
      const content = fs.readFileSync(candidate, 'utf8')
      return { text: text0 + '\n' + content, fileError: null }
    } catch (err) {
      attempts.push({ path: candidate, code: err.code || 'EUNKNOWN', message: err.message })
    }
  }
  return { text: text0, fileError: { rawPath, attempts } }
}

module.exports = {
  ATTRIBUTION_RE, QUALIFYING_TEST, NON_PLAYWRIGHT_LIVE, PLAYWRIGHT_RE,
  STATUS_DONE_ADDED_RE, DOCS_ONLY_PATH,
  EDITOR_CODE_PATH, SERIALIZER_CODE_PATH,
  FILE_SIZE_LIMIT, FILE_SIZE_EXT_RE, FILE_SIZE_EXCLUDE_RE,
  isRealUserPrompt, firstArray,
  isTaskMgmtAgentCall, scanTranscriptForTaskMgmtBrief,
  blobLineCount, readStagedTaskFrontmatter, detectClosingTasks,
  collectCommitMessageText, resolveActiveTranscriptPath,
}
