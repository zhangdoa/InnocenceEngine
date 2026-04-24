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
 * Five gates; all must pass:
 *
 * 1. Test-run gate — allow if any of:
 *    - transcript since last user message contains a Bash call matching
 *      QUALIFYING_TEST (playwright, Main.exe with frame flags, RenderTest,
 *      InteractiveTest.ps1)
 *    - all staged paths match DOCS_ONLY_PATH *and* no staged backlog task
 *      is flipping to `status: Done` (see gate 2)
 *    - commit message contains SKIP_SENTINEL
 *
 * 2. Closure-evidence gate — if any staged .backlog/tasks/*.md file has a
 *    `+status: Done` line in its staged diff (new file with Done, or
 *    existing task flipping In Progress / To Do → Done), the docs-only
 *    bypass from gate 1 does NOT apply. Closing a task asserts the work
 *    is validated; the claim must be backed by an integration-test run
 *    in the current turn. SKIP_SENTINEL still escapes for legitimate
 *    cases (abandoned work, retro housekeeping, superseded tasks).
 *
 * 3. File-size gate — soft ratchet against code/script files growing
 *    unchecked. For each staged code/script file (cpp/hpp/h/c/cc/cxx/
 *    inl/hlsl/hlsli/comp/py/js/mjs/ts/ps1/sh), compare staged-blob line
 *    count against HEAD blob line count; block if `new > FILE_SIZE_LIMIT`
 *    AND `new > old`. Files already over the limit can still be edited —
 *    just not grown further — so the threshold actively pressures size
 *    down over time without retroactively invalidating existing work.
 *    Vendored / generated trees (ThirdParty, External, dist, node_modules,
 *    Generated) are excluded. SKIP_SIZE_SENTINEL escapes for legitimate
 *    one-off additions where splitting is inappropriate.
 *
 * 4. Live-engine gate — if any staged file is editor-facing code
 *    (EDITOR_CODE_PATH), require one of:
 *    - a Playwright run against a spec that spawns the real engine
 *      (detected by `--engine=Main` in the spec source, or by running
 *      the full suite with no file argument)
 *    - a Main.exe frame-run, RenderTest, or InteractiveTest invocation
 *    - SKIP_SENTINEL in the commit message
 *   Mock-only Playwright specs alone don't qualify — they hide the
 *   optimistic-vs-server-truth races that only surface against a live
 *   engine.
 *
 * 5. Attribution gate — commit message must contain Code-AI-Generated-By:
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
  // Serialization determinism test
  String.raw`Main\.exe\b[^|&;]*-serialize_test\b`,
].join('|'))

const DOCS_ONLY_PATH = /^\.backlog\/|^Documents\/|\.md$|^\.claude\/|\.gitignore$/

// Staged-file paths that require live-engine validation: any change
// under the editor source tree or the IPC-facing engine service.
const EDITOR_CODE_PATH = /^Source\/(Editor-Next\/src\/|Engine\/Services\/EditorService\.)/

// Staged-file paths that require the serialize-determinism test.
// Any change to the JSON serializer or scene-service surfaces a round-trip risk.
const SERIALIZER_CODE_PATH = /^Source\/Engine\/(ThirdParty\/JSONWrapper\/|Services\/(AssetService|SceneService)\.)/

// Engine-truth tests that don't go through Playwright: if one of these
// ran we treat the live-engine gate as satisfied even without a
// browser-side Playwright invocation.
const NON_PLAYWRIGHT_LIVE = new RegExp([
  String.raw`Main\.exe\b[^|&;]*-(total_frames|reload_at_frame|bake|capture_frame)\b`,
  String.raw`RenderTest\.exe\b[^|&;]*-test\b`,
  String.raw`InteractiveTest\.ps1`,
].join('|'))

const PLAYWRIGHT_RE = /npx\s+playwright\s+test(?:\b|$)([^|&;\n]*)/

const SKIP_SENTINEL = '[skip-test-gate]'

// File-size gate configuration.
// Limit: 400 lines. Chosen so the current worst shader offenders
// (RadianceCacheRayGen.hlsl @ 429, GPUPathTracerRayGen.hlsl @ 468) are
// flagged, while typical C++ service files (100–300 lines) have headroom
// before the gate kicks in.
const FILE_SIZE_LIMIT = 400
const FILE_SIZE_EXT_RE = /\.(cpp|hpp|h|c|cc|cxx|inl|hlsl|hlsli|comp|py|js|mjs|ts|ps1|sh|bash|zsh)$/i
const FILE_SIZE_EXCLUDE_RE = /(^|\/)(ThirdParty|External|node_modules|Generated|dist)\//
const SKIP_SIZE_SENTINEL = '[skip-size-gate]'

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
  // `-c core.quotePath=false` keeps non-ASCII paths unquoted so the
  // DOCS_ONLY_PATH regex actually matches them (git would otherwise wrap
  // paths containing e.g. em-dash or × in double quotes + octal escapes,
  // and every `^\.backlog\/` / `^Documents\/` test would fail).
  let stagedRaw = ''
  try {
    stagedRaw = execSync('git -c core.quotePath=false diff --cached --name-only', { cwd, encoding: 'utf8' })
  } catch { /* no git; nothing we can do */ }
  const staged = stagedRaw.split('\n').map(s => s.trim()).filter(Boolean)

  // Closure-evidence gate. Detect backlog task files whose staged diff
  // flips `status:` to Done — i.e., the commit is *claiming completion*.
  // These do not get the docs-only bypass below: a completion claim must
  // be backed by a test run in the current turn.
  const closingTasks = detectClosingTasks(cwd, staged)

  // File-size gate. Runs before the docs-only bypass so it applies to
  // every CL that touches code/script files, independent of the test
  // story. The commit-message-level sentinel escapes it explicitly.
  if (!cmd.includes(SKIP_SIZE_SENTINEL)) {
    const msgText = collectCommitMessageText(cmd, cwd)
    if (!msgText.includes(SKIP_SIZE_SENTINEL)) {
      const sizeViolations = detectFileSizeViolations(cwd, staged)
      if (sizeViolations.length > 0) {
        blockFileSize(sizeViolations)
        return
      }
    }
  }

  if (closingTasks.length === 0
      && staged.length > 0
      && staged.every(f => DOCS_ONLY_PATH.test(f))) {
    return process.exit(0)
  }

  // Real test check — walk the transcript from the last *real user
  // prompt*. The transcript uses type: "user" for several distinct
  // things: tool-result feedback (content = array of tool_result blocks),
  // harness pseudo-prompts (<task-notification>, <system-reminder>,
  // <command-message>, <local-command*>), and actual user prompts
  // (string content). Only the last kind should bound the scan window,
  // otherwise we keep stopping at tool results from seconds ago and
  // miss the test that ran before them.
  const xp = input.transcript_path
  if (!xp || !fs.existsSync(xp)) return failOpen(new Error('transcript not accessible'))
  const lines = fs.readFileSync(xp, 'utf8').split('\n').filter(Boolean)
  let lastUserIdx = -1
  for (let i = lines.length - 1; i >= 0; i--) {
    try {
      const m = JSON.parse(lines[i])
      const t = m.type || m.role || m.message?.role
      if (t !== 'user') continue
      if (!isRealUserPrompt(m.message?.content ?? m.content)) continue
      lastUserIdx = i
      break
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
    if (closingTasks.length > 0) {
      blockNoCloseTest(closingTasks)
    } else {
      blockNoTest(staged)
    }
    return
  }

  // Live-engine gate. Only fires when editor-facing code is staged.
  const editorCodeStaged = staged.some(f => EDITOR_CODE_PATH.test(f))
  if (editorCodeStaged) {
    const liveRan = didLiveEngineTestRun(lines, lastUserIdx, cwd)
    if (!liveRan) {
      blockNoLiveEngine(staged)
      return
    }
  }

  // Serialize-test gate. Fires when JSONWrapper / AssetService / SceneService
  // is staged; requires a serialize-test run in the current turn.
  const serializerStaged = staged.some(f => SERIALIZER_CODE_PATH.test(f))
  if (serializerStaged) {
    const serializeRan = didSerializeTestRun(lines, lastUserIdx)
    if (!serializeRan) {
      blockNoSerializeTest(staged)
      return
    }
  }

  // Attribution gate. If the commit message — whether inline via -m /
  // HEREDOC or via -F <file> — contains a qualifying attribution header,
  // allow. Otherwise block.
  const message = collectCommitMessageText(cmd, cwd)
  if (ATTRIBUTION_RE.test(message)) return process.exit(0)
  blockMissingAttribution()
}

