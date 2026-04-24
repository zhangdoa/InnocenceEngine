// Shared constants, regexes, and git helpers used by the commit-gate
// dispatcher and per-gate modules. Keep this file small — if a helper
// is used by only one gate, keep it with that gate.

const fs = require('fs')
const { execSync } = require('child_process')

// Skip sentinels — escape hatches included in the commit message.
const SKIP_SENTINEL = '[skip-test-gate]'       // test-run / live-engine / serialize-test / paper-port
const SKIP_SIZE_SENTINEL = '[skip-size-gate]'  // file-size gate only

// Documents/commit-message-policy.md requires one of these headers on
// every AI-authored commit.
const ATTRIBUTION_RE = /^(Code-AI-Generated-By|Message-AI-Generated-By):\s*\S/m

// Which tests count as "integration test ran this turn".
const QUALIFYING_TEST = new RegExp([
  String.raw`npx\s+playwright\s+test`,
  String.raw`Main\.exe\b[^|&;]*-(total_frames|reload_at_frame|bake|capture_frame)\b`,
  String.raw`RenderTest\.exe\b[^|&;]*-test\b`,
  String.raw`InteractiveTest\.ps1`,
  String.raw`Main\.exe\b[^|&;]*-serialize_test\b`,
].join('|'))

// Tests that satisfy the live-engine requirement for editor-facing CLs
// without going through Playwright.
const NON_PLAYWRIGHT_LIVE = new RegExp([
  String.raw`Main\.exe\b[^|&;]*-(total_frames|reload_at_frame|bake|capture_frame)\b`,
  String.raw`RenderTest\.exe\b[^|&;]*-test\b`,
  String.raw`InteractiveTest\.ps1`,
].join('|'))

const PLAYWRIGHT_RE = /npx\s+playwright\s+test(?:\b|$)([^|&;\n]*)/

// A staged backlog task's diff has this line added when the task is
// flipping to `status: Done`. `-U0` on the caller's diff keeps context
// lines out so an unchanged neighbouring `status:` can't match.
const STATUS_DONE_ADDED_RE = /^\+status:\s*Done\b/mi

const DOCS_ONLY_PATH = /^\.backlog\/|^Documents\/|\.md$|^\.claude\/|^\.alignments\/|\.gitignore$/

// Staged-file paths that require live-engine validation.
const EDITOR_CODE_PATH = /^Source\/(Editor-Next\/src\/|Engine\/Services\/EditorService\.)/

// Staged-file paths that require the serialize-determinism test.
const SERIALIZER_CODE_PATH = /^Source\/Engine\/(ThirdParty\/JSONWrapper\/|Services\/(AssetService|SceneService)\.)/

// File-size-gate configuration.
const FILE_SIZE_LIMIT = 400
const FILE_SIZE_EXT_RE = /\.(cpp|hpp|h|c|cc|cxx|inl|hlsl|hlsli|comp|py|js|mjs|ts|ps1|sh|bash|zsh)$/i
const FILE_SIZE_EXCLUDE_RE = /(^|\/)(ThirdParty|External|node_modules|Generated|dist)\//

// Harness-generated pseudo-prompts masquerade as user messages in the
// transcript; filter them so "last real user prompt" lands where a
// human actually typed something.
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

function firstArray(...xs) {
  for (const x of xs) if (Array.isArray(x)) return x
  return []
}

// Line count of a git blob spec (`:path` = staged, `HEAD:path` = HEAD).
// Returns 0 for a missing blob (new file / deleted).
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

// Parse a staged backlog task's frontmatter for id + labels.
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

// Detect backlog task files whose staged diff contains +status: Done.
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

// Collect the effective commit message text from both inline flags and
// any -F / --file argument that points at a message file.
function collectCommitMessageText(cmd, cwd) {
  let text = cmd
  const fileArg = cmd.match(/\s(?:-F|--file|-c|--template)\s+("[^"]+"|'[^']+'|\S+)/)
  if (fileArg) {
    const p = fileArg[1].replace(/^['"]|['"]$/g, '')
    try {
      const abs = require('path').isAbsolute(p) ? p : require('path').join(cwd, p)
      text += '\n' + fs.readFileSync(abs, 'utf8')
    } catch { /* unreadable — attribution gate will fail naturally */ }
  }
  return text
}

module.exports = {
  SKIP_SENTINEL, SKIP_SIZE_SENTINEL,
  ATTRIBUTION_RE, QUALIFYING_TEST, NON_PLAYWRIGHT_LIVE, PLAYWRIGHT_RE,
  STATUS_DONE_ADDED_RE, DOCS_ONLY_PATH,
  EDITOR_CODE_PATH, SERIALIZER_CODE_PATH,
  FILE_SIZE_LIMIT, FILE_SIZE_EXT_RE, FILE_SIZE_EXCLUDE_RE,
  isRealUserPrompt, firstArray,
  blobLineCount, readStagedTaskFrontmatter, detectClosingTasks,
  collectCommitMessageText,
}
