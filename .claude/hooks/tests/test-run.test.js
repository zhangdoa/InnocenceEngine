// test-run gate tests — Closure-Reason: exemption (TASK-221).
// See commit-gate.test.js for the orchestrator.
//
// The exemption only affects the docs-only short-circuit at the top of
// run(); for those cases the gate never reaches `didQualifyingTestRun`,
// so a synthetic ctx with no transcript is sufficient. The code-bearing
// case below explicitly drives the transcript path with empty inputs to
// confirm the exemption does NOT excuse code from a qualifying test run.

const testRun = require('../gates/test-run')

function trCtx({ staged, closingTasks = [], messageText = '', transcript = null, lastUserIdx = -1 }) {
  return { staged, closingTasks, messageText, transcript, lastUserIdx }
}

function register({ assert, group }) {
  group('test-run gate — Closure-Reason: exemption', () => {
    // All-docs staged + closing task + Closure-Reason: present → pass.
    const r1 = testRun.run(trCtx({
      staged: ['.backlog/tasks/task-189 - example.md'],
      closingTasks: ['.backlog/tasks/task-189 - example.md'],
      messageText: [
        'docs(backlog): TASK-189 close as non-reproducible',
        '',
        'Closure-Reason: non-reproducible observation, never validated',
        'Reviewed-By: ai-expert',
        'Code-AI-Generated-By: Claude',
        '',
      ].join('\n'),
    }))
    assert(r1.ok === true, 'all-docs + closing + Closure-Reason: → pass')

    // All-docs staged + closing task + no Closure-Reason: → block (regression).
    const r2 = testRun.run(trCtx({
      staged: ['.backlog/tasks/task-189 - example.md'],
      closingTasks: ['.backlog/tasks/task-189 - example.md'],
      messageText: [
        'docs(backlog): TASK-189 close',
        '',
        'Reviewed-By: ai-expert',
        'Code-AI-Generated-By: Claude',
        '',
      ].join('\n'),
      transcript: [],
      lastUserIdx: -1,
    }))
    assert(r2.ok === false, 'all-docs + closing + no Closure-Reason: → block')
    assert(typeof r2.block === 'function', 'block callback present')

    // All-docs staged + no closing task + no marker → pass (regression).
    const r3 = testRun.run(trCtx({
      staged: ['.backlog/tasks/task-189 - example.md'],
      closingTasks: [],
      messageText: 'docs(backlog): TASK-189 update notes\n',
    }))
    assert(r3.ok === true, 'all-docs + no closing → pass (existing bypass)')

    // Closure-Reason without colon → does not bypass (block).
    const r4 = testRun.run(trCtx({
      staged: ['.backlog/tasks/task-189 - example.md'],
      closingTasks: ['.backlog/tasks/task-189 - example.md'],
      messageText: [
        'docs(backlog): TASK-189 close',
        '',
        'Closure-Reason obsolete',
        '',
      ].join('\n'),
      transcript: [],
      lastUserIdx: -1,
    }))
    assert(r4.ok === false, 'Closure-Reason without colon → block')

    // Closure-Reason: with empty value, EOF-terminated → does not bypass.
    const r5a = testRun.run(trCtx({
      staged: ['.backlog/tasks/task-189 - example.md'],
      closingTasks: ['.backlog/tasks/task-189 - example.md'],
      messageText: [
        'docs(backlog): TASK-189 close',
        '',
        'Closure-Reason:',
        '',
      ].join('\n'),
      transcript: [],
      lastUserIdx: -1,
    }))
    assert(r5a.ok === false, 'r5a: Closure-Reason: empty value, EOF → block')

    // Closure-Reason: empty value followed by other footers → does not bypass.
    // Guards against \s-includes-\n: a permissive regex would let the next
    // footer's first \S satisfy the match across the blank line.
    const r5b = testRun.run(trCtx({
      staged: ['.backlog/tasks/task-189 - example.md'],
      closingTasks: ['.backlog/tasks/task-189 - example.md'],
      messageText: [
        'docs(backlog): TASK-189 close',
        '',
        'Closure-Reason:',
        '',
        'Reviewed-By: ai',
        'Code-AI-Generated-By: Claude',
        '',
      ].join('\n'),
      transcript: [],
      lastUserIdx: -1,
    }))
    assert(r5b.ok === false, 'r5b: Closure-Reason: empty value + later footers → block')

    // Code staged + closing task + Closure-Reason: → still requires test-run.
    // Drive the transcript path with empty inputs to confirm the exemption
    // does not extend to code-bearing CLs.
    const r6 = testRun.run(trCtx({
      staged: ['Source/Engine/Foo.cpp'],
      closingTasks: ['.backlog/tasks/task-189 - example.md'],
      messageText: [
        'feat(engine): TASK-189 implement and close',
        '',
        'Closure-Reason: superseded',
        '',
      ].join('\n'),
      transcript: [],
      lastUserIdx: -1,
    }))
    assert(r6.ok === false, 'code staged + closing + Closure-Reason: → still requires test-run')
  })
}

module.exports = { register }
