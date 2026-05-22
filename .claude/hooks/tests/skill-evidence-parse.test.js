#!/usr/bin/env node

const path = require('path')
const { parseAlwaysApplySkills } = require('../lib/subagent-transcript')

let passed = 0
let failed = 0

function assert(cond, label) {
  if (cond) { console.log(`  PASS  ${label}`); passed++ }
  else      { console.log(`  FAIL  ${label}`); failed++ }
}
function group(name, fn) { console.log(`\n[${name}]`); fn() }

const REPO_ROOT = path.resolve(__dirname, '..', '..', '..')

group('parseAlwaysApplySkills — code-impl', () => {
  const s = parseAlwaysApplySkills(path.join(REPO_ROOT, '.claude', 'agents', 'code-impl.md'))
  assert(Array.isArray(s) && s.length > 0, 'returns non-empty list')
  assert(s.includes('backlog-workflow'), 'includes backlog-workflow')
  assert(s.includes('commit-message-policy'), 'includes commit-message-policy')
  assert(s.includes('peer-review-required'), 'includes peer-review-required')
})

group('parseAlwaysApplySkills — shader-impl', () => {
  const s = parseAlwaysApplySkills(path.join(REPO_ROOT, '.claude', 'agents', 'shader-impl.md'))
  assert(s.includes('backlog-workflow'), 'includes backlog-workflow')
  assert(s.includes('commit-message-policy'), 'includes commit-message-policy')
  assert(s.includes('peer-review-required'), 'includes peer-review-required')
})

group('parseAlwaysApplySkills — harness-impl', () => {
  const s = parseAlwaysApplySkills(path.join(REPO_ROOT, '.claude', 'agents', 'harness-impl.md'))
  assert(s.includes('backlog-workflow'), 'includes backlog-workflow')
  assert(s.includes('commit-message-policy'), 'includes commit-message-policy')
  assert(s.includes('peer-review-required'), 'includes peer-review-required')
})

group('parseAlwaysApplySkills — task-mgmt User-level continuation', () => {
  const s = parseAlwaysApplySkills(path.join(REPO_ROOT, '.claude', 'agents', 'task-mgmt.md'))
  assert(s.includes('backlog-workflow'), 'includes backlog-workflow')
  assert(s.includes('dispatch-briefs'), 'includes dispatch-briefs')
  assert(s.includes('agent-dispatch'),
    'includes User-level agent-dispatch (User-level: continues capture)')
})

group('parseAlwaysApplySkills — ci-build-impl', () => {
  const s = parseAlwaysApplySkills(path.join(REPO_ROOT, '.claude', 'agents', 'ci-build-impl.md'))
  assert(s.includes('backlog-workflow'), 'includes backlog-workflow')
  assert(s.includes('commit-message-policy'), 'includes commit-message-policy')
})

group('parseAlwaysApplySkills — missing file returns null', () => {
  const s = parseAlwaysApplySkills(path.join(REPO_ROOT, 'nonexistent-manifest.md'))
  assert(s === null, 'fail-open trigger')
})

if (require.main === module) {
  console.log(`\n${passed} passed, ${failed} failed`)
  process.exit(failed === 0 ? 0 : 1)
}

module.exports = { run() { return { passed, failed } } }
