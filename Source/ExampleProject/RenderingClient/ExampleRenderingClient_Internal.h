#pragma once
// Internal header for the partial-class TU split of
// ExampleRenderingClient. Declares ExampleRenderingClientImpl so each
// sibling _Section.cpp can define its own slice of the class. The public
// header (ExampleRenderingClient.h) only forward-declares Impl.
//
// Per `disciplines/on-implement/file-splitting.md`, this is the partial-
// class pattern: same class, multiple TUs, declarations live here.

#include "ExampleRenderingClient.h"

#include "../../Engine/Interface/IRenderingClient.h"

#include <atomic>
#include <functional>

namespace Inno
{
	class GPUResourceComponent;
	class RenderPassComponent;

	class ExampleRenderingClientImpl : public IRenderingClient
	{
	public:
		INNO_CLASS_CONCRETE_NON_COPYABLE(ExampleRenderingClientImpl);

		// Inherited via IRenderingClient
		bool Setup(IServiceConfig* systemConfig) override;
		bool Initialize() override;
		bool Update() override;
		bool PrepareCommands() override;
		bool ExecuteCommands(IRenderingConfig* renderingConfig = nullptr) override;
		bool FinalizeGPUResults() override;
		bool Terminate() override;

		ObjectStatus GetStatus() override;

		// Readback the final blend output and save gpu_output.png. Callable
		// from any point where the GPU is still alive (per-frame trigger or
		// the Engine::Terminate GPU-finalization phase). Idempotent — sets
		// m_autoCaptureWritten so later calls no-op.
		void TryWriteAutoCapture();

		// GPU-to-PNG writer used by both the one-shot auto-capture and the
		// per-frame `-dump_frames START-END` path. Synchronises on the
		// graphics queue, reads FinalBlendPass output back to CPU, converts
		// to sRGB 8-bit, and writes the file. Returns false on empty
		// readback or save failure; not idempotent — caller owns any
		// "already wrote" latching.
		bool WriteCaptureToFile(const char* filename);

		bool m_drawBRDFTest = false;
		// Desired is what the user/toggle asked for; Active is what the
		// frame loop has actually switched into. PrepareCommands reconciles
		// them at the next frame boundary so the toggle never lands mid-frame.
		bool m_PTDesired = false;
		bool m_PTActive  = false;
		bool m_saveScreenCapture = false;


		uint32_t m_autoCaptureFrameCount = 0;
		bool m_autoCaptureWritten = false;

		GPUResourceComponent* m_Canvas;
		RenderPassComponent* m_CanvasOwner;

		bool m_ExecuteOneShotCommands = true;

		void AuditDump();

		// Audit-mode trigger seams — call sites in Setup / ExecuteCommands;
		// bodies live in _AuditDump.cpp alongside AuditDump itself. Each is
		// a no-op when -audit is off.
		void RegisterAuditCallback();
		void HandleAuditTrigger();

		// Audit-mode trigger state (-audit only). Per
		// .claude/state/engine-invariants.md the SceneService callback fires
		// on the render thread (LoadSync runs only from SceneService::Update
		// inside the FMS upload-heap callback). The event flag is atomic so
		// a future async-load path cannot silently break the contract;
		// m_AuditPostLoadFrameCount + m_AuditCountingStarted are render-
		// thread-local — read and written only from HandleAuditTrigger.
		std::function<void()> m_AuditSceneLoadedCallback;
		std::atomic<bool>     m_AuditSceneLoadEvent{false};
		uint32_t              m_AuditPostLoadFrameCount = 0;
		bool                  m_AuditCountingStarted = false;

	private:
		// Setup helpers — extracted to keep _Setup.cpp under the file-size
		// ratchet. RegisterDevToggles registers all DevToggleRegistry entries
		// + applies env-var overrides; BootstrapAmbientCGTextures imports the
		// AmbientCG PBR sets the example scenes reference.
		void RegisterDevToggles();
		void BootstrapAmbientCGTextures();
		// Registers the named render-graph init/update hooks (imported-resource
		// creation + per-frame uploads) that let migrated passes be pure JSON
		// nodes. Called in Setup before LoadGraph. Lives in _Hooks.cpp.
		void RegisterGraphHooks();
		// ExecuteCommands tail seams — screenshot consumer + auto-capture
		// triggers (per-frame frame-dump and one-shot trigger). Live in
		// _Capture.cpp with the readback writers.
		void HandleScreenCapture();
		void HandleAutoCaptureTriggers();

		// Swap-chain CL has speculatively recorded the post-frame SRV
		// transition into the tracker but has not executed; restore tracker
		// to the actual GPU state (UAV) before issuing the readback barrier.
		void AlignTrackerForMidFrameReadback();

		ObjectStatus m_ObjectStatus;
	};
}
