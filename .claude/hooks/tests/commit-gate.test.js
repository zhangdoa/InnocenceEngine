#!/usr/bin/env node
// Standalone tests for commit-gate library helpers and per-gate logic.
//
// Runner: `node .claude/hooks/tests/commit-gate.test.js`. Zero deps —
// no Jest / Mocha — because this directory is not in any package.json
// install graph. Each test prints PASS or FAIL and the process exits
// non-zero on any failure so a future CI / pre-push hook can pick it up.
//
// Scope: the unit-testable helpers in lib/common.js plus per-gate run()
// logic. Integration tests (full dispatcher invocation with mocked
// stdin / git) are deliberately out of scope — they need a fixture-heavy
// harness this repo does not yet have, and the dispatcher logic is small
// enough to inspect.
//
// Structure: this file is the orchestrator. Each per-gate test module
// exports `register(harness)` and (optionally) `cleanup()`. The harness
// shares a single passed/failed counter set across the whole run so the
// final `<n> passed, <m> failed` line covers every module.

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

const harness = { assert, group }

const helpers = require('./helpers.test')
const peerReview = require('./peer-review.test')
const closureStaleness = require('./closure-staleness.test')
const testRun = require('./test-run.test')
const visualReview = require('./visual-review.test')

helpers.register(harness)
peerReview.register(harness)
closureStaleness.register(harness)
testRun.register(harness)
visualReview.register(harness)

// Cleanup. Each module that owns a fixture exposes its own teardown.
closureStaleness.cleanup()
helpers.cleanup()

console.log(`\n${passed} passed, ${failed} failed.`)
process.exit(failed === 0 ? 0 : 1)
