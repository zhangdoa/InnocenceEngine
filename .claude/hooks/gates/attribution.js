// attribution: commit message must contain Code-AI-Generated-By: or Message-AI-Generated-By:.

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
    'Add one of:',
    '  Code-AI-Generated-By: <model name>',
    '  Message-AI-Generated-By: <model name>',
    '',
  ].join('\n'))
  process.exit(2)
}

module.exports = { run, needsTranscript: false }
