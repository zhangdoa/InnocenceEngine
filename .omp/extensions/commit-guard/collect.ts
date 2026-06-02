// Git I/O for commit-guard: collect staged state + the commit message. All helpers
// fail soft (return empty) so a git hiccup never hard-blocks a commit by accident.
import { execSync } from "node:child_process";
import { readFileSync } from "node:fs";
import { isAbsolute, join } from "node:path";

export interface NameStatusEntry {
  status: string; // A | M | D | R | C | T ...
  path: string;
  oldPath?: string;
}

function git(cwd: string, args: string): string {
  try {
    return execSync(`git -c core.quotePath=false ${args}`, {
      cwd,
      encoding: "utf8",
      stdio: ["pipe", "pipe", "pipe"],
    });
  } catch {
    return "";
  }
}

export function stagedNameOnly(cwd: string): string[] {
  return git(cwd, "diff --cached --name-only")
    .split("\n")
    .map((s) => s.trim())
    .filter(Boolean);
}

export function stagedNameStatus(cwd: string): NameStatusEntry[] {
  const out: NameStatusEntry[] = [];
  for (const line of git(cwd, "diff --cached --name-status --find-renames").split("\n")) {
    if (!line.trim()) continue;
    const parts = line.split("\t");
    const status = parts[0];
    if (status[0] === "R" || status[0] === "C") {
      out.push({ status: status[0], oldPath: parts[1], path: parts[2] });
    } else {
      out.push({ status: status[0], path: parts[1] });
    }
  }
  return out;
}

export function blobLineCount(cwd: string, spec: string): number {
  const content = git(cwd, `show "${spec.replace(/"/g, '\\"')}"`);
  if (!content) return 0;
  const parts = content.split("\n");
  if (parts.length > 0 && parts[parts.length - 1] === "") parts.pop();
  return parts.length;
}

export function renameMap(cwd: string): Map<string, string> {
  const map = new Map<string, string>();
  for (const e of stagedNameStatus(cwd)) {
    if (e.status === "R" && e.oldPath) map.set(e.path, e.oldPath);
  }
  return map;
}

export function fileDiffCached(cwd: string, file: string): string {
  return git(cwd, `diff --cached --no-color -- "${file.replace(/"/g, '\\"')}"`);
}

export function gitignoreDiff(cwd: string): string {
  return git(cwd, "diff --cached -- .gitignore");
}

const STATUS_DONE_ADDED_RE = /^\+status:\s*Done\b/im;

export function detectClosingTasks(cwd: string, staged: string[]): string[] {
  const closing: string[] = [];
  for (const f of staged) {
    if (!f.startsWith(".backlog/tasks/")) continue;
    const diff = git(cwd, `diff --cached -U0 -- "${f.replace(/"/g, '\\"')}"`);
    if (STATUS_DONE_ADDED_RE.test(diff)) closing.push(f);
  }
  return closing;
}

// Effective status of TASK-<id>: prefer the staged blob (covers same-CL edits),
// else the on-disk file. Returns the raw status string or null.
export function taskStatus(cwd: string, staged: string[], id: number): string | null {
  let file: string | null = null;
  const stagedHit = staged.find(
    (f) => f.startsWith(".backlog/tasks/") && /(^|\/)task-(\d+) /i.test(f) && f.toLowerCase().includes(`task-${id} `),
  );
  if (stagedHit) file = stagedHit;
  if (!file) return null;
  const content = git(cwd, `show ":${file.replace(/"/g, '\\"')}"`);
  if (!content) return null;
  const fm = content.match(/^---\r?\n([\s\S]*?)\r?\n---/);
  if (!fm) return null;
  const m = fm[1].match(/^status:\s*(.+?)\s*$/im);
  return m ? m[1].trim() : null;
}

// Read a spec file referenced by a `playwright test <spec>` command (for live-engine).
export function readSpec(cwd: string, relOrAbs: string): string | null {
  const editorDir = join(cwd, "Source", "Editor-Next");
  const abs = isAbsolute(relOrAbs) ? relOrAbs : join(editorDir, relOrAbs);
  try {
    return readFileSync(abs, "utf8");
  } catch {
    return null;
  }
}

// Pull the commit message out of a `git commit` command: -F/--file content wins,
// else joined -m/--message values, else the raw command (so footer gates still scan).
export function extractMessage(cmd: string, cwd: string): string {
  const fileM = cmd.match(/\s(?:-F|--file)(?:=|\s+)("[^"]+"|'[^']+'|\S+)/);
  if (fileM) {
    const raw = fileM[1].replace(/^['"]|['"]$/g, "");
    const candidates = [isAbsolute(raw) ? raw : join(cwd, raw)];
    const msys = raw.replace(/^\/([a-zA-Z])\//, "$1:/");
    if (msys !== raw) candidates.push(msys);
    for (const c of candidates) {
      try {
        return readFileSync(c, "utf8");
      } catch {
        /* try next */
      }
    }
    return cmd;
  }
  const msgs: string[] = [];
  const re = /\s(?:-m|--message)(?:=|\s+)("(?:\\.|[^"\\])*"|'[^']*'|\S+)/g;
  let m: RegExpExecArray | null;
  while ((m = re.exec(cmd))) msgs.push(m[1].replace(/^['"]|['"]$/g, ""));
  return msgs.length ? msgs.join("\n\n") : cmd;
}
