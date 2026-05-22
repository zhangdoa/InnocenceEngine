#!/usr/bin/env node

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

closureStaleness.cleanup()
helpers.cleanup()

console.log(`\n${passed} passed, ${failed} failed.`)
process.exit(failed === 0 ? 0 : 1)
