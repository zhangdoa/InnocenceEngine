// Paper-port alignment gate — when a staged backlog task flips to
// `status: Done` AND its frontmatter labels include `paper-port`, a
// fresh `.alignments/<TASK-ID>...md` artifact must also be staged.
// The artifact is produced by the paper-auditor subagent running with
// fresh context — the structural intervention against main-session
// drift. Path-derived: only fires when a staged backlog task closes
// AND carries the `paper-port` label.

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
    'code row-by-row so divergences are visible at review time. Running it as',
    'a fresh-context subagent prevents the main session from confirm-biasing',
    'its way through the comparison (see .claude/agents/paper-auditor.md).',
    '',
    'To satisfy this gate:',
    '  1. Invoke the paper-auditor subagent with the paper section, reference',
    "     impl location, and our implementation files (from .claude/references.json).",
    '  2. The auditor writes `.alignments/<task-id>-<short-name>.md`.',
    '  3. Stage that artifact alongside the task-close diff, then commit.',
    '',
    'No string-based escape. The bypass is path-derived: this gate only fires',
    'when a staged task closes AND carries the `paper-port` label. An',
    'abandoned / superseded paper-port task should drop the `paper-port` label',
    'in the same commit; retro housekeeping that does not need audit should',
    'remove the label, not bypass the gate.',
    '',
  ].join('\n'))
  process.exit(2)
}

module.exports = { run, needsTranscript: false }
