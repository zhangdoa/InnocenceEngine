// Backlog status-drift auditor — surface tasks whose status field is still
// `To Do` / `In Progress` despite git log showing TASK-N references in
// landed commits. Diagnostic only; no auto-flips, no commits, no side
// effects. Companion to closure-staleness.js, which catches the same
// drift going forward at commit time; this lib catches accumulated drift
// before that gate's enforcement window opened, and at session start so
// task-mgmt surfaces it in the briefing.
//
// Public API:
//
//   audit({ cwd }) -> [
//     { id, status, title, candidate_commits: [{ sha, subject, signal }] }
//   ]
//
//   - `signal` is `'strong'` when TASK-N appears in the commit *subject*
//     (high-confidence: the subject line is where landing CLs cite the
//     task), `'weak'` when it appears only in the body (often a
//     cross-reference, follow-up note, or filing pointer).
//   - Only tasks with at least one candidate commit are returned.
//
// CLI shim (bottom of file):
//
//   node audit-backlog-drift.js              # human-readable
//   node audit-backlog-drift.js --json       # machine-readable
//   node audit-backlog-drift.js --quiet      # suppress chrome; data only
//
// Diagnostic-only contract: this script never mutates the backlog. The
// task-mgmt reads the output and decides per-candidate whether to retrofit-
// flip (the recipe in TASK-201's Implementation Notes still classifies
// each hit as code-closure / cross-reference / multi-CL / explicit-deferred).

const fs = require('fs')
const path = require('path')
const { execSync } = require('child_process')

const TASK_REF_RE = /\bTASK-(\d+)\b/g
const OPEN_STATUSES = new Set(['in progress', 'to do'])
const STATUS_RE = /^status:\s*(.+?)\s*$/mi
const ID_RE = /^id:\s*(\S+)/mi

// Title may be a bare string or a YAML folded-block-scalar `>-` whose
// continuation lines are indented. Capture both shapes.
const TITLE_BARE_RE = /^title:\s*(.+?)\s*$/mi
const TITLE_FOLDED_RE = /^title:\s*>-?\s*\r?\n((?:[ \t]+.+\r?\n?)+)/mi

