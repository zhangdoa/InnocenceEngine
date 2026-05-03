# Discipline: shader-standards

Engine-specific HLSL conventions (row-major matrices, identity-fallback transforms, cross-queue UAV barriers, workgroup-uniform `*WithGroupSync`) that the GPU and DX12 / Vulkan toolchains do not enforce at compile time.

## Matrix multiplication convention

The engine uploads **row-major** matrices. HLSL's default storage is **column-major** (GPU sees the transpose). Always use row-vector × matrix order:

```hlsl
float4 viewPos = mul(myVector, g_Frame.p_inv);     // CORRECT
// NOT: mul(g_Frame.p_inv, myVector);               // transposes the operation
```

Reference: `common/skyResolver.hlsl`.

## Default transform must be identity, not zero

When a transform is unavailable, fall back to **identity**, never `Mat4{}` (all zeros collapses geometry):

```cpp
Mat4 l_Transform = l_world ? l_world->m_WorldMatrix : Math::generateIdentityMatrix<float>();
```

## Cross-queue UAV writes require DeviceMemoryBarrier

Any compute shader that writes a UAV buffer consumed by another queue must issue `DeviceMemoryBarrier()` after all writes. Fence signalling guarantees command-list retirement, not write visibility through the GPU memory hierarchy.

```hlsl
u_DrawCommandBuffer[objectIndex] = BuildIndirectDrawCommand(objectIndex, modelData, isVisible);
DeviceMemoryBarrier(); // writes visible to graphics queue after fence
```

## No early return before `*WithGroupSync`

Every thread in a workgroup must reach every `GroupMemoryBarrierWithGroupSync` call (and every other `*WithGroupSync` variant). A thread that `return`s early while others block on the barrier deadlocks the group — manifests as GPU hang or silent corruption, never a compile error.

Use an `earlyExit` flag pattern: compute it at the top of `main`, keep participating in barriers, branch on the flag for the UAV-write / output phase:

```hlsl
[numthreads(8, 8, 1)]
void main(ComputeInputType input)
{
    bool earlyExit = <sky/oob/whatever>;

    // initialise shared state — ALL threads participate
    if (input.groupIndex == 0) sharedScore = 0xFFFFFFFF;
    GroupMemoryBarrierWithGroupSync();          // barrier 1 — uniform

    if (!earlyExit) { /* compute contribution */ }
    GroupMemoryBarrierWithGroupSync();          // barrier 2 — uniform

    if (earlyExit) { /* clear tile, return OK */ return; }
    /* normal write path */
}
```

Reference: `RadianceCacheReprojection.comp`. `DeviceMemoryBarrier` / `GroupMemoryBarrier` (no `WithGroupSync` suffix) are memory fences only and do *not* have this uniformity constraint.

## Cross-references

- `on-implement/safety-observability.md` — *no magic numbers* and *no copy-paste* apply to HLSL.
- `always/fundamentals.md` — *fail loudly* and *explicit contracts* are the underlying quality bar.
