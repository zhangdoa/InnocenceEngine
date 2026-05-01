#!/usr/bin/env node
// Standalone tests for the cross-subtree-stash gate (TASK-196).
//
// Runner: `node .claude/hooks/tests/cross-subtree-stash.test.js`. Zero
// deps — same shape as commit-gate.test.js. Each test prints PASS/FAIL
// and the process exits non-zero on any failure.
//
// Three layers tested:
//   1. parseGitStashCommand — destructive-vs-safe subcommand discrimination,
//      pathspec recognition.
//   2. resolveOwner — path → agent mapping accuracy on representative
//      paths from each owned subtree.
//   3. gate.run — end-to-end via a temp git repo: cross-agent dirty
//      blocks, single-owner dirty allows, sentinel allows, pop/list
//      always allow, scoped `push -- <path>` allows.

const fs = require('fs')
const os = require('os')
const path = require('path')
const { execSync } = require('child_process')
const { parseGitStashCommand, resolveOwner } = require('../lib/ownership')
const gate = require('../gates/cross-subtree-stash')

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

// ---------------------------------------------------------------------
// Layer 1: parseGitStashCommand
// ---------------------------------------------------------------------
group('parseGitStashCommand — destructive forms', () => {
  const cases = [
    'git stash',
    'git stash push',
    'git stash save "msg"',
    'git stash -m "msg"',
    'git stash --include-untracked',
    'git stash -u',
    'git stash push -m "TASK-X build-blocker"',
  ]
  for (const cmd of cases) {
    const r = parseGitStashCommand(cmd)
    assert(r !== null && r.subcommand === 'push' && r.hasPathFilter === false,
      `destructive: ${cmd}`)
  }
})

group('parseGitStashCommand — non-destructive forms (return null)', () => {
  const cases = [
    'git stash pop',
    'git stash list',
    'git stash show stash@{0}',
    'git stash drop',
    'git stash clear',
    'git stash branch wip stash@{0}',
    'git stash apply',
  ]
  for (const cmd of cases) {
    const r = parseGitStashCommand(cmd)
    assert(r === null, `non-destructive: ${cmd}`)
  }
})

group('parseGitStashCommand — path-scoped push (allowed by gate)', () => {
  const r = parseGitStashCommand('git stash push -m "scoped" -- Source/Engine/Common/foo.h')
  assert(r !== null && r.hasPathFilter === true, 'path-scoped push detected')
  const r2 = parseGitStashCommand('git stash push -- a.cpp b.cpp')
  assert(r2 !== null && r2.hasPathFilter === true, 'path-scoped push (no -m) detected')
})

group('parseGitStashCommand — non-stash commands', () => {
  assert(parseGitStashCommand('git status') === null, 'git status -> null')
  assert(parseGitStashCommand('git commit -m "x"') === null, 'git commit -> null')
  assert(parseGitStashCommand('echo hello') === null, 'echo -> null')
  assert(parseGitStashCommand('') === null, 'empty -> null')
})

// ---------------------------------------------------------------------
// Layer 2: resolveOwner
// ---------------------------------------------------------------------
group('resolveOwner — representative paths per subtree', () => {
  const cases = [
    ['.claude/hooks/session-gate.js',                          'ai-expert'],
    ['.claude/disciplines/fundamentals.md',                     'ai-expert'],
    ['.backlog/tasks/task-196.md',                              'producer'],
    ['.alignments/TASK-66-cube-shadow.md',                      'ai-expert'],
    ['CMake/Modules/InnoCommon.cmake',                          'ci-build-expert'],
    ['Scripts/build.ps1',                                       'ci-build-expert'],
    ['Source/Editor-Next/src/main.ts',                          'editor-tooling-expert'],
    ['Source/Engine/Common/EntityRegistry.h',                   'low-level-expert'],
    ['Source/Engine/Platform/WinWindow.cpp',                    'platform-expert'],
    ['Source/Engine/Services/DX12/DX12RenderingServer.cpp',     'graphics-api-expert'],
    ['Source/Engine/Services/VK/VKRenderingServer.cpp',         'graphics-api-expert'],
    ['Source/Engine/Services/AssetService.cpp',                 'software-architect'],
    ['Source/Engine/Services/SceneService.h',                   'software-architect'],
    ['Source/Engine/ThirdParty/JSONWrapper/JSONWrapper.cpp',    'software-architect'],
    ['Source/ExampleProject/LogicClient/Logic.cpp',             'test-expert'],
    ['Source/ExampleProject/RenderingClient/LightPass.cpp',     'rendering-researcher'],
    ['Source/ExampleProject/RenderingClient/SkyPass.h',         'rendering-researcher'],
    ['Source/Shaders/HLSL/lightPass.comp',                      'rendering-researcher'],
  ]
  for (const [p, expected] of cases) {
    assert(resolveOwner(p) === expected, `${p} -> ${expected}`)
  }
})

