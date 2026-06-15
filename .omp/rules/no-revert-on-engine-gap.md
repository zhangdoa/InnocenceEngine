---
name: no-revert-on-engine-gap
description: "Stop reverting working-tree changes mid-debug when the failure points at an engine gap — patch the engine instead"
condition: "(?:let\\s+me\\s+revert|going\\s+to\\s+revert|revert(?:ed|ing)?\\s+(?:the|this|everything|back|all|my)\\s+(?:migration|changes|state|tree|work)|git\\s+checkout\\s+HEAD\\s+--|revert\\s+the\\s+migration|revert\\s+and\\s+push|reverted\\s+to\\s+clean)"
scope: ["text", "tool:bash"]
---

The failure surfaced an engine primitive gap (e.g. depth-as-SRV, raster OM addressable by name, ping-pong parity, missing transition state). The fix is in the engine, not in discarding the migration. Keep going: add a primitive to RenderGraphDesc, refactor the resource, or write a hook that bridges the gap. Reverting to a clean tree wastes the diagnostic trail and the half-built work. Revert is only appropriate when the work is genuinely wrong, not when the engine simply doesn't have a missing knob yet. If the gap is too big to close in this turn, log a TODO with the exact failure site and continue with what does work — don't throw away the partial migration.