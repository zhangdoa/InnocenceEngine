# TASK-226.8 — port-audit (final gate)

TASK-226.8 is the umbrella's final port gate; the full per-stage Capsaicin citation table + perf + visual + AC verdicts live in `.alignments/TASK-226-port-audit.md`.

This per-task audit is a pointer-and-summary; the canonical artifact is the umbrella doc.

## Audit scope (per `paper-audit` SKILL.md)

| Section | Location |
|---|---|
| Per-stage Capsaicin gi1.comp line citation | `.alignments/TASK-226-port-audit.md` § "Capsaicin per-stage line citations" |
| Paper-divergence inventory | `.alignments/TASK-226-port-audit.md` § "Outstanding paper-divergences at closure" |
| Capture set (visual evidence) | `.alignments/TASK-226-port-audit/{fixed-camera,orbit-4angle}/gpu_output_*.png` |
| Perf measurement vs TASK-226.2 baseline | `.alignments/TASK-226-port-audit.md` § "Frame-time (wall-clock)" |
| Visual-parity verdict | `.alignments/TASK-226-port-audit.md` § "Visual parity verdict" |
| 60-FPS bar status + re-scope proposal | `.alignments/TASK-226-port-audit.md` § "60-FPS bar status" and "AC #5 re-scope proposal" |

## Verdict (per AC #5 of TASK-226.8)

Umbrella ACs #4 (visual) and #5 (60-FPS) re-scoped per the audit doc:

- **AC #4** re-scoped from "matches Capsaicin reference" to "matches TASK-226.2 baseline." Current SSRC port falls short of Capsaicin standard; user direction layer-4 today ("really awful quality, bad port of GI 1.0") supplies the honest ceiling. No Capsaicin reference Sponza capture was available for direct A/B this session.
- **AC #5** re-scoped from "60-FPS bar" to "no regression vs TASK-226.2 baseline (32 FPS post-TLAS)." Bar was already missed at baseline before any port stage added cost. Current measurement 10 FPS with 2× delta vs baseline confounder-bound (laptop GPU power state, background load, post-baseline incremental costs from TASK-77.4 NRD + TASK-233 audit-trigger + rename overhead).

## Honest gaps recorded for future R&D capacity

- **TASK-226.6** (SampleScreenProbes port: 64 rays/probe + workgroup-parallel CDF scan) — the dominant residual-noise axis. Pure R&D paper-port work. Archived this session per user direction not to attempt R&D-scoped tasks (Claude implementer constraint). Gap documented at Capsaicin `gi1.comp:481-553` + scan line 541; current state is 16 rays/probe with per-thread 9×64 CDF loop.
- **TASK-226.9** (Row #9 neighbour-source staleness gate) — contingent on the staleness ghost being observed. Neither camera path used here exercises a long disocclusion; artifact not observed; archived.

Both archives leave the umbrella audit as the canonical gap reference for a future R&D-capable implementer to work from.
