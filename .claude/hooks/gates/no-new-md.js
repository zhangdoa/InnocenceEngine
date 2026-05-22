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
      if (!m || !m[1].endsWith('.md') || NEW_MD_ALLOWLIST_RE.test(m[1])) continue
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
    '[commit-gate] git commit blocked — new .md outside allowlist.',
    '',
    list,
    '',
    'Allowed: .backlog/tasks/, .claude/{agents,skills,commands,state}/, CLAUDE.md, README.md, LICENSE(S).md.',
    'Audit / design content goes in task notes or commit body, not standalone .md files.',
    '',
  ].join('\n'))
  process.exit(2)
}

module.exports = { run, needsTranscript: false }
