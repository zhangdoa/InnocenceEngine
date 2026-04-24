# Discipline: visual-validation

A single static capture is never sufficient evidence for a change that affects rendered output. A visual claim requires:

- A frame sequence long enough to see temporal behaviour (flicker, boil, accumulation artefacts).
- Multiple camera angles — what's stable from one view may mask a defect visible from another.
- A reference render for ground-truth comparison where applicable.

Archive captures under `Build/captures/` with labels tied to the work. The archive is how the project proves quality goes up over time, not just sideways.
