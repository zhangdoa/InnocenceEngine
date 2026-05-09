// Attribution gate — the commit message must contain one of the
// AI-authorship headers per .claude/skills/commit-message-policy/SKILL.md.
// No escape sentinel: every Claude-issued commit is AI-authored.

const { ATTRIBUTION_RE } = require('../lib/common')

function run(ctx) {
  if (ATTRIBUTION_RE.test(ctx.messageText)) return { ok: true }
  return { ok: false, block: emit }
}

function emit() {
  process.stderr.write([
    '',
    '[commit-gate] git commit blocked — attribution header missing.',
    '',
    'Per .claude/skills/commit-message-policy/SKILL.md, every Claude-issued commit must',
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

module.exports = { run, needsTranscript: false }
