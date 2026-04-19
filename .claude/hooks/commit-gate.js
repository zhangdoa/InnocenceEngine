#!/usr/bin/env node
/**
 * PreToolUse hook: gate Bash `git commit`.
 *
 * Hook contract:
 *   stdin  = JSON { session_id, transcript_path, cwd, hook_event_name,
 *                   tool_name, tool_input: { command, description } }
 *   exit 0 = allow the tool call
 *   exit 2 = block the tool call; stderr is shown to Claude (and the user)
 *
 * Two gates, both must pass:
 *
 * 1. Test-run gate — allow if any of:
 *    - transcript since last user message contains a Bash call matching
 *      QUALIFYING_TEST (playwright, Main.exe with frame flags, RenderTest,
 *      InteractiveTest.ps1)
 *    - all staged paths match DOCS_ONLY_PATH
 *    - commit message contains SKIP_SENTINEL
 *
 * 2. Attribution gate — commit message must contain Code-AI-Generated-By:
 *    or Message-AI-Generated-By: per Documents/commit-message-policy.md.
 *    No escape; every Claude-issued commit is AI-authored.
 *
 * Fails OPEN on any internal error so a hook bug never bricks commits.
 */

const fs = require('fs')
const { execSync } = require('child_process')

const QUALIFYING_TEST = new RegExp([
  // Editor integration — Playwright specs in Source/Editor-Next/tests
  String.raw`npx\s+playwright\s+test`,
  // Engine runtime — Main.exe invoked with frame flags
  String.raw`Main\.exe\b[^|&;]*-(total_frames|reload_at_frame|bake|capture_frame)\b`,
  // RenderTest smoke
  String.raw`RenderTest\.exe\b[^|&;]*-test\b`,
  // Interactive scripted test
  String.raw`InteractiveTest\.ps1`,
].join('|'))

const DOCS_ONLY_PATH = /^\.backlog\/|^Documents\/|\.md$|^\.claude\/|\.gitignore$/

const SKIP_SENTINEL = '[skip-test-gate]'

// Documents/commit-message-policy.md requires one of these headers on
// every AI-authored commit. Matched on a commit-message line.
const ATTRIBUTION_RE = /^(Code-AI-Generated-By|Message-AI-Generated-By):\s*\S/m

let raw = ''
process.stdin.setEncoding('utf8')
process.stdin.on('data', (d) => { raw += d })
process.stdin.on('end', () => main().catch(failOpen))

