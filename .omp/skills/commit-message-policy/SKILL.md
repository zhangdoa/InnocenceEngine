---
name: commit-message-policy
description: "Commit message template. Write to Build/commit-message.txt with the write tool (not a heredoc — a blocked gate leaves stale heredoc content). commit-guard validates footers and the 40-line body cap."
---

`git commit -F Build/commit-message.txt`

    type(scope): summary

    <body, <= 40 lines>

    Code-AI-Generated-By: <model>   # and/or Message-AI-Generated-By: <model>
    Closure-Reason: <value>         # only on a closure CL exempt from test-run

Subject <= 72 chars, imperative.