group('resolveOwner — non-pass file in RenderingClient is NOT rendering-researcher', () => {
  // RenderingClient is a hybrid subtree. Pass files are rendering-researcher's;
  // non-pass files (e.g. ExampleRenderingClient.cpp itself) belong elsewhere.
  // The gate stays conservative by treating those as `<unowned>` — a separate
  // domain — so a stash mixing pass + non-pass in the same dir still trips.
  assert(resolveOwner('Source/ExampleProject/RenderingClient/ExampleRenderingClient.cpp') === null,
    'non-Pass file in RenderingClient -> null (conservative)')
})

group('resolveOwner — windows backslash paths normalised', () => {
  assert(resolveOwner('Source\\Shaders\\HLSL\\foo.hlsl') === 'rendering-researcher',
    'backslashes normalised')
})

// ---------------------------------------------------------------------
// Layer 3: gate.run end-to-end with a real temp git repo
// ---------------------------------------------------------------------
function makeRepo() {
  const dir = fs.mkdtempSync(path.join(os.tmpdir(), 'cross-stash-test-'))
  const sh = (cmd) => execSync(cmd, { cwd: dir, stdio: ['pipe', 'pipe', 'pipe'] })
  sh('git init -q')
  sh('git config user.email t@t.t')
  sh('git config user.name t')
  sh('git commit --allow-empty -q -m init')
  // Make every prefix the gate cares about exist with a committed file
  // so subsequent edits show up as "modified" (M) in --porcelain.
  fs.mkdirSync(path.join(dir, 'Source/Shaders/HLSL'), { recursive: true })
  fs.mkdirSync(path.join(dir, 'Source/Engine/Common'), { recursive: true })
  fs.mkdirSync(path.join(dir, '.claude/hooks'), { recursive: true })
  fs.writeFileSync(path.join(dir, 'Source/Shaders/HLSL/x.hlsl'), 'orig\n')
  fs.writeFileSync(path.join(dir, 'Source/Engine/Common/x.h'), 'orig\n')
  fs.writeFileSync(path.join(dir, '.claude/hooks/x.js'), 'orig\n')
  sh('git add -A')
  sh('git commit -q -m seed')
  return { dir, sh }
}

// Run gate.run; capture stderr + exit code without actually exiting our
// process. The gate calls process.exit(2) inside `block()`, so we wrap.
function runGate(input) {
  const result = gate.run(input)
  if (result.ok) return { blocked: false, stderr: '' }
  // Capture process.exit + stderr.write
  const realExit = process.exit
  const realWrite = process.stderr.write.bind(process.stderr)
  let captured = ''
  let exitCode = null
  process.stderr.write = (s) => { captured += s; return true }
  process.exit = (c) => { exitCode = c; throw new Error('__gate_exit__') }
  try { result.block() } catch (e) { if (e.message !== '__gate_exit__') throw e }
  finally {
    process.stderr.write = realWrite
    process.exit = realExit
  }
  return { blocked: true, stderr: captured, exitCode }
}

