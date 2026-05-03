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
const visualReview = require('../gates/visual-review')
const closureStaleness = require('../gates/closure-staleness')

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
  // The exact failing form from the original TASK-66 closure repro:
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

// ---------------------------------------------------------------------------
// closure-staleness gate (TASK-193)
// ---------------------------------------------------------------------------
// Exercises against a temp .backlog/tasks/ fixture so on-disk lookup
// runs without needing a real git checkout. Staged-content (git show)
// paths are not exercised here — they would require a fixture git repo.
// The on-disk-fallback covers all four AC scenarios:
//   1. code commit + In Progress task → block
//   2. [task-stays-open] sentinel → allow
//   3. all-docs (docs(backlog) flip) staged → allow
//   4. no TASK-N reference → allow

const csRoot = fs.mkdtempSync(path.join(os.tmpdir(), 'closure-staleness-test-'))
fs.mkdirSync(path.join(csRoot, '.backlog', 'tasks'), { recursive: true })
function writeTaskFixture(id, status, slug = 'sample-task') {
  const file = path.join(csRoot, '.backlog', 'tasks', `task-${id} - ${slug}.md`)
  fs.writeFileSync(file, [
    '---',
    `id: TASK-${id}`,
    'title: fixture',
    `status: ${status}`,
    '---',
    '',
    '## Description',
    '',
  ].join('\n'), 'utf8')
}
writeTaskFixture(900, 'In Progress')
writeTaskFixture(901, 'Done')
writeTaskFixture(902, 'To Do')

function csCtx({ message, staged }) {
  return { cwd: csRoot, staged, messageText: message, closingTasks: [] }
}

group('closure-staleness — parseTaskRefs', () => {
  assert(JSON.stringify(closureStaleness.parseTaskRefs('feat: TASK-176 ...')) === '[176]', 'single ref')
  assert(JSON.stringify(closureStaleness.parseTaskRefs('TASK-1 and TASK-22 and TASK-1 again').sort()) === '[1,22]', 'dedup multiple')
  assert(JSON.stringify(closureStaleness.parseTaskRefs('TASK-175-A title')) === '[175]', 'sub-slice suffix ignored')
  assert(JSON.stringify(closureStaleness.parseTaskRefs('no refs here')) === '[]', 'no match → empty')
  assert(JSON.stringify(closureStaleness.parseTaskRefs('TASK-abc not a number')) === '[]', 'non-digit ignored')
})

group('closure-staleness — findTaskFile (on-disk lookup)', () => {
  const hit = closureStaleness.findTaskFile(csRoot, [], 900)
  assert(hit !== null && hit.source === 'disk', 'finds existing task on disk')
  const miss = closureStaleness.findTaskFile(csRoot, [], 99999)
  assert(miss === null, 'missing id → null')
  // Discrimination: task-9 must not match task-90 / task-900 (the
  // dash-space separator is the disambiguator).
  writeTaskFixture(9, 'Done', 'short-id')
  const nine = closureStaleness.findTaskFile(csRoot, [], 9)
  assert(nine !== null, 'id=9 finds task-9 fixture')
  assert(!nine.file.includes('task-90'), 'id=9 does NOT match task-900 (dash-space disambiguator)')
})