function didLiveEngineTestRun(lines, lastUserIdx, cwd) {
  const path = require('path')
  const editorDir = path.join(cwd, 'Source', 'Editor-Next')

  for (let i = lastUserIdx + 1; i < lines.length; i++) {
    let m
    try { m = JSON.parse(lines[i]) } catch { continue }
    const blocks = firstArray(m.message?.content, m.content)
    for (const b of blocks) {
      if (b?.type !== 'tool_use' || b?.name !== 'Bash') continue
      const c = b.input?.command || ''
      if (NON_PLAYWRIGHT_LIVE.test(c)) return true

      const pw = c.match(PLAYWRIGHT_RE)
      if (!pw) continue
      // Playwright invocation; classify by the file arguments (if any).
      const args = (pw[1] || '').trim()
      if (!args) return true // running the whole suite → includes live specs
      const files = args.split(/\s+/).filter(s => s && !s.startsWith('-'))
      if (files.length === 0) return true // flags only, no file filter
      // At least one arg must be a live-engine spec for the run to
      // qualify. Resolve relative to Source/Editor-Next (the usual
      // playwright cwd) and grep for `--engine=Main` in the source.
      for (const f of files) {
        const abs = path.isAbsolute(f) ? f : path.join(editorDir, f)
        try {
          if (fs.readFileSync(abs, 'utf8').includes('--engine=Main')) return true
        } catch { /* unreadable — skip */ }
      }
    }
  }
  return false
}

