---
id: doc-1
title: TASK-227 Render-Graph Design RFC (Phase 0)
type: other
created_date: '2026-05-31 12:53'
---
RFC: Declarative Render-Graph for InnocenceEngine
=================================================

Status: Phase 0 design. Authored 2026-05-31. Grounds: two read-only codebase surveys (rendering-client pass anatomy + serialization infrastructure).

Supersedes the task's speculative `Source/Foundation/Serialize/` reference — that module does not exist. Decisions below are grounded in what the engine actually has today.

------------------------------------------------------------------
1. Problem restatement
------------------------------------------------------------------
Render-pass resource ownership, binding layout, and pass-to-pass ordering are expressed imperatively across 39 `*Pass.cpp` (7,723 LOC) + 4 `*ExecuteCommands*.cpp` (842 LOC) under `Source/ExampleProject/RenderingClient/`. ~74% of a representative `::Setup()` is mechanical binding/desc boilerplate; ~19% of ExecuteCommands is a stamped `WaitIfActive → Execute → Signal → Wait` cycle. Estimated eliminable: ~2,500 LOC (~29%).

Two distinct problems, separable:
- (P1) **Setup boilerplate** — resource decl + binding-layout array + RenderPassDesc config, per pass.
- (P2) **Dispatch ordering** — hand-written fence chains in ExecuteCommands, no declarative source of truth; a misordered `WaitIfActive` fails only at runtime.

------------------------------------------------------------------
2. What exists today (survey findings)
------------------------------------------------------------------
- **Target type is already data.** `RenderPassComponent` (Component/RenderPassComponent.h) = `m_RenderPassDesc` (RenderPassDesc) + `m_ResourceBindingLayoutDescs[]` (ResourceBindingLayoutDesc) + resolved pointers. The graph compiler's job is to *populate* these from serialized data instead of imperative C++. No type replacement needed (matches task out-of-scope).
- **Pass lifecycle** = `IRenderPass`: `Setup` (declare) → `Initialize` (service allocates RTVs/PSO/fences) → `PrepareCommandList` (per-frame record) → `Terminate`. Bypass attributes (`m_Bypassed`, `m_ClearOnBypass`, `RecordClearCommandList`) already live on the interface — direct home for TASK-182/171 as node attributes.
- **Dispatch** = `PassName::Get()` singletons + free-function `WaitIfActive`/`Execute`/`SignalOnGPU` (ExampleRenderingClient_Bypass.inl, GraphicsHardwareService). The cycle is mechanically uniform: `[Execute(graphicsCL)+Signal(Graphics)+WaitOnGPU(Compute,Graphics)]? → WaitIfActive(upstream, Compute, Compute) → Execute(computeCL)+Signal(Compute)`. What varies: upstream pass name, queue type, whether a graphics state-transition pre-pass exists. All derivable from per-node `reads`/`writes` + `queue`.
- **Serialization**: `nlohmann::ordered_json` (vendored submodule) + `JSONWrapper::Load/Save(const char*, json&)`. `INNO_ENUM` gives `ToString()` for all relevant enums. Scene/component load uses JSON sidecars + path search (`<Data>/<Project>/...`, `<Data>/Generated/...`). The `Reflector` libclang codegen exists but its `to_json` emitters are unwired dead code. `RenderPassComponent::to_json/from_json` declared, unimplemented (struct has `std::function<>` + raw pointers → not by-value serializable).

------------------------------------------------------------------
3. Decision D1 — Data format
------------------------------------------------------------------
**PICK: JSON via existing `nlohmann::ordered_json` + `JSONWrapper` file I/O. New hand-written `to_json`/`from_json` for new POD node structs, following `JSONSerializer_Components.cpp`. Enum fields as strings via `INNO_ENUM::ToString`.**

| Option | Verdict | Why |
|---|---|---|
| JSON on existing infra | **CHOSEN** | Parser already vendored + in use; zero new deps; readable diffs (`ordered_json` preserves order, `%.6g` float-normalize already solved). |
| YAML | Rejected | No parser vendored; new submodule for marginal readability gain over JSON. |
| In-house text format | Rejected | Reinvents a parser; no upside. |
| Reflector codegen for node structs | Deferred | Serializer emitters are dead code + naive (no pointer/function handling); wiring it is its own project. Revisit if node-struct count explodes. |
| Reuse component/scene loader (`AssetService::Load`) | Rejected for ownership | The graph is a *global pipeline description*, not per-entity scene data — it must not ride the entity/scene loader. **But** it reuses that subsystem's JSON parser + `Data/` path-search convention. |

Do NOT serialize `RenderPassComponent` directly. Define new POD description structs that the compiler reads, then *builds* the RenderPassComponent from. `std::function<>` callbacks become named kernel references (D4).

------------------------------------------------------------------
4. Decision D2 — Schema
------------------------------------------------------------------
A graph file = resource declarations + pass nodes. Sketch:

