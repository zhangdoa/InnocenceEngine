// Pure commit-guard gates. Each returns a block reason (string) or null to pass.
// Git-dependent gates take injected reader callbacks so they unit-test without a repo.
// Ported from the Claude-era .claude/hooks/gates/*.js (one tool_call interceptor now).
import * as C from "./constants.ts";
import type { NameStatusEntry } from "./collect.ts";

function fmtList(items: string[], cap = 12): string {
  const head = items.slice(0, cap).map((f) => `  ${f}`).join("\n");
  return items.length > cap ? `${head}\n  …and ${items.length - cap} more` : head;
}

// ---- commit-message gates (pure) -------------------------------------------

export function attribution(messageText: string): string | null {
  if (C.ATTRIBUTION_RE.test(messageText)) return null;
  return "attribution header missing — add `Code-AI-Generated-By: <model>` or `Message-AI-Generated-By: <model>`.";
}

export function peerReview(messageText: string): string | null {
  if (C.REVIEW_RE.test(messageText)) return null;
  return "peer-review footer missing — add `Reviewed-By: <reviewer>` (one or more) or `Review-Skipped: <reason>`.";
}

export function visualReview(messageText: string): string | null {
  if (!C.CAPTURE_PATH_RE.test(messageText)) return null;
  if (C.REVIEW_VISUAL_RE.test(messageText)) return null;
  return "body references Build/captures/ — add `Reviewed-Visually: <reviewer> — <improvement|regression|uncertain|per-scene-mixed>` or `Review-Skipped-Visual: <reason>`.";
}

export function countBodyLines(message: string): number {
  const lines = (message || "").split("\n");
  if (lines.length <= 1) return 0;
  let i = 1;
  while (i < lines.length && lines[i].trim() === "") i++;
  const bodyStart = i;
  let j = lines.length - 1;
  while (j >= bodyStart && lines[j].trim() === "") j--;
  let trailerStart = j + 1;
  while (trailerStart > bodyStart && C.TRAILER_RE.test(lines[trailerStart - 1])) trailerStart--;
  let bodyEnd = trailerStart;
  while (bodyEnd > bodyStart && lines[bodyEnd - 1].trim() === "") bodyEnd--;
  return Math.max(0, bodyEnd - bodyStart);
}

export function commitBodyCap(messageText: string): string | null {
  const n = countBodyLines(messageText);
  if (n <= C.BODY_LINE_CAP) return null;
  return `commit body ${n} lines > cap ${C.BODY_LINE_CAP} (trailers excluded). Trim the body — the diff says WHAT, the body says WHY.`;
}

// ---- staged-file gates (pure over name-status / staged list) ---------------

export function noImages(nameStatus: NameStatusEntry[]): string | null {
  const bad = nameStatus
    .filter((e) => (e.status === "A" || e.status === "M" || e.status === "R") && C.IMAGE_EXT_RE.test(e.path) && !C.NO_IMAGES_EXCLUDE_RE.test(e.path))
    .map((e) => e.path);
  if (!bad.length) return null;
  return `image files staged:\n${fmtList(bad)}\nImages are forbidden. Allowlist: Data/Engine/Icons/, Source/Editor-Next/tests/*-snapshots/. Use Build/captures/ (gitignored) for local outputs.`;
}

export function noNewMd(nameStatus: NameStatusEntry[]): string | null {
  const bad = nameStatus
    .filter((e) => e.status === "A" && e.path.endsWith(".md") && !C.NEW_MD_ALLOWLIST_RE.test(e.path))
    .map((e) => e.path);
  if (!bad.length) return null;
  return `new .md outside allowlist:\n${fmtList(bad)}\nAllowed: .backlog/tasks/, .omp/{agents,skills,commands,state,extensions}/, AGENTS/README/LICENSE.md. Audit/design content lives in task notes or the commit body.`;
}

