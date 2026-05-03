// Closure-staleness gate — block code-bearing commits that reference
// `TASK-N` without flipping that task in the same CL (or escaping with
// the `[task-stays-open]` sentinel).
//
// The symmetric / inverse gate is `closure-evidence` (inside test-run.js):
// closure-evidence catches "you claimed Done but no test ran"; this gate
// catches "you wrote code referencing TASK-N but didn't flip TASK-N."
// Same posture — fail open on any internal error so a hook bug never
// bricks commits.
//
// Behavior:
//   - Parse `TASK-\d+` IDs from the commit message.
//   - For each referenced ID, resolve its task file (staged-first so a
//     newly-filed task in the same CL is found, on-disk fallback) and
//     read the *effective* status — staged content if the file is
//     staged (covers same-CL Done flips), on-disk content otherwise.
//   - Block when (a) any referenced task is `In Progress` or `To Do`,
//     (b) at least one staged file is non-docs-only (so a pure
//     `docs(backlog)` flip passes), AND (c) no `[task-stays-open]`
//     sentinel is in the message.
//
// Self-consistency: when the gate's own closure CL flips TASK-193 to
// Done in the same diff, the *staged* content of the task file already
// shows `status: Done`, so the gate naturally allows itself. The MCP
// backlog tool's filename-rename idiosyncrasies don't matter — lookup
// is by ID-prefix glob, not exact filename.

const fs = require('fs')
const path = require('path')
const { execSync } = require('child_process')
const { DOCS_ONLY_PATH } = require('../lib/common')

const SKIP_STALENESS_SENTINEL = '[task-stays-open]'
const TASK_REF_RE = /\bTASK-(\d+)\b/g
const STATUS_RE = /^status:\s*(.+?)\s*$/mi

// Statuses that indicate "still open" — case-insensitive match against
// the YAML frontmatter `status:` value. `Done` and any future closed
// state (`Archived`, `Cancelled`, ...) pass; the gate is concerned only
// with the actively-open states.
const OPEN_STATUSES = new Set(['in progress', 'to do'])

function parseTaskRefs(messageText) {
  const ids = new Set()
  for (const m of messageText.matchAll(TASK_REF_RE)) ids.add(parseInt(m[1], 10))
  return [...ids]
}

// Find a task file by ID. Prefer the staged set (covers a same-CL new
// task file), fall back to globbing `.backlog/tasks/` on disk. Filename
// shape is `task-<id> - <slug>.md`; the dash-space separator distinguishes
// `task-1 - ...md` from `task-19 - ...md` (don't match `task-1*`).
function findTaskFile(cwd, staged, id) {
  const stagedHit = staged.find(f => {
    const base = path.basename(f).toLowerCase()
    return f.startsWith('.backlog/tasks/') && base.startsWith(`task-${id} `)
  })
  if (stagedHit) return { file: stagedHit, source: 'staged' }
  const dir = path.join(cwd, '.backlog', 'tasks')
  let entries
  try { entries = fs.readdirSync(dir) } catch { return null }
  const onDisk = entries.find(f => f.toLowerCase().startsWith(`task-${id} `))
  if (!onDisk) return null
  return { file: `.backlog/tasks/${onDisk}`, source: 'disk' }
}

// Read the effective frontmatter status. Staged files are read via
// `git show :path` so a flip-to-Done in the same CL is honored; on-disk
// files via fs.readFileSync.
function readEffectiveStatus(cwd, hit, staged) {
  let content
  try {
    if (staged.includes(hit.file)) {
      const escaped = hit.file.replace(/"/g, '\\"')
      content = execSync(
        `git -c core.quotePath=false show ":${escaped}"`,
        { cwd, encoding: 'utf8', stdio: ['pipe', 'pipe', 'pipe'] }
      )
    } else {
      content = fs.readFileSync(path.join(cwd, hit.file), 'utf8')
    }
  } catch { return null }
  const fm = content.match(/^---\r?\n([\s\S]*?)\r?\n---/)
  if (!fm) return null
  const m = fm[1].match(STATUS_RE)
  return m ? m[1].trim() : null
}

function run(ctx) {
  if (ctx.messageText.includes(SKIP_STALENESS_SENTINEL)) return { ok: true }

  const refs = parseTaskRefs(ctx.messageText)
  if (refs.length === 0) return { ok: true }

  // Pure docs-only CL (e.g. `docs(backlog)` flip, harness-internal)
  // is the legitimate "I'm flipping it now" venue — pass through.
  const allDocs = ctx.staged.length > 0 && ctx.staged.every(f => DOCS_ONLY_PATH.test(f))
  if (allDocs) return { ok: true }

  const stale = []
  for (const id of refs) {
    const hit = findTaskFile(ctx.cwd, ctx.staged, id)
    if (!hit) continue
    const status = readEffectiveStatus(ctx.cwd, hit, ctx.staged)
    if (!status) continue
    if (OPEN_STATUSES.has(status.toLowerCase())) {
      stale.push({ id, status, file: hit.file })
    }
  }

  if (stale.length === 0) return { ok: true }
  return { ok: false, block: () => emit(stale) }
}

function emit(stale) {
  const list = stale.map(s => `  TASK-${s.id}   status: ${s.status}   (${s.file})`).join('\n')
  process.stderr.write([
    '',
    '[commit-gate] git commit blocked — closure-staleness.',
    '',
    'This commit references task(s) that are still open:',
    list,
    '',
    'A code-bearing commit citing TASK-N normally implies the work for',
    'TASK-N is landing here — but the task file still reads as open. Per',
    '.claude/disciplines/always/backlog-workflow.md § "Cross-session continuity",',
    'closing flips must land with (or follow) the work; otherwise the next',
    'session has no record of what shipped.',
    '',
    'Pick one:',
    '  1. Flip the task to `status: Done` in this same CL (preferred —',
    '     stage the task file alongside the code change, retry the commit).',
    '  2. Land the code now and follow up with a `docs(backlog)` flip CL',
    '     in the same session before context boundary.',
    `  3. Add ${SKIP_STALENESS_SENTINEL} to the commit message if the work`,
    '     is genuinely partial and the task should stay open (rare; the',
    '     sentinel is the audit trail that this was deliberate).',
    '',
  ].join('\n'))
  process.exit(2)
}

module.exports = {
  run,
  needsTranscript: false,
  SKIP_STALENESS_SENTINEL,
  TASK_REF_RE,
  // Test seams.
  parseTaskRefs, findTaskFile, readEffectiveStatus,
}
