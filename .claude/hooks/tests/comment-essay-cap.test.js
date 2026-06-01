const gate = require('../gates/comment-essay-cap')

function diffOf(addedLines) {
  return ['@@ -0,0 +1,' + addedLines.length + ' @@', ...addedLines].join('\n')
}

function register({ assert, group }) {
  group('comment-essay-cap — essay run length', () => {
    const sixComments = diffOf([
      '+\t// one', '+\t// two', '+\t// three', '+\t// four', '+\t// five', '+\t// six',
    ])
    assert(gate.scanDiff(sixComments, 'a.cpp').essay.length === 1, '6 contiguous // lines → essay violation')

    const fiveComments = diffOf(['+\t// one', '+\t// two', '+\t// three', '+\t// four', '+\t// five'])
    assert(gate.scanDiff(fiveComments, 'a.cpp').essay.length === 0, '5 contiguous // lines → no essay violation')

    const brokenRun = diffOf(['+\t// one', '+\t// two', '+\tint x = 0;', '+\t// three', '+\t// four'])
    assert(gate.scanDiff(brokenRun, 'a.cpp').essay.length === 0, 'code between comments resets the run')
  })

  group('comment-essay-cap — tracker / design-doc references', () => {
    const cases = [
      ['+\t// TASK-227.2 coexistence seam: when true, graph-driven.', 'task id'],
      ['+\t// Clean-pass kernel (RFC §6 bin-a): binds declared resources.', 'RFC + bin'],
      ['+\t// Recognized built-in dynamic resource (primitive #1).', 'primitive #'],
      ['+\t// Phase-0 scope is one no-dependency node.', 'phase ref'],
      ['+\t// See §2.4 for the rationale.', 'section ref'],
    ]
    for (const [line, label] of cases)
      assert(gate.scanDiff(diffOf([line]), 'a.cpp').refs.length === 1, `flags ${label}`)
  })

  group('comment-essay-cap — clean WHY comments pass', () => {
    const clean = [
      '+\t// Re-resolved per frame because the buffer is double-buffered.',
      '+\t// Imported resources are owned by their producing pass, not the graph.',
      '+\t// Bind by array position: it maps directly to the root parameter index.',
    ]
    for (const line of clean) {
      const r = gate.scanDiff(diffOf([line]), 'a.cpp')
      assert(r.refs.length === 0 && r.essay.length === 0, `clean WHY passes: ${line.slice(4, 40)}`)
    }
  })
}

module.exports = { register }
