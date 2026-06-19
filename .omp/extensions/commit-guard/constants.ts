// Regexes, limits and path classifiers shared by the commit-guard gates.
// Ported verbatim from the Claude-era .claude/hooks/lib/common.js + gates/*, with
// venue paths widened to the omp layout (.omp/ harness dirs).

export const ATTRIBUTION_RE = /^(Code-AI-Generated-By|Message-AI-Generated-By):\s*\S/m;
export const CAPTURE_PATH_RE = /Build\/captures\//;
export const REVIEW_VISUAL_RE = /^(Reviewed-Visually|Review-Skipped-Visual):\s*\S/m;
// [ \t]* not \s* — \s includes \n, would let `Closure-Reason:\n\nCode-AI-Generated-By:` satisfy.
export const CLOSURE_REASON_RE = /^Closure-Reason:[ \t]*\S/m;

export const BODY_LINE_CAP = 40;
export const TRAILER_RE = /^[A-Z][\w-]*: \S/;

export const IMAGE_EXT_RE = /\.(png|jpe?g|gif|bmp|tga|webp|hdr|exr|pfm|tiff?|ico|dds|heic|psd)$/i;
export const NO_IMAGES_EXCLUDE_RE = /^Data\/Engine\/Icons\/|^Source\/Editor-Next\/tests\/.*-snapshots\//;

export const NEW_MD_ALLOWLIST_RE = new RegExp([
  "^\\.backlog/(tasks|docs)/",
  "^\\.omp/(agents|skills|commands|state|extensions|rules)/",
  "(^|/)AGENTS\\.md$",
  "(^|/)SYSTEM\\.md$",
  "(^|/)README\\.md$",
  "(^|/)LICENSES?\\.md$",
].join("|"));

export const DATA_GENERATED_PATH_RE = /^Data\/Generated\//;
export const PROTECTED_IGNORE_LINES = ["/Data/Generated/*", "Data/Generated/*"];
export const UNIGNORE_ADDED_RE = /^\+!\/?Data\/Generated\//m;

export const FILE_SIZE_LIMIT = 300;
export const FILE_SIZE_EXT_RE = /\.(cpp|hpp|h|c|cc|cxx|inl|hlsl|hlsli|comp|py|js|mjs|ts|ps1|sh|bash|zsh)$/i;
// Harness dir (.omp) is config, not engine source — exempt from the source-style
// size + comment gates, matching the old `harness-internal` skip.
export const HARNESS_OR_VENDOR_RE = /(^|\/)(ThirdParty|External|node_modules|Generated|dist)\/|(^|\/)\.omp\//;

export const DOCS_ONLY_PATH = /^\.backlog\/|\.md$|^\.omp\/|\.gitignore$/;
export const EDITOR_CODE_PATH = /^Source\/(Editor-Next\/src\/|Engine\/Services\/EditorService\.)/;
export const SERIALIZER_CODE_PATH = /^Source\/Engine\/(ThirdParty\/JSONWrapper\/|Services\/(AssetService|SceneService)\.)/;

export const SKIP_STALENESS_SENTINEL = "[task-stays-open]";
export const TASK_REF_RE = /\bTASK-(\d+)\b/g;
export const STATUS_RE = /^status:\s*(.+?)\s*$/im;
export const OPEN_STATUSES: Record<string, true> = { "in progress": true, "to do": true };

// Test/engine command shapes that satisfy the in-turn verification gates.
export const QUALIFYING_TEST = new RegExp([
  String.raw`npx\s+playwright\s+test`,
  String.raw`Main\.exe\b[^|&;]*-(total_frames|reload_at_frame|bake|capture_frame)\b`,
  String.raw`Main\.exe\b[^|&;]*-c\s+\S+\.json`,
  String.raw`RenderTest\.exe\b[^|&;]*-test\b`,
  String.raw`InteractiveTest\.ps1`,
  String.raw`(StartEngineWin|TestPT|TestGIScene|TestPTThreeScene)\.ps1`,
  String.raw`Main\.exe\b[^|&;]*-serialize_test\b`,
  String.raw`GPUUploadableTests_Standalone\.exe\b`,
].join("|"));
export const NON_PLAYWRIGHT_LIVE = new RegExp([
  String.raw`Main\.exe\b[^|&;]*-(total_frames|reload_at_frame|bake|capture_frame)\b`,
  String.raw`Main\.exe\b[^|&;]*-c\s+\S+\.json`,
  String.raw`RenderTest\.exe\b[^|&;]*-test\b`,
  String.raw`InteractiveTest\.ps1`,
  String.raw`(StartEngineWin|TestPT|TestGIScene|TestPTThreeScene)\.ps1`,
].join("|"));
export const PLAYWRIGHT_RE = /npx\s+playwright\s+test(?:\b|$)([^|&;\n]*)/;
export const SERIALIZE_TEST_RE = /Main\.exe\b[^|&;]*-serialize_test\b/;

export const ESSAY_CAP = 5;
export const COMMENT_CODE_EXT_RE = /\.(cpp|hpp|h|c|cc|cxx|inl|hlsl|hlsli|comp|frag|vert|js|mjs|ts|ps1|sh)$/i;
export const REF_PATTERNS = [
  /\b(?:TASK|ISSUE|BUG|JIRA|TICKET|PR|MR)-\d+/i,
  /\bRFC\b/i,
  /§\s*\d/,
  /\bprimitive\s*#?\d/i,
  /\bbin-[ab]\b/i,
  /\bPhase[-\s]?\d/i,
];
