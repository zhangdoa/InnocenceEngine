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
		bool m_GPUPathTracerDesired = false;
		bool m_GPUPathTracerActive  = false;
		bool m_saveScreenCapture = false;


		uint32_t m_autoCaptureFrameCount = 0;
		bool m_autoCaptureWritten = false;

		GPUResourceComponent* m_Canvas;
		RenderPassComponent* m_CanvasOwner;

		bool m_ExecuteOneShotCommands = true;

		void AuditDump();

	private:
		// Setup helpers — extracted to keep _Setup.cpp under the file-size
		// ratchet. RegisterDevToggles registers all DevToggleRegistry entries
		// + applies env-var overrides; BootstrapAmbientCGTextures imports the
		// AmbientCG PBR sets the example scenes reference.
		void RegisterDevToggles();
		void BootstrapAmbientCGTextures();

		// ExecuteCommands seam — the rasterizer-only pass chain runs only
		// when GPUPathTracer is inactive. Lives in
		// _ExecuteCommands_Rasterizer.cpp to keep both TUs under the ratchet.
		void ExecuteRasterizerPasses();

		// Sub-seam of the rasterizer chain — RadianceCache reproject /
		// raytrace / filter / integrate + GI denoise + GI filter. Pulled
		// into a dedicated TU so _ExecuteCommands_Rasterizer.cpp stays
		// under the file-size ratchet.
		void ExecuteGIPasses();

		// ExecuteCommands tail seams — screenshot consumer + auto-capture
		// triggers (per-frame frame-dump and one-shot trigger). Live in
		// _Capture.cpp with the readback writers.
		void HandleScreenCapture();
		void HandleAutoCaptureTriggers();

		ObjectStatus m_ObjectStatus;
	};
}