group('closure-staleness — run() acceptance scenarios', () => {
  // AC: gate fires on a code-bearing commit referencing an In Progress task → blocks.
  const r1 = closureStaleness.run(csCtx({
    message: 'feat(rendering): TASK-900 add foo\n\nCode-AI-Generated-By: Claude\n',
    staged: ['Source/Engine/Foo.cpp'],
  }))
  assert(r1.ok === false, 'code commit + In Progress TASK-900 → block')
  assert(typeof r1.block === 'function', 'block callback present')

  // AC: gate respects [task-stays-open] sentinel → allows.
  const r2 = closureStaleness.run(csCtx({
    message: 'feat: TASK-900 partial work [task-stays-open]\n\nCode-AI-Generated-By: Claude\n',
    staged: ['Source/Engine/Foo.cpp'],
  }))
  assert(r2.ok === true, '[task-stays-open] sentinel → allow')

  // AC: gate skips for docs(backlog) flip commits → allows.
  // (All-docs staged set is the structural marker for a flip CL;
  // the gate must let those through regardless of referenced status.)
  const r3 = closureStaleness.run(csCtx({
    message: 'docs(backlog): TASK-900 flip\n\nCode-AI-Generated-By: Claude\n',
    staged: ['.backlog/tasks/task-900 - sample-task.md'],
  }))
  assert(r3.ok === true, 'all-docs staged set → allow')

  // AC: gate skips when no TASK-N reference exists → allows.
  const r4 = closureStaleness.run(csCtx({
    message: 'feat(rendering): unrelated work\n\nCode-AI-Generated-By: Claude\n',
    staged: ['Source/Engine/Foo.cpp'],
  }))
  assert(r4.ok === true, 'no TASK-N reference → allow')

  // Done-status task referenced → allow (the work landed already).
  const r5 = closureStaleness.run(csCtx({
    message: 'feat: building on TASK-901\n\nCode-AI-Generated-By: Claude\n',
    staged: ['Source/Engine/Foo.cpp'],
  }))
  assert(r5.ok === true, 'Done-status TASK referenced → allow')

  // To Do-status task referenced → block (same posture as In Progress).
  const r6 = closureStaleness.run(csCtx({
    message: 'feat: TASK-902 implementing now\n\nCode-AI-Generated-By: Claude\n',
    staged: ['Source/Engine/Foo.cpp'],
  }))
  assert(r6.ok === false, 'To Do TASK referenced → block')

  // Mixed: one Done + one In Progress referenced → block (any-fail-blocks).
  const r7 = closureStaleness.run(csCtx({
    message: 'refactor: TASK-901 plus TASK-900\n\nCode-AI-Generated-By: Claude\n',
    staged: ['Source/Engine/Foo.cpp'],
  }))
  assert(r7.ok === false, 'mixed Done + In Progress → block')

  // Unknown TASK ID (file not found) → allow (gate fails open per AC).
  const r8 = closureStaleness.run(csCtx({
    message: 'feat: TASK-99999 phantom\n\nCode-AI-Generated-By: Claude\n',
    staged: ['Source/Engine/Foo.cpp'],
  }))
  assert(r8.ok === true, 'unknown TASK ID → allow (no file to evaluate)')
})

// Cleanup closure-staleness fixtures.
try { fs.rmSync(csRoot, { recursive: true, force: true }) } catch {}

// ---------------------------------------------------------------------------
// visual-review gate
// ---------------------------------------------------------------------------
// Trigger: any line in the commit body matches `Build/captures/`. When
// the trigger fires, the message must contain `Reviewed-Visually:` or
// `Review-Skipped-Visual:`. When the trigger does not fire, the gate is
// a no-op (PASS regardless of footer content) — so non-rendering CLs
// don't accumulate a vestigial footer.

group('visual-review gate — no trigger (no Build/captures/ reference)', () => {
  // Plain feature CL with no capture reference → gate must PASS even
  // without any visual-review footer.
  const r1 = visualReview.run(ctxOf([
    'feat(engine): TASK-1 add foo',
    '',
    'Body without any capture path reference.',
    '',
    'Reviewed-By: ai-expert',
    'Code-AI-Generated-By: Claude',
    '',
  ].join('\n')))
  assert(r1.ok === true, 'no Build/captures/ reference → pass (no-op)')

  // Even if the body mentions "captures" as a noun, no path → no trigger.
  const r2 = visualReview.run(ctxOf([
    'feat: refactor capture infra',
    '',
    'Renames the captures helper. No path references.',
    '',
    'Reviewed-By: ai-expert',
    '',
  ].join('\n')))
  assert(r2.ok === true, 'word "captures" without path → pass')
})

