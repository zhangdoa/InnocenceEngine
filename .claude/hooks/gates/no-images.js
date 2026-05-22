// No-images gate — committing image files is forbidden by default.
// Image-shaped artifacts (renderer captures, audit screenshots, screen
// grabs, etc.) belong on local disk (Build/captures/, scratch/, …) and
// in commit-message prose / .md artifact references, not in git. They
// bloat the repo and rot relative to the code they purport to validate.
//
// Exempt paths cover UI assets the engine ships at runtime + Playwright
// visual-regression test snapshots (the test driver compares against them
// programmatically; they're load-bearing test fixtures).
//
// Path-derived bypass — there is no string-escape sentinel.
// If a new legitimate image path appears, add it to NO_IMAGES_EXCLUDE_RE
// below with a one-line justification.

const { execSync } = require('child_process')

const IMAGE_EXT_RE = /\.(png|jpe?g|gif|bmp|tga|webp|hdr|exr|pfm|tiff?|ico|dds|heic|psd)$/i

const NO_IMAGES_EXCLUDE_RE = new RegExp([
  // Runtime-shipped UI icons (engine loads these).
  '^Data/Engine/Icons/',
  // Playwright visual-regression test snapshots (test driver diffs against these).
  '^Source/Editor-Next/tests/.*-snapshots/',
].join('|'))

// Only block ADD / MODIFY / RENAME-to. Deletions of pre-existing images
// must always pass — otherwise the gate prevents cleanup of its own
// pre-existing violations.
function getAddedOrModifiedImages(cwd) {
  try {
    const out = execSync(
      `git diff --cached --name-status --find-renames`,
      { cwd, encoding: 'utf8' }
    )
    const added = []
    for (const line of out.split('\n')) {
      const m = line.match(/^([AMR])\d*\t(?:.+\t)?(.+)$/)
      if (!m) continue
      const path = m[2]
      if (IMAGE_EXT_RE.test(path) && !NO_IMAGES_EXCLUDE_RE.test(path)) {
        added.push(path)
      }
    }
    return added
  } catch { return [] }
}

function run(ctx) {
  const violations = getAddedOrModifiedImages(ctx.cwd)
  if (violations.length === 0) return { ok: true }
  return { ok: false, block: () => emit(violations) }
}

function emit(violations) {
  const list = violations.slice(0, 20).map(f => '  ' + f).join('\n')
  const more = violations.length > 20
    ? `\n  …and ${violations.length - 20} more` : ''
  process.stderr.write([
    '',
    '[commit-gate] git commit blocked — image files staged.',
    '',
    'Image files in this CL:',
    list + more,
    '',
    'Renderer captures, audit screenshots, debug grabs do NOT belong in',
    'git. They bloat the repo and rot relative to the code they validate.',
    'Use Build/captures/ (gitignored) for local outputs and reference them',
    'by path + commit SHA in commit messages or alignment artifacts (.md).',
    '',
    'Exempt locations (already path-allowed):',
    '  • Data/Engine/Icons/                      runtime-shipped UI assets',
    '  • Source/Editor-Next/tests/*-snapshots/   Playwright visual-regression fixtures',
    '',
    'If a new legitimate image path is genuinely required, add it to',
    'NO_IMAGES_EXCLUDE_RE in .claude/hooks/gates/no-images.js with a',
    'one-line justification. There is no string-escape sentinel.',
    '',
  ].join('\n'))
  process.exit(2)
}

module.exports = { run, needsTranscript: false }
