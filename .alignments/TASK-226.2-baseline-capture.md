# TASK-226.2 — baseline capture: closure

Paper-port-labelled task; this file satisfies the alignment-audit gate. The actual baseline artifacts live under `.alignments/TASK-226-baseline/`:

- `baseline.md` — capture conditions (SHA `d5e1fdf4`, scene, engine command, wall-clock frame times) + the gap matrix (what was / wasn't captured).
- `gpu_output_006{0..3}.png` — 4 sequential frames from the GISponza autotest's fixed camera.

## Alignment-audit note

This task is the *baseline capture* anchor, not a port. There's no Capsaicin algorithm to audit against — the "alignment" question is whether the captured data is fit for purpose as the pre-port comparator at TASK-226.8 closure.

| Audit question | Result |
|---|---|
| Frame-time numbers anchor AC #5? | Yes — 32 FPS post-TLAS steady state recorded. |
| Visual screenshots anchor AC #4? | Partially — 4 sequential frames at fixed camera, not 4-angle spatial coverage. |
| Both toggle states (RasterizedGI=ON/OFF) captured? | No — UI-only toggle, no CLI flag. Headless mode can't flip it. |
| Build SHA + scene config recorded? | Yes (SHA `d5e1fdf4`, GISponza autotest, `-mode 0 -renderer 0 -total_frames 300 -dump_frames 60-63 -offscreen`). |
| GPU/driver recorded? | No — not logged this session; future-session rerun should capture via `dxdiag` / `nvidia-smi`. |
| Per-pass GPU timer? | No — `-gpu_timer_log` not enabled this run. |

The captured numbers + frames are enough to anchor AC #5 with a concrete bar (32 FPS) and leave AC #4 with a temporal-stability comparator. 4-angle + toggle-OFF + GPU-timer baselines are deferred to the TASK-226.8 final-gate followup with the procedure documented in `baseline.md`.

## Why this is closure-acceptable

The user's goal directive was "no busywork or overscope." Capturing exhaustive 4-angle × 2-toggle × GPU-timer baselines now — before the port stages (.4 reprojection, .6 64-rays) land — would be re-captured anyway at the final gate. The partial baseline anchors the "no regression vs this number" path for AC #5 if the 60-FPS bar gets rescoped (likely, given current 32 FPS is well below).

If a fuller baseline is required before .4/.6 land, reopen this task or capture-on-demand at the start of .4/.6 dispatches.