group('visual-review gate — trigger fires, footer present → pass', () => {
  const r1 = visualReview.run(ctxOf([
    'feat(rendering): TASK-77 mip-cascade read',
    '',
    'Captures (gitignored):',
    '- Build/captures/TASK-77/toggle0/gisponza/',
    '- Build/captures/TASK-77/toggle1/gisponza/',
    '',
    'Reviewed-By: graphics-api-expert',
    'Reviewed-Visually: graphics-api-expert — improvement',
    'Code-AI-Generated-By: Claude',
    '',
  ].join('\n')))
  assert(r1.ok === true, 'capture path + Reviewed-Visually: → pass')

  // Per-scene-mixed verdict shape from the discipline.
  const r2 = visualReview.run(ctxOf([
    'feat(rendering): cache test',
    '',
    'See Build/captures/TASK-X/toggle1/.',
    '',
    'Reviewed-Visually: graphics-api-expert — per-scene-mixed',
    '',
  ].join('\n')))
  assert(r2.ok === true, 'per-scene-mixed verdict → pass')

  // Multiple Reviewed-Visually: lines (peer + architect, or per-scene).
  const r3 = visualReview.run(ctxOf([
    'feat(rendering): foo',
    '',
    'Build/captures/foo/',
    '',
    'Reviewed-Visually: graphics-api-expert — improvement',
    'Reviewed-Visually: software-architect — improvement',
    '',
  ].join('\n')))
  assert(r3.ok === true, 'multiple Reviewed-Visually: lines → pass')
})

group('visual-review gate — trigger fires, opt-out present → pass', () => {
  // Test-infra CL that produces captures but does not claim visual quality.
  const r1 = visualReview.run(ctxOf([
    'feat(test-infra): three-scene capture driver',
    '',
    'Dumps into Build/captures/<RunTag>/{unittest,gitestbox,gisponza}/.',
    '',
    'Reviewed-By: ci-build-expert',
    'Review-Skipped-Visual: test-infra — produces captures, does not claim visual quality',
    '',
  ].join('\n')))
  assert(r1.ok === true, 'Review-Skipped-Visual: → pass')
})

group('visual-review gate — trigger fires, footer missing → block', () => {
  // The exact failure shape this gate exists to catch: rendering CL
  // with capture paths in body, peer-review present, visual-review
  // absent. This is the TASK-77.1 rework chain pattern.
  const r1 = visualReview.run(ctxOf([
    'feat(rendering): TASK-77 site-3 read',
    '',
    'Captures:',
    '- Build/captures/TASK-77/toggle0/',
    '- Build/captures/TASK-77/toggle1/',
    '',
    'Reviewed-By: graphics-api-expert',
    'Code-AI-Generated-By: Claude',
    '',
  ].join('\n')))
  assert(r1.ok === false, 'capture path + no visual footer → block')
  assert(typeof r1.block === 'function', 'block callback is a function')

  // Reviewed-Visually without colon — same posture as attribution gate.
  const r2 = visualReview.run(ctxOf([
    'feat: rendering CL',
    '',
    'Build/captures/foo/',
    '',
    'Reviewed-Visually graphics-api-expert improvement',
    '',
  ].join('\n')))
  assert(r2.ok === false, 'Reviewed-Visually without colon → block')

  // Reviewed-By present but no Reviewed-Visually: → still block. The
  // two artifacts are independent — peer-review covers diff hygiene,
  // visual-review covers the frame.
  const r3 = visualReview.run(ctxOf([
    'feat: rendering CL',
    '',
    'Build/captures/foo/',
    '',
    'Reviewed-By: graphics-api-expert',
    '',
  ].join('\n')))
  assert(r3.ok === false, 'peer-review present, visual-review absent → still block')
})

// Cleanup.
try { fs.unlinkSync(msgFile) } catch {}
try { fs.rmdirSync(tmpRoot) } catch {}

console.log(`\n${passed} passed, ${failed} failed.`)
process.exit(failed === 0 ? 0 : 1)
