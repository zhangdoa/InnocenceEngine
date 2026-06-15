import { test } from "node:test";
import assert from "node:assert/strict";
import * as g from "../gates.ts";

const ATTR = "Code-AI-Generated-By: claude";
const REV = "Reviewed-By: reviewer";

test("attribution: requires AI-generated footer", () => {
  assert.equal(g.attribution(`subject\n\n${ATTR}`), null);
  assert.equal(g.attribution("Message-AI-Generated-By: x"), null);
  assert.ok(g.attribution("subject\n\nReviewed-By: x"));
});

test("peerReview: Reviewed-By or Review-Skipped", () => {
  assert.equal(g.peerReview("s\n\nReviewed-By: r"), null);
  assert.equal(g.peerReview("s\n\nReview-Skipped: backlog-only"), null);
  assert.ok(g.peerReview("s\n\nnope"));
});

test("visualReview: only when body cites Build/captures/", () => {
  assert.equal(g.visualReview("no captures here"), null);
  assert.ok(g.visualReview("see Build/captures/x.png"));
  assert.equal(g.visualReview("Build/captures/x.png\n\nReviewed-Visually: r — improvement"), null);
  assert.equal(g.visualReview("Build/captures/x.png\n\nReview-Skipped-Visual: n/a"), null);
});

test("countBodyLines: trailers + blank lines excluded", () => {
  assert.equal(g.countBodyLines("subject\n\nbody1\nbody2\n\nReviewed-By: r"), 2);
  assert.equal(g.countBodyLines("subject only"), 0);
});

test("commitBodyCap: blocks > 40 body lines", () => {
  const body = Array.from({ length: 45 }, (_, i) => `line ${i}`).join("\n");
  assert.ok(g.commitBodyCap(`subject\n\n${body}`));
  const ok = Array.from({ length: 40 }, (_, i) => `line ${i}`).join("\n");
  assert.equal(g.commitBodyCap(`subject\n\n${ok}`), null);
});

test("noImages: blocks A/M/R images outside allowlist", () => {
  assert.ok(g.noImages([{ status: "A", path: "Foo/bar.png" }]));
  assert.equal(g.noImages([{ status: "A", path: "Data/Engine/Icons/x.png" }]), null);
  assert.equal(g.noImages([{ status: "A", path: "Source/Editor-Next/tests/a-snapshots/x.png" }]), null);
  assert.equal(g.noImages([{ status: "D", path: "Foo/bar.png" }]), null); // deletes allowed
  assert.equal(g.noImages([{ status: "A", path: "Source/foo.cpp" }]), null);
});

test("noNewMd: blocks new .md outside allowlist (renames/mods pass)", () => {
  assert.ok(g.noNewMd([{ status: "A", path: "Notes/random.md" }]));
  assert.equal(g.noNewMd([{ status: "A", path: ".omp/skills/x/SKILL.md" }]), null);
  assert.equal(g.noNewMd([{ status: "A", path: ".backlog/tasks/task-1 - x.md" }]), null);
  assert.equal(g.noNewMd([{ status: "A", path: "README.md" }]), null);
  assert.equal(g.noNewMd([{ status: "A", path: "AGENTS.md" }]), null);
  assert.equal(g.noNewMd([{ status: "A", path: ".omp/agents/x.md" }]), null);
  assert.ok(g.noNewMd([{ status: "A", path: ".claude/skills/x/SKILL.md" }])); // de-claude-codized: no longer allowlisted
  assert.equal(g.noNewMd([{ status: "M", path: "Notes/random.md" }]), null); // modify allowed
  assert.equal(g.noNewMd([{ status: "R", oldPath: "a.md", path: "b.md" }]), null); // rename allowed
});

test("dataGenerated: staged path or loosened ignore", () => {
  assert.ok(g.dataGenerated(["Data/Generated/x.bin"], ""));
  assert.ok(g.dataGenerated([], "-/Data/Generated/*"));
  assert.ok(g.dataGenerated([], "+!/Data/Generated/keep"));
  assert.equal(g.dataGenerated(["Source/foo.cpp"], ""), null);
});

test("fileSize: blocks > limit AND growing; renames baseline; no-grow passes", () => {
  const renames = new Map<string, string>();
  const block = g.fileSize(["Source/a.cpp"], (spec) => (spec.startsWith(":") ? 320 : 100), renames);
  assert.ok(block);
  assert.equal(g.fileSize(["Source/a.cpp"], (spec) => (spec.startsWith(":") ? 320 : 330), renames), null); // shrank
  assert.equal(g.fileSize(["Source/a.cpp"], () => 200, renames), null); // under limit
  assert.equal(g.fileSize([".omp/extensions/commit-guard/index.ts"], () => 999, renames), null); // harness exempt
  renames.set("Source/b.cpp", "Source/old.cpp");
  assert.equal(g.fileSize(["Source/b.cpp"], (spec) => (spec === ":Source/b.cpp" ? 320 : spec === "HEAD:Source/old.cpp" ? 330 : 0), renames), null); // rename baseline shrank
});

