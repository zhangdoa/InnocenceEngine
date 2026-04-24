// Paper-port alignment gate — when a staged backlog task flips to
// `status: Done` AND its frontmatter labels include `paper-port`, a
// fresh `.alignments/<TASK-ID>...md` artifact must also be staged.
// The artifact is produced by the paper-auditor subagent running with
// fresh context — the structural intervention against main-session
// drift. SKIP_SENTINEL escapes for legitimate closure-without-audit.

const { readStagedTaskFrontmatter, SKIP_SENTINEL } = require('../lib/common')

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
  if (ctx.messageText.includes(SKIP_SENTINEL)) return { ok: true }
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
    `Escape hatch: include ${SKIP_SENTINEL} if this closure legitimately`,
    'cannot be audited (abandoned / superseded task, retro housekeeping).',
    '',
  ].join('\n'))
  process.exit(2)
}

module.exports = { run }
