// commit-guard — project commit gates for InnocenceEngine, ported from the
// Claude-era .claude/hooks (PreToolUse + git-commit dispatcher) to one omp
// tool_call interceptor. Loads only in this repo (project .omp/ extension).
//
// Pre-execution: every `git commit` run through omp's bash tool is checked; a
// failing gate returns { block: true, reason } so the model sees why and retries.
// In-turn gates (test-run / live-engine / serialize-test) consume turn-state
// collected from the bash commands issued since the last turn_start.
import type { ExtensionAPI } from "@oh-my-pi/pi-coding-agent";
import * as collect from "./collect.ts";
import * as gates from "./gates.ts";
import type { NameStatusEntry } from "./collect.ts";

interface TurnState {
  qualifyingTestRan: boolean;
  liveEngineRan: boolean;
  serializeTestRan: boolean;
}

// Strip quoted regions before testing, so a `git commit` inside a quoted string
// (e.g. an echo) does not trip the gate — mirrors the old commit-gate dispatcher.
function isGitCommit(cmd: string): boolean {
  const unquoted = cmd.replace(/"(?:\\.|[^"\\])*"/g, '""').replace(/'(?:[^'])*'/g, "''");
  return /\bgit\s+commit\b/.test(unquoted);
}

function evaluate(cmd: string, cwd: string, turn: TurnState): string | null {
  const messageText = collect.extractMessage(cmd, cwd);
  const staged = collect.stagedNameOnly(cwd);
  if (staged.length === 0) return null; // nothing staged → let git reject the empty commit
  const nameStatus = collect.stagedNameStatus(cwd);
  const closingTasks = collect.detectClosingTasks(cwd, staged);

  const renames = new Map<string, string>();
  for (const e of nameStatus as NameStatusEntry[]) {
    if (e.status === "R" && e.oldPath) renames.set(e.path, e.oldPath);
  }

  // Order mirrors the Claude-era commit-gate dispatcher; first failing gate wins.
  return (
    gates.dataGenerated(staged, collect.gitignoreDiff(cwd)) ??
    gates.noImages(nameStatus) ??
    gates.noNewMd(nameStatus) ??
    gates.commitBodyCap(messageText) ??
    gates.commentEssayCap(staged, (f) => collect.fileDiffCached(cwd, f)) ??
    gates.fileSize(staged, (spec) => collect.blobLineCount(cwd, spec), renames) ??
    gates.closureStaleness(messageText, staged, (id) => collect.taskStatus(cwd, staged, id)) ??
    gates.peerReview(messageText) ??
    gates.visualReview(messageText) ??
    gates.testRun(staged, closingTasks, messageText, turn.qualifyingTestRan) ??
    gates.liveEngine(staged, turn.liveEngineRan) ??
    gates.serializeTest(staged, turn.serializeTestRan) ??
    gates.attribution(messageText)
  );
}

export default function commitGuard(pi: ExtensionAPI): void {
  pi.setLabel?.("commit-guard");
  const turn: TurnState = { qualifyingTestRan: false, liveEngineRan: false, serializeTestRan: false };

  pi.on("turn_start", async () => {
    turn.qualifyingTestRan = false;
    turn.liveEngineRan = false;
    turn.serializeTestRan = false;
  });

  pi.on("tool_call", async (event, ctx) => {
    if (event.toolName !== "bash") return;
    const cmd = String(event.input?.command ?? "");
    if (!cmd) return;
    const cwd = ctx.cwd;

    // Record qualifying test/engine commands as they are issued this turn.
    const cls = gates.classifyTestCommand(cmd, (rel) => collect.readSpec(cwd, rel));
    if (cls.qualifying) turn.qualifyingTestRan = true;
    if (cls.live) turn.liveEngineRan = true;
    if (cls.serialize) turn.serializeTestRan = true;

    if (!isGitCommit(cmd)) return;

    const unsafe = gates.unsafeCommitInvocation(cmd);
    if (unsafe) return { block: true, reason: `[commit-guard] git commit blocked —\n${unsafe}` };

    let reason: string | null = null;
    try {
      reason = evaluate(cmd, cwd, turn);
    } catch {
      return; // fail-open: an internal error must never wedge commits
    }
    if (reason) return { block: true, reason: `[commit-guard] git commit blocked —\n${reason}` };
  });
}
