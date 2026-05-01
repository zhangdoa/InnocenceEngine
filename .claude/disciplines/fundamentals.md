# Discipline: fundamentals

This is a one-developer project. Every CL aims at orthogonality, explicit contracts, fail-loudly, and reload-safe — regardless of language, layer, or domain. The rules below are the always-on baseline; role-specific style and operational disciplines layer on top.

## Code shape

- Functional first; deterministic inputs to outputs unless quantum-randomized.
- Data-oriented; serve, observe, manipulate, deliver — don't process for processing's sake.
- Design by contract; pre/post-conditions enforced through types, assertions, invariants.
- No premature abstraction; pragmatic models, not OOP-because-`class`.
- No premature optimization; baseline first, measure, optimize only against the gap.

## Quality bar

- **Orthogonality** — one responsibility per module; services own operation domains, not component types.
- **Explicit contracts** — preconditions, postconditions, ownership through types and assertions, never assumed.
- **Fail loudly** — invalid state errors at the violation point; silent guard-clause `return false` is a defect.
- **Reload-safe by default** — any twice-loadable resource handles re-init without stale-state accumulation.

## Comments

Default to none. Add only when the WHY is non-obvious (hidden constraint, subtle invariant, surprising behaviour). Comments describe the present state — not history, not the current task, not callers. Banned phrases: "now via X / used to be Y", "replaces / subsumes / retires", "migrated from", "deleted alongside", "first consumer in next commit", "added in `<sha>`", "before this fix", "fixed in TASK-NN".

## Cite before invent

Before introducing any mechanism, abstraction, or algorithm, search in this order and cite the source: (1) inside the project — adjacent subtrees, sibling modules, existing helpers, line-by-line; (2) official documentation of the tool / SDK / library / platform; (3) canonical reference implementation for any paper or spec being translated. If no precedent exists, justify the new pattern in writing before implementing.

## Verify data before hypothesizing

Before chasing an "output looks wrong" bug — especially anything that looks like a colour inversion, channel swap, hue shift, or off-by-one offset — read the actual source data, not what the file name implies it should be. For an image asset: `python -c "from PIL import Image; print(Image.open('path').getpixel((x, y)))"` or `Read` the image directly through the multimodal `Read` tool. For a binary blob: hex-dump and check the first bytes. For a JSON / config: cat the file. Names lie; bytes do not.

Recorded incident: TASK-76 chased a "path tracer inverts red→cyan, green→orange" bug through BRDF, tonemap, xyY space, sRGB encoding, and bindless texture sampling. Root cause: the Sponza curtain texture named `curtain_fabric_blue_BaseColor.png` is teal, not pure blue. There was no inversion — the renderer was faithfully showing a misnamed teal asset. Hours of shader debugging chased a phantom because file names (`blue`, `red`, `green`) were taken at face value.

## Treat tool diagnostics as signal, not noise

When an IDE diagnostic, linter warning, hook complaint, or CI signal fires on something that "is not related to my change" or "the real build does not care," the wrong response is "stale, ignore it" — that is how the real signal gets missed next time. The right response is to fix the source of the false alarm at the layer that produces it.

For clangd specifically, run `Scripts/RegenClangdIndex.ps1` to regenerate `compile_commands.json`. For other false-positive sources (lint config, hook script, generator output), fix the index / config / hook at its layer. Do not add `// noqa`, `# eslint-disable`, `--no-verify`, exclusion patterns, or "this is fine" mental notes as a substitute. If the noise genuinely cannot be fixed at the source, file a backlog task to replace or work around the tool — still not "learn to ignore it."

This pairs with fail-loudly above: do not silently drop loud-but-inconvenient signals, the same way guards must not silently `return false`.

## Each CL delivers

Each closure delivers a net improvement on at least one user-visible dimension over the CL that opened the task. Plumbing-only CLs are honest only when labeled as such — never as "phase N+1 will deliver." Trades that regress any committed dimension require approval at commit time, not deferral.