```json
{
  "Name": "ExampleRenderGraph",
  "Resources": [
    { "Name": "BRDF LUT", "Type": "Texture",
      "Width": 512, "Height": 512, "DepthOrArraySize": 1,
      "Sampler": "Sampler2D", "Usage": "ComputeOnly",
      "PixelDataType": "Float16", "PixelDataFormat": "RGBA",
      "GPUAccessibility": "ReadWrite", "Lifetime": "Persistent" }
  ],
  "Passes": [
    { "Name": "BRDFLUTPass", "Queue": "Compute", "Kernel": "Default",
      "Shader": { "CS": "BRDFLUTPass.comp" },
      "Reads": [], "Writes": ["BRDF LUT"],
      "Bindings": [
        { "Resource": "BRDF LUT", "Type": "Image", "Set": 0, "Index": 0,
          "BindingAccess": "ReadWrite", "ResourceAccess": "ReadWrite",
          "TextureUsage": "ComputeOnly", "Stage": "Compute" } ],
      "Dispatch": { "X": 32, "Y": 32, "Z": 1 },
      "Bypass": { "Enabled": false, "ClearOnBypass": false },
      "OneShot": true }
  ]
}
```

- `Reads`/`Writes` name resources. The compiler derives edges as `writes(producer) ∩ reads(consumer)` → DAG. No manual WaitIfActive.
- `Queue` per node drives cross-queue fence insertion.
- `Bypass` folds TASK-182/171 into data.
- `Dispatch` is static; dynamic sizes come from a kernel hook (D4), not data.

------------------------------------------------------------------
5. Decision D3 — Compiler shape
------------------------------------------------------------------
**PICK: startup-load + in-memory compile.** New `RenderGraphService` at engine init:
1. `JSONWrapper::Load` the graph file → deserialize into `RenderGraphDesc`.
2. Resources → `*ResourceService::Add` + populate desc + `Initialize` (replaces per-pass Setup resource creation).
3. Pass nodes → `RenderPassResourceService::Add`, populate `m_RenderPassDesc` + `m_ResourceBindingLayoutDescs` from data, attach kernel, create CLs (replaces Setup binding boilerplate).
4. Build DAG from reads/writes, topo-sort, assign per-edge semaphore/fence schedule.
5. Emit a compiled schedule = ordered dispatch steps + fence ops (replaces hand-written ExecuteCommands).

| Option | Verdict | Why |
|---|---|---|
| Startup-load + in-memory | **CHOSEN** | Graph ~40 nodes; parse + compile is sub-ms one-time → AC#6 (60 FPS) trivially met (zero per-frame cost). Matches O3DE pass-system / Frostbite FrameGraph init model. |
| Hot-reload | Deferred (Phase N+) | Needs GPU-idle + resource teardown/rebuild mid-run; large surface, no Phase-0 value. Data stays text-editable regardless. |
| Offline-compiled binary | Rejected | Adds a build-pipeline step for sub-ms parse cost. Offline compilation belongs to PSOs, not graph topology. |

------------------------------------------------------------------
6. Decision D4 — Kernel registration (the crux)
------------------------------------------------------------------
Data expresses resources + bindings + ordering. It CANNOT express the command-recording body, dynamic dispatch sizes, or deferred RT-creation. Each node names a **kernel** resolved from a registry. Kernel interface (slim):

```cpp
class IRenderGraphKernel {
public:
  virtual bool Record(RenderGraphPassContext& ctx) = 0;          // issue Dispatch/Draw
  virtual bool ResolveDispatch(RenderGraphPassContext&, uint32_t& x, uint32_t& y, uint32_t& z) { return false; } // dynamic size
  virtual bool CreateRenderTargets(RenderGraphPassContext&) { return false; }  // deferred RT (replaces m_RenderTargetsInitializationFunc)
};
```

