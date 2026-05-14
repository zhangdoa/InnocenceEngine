#!/usr/bin/env node
// parseAlwaysApplySkills round-trip tests for the skill-evidence gate
// (TASK-187). Split from skill-evidence.test.js to keep both files under
// the 300-line gate after the task-mgmt + ci-build-impl extension.
//
// Runner: `node .claude/hooks/tests/skill-evidence-parse.test.js`.
// skill-evidence.test.js auto-runs this layer via require() so the
// canonical entry point keeps full coverage.

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
  assert(s.includes('cpp-style'), 'includes cpp-style')
  assert(s.includes('safety-principles'), 'includes user-level safety-principles')
  assert(!s.includes('paper-port'), 'excludes conditional paper-port')
  assert(!s.includes('commit-message-policy'), 'excludes on-commit policies')
})

group('parseAlwaysApplySkills — shader-impl', () => {
  const s = parseAlwaysApplySkills(path.join(REPO_ROOT, '.claude', 'agents', 'shader-impl.md'))
  assert(s.includes('shader-standards'), 'includes shader-standards')
  assert(s.includes('safety-principles'), 'includes user-level safety-principles')
  assert(!s.includes('visual-validation'), 'excludes conditional visual-validation')
})

group('parseAlwaysApplySkills — harness-impl conditional truncation', () => {
  const s = parseAlwaysApplySkills(path.join(REPO_ROOT, '.claude', 'agents', 'harness-impl.md'))
  assert(s.includes('persistence-venue'), 'includes persistence-venue')
  assert(!s.includes('commit-message-policy'),
    'excludes on-commit commit-message-policy (same line, different clause)')
  assert(!s.includes('peer-review-required'),
    'excludes on-commit peer-review-required')
})

group('parseAlwaysApplySkills — task-mgmt User-level continuation', () => {
  const s = parseAlwaysApplySkills(path.join(REPO_ROOT, '.claude', 'agents', 'task-mgmt.md'))
  assert(s.includes('backlog-workflow'), 'includes backlog-workflow')
  assert(s.includes('session-start'), 'includes session-start')
  assert(s.includes('agent-dispatch'),
    'includes User-level agent-dispatch (User-level: continues capture)')
  assert(s.includes('surface-dont-chase'),
    'includes User-level surface-dont-chase')
})

group('parseAlwaysApplySkills — ci-build-impl On-clause truncation', () => {
  const s = parseAlwaysApplySkills(path.join(REPO_ROOT, '.claude', 'agents', 'ci-build-impl.md'))
  assert(s.includes('fundamentals'), 'includes fundamentals')
  assert(s.includes('workspace-hygiene'), 'includes workspace-hygiene')
  assert(!s.includes('regression-build-chain'),
    'excludes On-bug regression-build-chain')
  assert(!s.includes('commit-message-policy'),
    'excludes On-commit commit-message-policy')
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