function blockNoLiveEngine(staged) {
  const filesList = staged.length
    ? staged.filter(f => EDITOR_CODE_PATH.test(f)).slice(0, 10).map(f => '  ' + f).join('\n')
    : '  (no editor code detected — bug?)'
  process.stderr.write([
    '',
    '[commit-gate] git commit blocked — editor code staged but no live-engine test ran.',
    '',
    'Editor-facing staged paths:',
    filesList,
    '',
    'Mock-only Playwright specs (scene-vertical, inspector-rotation, theme-reactivity,',
    'ipc-contract, ux-audit) hide optimistic-vs-server-truth races. Run at least one of:',
    '  • npx playwright test                                   (full suite — includes live)',
    '  • npx playwright test tests/render-toggles.spec.js      (live engine)',
    '  • npx playwright test tests/render-target-debugger.spec.js',
    '  • npx playwright test tests/scene-load.spec.js',
    '  • npx playwright test tests/editor.spec.js',
    '  • npx playwright test tests/window-menu.spec.js',
    '  • Bin\\RelWithDebInfo\\Main.exe -total_frames N          (engine frame-run)',
    '  • Bin\\RelWithDebInfo\\RenderTest.exe -test <name>',
    '',
    `Escape hatch: include ${SKIP_SENTINEL} if this commit genuinely cannot`,
    'be validated end-to-end (e.g. a typo fix in a comment).',
    '',
  ].join('\n'))
  process.exit(2)
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

// Harness-generated pseudo-prompts masquerade as user messages in the
// transcript. Filter them so the "last real user prompt" marker lands
// where a human actually typed something.
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

// Matches an added line (single leading '+', never the '+++' file header)
// in a staged diff that writes `status: Done`. Case-insensitive on the key
// so `Status: Done` is caught too; value match is strict `Done` with a word
// boundary so `Done-ish` or `Doner` don't qualify.
const STATUS_DONE_ADDED_RE = /^\+status:\s*Done\b/mi

function detectClosingTasks(cwd, staged) {
  const backlogFiles = staged.filter(f => f.startsWith('.backlog/tasks/'))
  const closing = []
  for (const f of backlogFiles) {
    try {
      const escaped = f.replace(/"/g, '\\"')
      // -U0: drop context lines so a neighbouring unchanged `status:` line
      // in a task that isn't flipping status can't match our regex.
      const diff = execSync(
        `git -c core.quotePath=false diff --cached -U0 -- "${escaped}"`,
        { cwd, encoding: 'utf8' }
      )
      if (STATUS_DONE_ADDED_RE.test(diff)) closing.push(f)
    } catch { /* per-file diff unavailable — skip */ }
  }
  return closing
}

function blockNoCloseTest(closingTasks) {
  const filesList = closingTasks.slice(0, 10).map(f => '  ' + f).join('\n')
    + (closingTasks.length > 10 ? `\n  …and ${closingTasks.length - 10} more` : '')
  process.stderr.write([
    '',
    '[commit-gate] git commit blocked — task closure without integration test.',
    '',
    'Task(s) flipping to status: Done this commit:',
    filesList,
    '',
    'Closing a task asserts the work is validated. The docs-only bypass does',
    'NOT apply to a completion claim — a closing CL must be backed by a test',
    'run in the current turn, same as a code CL.',
    '',
    'Run one of the following in this turn before committing:',
    '  • Bin\\RelWithDebInfo\\Main.exe -total_frames N',
    '  • Bin\\RelWithDebInfo\\Main.exe -total_frames N -reload_at_frame M',
    '  • Bin\\RelWithDebInfo\\RenderTest.exe -test <name>',
    '  • Bin\\RelWithDebInfo\\Main.exe -capture_frame N',
    '  • InteractiveTest.ps1',
    '',
    `Escape hatch: include ${SKIP_SENTINEL} if this closure genuinely cannot`,
    'be validated by a test (abandoned/superseded task, retro housekeeping).',
    '',
  ].join('\n'))
  process.exit(2)
}

// Line count of a git blob spec (e.g. `:path` for staged, `HEAD:path` for
// HEAD's version). Returns 0 if the blob doesn't exist (new file / deleted).
// Path is wrapped in quotes and internal quotes escaped to tolerate names
// with spaces. stdio pipe captures both stdout and stderr; stderr is
// swallowed on missing-blob errors so we don't spam the user.
function blobLineCount(cwd, spec) {
  const escaped = spec.replace(/"/g, '\\"')
  try {
    const content = execSync(`git -c core.quotePath=false show "${escaped}"`,
      { cwd, encoding: 'utf8', stdio: ['pipe', 'pipe', 'pipe'] })
    // split by \n — the trailing newline adds one empty element we drop.
    const parts = content.split('\n')
    if (parts.length > 0 && parts[parts.length - 1] === '') parts.pop()
    return parts.length
  } catch {
    return 0
  }
}

function detectFileSizeViolations(cwd, staged) {
  const violations = []
  for (const f of staged) {
    if (!FILE_SIZE_EXT_RE.test(f)) continue
    if (FILE_SIZE_EXCLUDE_RE.test(f)) continue
    const newLines = blobLineCount(cwd, `:${f}`)
    if (newLines <= FILE_SIZE_LIMIT) continue
    const oldLines = blobLineCount(cwd, `HEAD:${f}`)
    if (newLines > oldLines) {
      violations.push({ file: f, oldLines, newLines })
    }
  }
  return violations
}

function blockFileSize(violations) {
  const list = violations.slice(0, 10).map(v =>
    `  ${v.file}: ${v.oldLines} → ${v.newLines} (+${v.newLines - v.oldLines})`
  ).join('\n')
  const more = violations.length > 10 ? `\n  …and ${violations.length - 10} more` : ''
  process.stderr.write([
    '',
    `[commit-gate] git commit blocked — code/script file(s) grew past the ${FILE_SIZE_LIMIT}-line soft ratchet.`,
    '',
    'Files over limit that grew in this CL:',
    list + more,
    '',
    'A growing oversized file usually means the responsibility belongs in a',
    'separate translation unit. Common responses:',
    '  • Split into multiple files (#include-based for shaders; new .cpp/.h for C++).',
    '  • Extract helper functions or pass objects into a common header.',
    '  • If the addition itself is small but the file is already way over,',
    '    shrink the file first (delete dead code, inline one-shot utilities, etc).',
    '',
    `Already-oversized files are grandfathered — as long as they don't GROW`,
    'the commit passes. The threshold only pressures files that are both',
    'over and getting larger.',
    '',
    `Escape hatch: include ${SKIP_SIZE_SENTINEL} in the commit message if`,
    'this is a legitimate one-off (e.g. auto-generated file, necessary migration).',
    '',
  ].join('\n'))
  process.exit(2)
}

const SERIALIZE_TEST_RE = /Main\.exe\b[^|&;]*-serialize_test\b/

function didSerializeTestRun(lines, lastUserIdx) {
  for (let i = lastUserIdx + 1; i < lines.length; i++) {
    let m
    try { m = JSON.parse(lines[i]) } catch { continue }
    const blocks = firstArray(m.message?.content, m.content)
    for (const b of blocks) {
      if (b?.type === 'tool_use' && b?.name === 'Bash') {
        if (SERIALIZE_TEST_RE.test(b.input?.command || '')) return true
      }
    }
  }
  return false
}

function blockNoSerializeTest(staged) {
  const filesList = staged.length
    ? staged.filter(f => SERIALIZER_CODE_PATH.test(f)).slice(0, 10).map(f => '  ' + f).join('\n')
    : '  (no serializer code detected — bug?)'
  process.stderr.write([
    '',
    '[commit-gate] git commit blocked — serializer code staged but no serialize-test ran.',
    '',
    'Serializer-facing staged paths:',
    filesList,
    '',
    'Run the serialize-determinism test in this turn before committing:',
    '  Main.exe -mode 0 -renderer 0 -loglevel 0 -offscreen -serialize_test ExampleProject/Scenes/UnitTest.InnoScene',
    '',
    `Escape hatch: include ${SKIP_SENTINEL} if this change genuinely cannot`,
    'be validated by a serialize-test (e.g. a rename with no logic change).',
    '',
  ].join('\n'))
  process.exit(2)
}

function failOpen(err) {
  process.stderr.write(`[commit-gate] internal error — failing open: ${err?.message || err}\n`)
  process.exit(0)
}