export function dataGenerated(staged: string[], gitignoreDiffText: string): string | null {
  const stagedGen = staged.filter((f) => C.DATA_GENERATED_PATH_RE.test(f));
  const ignoreViol: string[] = [];
  for (const raw of (gitignoreDiffText || "").split("\n")) {
    if (raw.startsWith("---") || raw.startsWith("+++")) continue;
    if (raw.startsWith("-") && C.PROTECTED_IGNORE_LINES.includes(raw.slice(1).trim())) ignoreViol.push(`removes ignore mask: ${raw}`);
    else if (raw.startsWith("+") && C.UNIGNORE_ADDED_RE.test(raw)) ignoreViol.push(`adds un-ignore: ${raw}`);
  }
  if (!stagedGen.length && !ignoreViol.length) return null;
  const parts: string[] = ["Data/Generated/ is derived runtime output and must not be tracked."];
  if (stagedGen.length) parts.push(`staged under Data/Generated/:\n${fmtList(stagedGen)}`);
  if (ignoreViol.length) parts.push(`.gitignore loosens the mask:\n${fmtList(ignoreViol)}`);
  return parts.join("\n");
}

export function fileSize(staged: string[], blobCount: (spec: string) => number, renames: Map<string, string>): string | null {
  const viol: string[] = [];
  for (const f of staged) {
    if (!C.FILE_SIZE_EXT_RE.test(f) || C.HARNESS_OR_VENDOR_RE.test(f)) continue;
    const newLines = blobCount(`:${f}`);
    if (newLines <= C.FILE_SIZE_LIMIT) continue;
    let oldLines = blobCount(`HEAD:${f}`);
    if (oldLines === 0 && renames.has(f)) oldLines = blobCount(`HEAD:${renames.get(f)}`);
    if (newLines <= oldLines) continue;
    viol.push(`${f}: ${oldLines} → ${newLines}`);
  }
  if (!viol.length) return null;
  return `file(s) > ${C.FILE_SIZE_LIMIT} lines AND growing:\n${fmtList(viol)}\nNo-growth touches pass; renames are followed. Split per the file-splitting skill.`;
}

export function parseTaskRefs(messageText: string): number[] {
  const ids = new Set<number>();
  for (const m of messageText.matchAll(C.TASK_REF_RE)) ids.add(parseInt(m[1], 10));
  return [...ids];
}

export function closureStaleness(messageText: string, staged: string[], statusOf: (id: number) => string | null): string | null {
  if (messageText.includes(C.SKIP_STALENESS_SENTINEL)) return null;
  const refs = parseTaskRefs(messageText);
  if (!refs.length) return null;
  if (staged.length > 0 && staged.every((f) => C.DOCS_ONLY_PATH.test(f))) return null;
  const stale: string[] = [];
  for (const id of refs) {
    const s = statusOf(id);
    if (s && C.OPEN_STATUSES[s.toLowerCase()]) stale.push(`TASK-${id} (status: ${s})`);
  }
  if (!stale.length) return null;
  return `closure-staleness — open tasks referenced by a code-bearing commit:\n${fmtList(stale)}\nFlip the task to Done in this CL, land now + follow up with a docs(backlog) flip CL, or add ${C.SKIP_STALENESS_SENTINEL} for genuinely partial work.`;
}

export function scanDiff(diff: string): { essay: number; refs: number } {
  let runLen = 0;
  let reported = false;
  let essay = 0;
  let refs = 0;
  for (const line of diff.split("\n")) {
    if (line.match(/^@@ /)) { runLen = 0; reported = false; continue; }
    if (line.startsWith("+++") || line.startsWith("---")) continue;
    if (line.startsWith("+")) {
      if (/^\+\s*\/\//.test(line)) {
        if (C.REF_PATTERNS.some((re) => re.test(line))) refs++;
        runLen++;
        if (runLen > C.ESSAY_CAP && !reported) { essay++; reported = true; }
      } else runLen = 0;
    } else if (line.startsWith(" ")) runLen = 0;
  }
  return { essay, refs };
}

export function commentEssayCap(staged: string[], diffOf: (f: string) => string): string | null {
  let essay = 0;
  let refs = 0;
  for (const f of staged) {
    if (!C.COMMENT_CODE_EXT_RE.test(f) || C.HARNESS_OR_VENDOR_RE.test(f)) continue;
    const r = scanDiff(diffOf(f));
    essay += r.essay;
    refs += r.refs;
  }
  if (!essay && !refs) return null;
  const parts: string[] = [];
  if (essay) parts.push(`${essay} added comment run(s) > ${C.ESSAY_CAP} contiguous // lines — replace explanatory essays with a one-line WHY or delete.`);
  if (refs) parts.push("added comment(s) cite a tracker id / RFC / phase — those live in the tracker, not source.");
  return parts.join("\n");
}

