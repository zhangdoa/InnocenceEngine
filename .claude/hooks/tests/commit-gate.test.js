#!/usr/bin/env node
// Standalone tests for commit-gate library helpers.
//
// Runner: `node .claude/hooks/tests/commit-gate.test.js`. Zero deps —
// no Jest / Mocha — because this directory is not in any package.json
// install graph. Each test prints PASS or FAIL and the process exits
// non-zero on any failure so a future CI / pre-push hook can pick it up.
//
// Scope: the unit-testable helpers in lib/common.js. Integration tests
// (full dispatcher invocation with mocked stdin / git) are deliberately
// out of scope — they need a fixture-heavy harness this repo does not
// yet have, and the dispatcher logic is small enough to inspect.

const fs = require('fs')
const os = require('os')
const path = require('path')
const { collectCommitMessageText, ATTRIBUTION_RE } = require('../lib/common')
const peerReview = require('../gates/peer-review')

let passed = 0
let failed = 0

function assert(cond, label) {
  if (cond) { console.log(`  PASS  ${label}`); passed++ }
  else      { console.log(`  FAIL  ${label}`); failed++ }
}

function group(name, fn) {
  console.log(`\n[${name}]`)
  fn()
}

// Workspace setup — temp dir we write a fake commit message file into.
const tmpRoot = fs.mkdtempSync(path.join(os.tmpdir(), 'commit-gate-test-'))
const msgFile = path.join(tmpRoot, 'commit-message.txt')
const msgContent = 'Test commit subject\n\nBody.\n\nCode-AI-Generated-By: Claude Opus 4.7 (1M context)\n'
fs.writeFileSync(msgFile, msgContent, 'utf8')

// Sanitize path separators: windows tmp paths use backslashes, the gate
// regex tolerates either but the MSYS translation only fires for
// forward-slash paths. Keep both forms around.
const nativeAbs = msgFile.replace(/\\/g, '/')
const msysStyle = nativeAbs.replace(/^([a-zA-Z]):\//, '/$1/')

group('collectCommitMessageText — happy paths', () => {
  // Native absolute path on Windows (the form Node expects).
  const r1 = collectCommitMessageText(`git commit -F ${nativeAbs}`, tmpRoot)
  assert(r1.fileError === null, 'native abs path: no fileError')
  assert(r1.text.includes('Code-AI-Generated-By:'), 'native abs path: file content appended')

  // Relative path resolved against cwd.
  const r2 = collectCommitMessageText('git commit -F commit-message.txt', tmpRoot)
  assert(r2.fileError === null, 'relative path: no fileError')
  assert(r2.text.includes('Code-AI-Generated-By:'), 'relative path: file content appended')

  // No -F at all — text == cmd, fileError null.
  const r3 = collectCommitMessageText('git commit -m "subject"', tmpRoot)
  assert(r3.fileError === null, 'no -F: no fileError')
  assert(r3.text === 'git commit -m "subject"', 'no -F: text unchanged')
})

group('collectCommitMessageText — MSYS-style absolute path (the TASK-154 repro)', () => {
  // The exact failing form from the producer's TASK-66 closure repro:
  // `git commit -F /c/GitRepo/InnocenceEngine/Build/commit-message.txt`.
  // We stand in for it with our temp file rendered in MSYS style.
  if (process.platform !== 'win32') {
    console.log('  SKIP  not on win32 (MSYS translation only matters here)')
    return
  }
  const r = collectCommitMessageText(`git commit -F ${msysStyle}`, tmpRoot)
  assert(r.fileError === null, `MSYS path: no fileError (raw=${msysStyle})`)
  assert(r.text.includes('Code-AI-Generated-By:'), 'MSYS path: file content appended after translation')
})

group('collectCommitMessageText — loud failure when path is unreadable in any form', () => {
  // Path neither valid native nor valid MSYS. Gate must NOT silently
  // fall through; it must surface the failure for the dispatcher to
  // block the commit.
  const ghost = '/q/does/not/exist/anywhere.txt'
  const r = collectCommitMessageText(`git commit -F ${ghost}`, tmpRoot)
  assert(r.fileError !== null, 'unreadable path: fileError populated')
  assert(r.fileError.rawPath === ghost, 'unreadable path: rawPath surfaced')
  // We expect at least one attempt; on win32 MSYS translation gives a
  // second attempt.
  assert(r.fileError.attempts.length >= 1, 'unreadable path: at least one attempt recorded')
  for (const a of r.fileError.attempts) {
    assert(typeof a.code === 'string' && a.code.length > 0, `attempt code recorded (${a.code})`)
  }
})

group('collectCommitMessageText — quoted -F argument', () => {
  const r = collectCommitMessageText(`git commit -F "${nativeAbs}"`, tmpRoot)
  assert(r.fileError === null, 'quoted path: no fileError')
  assert(r.text.includes('Code-AI-Generated-By:'), 'quoted path: file content appended')
})

// ---------------------------------------------------------------------------
// peer-review gate (TASK-167)
// ---------------------------------------------------------------------------
// The gate's run() takes a context object with `messageText`. PASS returns
// `{ ok: true }`; BLOCK returns `{ ok: false, block: <fn> }`. We don't
// invoke `block()` here because it process.exits — presence of the block
// callback is the assertion. Direct ATTRIBUTION_RE checks in the
// attribution-without-review case keep the gate-pair semantics explicit.

function ctxOf(text) { return { messageText: text } }

group('peer-review gate — pass forms', () => {
  const r1 = peerReview.run(ctxOf('Subject\n\nBody.\n\nReviewed-By: ai-expert\nCode-AI-Generated-By: Claude\n'))
  assert(r1.ok === true, 'Reviewed-By: present → pass')

  const r2 = peerReview.run(ctxOf('Subject\n\nBody.\n\nReview-Skipped: hook-internal\nCode-AI-Generated-By: Claude\n'))
  assert(r2.ok === true, 'Review-Skipped: present → pass')

  const r3 = peerReview.run(ctxOf([
    'Subject',
    '',
    'Body.',
    '',
    'Reviewed-By: ai-expert',
    'Reviewed-By: software-architect',
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
  const r3 = peerReview.run(ctxOf('Subject\n\nReviewed-By ai-expert\n\nCode-AI-Generated-By: Claude\n'))
  assert(r3.ok === false, 'Reviewed-By without colon → block')
})

group('peer-review gate — file-mode (-F) integration', () => {
  // Write a fresh message file with the review line in it; run
  // collectCommitMessageText to produce the gate's effective input;
  // confirm the gate sees the line and passes.
  const reviewMsgFile = path.join(tmpRoot, 'review-message.txt')
  fs.writeFileSync(reviewMsgFile, [
    'Subject',
    '',
    'Body.',
    '',
    'Reviewed-By: ai-expert',
    'Code-AI-Generated-By: Claude',
    '',
  ].join('\n'), 'utf8')
  const collected = collectCommitMessageText(`git commit -F ${reviewMsgFile.replace(/\\/g, '/')}`, tmpRoot)
  assert(collected.fileError === null, 'file-mode: file readable')
  const r = peerReview.run({ messageText: collected.text })
  assert(r.ok === true, 'file-mode: Reviewed-By: in -F file → pass')
  try { fs.unlinkSync(reviewMsgFile) } catch {}
})

// Cleanup.
try { fs.unlinkSync(msgFile) } catch {}
try { fs.rmdirSync(tmpRoot) } catch {}

console.log(`\n${passed} passed, ${failed} failed.`)
process.exit(failed === 0 ? 0 : 1)