test("closureStaleness: open ref blocks unless sentinel/docs-only", () => {
  const open = (_id: number) => "In Progress";
  const done = (_id: number) => "Done";
  assert.ok(g.closureStaleness("fix TASK-5", ["Source/a.cpp"], open));
  assert.equal(g.closureStaleness("fix TASK-5 [task-stays-open]", ["Source/a.cpp"], open), null);
  assert.equal(g.closureStaleness("fix TASK-5", [".backlog/tasks/task-5 - x.md"], open), null); // docs-only
  assert.equal(g.closureStaleness("fix TASK-5", ["Source/a.cpp"], done), null); // already done
  assert.equal(g.closureStaleness("no refs", ["Source/a.cpp"], open), null);
});

test("scanDiff: essay run > cap and ref patterns", () => {
  const essayDiff = "@@ -1 +1 @@\n" + Array.from({ length: 7 }, () => "+ // explanatory line").join("\n");
  assert.equal(g.scanDiff(essayDiff).essay, 1);
  assert.equal(g.scanDiff("@@ -1 +1 @@\n+ // see TASK-12").refs, 1);
  assert.equal(g.scanDiff("@@ -1 +1 @@\n+ int x = 1; // ok").essay, 0);
});

test("commentEssayCap: skips harness/vendor, flags code", () => {
  const bad = "@@ -1 +1 @@\n" + Array.from({ length: 7 }, () => "+ // essay").join("\n");
  assert.ok(g.commentEssayCap(["Source/a.cpp"], () => bad));
  assert.equal(g.commentEssayCap([".omp/extensions/commit-guard/index.ts"], () => bad), null);
  assert.equal(g.commentEssayCap(["Source/a.cpp"], () => "@@ -1 +1 @@\n+int x;"), null);
  // renamed paths are skipped: a moved file's pre-existing comments are not new essays
  const ren = new Map([["Source/_Archive/a.cpp", "Source/a.cpp"]]);
  assert.equal(g.commentEssayCap(["Source/_Archive/a.cpp"], () => bad, ren), null);
});

test("testRun: docs-only skip, closure exemption, turn flag", () => {
  assert.equal(g.testRun([".backlog/tasks/x.md"], [], "", false), null); // docs-only, no closure
  assert.ok(g.testRun(["Source/a.cpp"], [], "", false)); // code, no test
  assert.equal(g.testRun(["Source/a.cpp"], [], "", true), null); // test ran
  assert.ok(g.testRun([".backlog/tasks/x.md"], [".backlog/tasks/x.md"], "no reason", false)); // closing docs, no reason, no test
  assert.equal(g.testRun([".backlog/tasks/x.md"], [".backlog/tasks/x.md"], "Closure-Reason: superseded", false), null);
});

test("liveEngine / serializeTest: path-gated on turn flag", () => {
  assert.ok(g.liveEngine(["Source/Editor-Next/src/app.ts"], false));
  assert.equal(g.liveEngine(["Source/Editor-Next/src/app.ts"], true), null);
  assert.equal(g.liveEngine(["Source/Engine/foo.cpp"], false), null);
  assert.ok(g.serializeTest(["Source/Engine/Services/AssetService.cpp"], false));
  assert.equal(g.serializeTest(["Source/Engine/Services/AssetService.cpp"], true), null);
  assert.equal(g.serializeTest(["Source/Engine/foo.cpp"], false), null);
});

test("classifyTestCommand: qualifying / live / serialize / playwright spec", () => {
  assert.deepEqual(g.classifyTestCommand("Bin/RelWithDebInfo/Main.exe -total_frames 5", () => null), { qualifying: true, live: true, serialize: false });
  assert.deepEqual(g.classifyTestCommand("Main.exe -c Engine/Configuration/Presets/Smoke.json", () => null), { qualifying: true, live: true, serialize: false });
  assert.equal(g.classifyTestCommand("Main.exe -serialize_test scene", () => null).serialize, true);
  assert.equal(g.classifyTestCommand("npx playwright test", () => null).live, true); // full suite
  assert.equal(g.classifyTestCommand("npx playwright test tests/mock.spec.js", () => "no engine").live, false);
  assert.equal(g.classifyTestCommand("npx playwright test tests/live.spec.js", () => "uses --engine=Main").live, true);
  assert.equal(g.classifyTestCommand("git commit -m x", () => null).qualifying, false);
});

test("parseTaskRefs: dedupes TASK ids", () => {
  assert.deepEqual(g.parseTaskRefs("TASK-1 and TASK-1 and TASK-23").sort((a, b) => a - b), [1, 23]);
});

test("unsafeCommitInvocation: rejects chained staging + auto-stage, allows bare commit", () => {
  assert.ok(g.unsafeCommitInvocation("git add . && git commit -m x"));
  assert.ok(g.unsafeCommitInvocation("git stage foo; git commit -F m"));
  assert.ok(g.unsafeCommitInvocation("git restore --staged a && git commit -m x"));
  assert.ok(g.unsafeCommitInvocation("git commit -am 'x'"));
  assert.ok(g.unsafeCommitInvocation("git commit -a -m x"));
  assert.ok(g.unsafeCommitInvocation("git commit --all -m x"));
  assert.equal(g.unsafeCommitInvocation("git commit -m x"), null);
  assert.equal(g.unsafeCommitInvocation("git commit -F Build/commit-message.txt"), null);
  assert.equal(g.unsafeCommitInvocation("git commit --amend -m x"), null);
  assert.equal(g.unsafeCommitInvocation('git commit -m "remember to git add later"'), null);
  assert.equal(g.unsafeCommitInvocation("cd src && git commit -m x"), null);
});
