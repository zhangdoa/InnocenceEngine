---
name: renderdoc-headless-inspect
description: "Inspect a RenderDoc .rdc capture headless (no GUI, no Python). scripts/rdc-to-xml.sh converts to greppable XML; grep the markers below. Use to verify indirect draws, command signatures, strides, bindings, barriers, or a struct layout the GPU actually saw."
---

`scripts/rdc-to-xml.sh <capture.rdc> <out.xml> [xml|zip.xml]` (`zip.xml` includes buffer/texture bytes)

| Looking for | Grep |
|---|---|
| GPU-driven indirect draws | `ExecuteIndirect` |
| indirect command layout | `CreateCommandSignature` (ByteStride) |
| SRV / structured-buffer stride | `StructureByteStride` |
| compute work | `Dispatch` |
| direct draws | `DrawIndexedInstanced` / `DrawInstanced` |
| state transitions | `ResourceBarrier` (StateBefore / StateAfter) |
| resource creation | `CreateCommittedResource` / `CreatePlacedResource` |
| views | `CreateShaderResourceView` / `CreateUnorderedAccessView` |
| root constants | `SetGraphicsRoot32BitConstant(s)` |

Distinct values of a field: `grep StructureByteStride out.xml | grep -oE ">[0-9]+<" | tr -d "><" | sort -n | uniq -c`

Cross-check GPU strides / sizes / offsets against the CPU and shader struct definitions — a mismatch is the bug. A written `.rdc` with exit 0 is not proof of capture: confirm a non-trivial file size and begin/end markers first.