// ---- in-turn verification gates (consume turn-state flags) -----------------

export function testRun(staged: string[], closingTasks: string[], messageText: string, ran: boolean): string | null {
  const allDocs = staged.length > 0 && staged.every((f) => C.DOCS_ONLY_PATH.test(f));
  const closureExempt = closingTasks.length > 0 && C.CLOSURE_REASON_RE.test(messageText);
  if (allDocs && (closingTasks.length === 0 || closureExempt)) return null;
  if (ran) return null;
  const head = closingTasks.length > 0
    ? "task closure without an integration test this turn (add `Closure-Reason: <value>` for obsolete/non-reproducible/superseded closures)."
    : "no integration test run in this turn.";
  return `${head}\nRun one: Main.exe -total_frames N | Main.exe -capture_frame N | RenderTest.exe -test <name> | Main.exe -serialize_test <scene> | InteractiveTest.ps1 | npx playwright test tests/<spec>.spec.js`;
}

export function liveEngine(staged: string[], ran: boolean): string | null {
  if (!staged.some((f) => C.EDITOR_CODE_PATH.test(f) && !C.DOCS_ONLY_PATH.test(f))) return null;
  if (ran) return null;
  return "editor code staged but no live-engine test ran. Run `npx playwright test` against a real-engine spec (`--engine=Main`), or Main.exe -total_frames N / RenderTest.exe -test <name>. Mock-only specs hide optimistic-vs-server races.";
}

export function serializeTest(staged: string[], ran: boolean): string | null {
  if (!staged.some((f) => C.SERIALIZER_CODE_PATH.test(f) && !C.DOCS_ONLY_PATH.test(f))) return null;
  if (ran) return null;
  return "serializer code staged but no serialize-test ran. Run: Main.exe -mode 0 -renderer 0 -loglevel 0 -offscreen -serialize_test ExampleProject/Scenes/UnitTest.InnoScene";
}

// ---- test-command classification (drives turn-state in index.ts) -----------

export function classifyTestCommand(cmd: string, readSpec: (rel: string) => string | null): { qualifying: boolean; live: boolean; serialize: boolean } {
  const qualifying = C.QUALIFYING_TEST.test(cmd);
  const serialize = C.SERIALIZE_TEST_RE.test(cmd);
  let live = C.NON_PLAYWRIGHT_LIVE.test(cmd);
  if (!live) {
    const pw = cmd.match(C.PLAYWRIGHT_RE);
    if (pw) {
      const args = (pw[1] || "").trim();
      const files = args ? args.split(/\s+/).filter((s) => s && !s.startsWith("-")) : [];
      if (files.length === 0) live = true;
      else for (const f of files) { const c = readSpec(f); if (c && c.includes("--engine=Main")) { live = true; break; } }
    }
  }
  return { qualifying, live, serialize };
}

// ---- untrustworthy commit invocations --------------------------------------
// The gate reads the staged index (`git diff --cached`) at the moment it intercepts
// the bash `git commit`. Two shapes make that view a lie:
//   1. chained staging — `git add … && git commit …` runs add AFTER interception;
//   2. auto-stage — `git commit -a/--all` commits unstaged tracked changes too.
// Both are rejected so staging is explicit and the gate's index view is authoritative.
const STAGING_VERB_RE = /\bgit\s+(?:add|stage|rm|mv|restore|reset)\b/;
const AUTO_STAGE_RE = /(?:^|\s)-[A-Za-z]*a[A-Za-z]*(?=\s|=|$)|--all\b/;

export function unsafeCommitInvocation(cmd: string): string | null {
  const u = cmd.replace(/"(?:\\.|[^"\\])*"/g, '""').replace(/'(?:[^'])*'/g, "''");
  if (STAGING_VERB_RE.test(u)) {
    return "stage in a separate command, then commit. A chained `git add … && git commit …` is intercepted before the staging runs, so commit-guard would gate a stale index. Run the staging step alone, then a bare `git commit`.";
  }
  if (AUTO_STAGE_RE.test(u)) {
    return "avoid `git commit -a` / `--all` — it commits unstaged tracked changes that commit-guard (which reads the staged index) cannot see. Stage explicitly with `git add …`, then a bare `git commit`.";
  }
  return null;
}
