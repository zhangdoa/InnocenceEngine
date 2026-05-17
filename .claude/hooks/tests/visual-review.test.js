// visual-review gate tests.
// See commit-gate.test.js for the orchestrator.
//
// Trigger: any line in the commit body matches `Build/captures/`. When
// the trigger fires, the message must contain `Reviewed-Visually:` or
// `Review-Skipped-Visual:`. When the trigger does not fire, the gate is
// a no-op (PASS regardless of footer content) — so non-rendering CLs
// don't accumulate a vestigial footer.

const visualReview = require('../gates/visual-review')

function ctxOf(text) { return { messageText: text } }

function register({ assert, group }) {
  group('visual-review gate — no trigger (no Build/captures/ reference)', () => {
    // Plain feature CL with no capture reference → gate must PASS even
    // without any visual-review footer.
    const r1 = visualReview.run(ctxOf([
      'feat(engine): TASK-1 add foo',
      '',
      'Body without any capture path reference.',
      '',
      'Reviewed-By: code-review',
      'Code-AI-Generated-By: Claude',
      '',
    ].join('\n')))
    assert(r1.ok === true, 'no Build/captures/ reference → pass (no-op)')

    // Even if the body mentions "captures" as a noun, no path → no trigger.
    const r2 = visualReview.run(ctxOf([
      'feat: refactor capture infra',
      '',
      'Renames the captures helper. No path references.',
      '',
      'Reviewed-By: code-review',
      '',
    ].join('\n')))
    assert(r2.ok === true, 'word "captures" without path → pass')
  })

  group('visual-review gate — trigger fires, footer present → pass', () => {
    const r1 = visualReview.run(ctxOf([
      'feat(rendering): TASK-77 mip-cascade read',
      '',
      'Captures (gitignored):',
      '- Build/captures/TASK-77/toggle0/gisponza/',
      '- Build/captures/TASK-77/toggle1/gisponza/',
      '',
      'Reviewed-By: code-review',
      'Reviewed-Visually: code-review — improvement',
      'Code-AI-Generated-By: Claude',
      '',
    ].join('\n')))
    assert(r1.ok === true, 'capture path + Reviewed-Visually: → pass')

    // Per-scene-mixed verdict shape from the discipline.
    const r2 = visualReview.run(ctxOf([
      'feat(rendering): cache test',
      '',
      'See Build/captures/TASK-X/toggle1/.',
      '',
      'Reviewed-Visually: code-review — per-scene-mixed',
      '',
    ].join('\n')))
    assert(r2.ok === true, 'per-scene-mixed verdict → pass')

    // Multiple Reviewed-Visually: lines (peer + architect, or per-scene).
    const r3 = visualReview.run(ctxOf([
      'feat(rendering): foo',
      '',
      'Build/captures/foo/',
      '',
      'Reviewed-Visually: code-review — improvement',
      'Reviewed-Visually: code-review — improvement',
      '',
    ].join('\n')))
    assert(r3.ok === true, 'multiple Reviewed-Visually: lines → pass')
  })

  group('visual-review gate — trigger fires, opt-out present → pass', () => {
    // Test-infra CL that produces captures but does not claim visual quality.
    const r1 = visualReview.run(ctxOf([
      'feat(test-infra): three-scene capture driver',
      '',
      'Dumps into Build/captures/<RunTag>/{unittest,gitestbox,gisponza}/.',
      '',
      'Reviewed-By: ci-build-impl',
      'Review-Skipped-Visual: test-infra — produces captures, does not claim visual quality',
      '',
    ].join('\n')))
    assert(r1.ok === true, 'Review-Skipped-Visual: → pass')
  })

  group('visual-review gate — trigger fires, footer missing → block', () => {
    // The exact failure shape this gate exists to catch: rendering CL
    // with capture paths in body, peer-review present, visual-review
    // absent. This is the TASK-77.1 rework chain pattern.
    const r1 = visualReview.run(ctxOf([
      'feat(rendering): TASK-77 site-3 read',
      '',
      'Captures:',
      '- Build/captures/TASK-77/toggle0/',
      '- Build/captures/TASK-77/toggle1/',
      '',
      'Reviewed-By: code-review',
      'Code-AI-Generated-By: Claude',
      '',
    ].join('\n')))
    assert(r1.ok === false, 'capture path + no visual footer → block')
    assert(typeof r1.block === 'function', 'block callback is a function')

    // Reviewed-Visually without colon — same posture as attribution gate.
    const r2 = visualReview.run(ctxOf([
      'feat: rendering CL',
      '',
      'Build/captures/foo/',
      '',
      'Reviewed-Visually code-review improvement',
      '',
    ].join('\n')))
    assert(r2.ok === false, 'Reviewed-Visually without colon → block')

    // Reviewed-By present but no Reviewed-Visually: → still block. The
    // two artifacts are independent — peer-review covers diff hygiene,
    // visual-review covers the frame.
    const r3 = visualReview.run(ctxOf([
      'feat: rendering CL',
      '',
      'Build/captures/foo/',
      '',
      'Reviewed-By: code-review',
      '',
    ].join('\n')))
    assert(r3.ok === false, 'peer-review present, visual-review absent → still block')
  })
}

module.exports = { register }
