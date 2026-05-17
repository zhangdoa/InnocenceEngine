// peer-review gate tests (TASK-167).
// See commit-gate.test.js for the orchestrator.
//
// The gate's run() takes a context object with `messageText`. PASS returns
// `{ ok: true }`; BLOCK returns `{ ok: false, block: <fn> }`. We don't
// invoke `block()` here because it process.exits — presence of the block
// callback is the assertion. Direct ATTRIBUTION_RE checks in the
// attribution-without-review case keep the gate-pair semantics explicit.

const fs = require('fs')
const path = require('path')
const { collectCommitMessageText, ATTRIBUTION_RE } = require('../lib/common')
const peerReview = require('../gates/peer-review')
const helpers = require('./helpers.test')

function ctxOf(text) { return { messageText: text } }

function register({ assert, group }) {
  group('peer-review gate — pass forms', () => {
    const r1 = peerReview.run(ctxOf('Subject\n\nBody.\n\nReviewed-By: code-review\nCode-AI-Generated-By: Claude\n'))
    assert(r1.ok === true, 'Reviewed-By: present → pass')

    const r2 = peerReview.run(ctxOf('Subject\n\nBody.\n\nReview-Skipped: hook-internal\nCode-AI-Generated-By: Claude\n'))
    assert(r2.ok === true, 'Review-Skipped: present → pass')

    const r3 = peerReview.run(ctxOf([
      'Subject',
      '',
      'Body.',
      '',
      'Reviewed-By: code-review',
      'Reviewed-By: harness-impl',
      'Code-AI-Generated-By: Claude',
      '',
    ].join('\n')))
    assert(r3.ok === true, 'multiple Reviewed-By: lines → pass')

    // Skip categories from peer-review-required.md § "When required" all
    // satisfy the gate (it does not enumerate them — reviewer discipline does).
    for (const reason of ['backlog-only', 'mechanical-rename', 'bootstrap']) {
      const r = peerReview.run(ctxOf(`Subject\n\nBody.\n\nReview-Skipped: ${reason}\nCode-AI-Generated-By: Claude\n`))
      assert(r.ok === true, `Review-Skipped: ${reason} → pass`)
    }
  })

  group('peer-review gate — block forms', () => {
    const r1 = peerReview.run(ctxOf('Subject\n\nBody.\n\nCode-AI-Generated-By: Claude\n'))
    assert(r1.ok === false, 'attribution present but no review line → block')
    assert(typeof r1.block === 'function', 'block callback is a function')
    // Confirm the fixture really does have attribution — i.e. attribution
    // gate would NOT block; only peer-review does. Pair-semantics check.
    assert(ATTRIBUTION_RE.test('Code-AI-Generated-By: Claude\n'), 'fixture has attribution (pair-semantics check)')

    const r2 = peerReview.run(ctxOf('Subject\n\nBody only, no footer at all.\n'))
    assert(r2.ok === false, 'no review line and no attribution → block')

    // Bare "Reviewed-By" without colon at all — mirrors attribution gate's
    // colon-required shape.
    const r3 = peerReview.run(ctxOf('Subject\n\nReviewed-By code-review\n\nCode-AI-Generated-By: Claude\n'))
    assert(r3.ok === false, 'Reviewed-By without colon → block')
  })

  group('peer-review gate — file-mode (-F) integration', () => {
    // Write a fresh message file with the review line in it; run
    // collectCommitMessageText to produce the gate's effective input;
    // confirm the gate sees the line and passes.
    const reviewMsgFile = path.join(helpers.tmpRoot, 'review-message.txt')
    fs.writeFileSync(reviewMsgFile, [
      'Subject',
      '',
      'Body.',
      '',
      'Reviewed-By: code-review',
      'Code-AI-Generated-By: Claude',
      '',
    ].join('\n'), 'utf8')
    const collected = collectCommitMessageText(`git commit -F ${reviewMsgFile.replace(/\\/g, '/')}`, helpers.tmpRoot)
    assert(collected.fileError === null, 'file-mode: file readable')
    const r = peerReview.run({ messageText: collected.text })
    assert(r.ok === true, 'file-mode: Reviewed-By: in -F file → pass')
    try { fs.unlinkSync(reviewMsgFile) } catch {}
  })
}

module.exports = { register }