This maps 1:1 onto the survey's three difficulty bins:
- **(a) clean** → `"Kernel": "Default"`. The DefaultKernel binds every Reads/Writes resource per the binding table and issues the static `Dispatch`. **Zero per-pass C++.** ~19 passes.
- **(b) moderate** → a small registered kernel overriding `ResolveDispatch` (dynamic size from texture dims) and/or `CreateRenderTargets` (deferred RT, ping-pong). Ping-pong becomes a node attribute → may obsolete TASK-128. ~16 passes.
- **(c) exotic** → exempt from Setup-datafication; keep imperative `IRenderPass`. BUT still schedulable as an **opaque-kernel node** (data declares reads/writes + queue; kernel = the existing pass's PrepareCommandList). This datafies their *ordering* (P2/AC#5) even while their *Setup* stays code (P1/AC#4 exemption). ~5 passes.

Key architectural win: P1 and P2 decouple. Even un-migratable Setups still get data-driven dispatch ordering.

------------------------------------------------------------------
7. Decision D5 — Migration order (one-at-a-time)
------------------------------------------------------------------
One-at-a-time, not all-at-once: each migration is independently build + runtime + visual-parity verifiable (AC#7). All-at-once = one untestable mega-diff.

- **Phase 0 (this task)**: BRDFLUTPass POC — DefaultKernel, single output, OneShot. Proves resource-decl + binding-decl + DefaultKernel + startup-compile + schedule-emit end to end. Runs alongside the existing imperative path (graph drives only BRDFLUTPass; everything else unchanged).
- **Phase 1**: remaining bin-(a) clean compute passes → DefaultKernel. Largest LOC win, lowest risk.
- **Phase 2**: bin-(b) moderate → kernel hooks (deferred RT, dynamic dispatch, ping-pong-as-attribute).
- **Phase 3**: ExecuteCommands replacement — replace each `_ExecuteCommands_*.cpp` chain with a graph-walk over the compiled schedule (AC#5). Exotic passes participate as opaque-kernel nodes.
- **Phase 4**: exotic Setup — per-pass decide schema extension vs permanent exemption.

------------------------------------------------------------------
8. Decision D6 — Code + data layout
------------------------------------------------------------------
- New module `Source/Engine/RenderGraph/`:
  - `RenderGraphDesc.h` — POD structs (ResourceDesc, PassNodeDesc, BindingDesc, RenderGraphDesc).
  - `RenderGraphSerializer.cpp` — hand-written to_json/from_json.
  - `RenderGraphCompiler.cpp` — DAG build + topo-sort + schedule.
  - `RenderGraphService.{h,cpp}` — load, own compiled graph, drive dispatch.
  - `IRenderGraphKernel.h` + `DefaultKernel.{h,cpp}`.
- Kernel registry: name→kernel map, populated by explicit register calls in client startup.
- Data: `Data/ExampleProject/RenderGraph/ExampleRenderGraph.json` (found via existing AssetService path search).

------------------------------------------------------------------
9. Pass inventory binned by migration difficulty (AC#1)
------------------------------------------------------------------
**(a) Clean — DefaultKernel, easy (~19):** BRDFLUTPass(98), BRDFLUTMSPass(101), BillboardPass(93), SkyPass(108), LuminanceAveragePass(112), MotionBlurPass(105), PostTAAPass(115), BSDFTestPass(141), TiledFrustumGenerationPass(131), PTHashGridCachePurgeTilesPass(145), PTHashGridCacheUpdateTilesPass(223), PTHashGridCacheMipCascadeBuildPass(163), SSRCFilterHorizontalPass(152), SSRCFilterVerticalPass(153), SSRCSpatialHorizontalPass(131), SSRCSpatialVerticalPass(158), SSRCIntegrationPass(143), TransparentBlendPass(112), OpaqueCullingPass(~30 subclass + 162 shared base ComputeCullingPass). Note: PTHashGridCache* carry a compile-time `if constexpr (ENABLED)` gate + `Update()` activation gate — handle via conditional node inclusion at load.

**(b) Moderate — kernel hook (~16):** SSRCReprojectionPass(335, deferred-RT + ping-pong), SSRCRaytracingPass(205, dynamic dispatch), SSRCTemporalPass(332), LuminanceHistogramPass(114, conditional input), PreTAAPass(127), TAAPass(158, ping-pong history), SSAOPass(207), LightCullingPass(235, two-phase), TransparentGeometryProcessPass(177, per-object dynamic bind), AnimationPass(187, custom RT reservation), OpaquePass(158, IndirectDraw + CrossQueueExit::ToCommon + root constant), SunShadowRTPass(177, raytracing DispatchRays), FinalBlendPass(162), PTNRDFormatConvertPass(280), PTNRDDenoisePass(203), PTNRDCompositionPass(209).

**(c) Exotic — Setup-exempt, opaque-kernel for ordering only (~5):**
- PTPass (748, 6 TUs) — compile-time feature-gated binding count (19–26 entries via `if constexpr`), scene-load callbacks, material-buffer rebuild, TLAS-readiness poll. Setup stays code.
- LightPass (321) — 21 bindings from 6+ upstream passes, conditional SSRC source. Candidate for moderate if conditional-binding schema added.
- NRDIntegrationAdapter (~700, 5 TUs) — bypasses RenderPassComponent binding model, raw `ID3D12GraphicsCommandList*`, own NRD instance/heaps. **Permanent Setup exemption.**
- VolumetricPass (500, 3 TUs) — large commented-out stub, own internal dispatch graph, not wired to main loop. Exempt until un-stubbed.
- DebugPass (129) — CPU readback + conditional texture selection.

------------------------------------------------------------------
10. Risks / open questions
------------------------------------------------------------------
- **Resource lifetime/aliasing**: Phase-0 treats all resources as `Persistent` (current behavior — passes own their textures for the engine lifetime). Transient/aliased lifetime tiers are a later optimization, not Phase 0.
- **Binding-table fidelity**: DefaultKernel must reproduce exact `BindGPUResource` order + descriptor set/index. POC validates this against BRDFLUTPass's single binding before scaling.
- **Bypass barrier safety** (IRenderPass.h:42-46): bypass elides `CrossQueueExit::ToCommon` barriers — safe only under DX12 COMMON implicit-promotion. The bypass node attribute must carry the same constraint; document per-node.
- **Coexistence**: POC must run the graph-driven BRDFLUTPass alongside the unmodified imperative dispatch for all other passes — needs a clean seam where the old Setup/dispatch is skipped only for migrated passes.
