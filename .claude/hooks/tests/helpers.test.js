// collectCommitMessageText helper tests (lib/common.js).
// See commit-gate.test.js for the orchestrator.

const fs = require('fs')
const os = require('os')
const path = require('path')
const { collectCommitMessageText } = require('../lib/common')

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

function register({ assert, group }) {
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
}

function cleanup() {
  try { fs.unlinkSync(msgFile) } catch {}
  try { fs.rmdirSync(tmpRoot) } catch {}
}

module.exports = { register, cleanup, tmpRoot, msgFile, nativeAbs }