function parseFrontmatter(content) {
  const fm = content.match(/^---\r?\n([\s\S]*?)\r?\n---/)
  if (!fm) return null
  const body = fm[1]
  const idMatch = body.match(ID_RE)
  const statusMatch = body.match(STATUS_RE)
  let title = null
  const folded = body.match(TITLE_FOLDED_RE)
  if (folded) {
    title = folded[1].split(/\r?\n/).map(l => l.trim()).filter(Boolean).join(' ')
  } else {
    const bare = body.match(TITLE_BARE_RE)
    if (bare) title = bare[1].replace(/^['"]|['"]$/g, '')
  }
  return {
    id: idMatch ? idMatch[1] : null,
    status: statusMatch ? statusMatch[1].trim() : null,
    title,
  }
}

function listOpenTasks(cwd) {
  const dir = path.join(cwd, '.backlog', 'tasks')
  let entries
  try { entries = fs.readdirSync(dir) } catch { return [] }
  const open = []
  for (const name of entries) {
    if (!name.endsWith('.md')) continue
    if (name.startsWith('_')) continue
    let content
    try { content = fs.readFileSync(path.join(dir, name), 'utf8') } catch { continue }
    const fm = parseFrontmatter(content)
    if (!fm || !fm.id || !fm.status) continue
    if (!OPEN_STATUSES.has(fm.status.toLowerCase())) continue
    const idMatch = fm.id.match(/^TASK-(\d+)$/i)
    if (!idMatch) continue
    open.push({
      numericId: parseInt(idMatch[1], 10),
      id: fm.id,
      status: fm.status,
      title: fm.title || '',
      file: `.backlog/tasks/${name}`,
    })
  }
  return open
}

// One git invocation; per-task lookup against an in-memory map. The
// `--grep=TASK-` filter narrows the corpus to commits that mention any
// task at all (much smaller than --all by itself), and the %x09 (TAB)
// separators give a parser-friendly record format that survives subjects
// containing every other plausible delimiter.
function loadTaskCommits(cwd) {
  let raw
  try {
    raw = execSync(
      'git log --all --grep=TASK- --extended-regexp --format=%H%x09%s%x09%b%x1e',
      { cwd, encoding: 'utf8', maxBuffer: 64 * 1024 * 1024, stdio: ['pipe', 'pipe', 'pipe'] }
    )
  } catch { return new Map() }

  const byId = new Map()
  const records = raw.split('\x1e')
  for (const rec of records) {
    const trimmed = rec.replace(/^\r?\n/, '')
    if (!trimmed) continue
    const tab1 = trimmed.indexOf('\t')
    if (tab1 < 0) continue
    const tab2 = trimmed.indexOf('\t', tab1 + 1)
    const sha = trimmed.slice(0, tab1)
    const subject = tab2 < 0 ? trimmed.slice(tab1 + 1) : trimmed.slice(tab1 + 1, tab2)
    const body = tab2 < 0 ? '' : trimmed.slice(tab2 + 1)

    const subjectIds = new Set()
    for (const m of subject.matchAll(TASK_REF_RE)) subjectIds.add(parseInt(m[1], 10))
    const bodyIds = new Set()
    for (const m of body.matchAll(TASK_REF_RE)) bodyIds.add(parseInt(m[1], 10))

    for (const id of subjectIds) {
      if (!byId.has(id)) byId.set(id, [])
      byId.get(id).push({ sha, subject, signal: 'strong' })
    }
    for (const id of bodyIds) {
      if (subjectIds.has(id)) continue
      if (!byId.has(id)) byId.set(id, [])
      byId.get(id).push({ sha, subject, signal: 'weak' })
    }
  }
  return byId
}

function audit({ cwd }) {
  const open = listOpenTasks(cwd)
  if (open.length === 0) return []
  const commitsById = loadTaskCommits(cwd)
  const out = []
  for (const task of open) {
    const hits = commitsById.get(task.numericId)
    if (!hits || hits.length === 0) continue
    const sorted = hits.slice().sort((a, b) => {
      if (a.signal !== b.signal) return a.signal === 'strong' ? -1 : 1
      return 0
    })
    out.push({
      id: task.id,
      status: task.status,
      title: task.title,
      candidate_commits: sorted.map(h => ({
        sha: h.sha.slice(0, 12),
        subject: h.subject,
        signal: h.signal,
      })),
    })
  }
  out.sort((a, b) => {
    const an = parseInt(a.id.replace(/^TASK-/i, ''), 10)
    const bn = parseInt(b.id.replace(/^TASK-/i, ''), 10)
    return an - bn
  })
  return out
}

function formatHuman(results, { quiet }) {
  const lines = []
  if (!quiet) {
    lines.push(`backlog-drift audit — ${results.length} candidate task(s) with TASK-N commit references`)
    lines.push('')
  }
  if (results.length === 0) {
    if (!quiet) lines.push('  (no drift candidates — every open task lacks a TASK-N commit reference)')
    return lines.join('\n')
  }
  for (const r of results) {
    lines.push(`${r.id}  [${r.status}]  ${r.title}`)
    for (const c of r.candidate_commits) {
      const tag = c.signal === 'strong' ? 'subject' : 'body'
      lines.push(`  ${c.sha}  (${tag})  ${c.subject}`)
    }
    lines.push('')
  }
  if (!quiet) {
    lines.push('Each candidate needs human classification: code-closure (retrofit-flip),')
    lines.push('cross-reference (leave open), multi-CL (partially landed), or explicit-')
    lines.push('deferred (landed work, follow-up still pending). Recipe: TASK-201.')
  }
  return lines.join('\n')
}

if (require.main === module) {
  const args = new Set(process.argv.slice(2))
  const json = args.has('--json')
  const quiet = args.has('--quiet')
  const results = audit({ cwd: process.cwd() })
  if (json) {
    process.stdout.write(JSON.stringify(results, null, 2) + '\n')
  } else {
    process.stdout.write(formatHuman(results, { quiet }) + '\n')
  }
}

module.exports = {
  audit,
  // Test seams.
  parseFrontmatter, listOpenTasks, loadTaskCommits, formatHuman,
  TASK_REF_RE, OPEN_STATUSES,
}
