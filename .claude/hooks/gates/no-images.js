const { execSync } = require('child_process')

const IMAGE_EXT_RE = /\.(png|jpe?g|gif|bmp|tga|webp|hdr|exr|pfm|tiff?|ico|dds|heic|psd)$/i

const NO_IMAGES_EXCLUDE_RE = new RegExp([
  '^Data/Engine/Icons/',
  '^Source/Editor-Next/tests/.*-snapshots/',
].join('|'))

function getAddedOrModifiedImages(cwd) {
  try {
    const out = execSync(`git diff --cached --name-status --find-renames`, { cwd, encoding: 'utf8' })
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
  const more = violations.length > 20 ? `\n  …and ${violations.length - 20} more` : ''
  process.stderr.write([
    '',
    '[commit-gate] git commit blocked — image files staged.',
    '',
    list + more,
    '',
    'Image files are forbidden. Allowlist: Data/Engine/Icons/, Source/Editor-Next/tests/*-snapshots/.',
    'Use Build/captures/ (gitignored) for local outputs. Edit NO_IMAGES_EXCLUDE_RE in .claude/hooks/gates/no-images.js to add a new exempt path.',
    '',
  ].join('\n'))
  process.exit(2)
}

module.exports = { run, needsTranscript: false }
