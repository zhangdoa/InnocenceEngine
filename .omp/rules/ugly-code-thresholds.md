---
name: ugly-code-thresholds
description: "When code is flagged ugly, hit code-organization thresholds — function >40 lines, nesting >3, mixed responsibilities — re-architect, not reformat"
condition: "\\b(ugly|monolithic|spaghetti|unorganized|unorganised|messy|disorganized)\\b"
scope: "text"
---

When the user flags code ugly, treat it as a code-architecture complaint, not cosmetics. Apply the `code-organization` skill thresholds: function body > 40 lines OR nesting > 3 levels OR mixed responsibilities inside one function → re-architect by splitting into focused substeps, extracting helpers, or splitting the file. Reformatting (whitespace, comment style, brace style) is not the answer. Also check: monolithic files > 500 lines, files mixing multiple responsibilities, semantically empty names (`util`/`helper`/`common`/`misc`/`data`/`stuff`).

Right reaction: 'function X is 120 lines and does 4 things — split into StageA, StageB, StageC, return merge'.

Wrong reaction: 'let me reformat the braces' or 'let me rename the local variable'.