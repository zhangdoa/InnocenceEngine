// Shared constants, regexes, and git helpers used by the commit-gate
// dispatcher and per-gate modules. Keep this file small — if a helper
// is used by only one gate, keep it with that gate.

const fs = require('fs')
const { execSync } = require('child_process')

// .claude/skills/commit-message-policy/SKILL.md requires one of these
// headers on every AI-authored commit.
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

const DOCS_ONLY_PATH = /^\.backlog\/|\.md$|^\.claude\/|^\.alignments\/|\.gitignore$/

// Staged-file paths that require live-engine validation.
const EDITOR_CODE_PATH = /^Source\/(Editor-Next\/src\/|Engine\/Services\/EditorService\.)/

// Staged-file paths that require the serialize-determinism test.
const SERIALIZER_CODE_PATH = /^Source\/Engine\/(ThirdParty\/JSONWrapper\/|Services\/(AssetService|SceneService)\.)/

// File-size-gate configuration.
const FILE_SIZE_LIMIT = 300
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

// `Agent` is the current Claude Code serialization for the subagent dispatch
// tool; older / future variants may use `Task`. Accept either.
function isTaskMgmtAgentCall(toolName, toolInput) {
  if (toolName !== 'Agent' && toolName !== 'Task') return false
  return (toolInput?.subagent_type || '') === 'task-mgmt'
}

// Scan a transcript JSONL for (1) any prior Agent(subagent_type=task-mgmt)
// tool_use, and (2) at least one real (non-pseudo) user prompt. Used by the
// task-mgmt-briefing gates on PreToolUse and SessionStart.
//
// Returns { taskMgmtSeen, hasRealUserPrompt } or null on I/O error.
//
// The `hasRealUserPrompt` distinction matters because subagent transcripts
// only contain the synthetic prompt the parent passed; gates that govern
// "the main session" must fail open on subagent transcripts.
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

// When a `git commit` Bash call originates from a sub-agent (sidechain),
// Claude Code passes the PARENT session's transcript path as
// `transcript_path` — not the sidechain's own JSONL. The transcript-
// dependent gates then scan the wrong file: the sub-agent's test runs,
// task-closing intent, and qualifying tool_use blocks all live in
// `<parent-dir>/<sessionId>/subagents/agent-<agentId>.jsonl`, invisible
// to a parent-only scan. Result: every test-validated sub-agent commit
// gets blocked despite legitimate evidence.
//
// Resolution: detect the active sidechain by matching the in-flight
// command. Sub-agent JSONLs sit in a predictable location; the in-flight
// commit appears as the LAST assistant Bash `tool_use` in exactly one of
// them (no following tool_result yet — we are in PreToolUse). If a
// unique match exists, swap that path in for the gate's transcript scan;
// the gate's own semantics are preserved (each sub-agent must show its
// own evidence in its own transcript), and chained dispatches work
// transparently because the sub-agents/ directory is flat.
//
// Fails open: returns the original `xpFromHook` if the sub-agents
// directory is absent, no candidate matches, or multiple match
// (ambiguous). The transcript-dependent gates then run against the
// parent transcript as before.
function resolveActiveTranscriptPath(xpFromHook, currentCmd) {
  if (!xpFromHook || !currentCmd) return xpFromHook
  const { resolveActiveSubagentTranscript } = require('./subagent-transcript')
  return resolveActiveSubagentTranscript(xpFromHook, (b) =>
    b?.type === 'tool_use' && b?.name === 'Bash' && (b.input?.command || '') === currentCmd
  )
}

// Collect the effective commit message text from both inline flags and
// any -F / --file / -c / --template argument that points at a message file.
//
// Returns { text, fileError }:
//   - `text`     — `cmd` plus any successfully-read file content.
//   - `fileError`— null on success (or no -F arg). When -F was passed but
//                  every read attempt failed, an object describing the
//                  attempts so the dispatcher can block loudly. The gate
//                  must NOT silently fall through to "empty message" —
//                  that hides bugs (per feedback_silent_failures.md).
//
// MSYS handling: Git Bash on Windows produces absolute paths of the form
// `/c/GitRepo/...` (POSIX-style with drive letter). Node's Win32 path
// parser flags those as absolute, so the join-with-cwd branch is skipped,
// but `fs.readFileSync('/c/GitRepo/...')` resolves it as drive-relative
// (`C:\c\GitRepo\...`) and fails with ENOENT. Translate `/<letter>/...`
// → `<letter>:/...` and retry before declaring the path unreadable.
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
