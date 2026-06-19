# Shaders

- `HLSL/` — engine HLSL. Compiled to DXIL by `Scripts/HLSL2DXIL.ps1`.
- `HLSL/common/` — shared headers / generic helpers.
- `MSL/` — legacy Metal sources (build-disabled).

Naming: PT prefix → GPU path tracer. SSRC prefix → screen-space radiance cache (GI 1.0 port).

## HLSL struct layout — C++ ↔ HLSL agreement

Two distinct packing contexts; don't conflate them.

### StructuredBuffer<T> / RWStructuredBuffer<T>

Largest-member-alignment tail padding. No 16B row rule. No `cbuffer` rules apply.

Rules (DXC, current):
- Members packed at their natural alignment: scalars self-align to size, vectors to their component scalar size.
- Final struct size is rounded up to a multiple of the type's alignment (= largest member alignment).
- A struct with only `uint`-sizes fields is 4-aligned; only `uint64_t` fields bump to 8-aligned.
- A struct whose natural size already meets the type's alignment has **no** tail padding.

C++ side: do **not** use `alignas(N)` for `N` larger than the type's natural alignment. `alignas(16)` on a struct that only needs 8B alignment forces C++ to round up to 16B; HLSL stays at the natural 8B-multiple, and the strides drift.

Verify with `static_assert(sizeof(T) == N)` and match `N` to HLSL by reasoning from the field sizes. When in doubt, run `dxc -dump-rootsig` (or check the DXIL reflection) for the leaf shader that consumes the `StructuredBuffer<T>`; the stride is in the resource binding metadata.

### cbuffer / ConstantBuffer<T> (legacy constant buffer)

16-byte row alignment. Cross-vec4 fields bounce to the next 4-vector. `HLSL packing rules are similar to #pragma pack 4 + does-not-cross-16B-boundary` (Microsoft Learn). This is **not** the same as StructuredBuffer packing. When matching C++ to a `cbuffer`, mirror the same rules: 4B pack + 16B row.

### Quick decision

| buffer kind | HLSL rule | C++ `alignas` |
|---|---|---|
| `StructuredBuffer<T>` / `RWStructuredBuffer<T>` | largest-member tail pad | match natural alignment; do **not** over-align |
| `cbuffer` / `ConstantBuffer<T>` | 16B row alignment, 4B pack | mirror the HLSL rule by hand or use a std140 generator |

### Worked example: `MeshGeometry` (StructuredBuffer, 32B)

```
uint64_t m_VertexBufferAddress;   // 8, align 8
uint64_t m_IndexBufferAddress;    // 8, align 8
uint m_VertexCount;               // 4, align 4
uint m_IndexCount;                // 4, align 4
uint m_VertexStride;              // 4, align 4
uint m_IndexStride;               // 4, align 4
```

Largest alignment = 8. Field sum = 32. 32 is a multiple of 8 → no tail padding. HLSL size = 32B. C++ with `alignas(16)` stays at 32B (32 is a multiple of 16 too). Match.

### Sources

- [Microsoft Learn — Packing rules for constant variables](https://learn.microsoft.com/en-us/windows/win32/direct3dhlsl/dx-graphics-hlsl-packing-rules) (cbuffer rule, authoritative)
- [Microsoft Learn — Struct Type](https://learn.microsoft.com/en-us/windows/win32/direct3dhlsl/dx-graphics-hlsl-struct)
- [DXC wiki — Buffer Packing](https://github.com/microsoft/DirectXShaderCompiler/wiki/Buffer-Packing) (StructuredBuffer rule, authoritative)
- [DXC issue #121010](https://github.com/llvm/llvm-project/issues/121010) (HLSL packed struct codegen: "dxc implements this by hacking padding off of all structures")
- [maraneshi — HLSL Constant Buffer Packing Rules & Layout Visualizer](https://maraneshi.github.io/HLSL-ConstantBufferLayoutVisualizer/) (interactive; the visualizer's "largest alignment" wording is the canonical formulation)
