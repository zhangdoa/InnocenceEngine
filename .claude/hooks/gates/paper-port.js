// Paper-port alignment gate — when a staged backlog task flips to
// `status: Done` AND its frontmatter labels include `paper-port`, a
// fresh `.alignments/<TASK-ID>...md` artifact must also be staged.
// Path-derived: fires only on staged paper-port-labelled task closure.

const { readStagedTaskFrontmatter } = require('../lib/common')

function detectPaperPortClosures(cwd, closingFiles) {
  const paperPort = []
  for (const f of closingFiles) {
    const fm = readStagedTaskFrontmatter(cwd, f)
    if (!fm || !fm.id) continue
    if (!fm.labels.some(l => l.toLowerCase() === 'paper-port')) continue
    paperPort.push({ file: f, id: fm.id })
  }
  return paperPort
}

function findMissingAlignments(staged, paperPortClosures) {
  const missing = []
  for (const pp of paperPortClosures) {
    const hasAlignment = staged.some(f =>
      f.startsWith('.alignments/') && f.includes(pp.id))
    if (!hasAlignment) missing.push(pp)
  }
  return missing
}

function run(ctx) {
  if (ctx.closingTasks.length === 0) return { ok: true }
  const paperPortClosures = detectPaperPortClosures(ctx.cwd, ctx.closingTasks)
  if (paperPortClosures.length === 0) return { ok: true }
  const missing = findMissingAlignments(ctx.staged, paperPortClosures)
  if (missing.length === 0) return { ok: true }
  return { ok: false, block: () => emit(missing) }
}

function emit(missing) {
  const list = missing.map(m => `  ${m.id}   (from ${m.file})`).join('\n')
  process.stderr.write([
    '',
    '[commit-gate] git commit blocked — paper-port task closure without alignment audit.',
    '',
    'Paper-port tasks flipping to Done:',
    list,
    '',
    'Closing a paper-port task requires a fresh alignment audit. The audit',
    'compares the paper spec, the canonical reference implementation, and our',
    'code row-by-row so divergences are visible at review time.',
    '',
    'To satisfy this gate:',
    '  1. Produce `.alignments/<task-id>-<short-name>.md` per',
    '     `.claude/skills/paper-audit/SKILL.md` (audit format and',
    '     hard rules). The impl stage that did the port runs the audit at',
    "     closure (see `.claude/skills/paper-port/SKILL.md` step 5).",
    '  2. Stage that artifact alongside the task-close diff, then commit.',
    '',
    'No string-based escape. Bypass is path-derived: this gate only fires when',
    'a staged task closes AND carries the `paper-port` label. An abandoned /',
    'superseded paper-port task should drop the `paper-port` label in the same',
    'commit; retro housekeeping that needs no audit should remove the label.',
    '',
  ].join('\n'))
  process.exit(2)
}

module.exports = { run, needsTranscript: false }
