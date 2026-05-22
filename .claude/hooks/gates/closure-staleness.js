// closure-staleness: block code-bearing commits citing TASK-N when the task is still open
// and no [task-stays-open] sentinel is present. Symmetric to closure-evidence in test-run.js.

const fs = require('fs')
const path = require('path')
const { execSync } = require('child_process')
const { DOCS_ONLY_PATH } = require('../lib/common')

const SKIP_STALENESS_SENTINEL = '[task-stays-open]'
const TASK_REF_RE = /\bTASK-(\d+)\b/g
const STATUS_RE = /^status:\s*(.+?)\s*$/mi

const OPEN_STATUSES = new Set(['in progress', 'to do'])

function parseTaskRefs(messageText) {
  const ids = new Set()
  for (const m of messageText.matchAll(TASK_REF_RE)) ids.add(parseInt(m[1], 10))
  return [...ids]
}

// Find a task file by ID. Prefer staged set (covers same-CL new task), fall back to glob.
// Filename: `task-<id> - <slug>.md`; the dash-space separator distinguishes `task-1 -` from `task-19 -`.
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
    'Open tasks referenced in this commit:',
    list,
    '',
    'Pick one:',
    '  1. Flip the task to `status: Done` in this same CL.',
    '  2. Land the code now and follow up with a `docs(backlog)` flip CL.',
    `  3. Add ${SKIP_STALENESS_SENTINEL} to the commit message if the work is genuinely partial.`,
    '',
  ].join('\n'))
  process.exit(2)
}

module.exports = {
  run,
  needsTranscript: false,
  SKIP_STALENESS_SENTINEL,
  TASK_REF_RE,
  parseTaskRefs, findTaskFile, readEffectiveStatus,
}
