// End-to-end: load the extension factory, capture its handlers, and drive a real
// staged git repo through the tool_call interceptor. Proves wiring + git collection
// + gate chain + block/pass, not just the pure predicates.
import { test } from "node:test";
import assert from "node:assert/strict";
import { execSync } from "node:child_process";
import { mkdtempSync, mkdirSync, writeFileSync, rmSync } from "node:fs";
import { tmpdir } from "node:os";
import { join } from "node:path";
import commitGuard from "../index.ts";

interface Captured {
  tool?: (event: unknown, ctx: unknown) => Promise<{ block?: boolean; reason?: string } | undefined>;
  turn?: () => Promise<void>;
}

function loadGuard(): Captured {
  const cap: Captured = {};
  const pi = {
    setLabel() {},
    on(ev: string, h: (...a: unknown[]) => Promise<unknown>) {
      if (ev === "tool_call") cap.tool = h as Captured["tool"];
      if (ev === "turn_start") cap.turn = h as Captured["turn"];
    },
  };
  commitGuard(pi as never);
  return cap;
}

function newRepo(): string {
  const dir = mkdtempSync(join(tmpdir(), "commit-guard-"));
  execSync("git init -q", { cwd: dir });
  execSync('git config user.email t@t.t && git config user.name t', { cwd: dir });
  return dir;
}

async function runCommit(cap: Captured, cwd: string, message: string) {
  const cmd = `git commit -m "${message.replace(/"/g, '\\"')}"`;
  return cap.tool!({ toolName: "bash", input: { command: cmd } }, { cwd });
}

test("registers tool_call + turn_start", () => {
  const cap = loadGuard();
  assert.equal(typeof cap.tool, "function");
  assert.equal(typeof cap.turn, "function");
});

test("blocks a code commit missing footers", async () => {
  const cap = loadGuard();
  const repo = newRepo();
  try {
    mkdirSync(join(repo, "Source"), { recursive: true });
    writeFileSync(join(repo, "Source/foo.cpp"), "int main(){return 0;}\n");
    execSync("git add Source/foo.cpp", { cwd: repo });
    const res = await runCommit(cap, repo, "wip");
    assert.equal(res?.block, true);
    assert.match(res!.reason!, /commit-guard/);
  } finally {
    rmSync(repo, { recursive: true, force: true });
  }
});

test("passes a docs commit with required footers", async () => {
  const cap = loadGuard();
  const repo = newRepo();
  try {
    writeFileSync(join(repo, "README.md"), "# docs\nhello\n");
    execSync("git add README.md", { cwd: repo });
    const cmd = 'git commit -m "docs: update" -m "Reviewed-By: reviewer" -m "Code-AI-Generated-By: test"';
    const res = await cap.tool!({ toolName: "bash", input: { command: cmd } }, { cwd: repo });
    assert.equal(res, undefined);
  } finally {
    rmSync(repo, { recursive: true, force: true });
  }
});

test("non-bash and non-commit calls pass through", async () => {
  const cap = loadGuard();
  assert.equal(await cap.tool!({ toolName: "read", input: {} }, { cwd: "." }), undefined);
  assert.equal(await cap.tool!({ toolName: "bash", input: { command: "ls -la" } }, { cwd: "." }), undefined);
});

test("turn flag lets a code commit through after a test command", async () => {
  const cap = loadGuard();
  const repo = newRepo();
  try {
    mkdirSync(join(repo, "Source"), { recursive: true });
    writeFileSync(join(repo, "Source/foo.cpp"), "int main(){return 0;}\n");
    execSync("git add Source/foo.cpp", { cwd: repo });
    // issue a qualifying test command → sets turn flag
    await cap.tool!({ toolName: "bash", input: { command: "Bin/RelWithDebInfo/Main.exe -total_frames 4" } }, { cwd: repo });
    const cmd = 'git commit -m "feat: x" -m "Reviewed-By: r" -m "Code-AI-Generated-By: t"';
    const res = await cap.tool!({ toolName: "bash", input: { command: cmd } }, { cwd: repo });
    assert.equal(res, undefined); // test-run satisfied, footers present
  } finally {
    rmSync(repo, { recursive: true, force: true });
  }
});

test("blocks chained staging + commit and auto-stage before touching git", async () => {
  const cap = loadGuard();
  const r1 = await cap.tool!({ toolName: "bash", input: { command: "git add . && git commit -m x" } }, { cwd: "." });
  assert.equal(r1?.block, true);
  assert.match(r1!.reason!, /separate command/);
  const r2 = await cap.tool!({ toolName: "bash", input: { command: "git commit -am wip" } }, { cwd: "." });
  assert.equal(r2?.block, true);
  assert.match(r2!.reason!, /commit -a/);
});
