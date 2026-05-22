const testRun = require('../gates/test-run')

function trCtx({ staged, closingTasks = [], messageText = '', transcript = null, lastUserIdx = -1 }) {
  return { staged, closingTasks, messageText, transcript, lastUserIdx }
}

function register({ assert, group }) {
  group('test-run gate — Closure-Reason: exemption', () => {
    const r1 = testRun.run(trCtx({
      staged: ['.backlog/tasks/task-189 - example.md'],
      closingTasks: ['.backlog/tasks/task-189 - example.md'],
      messageText: [
        'docs(backlog): TASK-189 close as non-reproducible',
        '',
        'Closure-Reason: non-reproducible observation, never validated',
        'Reviewed-By: code-review',
        'Code-AI-Generated-By: Claude',
        '',
      ].join('\n'),
    }))
    assert(r1.ok === true, 'all-docs + closing + Closure-Reason: → pass')

    const r2 = testRun.run(trCtx({
      staged: ['.backlog/tasks/task-189 - example.md'],
      closingTasks: ['.backlog/tasks/task-189 - example.md'],
      messageText: [
        'docs(backlog): TASK-189 close',
        '',
        'Reviewed-By: code-review',
        'Code-AI-Generated-By: Claude',
        '',
      ].join('\n'),
      transcript: [],
      lastUserIdx: -1,
    }))
    assert(r2.ok === false, 'all-docs + closing + no Closure-Reason: → block')
    assert(typeof r2.block === 'function', 'block callback present')

    const r3 = testRun.run(trCtx({
      staged: ['.backlog/tasks/task-189 - example.md'],
      closingTasks: [],
      messageText: 'docs(backlog): TASK-189 update notes\n',
    }))
    assert(r3.ok === true, 'all-docs + no closing → pass (existing bypass)')

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
