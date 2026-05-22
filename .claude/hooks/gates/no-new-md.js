// No-new-.md gate — new .md files only land in whitelisted locations.
// Kills the .alignments/-style artifact-dumping pattern at the source.
// Modifications to existing .md files are unaffected.

const { execSync } = require('child_process')

const NEW_MD_ALLOWLIST_RE = new RegExp([
  '^\\.backlog/tasks/',
  '^\\.claude/agents/',
  '^\\.claude/skills/',
  '^\\.claude/commands/',
  '^\\.claude/state/',
  '(^|/)CLAUDE\\.md$',
  '(^|/)README\\.md$',
  '(^|/)LICENSES?\\.md$',
].join('|'))

function getNewMdFiles(cwd) {
  try {
    const out = execSync(`git diff --cached --name-status`, { cwd, encoding: 'utf8' })
    const added = []
    for (const line of out.split('\n')) {
      const m = line.match(/^A\t(.+)$/)
      if (!m) continue
      if (!m[1].endsWith('.md')) continue
      if (NEW_MD_ALLOWLIST_RE.test(m[1])) continue
      added.push(m[1])
    }
    return added
  } catch { return [] }
}

function run(ctx) {
  const violations = getNewMdFiles(ctx.cwd)
  if (violations.length === 0) return { ok: true }
  return { ok: false, block: () => emit(violations) }
}

function emit(violations) {
  const list = violations.slice(0, 10).map(f => '  ' + f).join('\n')
  process.stderr.write([
    '',
    '[commit-gate] git commit blocked — new .md file outside allowlist.',
    '',
    'New .md files:',
    list,
    '',
    'New .md files only land in:',
    '  • .backlog/tasks/                  (task records)',
    '  • .claude/{agents,skills,commands,state}/   (harness)',
    '  • CLAUDE.md, README.md, LICENSE(S).md (any directory)',
    '',
    'Audit / design / "alignment" content goes in the task notes or commit',
    'message body, not in standalone .md files. Modifications to existing',
    '.md files are unaffected. No string-escape sentinel.',
    '',
  ].join('\n'))
  process.exit(2)
}

module.exports = { run, needsTranscript: false }
