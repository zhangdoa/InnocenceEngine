const fs = require('fs')
const os = require('os')
const path = require('path')
const closureStaleness = require('../gates/closure-staleness')

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

function register({ assert, group }) {
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
    writeTaskFixture(9, 'Done', 'short-id')
    const nine = closureStaleness.findTaskFile(csRoot, [], 9)
    assert(nine !== null, 'id=9 finds task-9 fixture')
    assert(!nine.file.includes('task-90'), 'id=9 does NOT match task-900 (dash-space disambiguator)')
  })

  group('closure-staleness — run() acceptance scenarios', () => {
    const r1 = closureStaleness.run(csCtx({
      message: 'feat(rendering): TASK-900 add foo\n\nCode-AI-Generated-By: Claude\n',
      staged: ['Source/Engine/Foo.cpp'],
    }))
    assert(r1.ok === false, 'code commit + In Progress TASK-900 → block')
    assert(typeof r1.block === 'function', 'block callback present')

    const r2 = closureStaleness.run(csCtx({
      message: 'feat: TASK-900 partial work [task-stays-open]\n\nCode-AI-Generated-By: Claude\n',
      staged: ['Source/Engine/Foo.cpp'],
    }))
    assert(r2.ok === true, '[task-stays-open] sentinel → allow')

    const r3 = closureStaleness.run(csCtx({
      message: 'docs(backlog): TASK-900 flip\n\nCode-AI-Generated-By: Claude\n',
      staged: ['.backlog/tasks/task-900 - sample-task.md'],
    }))
    assert(r3.ok === true, 'all-docs staged set → allow')

    const r4 = closureStaleness.run(csCtx({
      message: 'feat(rendering): unrelated work\n\nCode-AI-Generated-By: Claude\n',
      staged: ['Source/Engine/Foo.cpp'],
    }))
    assert(r4.ok === true, 'no TASK-N reference → allow')

    const r5 = closureStaleness.run(csCtx({
      message: 'feat: building on TASK-901\n\nCode-AI-Generated-By: Claude\n',
      staged: ['Source/Engine/Foo.cpp'],
    }))
    assert(r5.ok === true, 'Done-status TASK referenced → allow')

    const r6 = closureStaleness.run(csCtx({
      message: 'feat: TASK-902 implementing now\n\nCode-AI-Generated-By: Claude\n',
      staged: ['Source/Engine/Foo.cpp'],
    }))
    assert(r6.ok === false, 'To Do TASK referenced → block')

    const r7 = closureStaleness.run(csCtx({
      message: 'refactor: TASK-901 plus TASK-900\n\nCode-AI-Generated-By: Claude\n',
      staged: ['Source/Engine/Foo.cpp'],
    }))
    assert(r7.ok === false, 'mixed Done + In Progress → block')

    const r8 = closureStaleness.run(csCtx({
      message: 'feat: TASK-99999 phantom\n\nCode-AI-Generated-By: Claude\n',
      staged: ['Source/Engine/Foo.cpp'],
    }))
    assert(r8.ok === true, 'unknown TASK ID → allow (no file to evaluate)')
  })
}

function cleanup() {
  try { fs.rmSync(csRoot, { recursive: true, force: true }) } catch {}
}

module.exports = { register, cleanup }