group('gate.run — cross-subtree dirty -> BLOCK', () => {
  const { dir } = makeRepo()
  fs.writeFileSync(path.join(dir, 'Source/Shaders/HLSL/x.hlsl'), 'edited by rendering\n')
  fs.writeFileSync(path.join(dir, '.claude/hooks/x.js'), 'edited by ai\n')
  const r = runGate({
    tool_name: 'Bash',
    tool_input: { command: 'git stash push -m "TASK-X build-blocker"' },
    cwd: dir,
  })
  assert(r.blocked, 'cross-subtree stash blocked')
  assert(r.exitCode === 2, 'exit code 2')
  assert(r.stderr.includes('rendering-researcher'), 'block message names rendering-researcher')
  assert(r.stderr.includes('ai-expert'), 'block message names ai-expert')
  assert(r.stderr.includes('Source/Shaders/HLSL/x.hlsl'), 'block message lists rendering path')
  assert(r.stderr.includes('.claude/hooks/x.js'), 'block message lists ai path')
})

group('gate.run — same-agent dirty -> ALLOW', () => {
  const { dir } = makeRepo()
  fs.writeFileSync(path.join(dir, 'Source/Shaders/HLSL/x.hlsl'), 'edit 1\n')
  // Add a second rendering file so we have multiple dirty paths but one owner.
  fs.writeFileSync(path.join(dir, 'Source/Shaders/HLSL/y.hlsl'), 'edit 2\n')
  const r = runGate({
    tool_name: 'Bash',
    tool_input: { command: 'git stash push -m "TASK-X scoped"' },
    cwd: dir,
  })
  assert(!r.blocked, 'same-agent stash allowed')
})

group('gate.run — sentinel -> ALLOW even on cross-subtree', () => {
  const { dir } = makeRepo()
  fs.writeFileSync(path.join(dir, 'Source/Shaders/HLSL/x.hlsl'), 'edit\n')
  fs.writeFileSync(path.join(dir, '.claude/hooks/x.js'), 'edit\n')
  const r = runGate({
    tool_name: 'Bash',
    tool_input: { command: 'git stash push -m "tree-wide" # [stash-cross-subtree-OK]' },
    cwd: dir,
  })
  assert(!r.blocked, 'sentinel opts out')
})

group('gate.run — `git stash pop` -> ALLOW (non-destructive)', () => {
  const { dir } = makeRepo()
  fs.writeFileSync(path.join(dir, 'Source/Shaders/HLSL/x.hlsl'), 'edit\n')
  fs.writeFileSync(path.join(dir, '.claude/hooks/x.js'), 'edit\n')
  const r = runGate({
    tool_name: 'Bash',
    tool_input: { command: 'git stash pop' },
    cwd: dir,
  })
  assert(!r.blocked, 'git stash pop bypasses gate entirely')
})

group('gate.run — path-scoped push -> ALLOW (caller already scoped)', () => {
  const { dir } = makeRepo()
  fs.writeFileSync(path.join(dir, 'Source/Shaders/HLSL/x.hlsl'), 'edit\n')
  fs.writeFileSync(path.join(dir, '.claude/hooks/x.js'), 'edit\n')
  const r = runGate({
    tool_name: 'Bash',
    tool_input: { command: 'git stash push -m "scoped" -- Source/Shaders/HLSL/x.hlsl' },
    cwd: dir,
  })
  assert(!r.blocked, 'path-scoped push allowed')
})

group('gate.run — clean tree -> ALLOW (nothing to sweep)', () => {
  const { dir } = makeRepo()
  const r = runGate({
    tool_name: 'Bash',
    tool_input: { command: 'git stash push -m "noop"' },
    cwd: dir,
  })
  assert(!r.blocked, 'clean tree allowed')
})

group('gate.run — non-Bash tool -> ALLOW (gate ignores)', () => {
  const r = runGate({ tool_name: 'Read', tool_input: { file_path: 'x' }, cwd: '/' })
  assert(!r.blocked, 'Read tool ignored')
})

// ---------------------------------------------------------------------
// Summary
// ---------------------------------------------------------------------
console.log(`\n${passed} passed, ${failed} failed`)
process.exit(failed === 0 ? 0 : 1)
