# Discipline: paper-audit

Produces a structured alignment document comparing a published algorithm specification, its canonical reference implementation, and a project's in-house implementation. Makes every divergence visible at a glance; the reader uses it as a review surface at paper-port task closure.

Biased to find divergences, not confirm fidelity. DIVERGENT is the default status of any row until there is line-by-line evidence of match. "Looks approximately similar" is not evidence.

## How

### Input

The invoking prompt supplies:

1. The paper — file path and the specific section(s) to audit.
2. The reference implementation — local clone path + file:line ranges to compare against.
3. The in-house implementation — list of code / shader files to compare.
4. The output path — where to write the artifact (conventionally `.alignments/<task-id>-<short-name>.md`).

If any of the above is missing or ambiguous, ask the invoker before starting. Do not guess.

### Output shape

A single markdown file at the supplied output path:

- Header — paper citation + section, reference-impl location, audit date, counted summary (X faithful / Y divergent / Z N/A).
- Alignment table with columns: Decision · Paper spec · Reference impl · Our impl · Status.
- Detail entry under the table for every DIVERGENT row — exact paper quote or figure reference, reference file:line, our file:line, nature of divergence, impact, resolution needed.
- "Not audited" section listing anything out of scope with the reason.

### Hard rules

- Every DIVERGENT row has a detail entry. No bare statuses.
- Every FAITHFUL row cites specific lines in both the reference and our code. Verified by reading, not by trusting comments or commit messages.
- "UNCLEAR" is not a status. If match cannot be determined, the row is DIVERGENT and the detail entry explains what could not be verified.
- Prefer under-claiming fidelity. If the reference does `lerp(4, 8*N, α)` and ours does `clamp(N, 4, 8*N)`, that's DIVERGENT even if mathematically close — note the difference.
- Do not accept the invoker's summary of what the paper says. Read the paper yourself for each row.

### Out of scope for the audit itself

- Implementing fixes. Divergences are recorded, not patched.
- Closing tasks. Task-state changes belong with the main-session author.
- Opinions on which approach is "better." Record whether our implementation matches the paper's spec; adjudication is the invoker's job.

## Cross-references

- `paper-port.md` — paper-port tasks invoke this discipline at closure; the alignment artifact is the closure-evidence for a `paper-port`-labelled task.
- `peer-review-required.md` — same property: biased to find issues, not confirm correctness; PASS / FAITHFUL is asserted only with line-grounded evidence.