async function main() {
  let input
  try { input = JSON.parse(raw) } catch { return process.exit(0) }
  if (input.tool_name !== 'Bash') return process.exit(0)
  const cmd = input.tool_input?.command || ''
  // Strip quoted string literals before matching so `echo "git commit"` or
  // any command that echoes the phrase inside a test harness doesn't trip
  // the gate. A real commit always has the phrase as an actual command
  // token, outside any quote context.
  const unquoted = cmd
    .replace(/"(?:\\.|[^"\\])*"/g, '""')
    .replace(/'(?:[^'])*'/g, "''")
  if (!/\bgit\s+commit\b/.test(unquoted)) return process.exit(0)

  const cwd = input.cwd || process.cwd()

  // Escape hatch — the commit message itself asks us to skip.
  if (cmd.includes(SKIP_SENTINEL)) {
    process.stderr.write(`[commit-gate] ${SKIP_SENTINEL} in commit — skipping test check\n`)
    return process.exit(0)
  }
  // Also recognise sentinel in a -F message file.
  const fMatch = cmd.match(/-F\s+("[^"]+"|'[^']+'|\S+)/)
  if (fMatch) {
    const file = fMatch[1].replace(/^['"]|['"]$/g, '')
    try {
      const body = fs.readFileSync(file, 'utf8')
      if (body.includes(SKIP_SENTINEL)) {
        process.stderr.write(`[commit-gate] ${SKIP_SENTINEL} in ${file} — skipping test check\n`)
        return process.exit(0)
      }
    } catch { /* missing or unreadable commit-message file — treat as no sentinel */ }
  }

  // Docs-only escape.
  let stagedRaw = ''
  try {
    stagedRaw = execSync('git diff --cached --name-only', { cwd, encoding: 'utf8' })
  } catch { /* no git; nothing we can do */ }
  const staged = stagedRaw.split('\n').map(s => s.trim()).filter(Boolean)
  if (staged.length > 0 && staged.every(f => DOCS_ONLY_PATH.test(f))) {
    return process.exit(0)
  }

  // Real test check — walk the transcript from the last user message.
  const xp = input.transcript_path
  if (!xp || !fs.existsSync(xp)) return failOpen(new Error('transcript not accessible'))
  const lines = fs.readFileSync(xp, 'utf8').split('\n').filter(Boolean)
  let lastUserIdx = -1
  for (let i = lines.length - 1; i >= 0; i--) {
    try {
      const m = JSON.parse(lines[i])
      const t = m.type || m.role || m.message?.role
      if (t === 'user') { lastUserIdx = i; break }
    } catch { /* skip */ }
  }

  let ran = false
  for (let i = lastUserIdx + 1; i < lines.length; i++) {
    try {
      const m = JSON.parse(lines[i])
      const blocks = firstArray(m.message?.content, m.content)
      for (const b of blocks) {
        if (b?.type === 'tool_use' && b?.name === 'Bash') {
          const c = b.input?.command || ''
          if (QUALIFYING_TEST.test(c)) { ran = true; break }
        }
      }
      if (ran) break
    } catch { /* skip */ }
  }
  if (!ran) {
    blockNoTest(staged)
    return
  }

  // Attribution gate. If the commit message — whether inline via -m /
  // HEREDOC or via -F <file> — contains a qualifying attribution header,
  // allow. Otherwise block.
  const message = collectCommitMessageText(cmd, cwd)
  if (ATTRIBUTION_RE.test(message)) return process.exit(0)
  blockMissingAttribution()
}

function collectCommitMessageText(cmd, cwd) {
  // Start with the whole command (covers -m "..." and HEREDOC bodies that
  // appear inline).
  let text = cmd
  // Pull in the contents of any -F / -c / --file / --template argument.
  const fileArg = cmd.match(/\s(?:-F|--file|-c|--template)\s+("[^"]+"|'[^']+'|\S+)/)
  if (fileArg) {
    const p = fileArg[1].replace(/^['"]|['"]$/g, '')
    try {
      const abs = require('path').isAbsolute(p) ? p : require('path').join(cwd, p)
      text += '\n' + require('fs').readFileSync(abs, 'utf8')
    } catch { /* unreadable file — attribution check will simply fail */ }
  }
  return text
}

function blockNoTest(staged) {
  const filesList = staged.length
    ? staged.slice(0, 10).map(f => '  ' + f).join('\n') + (staged.length > 10 ? `\n  …and ${staged.length - 10} more` : '')
    : '  (no staged files detected)'
  process.stderr.write([
    '',
    '[commit-gate] git commit blocked — no integration test run in this turn.',
    '',
    'Staged files:',
    filesList,
    '',
    'Run one of the following in this turn before committing (or stage only docs/backlog/.claude):',
    '  • npx playwright test tests/<spec>.spec.js        (editor integration)',
    '  • Bin\\RelWithDebInfo\\Main.exe -total_frames N    (engine integration)',
    '  • Bin\\RelWithDebInfo\\RenderTest.exe -test <name> (render smoke)',
    '',
    `Escape hatch: include ${SKIP_SENTINEL} in the commit message if this commit`,
    'legitimately cannot be validated by a test (commit-message-only edit, hook fix, etc).',
    '',
  ].join('\n'))
  process.exit(2)
}

function blockMissingAttribution() {
  process.stderr.write([
    '',
    '[commit-gate] git commit blocked — attribution header missing.',
    '',
    'Per Documents/commit-message-policy.md, every Claude-issued commit must',
    'end with one of:',
    '  Code-AI-Generated-By: <model name>',
    '  Message-AI-Generated-By: <model name>',
    '',
    'Add the line to the commit message (inline with -m, or in the file passed',
    'to -F / -c) and retry.',
    '',
  ].join('\n'))
  process.exit(2)
}

function firstArray(...xs) {
  for (const x of xs) if (Array.isArray(x)) return x
  return []
}

function failOpen(err) {
  process.stderr.write(`[commit-gate] internal error — failing open: ${err?.message || err}\n`)
  process.exit(0)
}
