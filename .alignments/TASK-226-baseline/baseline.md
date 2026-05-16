# TASK-226 baseline capture — partial

Initial baseline for TASK-226 umbrella AC #4 (visual parity) and AC #5 (60-FPS bar). Captured against HEAD post-TASK-226.{1,3,5,7} commits — i.e. post-WorldTileGrid-nuke, post audit-only closures.

## Capture conditions

- **Build SHA**: `d5e1fdf4` (most recent commit at capture time — TASK-226.7 bent-cone audit).
- **Engine binary**: `Bin/RelWithDebInfo/Main.exe`, built `2026-05-16 03:59 UTC` (TASK-226.1 nuke build).
- **Engine command**: `Main.exe -mode 0 -renderer 0 -loglevel 0 -total_frames 300 -dump_frames 60-63 -offscreen`
- **Scene**: GISponza autotest (loaded at frame 5).
- **CWD**: `Bin/RelWithDebInfo/`.
- **GPU/driver**: not recorded in this session — see "what is NOT captured" below.
- **Engine log**: `Bin/RelWithDebInfo/[2026-5-16-19-22-57-199].Log`.

## Frame-time numbers (wall-clock)

From log timestamps:

| Span | Frames | Wall-clock | Per-frame avg | Equivalent FPS |
|---|---|---|---|---|
| Scene loaded → terminate (frame 5 → 300) | 295 | 15.108 s | 51.2 ms | 19.5 FPS |
| Post-TLAS-rebuild steady state (frame 7 → 300) | 293 | 9.114 s | 31.1 ms | 32.1 FPS |

The "scene loaded → terminate" span includes the TLAS-rebuild stutter at frame 7 (instance count 56 → 94 on the GISponza activation second pass). The 32 FPS post-TLAS steady-state number is the representative baseline.

**60-FPS bar status against this baseline**: NOT MET. 32 FPS is roughly half the bar. AC #5 needs rescoping at TASK-226.8 closure time — likely to "no regression vs this baseline" given the port stages (.4 reprojection rewrite, .6 64-rays per probe) will add cost rather than remove it.

## Captured frames

`gpu_output_0060.png` through `gpu_output_0063.png` — 4 consecutive frames from the GISponza autotest's fixed-camera position, captured after scene + TLAS stabilization (frame 7+) and well into the steady-state window.

These 4 frames are a **temporal-stability** baseline (4 consecutive frames at the same camera), not the **4-angle spatial** baseline TASK-226.2 originally asked for.

## What is NOT captured

- **4-angle spatial coverage**: the GISponza autotest uses a fixed camera. To capture 4 angles, the run needs `-camera_orbit PITCH,RADIUS,DURATION` rotating the camera plus `-dump_frames` at 4 distinct phases. Not done in this session — engine ran with the fixed-camera default.
- **RasterizedGI=OFF toggle state**: the `RasterizedGI` dev-toggle (`ExampleRenderingClient_Setup.cpp:62-76`) flips the radiance-cache pass group on/off, but it's UI-driven only. Headless `-offscreen` mode has no way to flip it. A CLI flag for the toggle (e.g. `-rasterized_gi 0`) would unblock this; one task to file if needed.
- **GPU model + driver version**: not recorded. `dxdiag` or `nvidia-smi` would supply this; recommended for future-session reruns.
- **Per-pass GPU timer breakdown**: available via `-gpu_timer_log` flag, not enabled this run. Useful for AC #5 perf-regression analysis when .4/.6 port stages land.
- **CPU vs GPU bound determination**: the 32 FPS could reflect CPU-side autotest overhead, not pure GPU cost. The autotest runs a CPU path-tracer reference render at termination (visible in log) — that's not in the 5-300 frame range but indicates CPU work happens.

## Followup for TASK-226.8 (final port gate)

When porting completes:
1. Rerun with `-camera_orbit 15,5,300 -dump_frames 75-78 -total_frames 300` (or similar) for 4-angle coverage at the same orbit phase.
2. Rerun with `-gpu_timer_log` for per-pass GPU timings.
3. Add a CLI flag for `RasterizedGI` if a RasterizedGI=OFF comparison is desired (likely as part of TASK-227 declarative render-graph effort — toggle could become a data-driven config).
4. Diff captured frames vs these baseline frames; diff frame-times vs this 32 FPS number.

This partial baseline is enough to anchor the AC #5 "no regression" path at closure if AC #5's 60-FPS bar is rescoped (recommended given the gap matrix flagged perf risk on TASK-226.6).
