const fs = require('fs')
const os = require('os')
const path = require('path')
const { collectCommitMessageText } = require('../lib/common')

const tmpRoot = fs.mkdtempSync(path.join(os.tmpdir(), 'commit-gate-test-'))
const msgFile = path.join(tmpRoot, 'commit-message.txt')
const msgContent = 'Test commit subject\n\nBody.\n\nCode-AI-Generated-By: Claude Opus 4.7 (1M context)\n'
fs.writeFileSync(msgFile, msgContent, 'utf8')

const nativeAbs = msgFile.replace(/\\/g, '/')
const msysStyle = nativeAbs.replace(/^([a-zA-Z]):\//, '/$1/')

function register({ assert, group }) {
  group('collectCommitMessageText — happy paths', () => {
    const r1 = collectCommitMessageText(`git commit -F ${nativeAbs}`, tmpRoot)
    assert(r1.fileError === null, 'native abs path: no fileError')
    assert(r1.text.includes('Code-AI-Generated-By:'), 'native abs path: file content appended')

    const r2 = collectCommitMessageText('git commit -F commit-message.txt', tmpRoot)
    assert(r2.fileError === null, 'relative path: no fileError')
    assert(r2.text.includes('Code-AI-Generated-By:'), 'relative path: file content appended')

    const r3 = collectCommitMessageText('git commit -m "subject"', tmpRoot)
    assert(r3.fileError === null, 'no -F: no fileError')
    assert(r3.text === 'git commit -m "subject"', 'no -F: text unchanged')
  })

  group('collectCommitMessageText — MSYS-style absolute path', () => {
    if (process.platform !== 'win32') {
      console.log('  SKIP  not on win32')
      return
    }
    const r = collectCommitMessageText(`git commit -F ${msysStyle}`, tmpRoot)
    assert(r.fileError === null, `MSYS path: no fileError (raw=${msysStyle})`)
    assert(r.text.includes('Code-AI-Generated-By:'), 'MSYS path: file content appended after translation')
  })

  group('collectCommitMessageText — loud failure when path is unreadable', () => {
    const ghost = '/q/does/not/exist/anywhere.txt'
    const r = collectCommitMessageText(`git commit -F ${ghost}`, tmpRoot)
    assert(r.fileError !== null, 'unreadable path: fileError populated')
    assert(r.fileError.rawPath === ghost, 'unreadable path: rawPath surfaced')
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
