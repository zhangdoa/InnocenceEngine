#pragma once
#include "ExampleRenderingClient.h"
#include "BRDFLUTPass.h"
#include "BRDFLUTMSPass.h"
#include "SunShadowCullingPass.h"
#include "SunShadowGeometryProcessPass.h"
#include "SunShadowBlurOddPass.h"
#include "SunShadowBlurEvenPass.h"
#include "OpaqueCullingPass.h"
#include "OpaquePass.h"
#include "AnimationPass.h"
#include "SSAOPass.h"
#include "RadianceCacheReprojectionPass.h"
#include "RadianceCacheRaytracingPass.h"
#include "RadianceCacheFilterHorizontalPass.h"
#include "RadianceCacheFilterVerticalPass.h"
#include "RadianceCacheIntegrationPass.h"
#include "GIDenoisePass.h"
#include "GIATrous1Pass.h"
#include "GIATrous2Pass.h"
#include "GIATrous4Pass.h"
#include "TiledFrustumGenerationPass.h"
#include "LightCullingPass.h"
#include "LightPass.h"
#include "SkyPass.h"
#include "PreTAAPass.h"
#include "TransparentGeometryProcessPass.h"
#include "TransparentBlendPass.h"
#include "VolumetricPass.h"
#include "TAAPass.h"
#include "LuminanceHistogramPass.h"
#include "LuminanceAveragePass.h"
#include "MotionBlurPass.h"
#include "BillboardPass.h"
#include "DebugPass.h"
#include "FinalBlendPass.h"
#include "GPUPathTracerPass.h"

#include "BSDFTestPass.h"

#include "../../Engine/Services/HIDService.h"
#include "../../Engine/Services/DevToggleRegistry.h"
#include "../../Engine/Services/RenderPassResourceService.h"
#include "../../Engine/Services/ViewportSourceOverride.h"
#include "../../Engine/Services/RenderingConfigurationService.h"
#include "../../Engine/Services/AssetService.h"
#include "../../Engine/Common/IOService.h"
#include "../../Engine/Services/GraphicsHardwareService.h"
#include "../../Engine/Common/Task.h"
#include "../../Engine/Common/TaskScheduler.h"

#include "../../Engine/Engine.h"
#include "../../Engine/Services/TextureResourceService.h"
#include "../../Engine/Services/FrameManagementService.h"

#include <cstdlib>

using namespace Inno;

namespace Inno
{
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
		ObjectStatus m_ObjectStatus;
	};

	bool ExampleRenderingClientImpl::Setup(IServiceConfig* systemConfig)
	{
		// Getter reports the desired state — i.e. what the user's last click
		// asked for — so callers that read back immediately after Set() (the
		// IPC setter-reply convention) see their own write, not yesterday's
		// frame state. The frame loop reconciles Active with Desired at the
		// next boundary so the toggle never lands mid-frame.
		DevToggleRegistry::RegisterToggle("GPUPathTracer",
			[this]() { return m_GPUPathTracerDesired; },
			[this](bool desired) { m_GPUPathTracerDesired = desired; });

		DevToggleRegistry::RegisterAction("Screenshot", [this]() { m_saveScreenCapture = true; });

		if (strcmp(g_Engine->getInitConfig().testCase, "gpu_path_tracer") == 0)
		{
			m_GPUPathTracerDesired = true;
			m_GPUPathTracerActive  = true;
		}

		// Idempotent bootstrap of AmbientCG PBR sets that materials in
		// ExampleProject scenes reference. Each ImportTexture writes a
		// TextureComponent JSON + binary if the JSON is missing; existing
		// JSONs short-circuit the load → recompress → save chain.
		struct PBRSetSlot { const char* slotName; uint32_t slotIndex; bool isSRGB; };
		static const PBRSetSlot s_AmbientCGSlots[] = {
			{ "NormalGL",  0u, false },
			{ "Color",     1u, true  },
			{ "Metalness", 2u, false },
			{ "Roughness", 3u, false },
		};
		static const char* s_AmbientCGSets[] = { "Concrete007", "Ground037", "Metal032", "Tiles074" };
		auto* l_io = g_Engine->Get<IOService>();
		auto l_dataDir = l_io->getDataDirectory();
		for (const char* setName : s_AmbientCGSets)
		{
			std::string l_setDir = std::string("../OriginalAssets/Textures/") + setName + "/";
			std::string l_baseName = std::string(setName) + "_1K-PNG";
			for (const auto& slot : s_AmbientCGSlots)
			{
				std::string l_pngPath = l_setDir + l_baseName + "_" + slot.slotName + ".png";
				if (!l_io->isFileExist(l_pngPath.c_str()))
					continue;

				std::string l_instanceName = std::string(setName) + "_" + slot.slotName + ".TextureComponent";

				auto l_destJson = l_dataDir + AssetService::GetAssetFilePath(l_instanceName.c_str());
				if (l_io->isFileExist(l_destJson.c_str()))
					continue;

				std::string l_absPath = l_dataDir + l_pngPath;
				AssetService::ImportTexture(l_absPath.c_str(),
					TextureSampler::Sampler2D, TextureUsage::Sample,
					slot.isSRGB, slot.slotIndex, l_instanceName.c_str());
			}
		}

		BRDFLUTPass::Get().Setup();
		BRDFLUTMSPass::Get().Setup();

		SunShadowCullingPass::Get().Setup();
		SunShadowGeometryProcessPass::Get().Setup();

		OpaqueCullingPass::Get().Setup();
		OpaquePass::Get().Setup();

		RadianceCacheReprojectionPass::Get().Setup();
		RadianceCacheRaytracingPass::Get().Setup();
		RadianceCacheFilterHorizontalPass::Get().Setup();
		RadianceCacheFilterVerticalPass::Get().Setup();
		RadianceCacheIntegrationPass::Get().Setup();
		GIDenoisePass::Get().Setup();
		GIATrous1Pass::Get().Setup();
		GIATrous2Pass::Get().Setup();
		GIATrous4Pass::Get().Setup();

		SSAOPass::Get().Setup();

		TiledFrustumGenerationPass::Get().Setup();
		LightCullingPass::Get().Setup();

		LightPass::Get().Setup();

		SkyPass::Get().Setup();

		PreTAAPass::Get().Setup();
		TAAPass::Get().Setup();

		LuminanceHistogramPass::Get().Setup();
		LuminanceAveragePass::Get().Setup();

		FinalBlendPass::Get().Setup();
		GPUPathTracerPass::Get().Setup();

		// SunShadowBlurOddPass::Get().Setup();
		// SunShadowBlurEvenPass::Get().Setup();

		// AnimationPass::Get().Setup();

		// TransparentGeometryProcessPass::Get().Setup();
		// TransparentBlendPass::Get().Setup();
		// VolumetricPass::Setup();

		// MotionBlurPass::Get().Setup();
		// BillboardPass::Get().Setup();
		// DebugPass::Get().Setup();

		// BSDFTestPass::Get().Setup();

		auto f_getUserPipelineOutputFunc = [this]()
			{
				return m_Canvas;
			};

	auto l_fmService = g_Engine->Get<FrameManagementService>();

		l_fmService->SetUserPipelineOutput(std::move(f_getUserPipelineOutputFunc));

		m_ObjectStatus = ObjectStatus::Created;

		return true;
	}

	bool ExampleRenderingClientImpl::Initialize()
	{

		BRDFLUTPass::Get().Initialize();
		BRDFLUTMSPass::Get().Initialize();

		SunShadowCullingPass::Get().Initialize();
		SunShadowGeometryProcessPass::Get().Initialize();

		OpaqueCullingPass::Get().Initialize();
		OpaquePass::Get().Initialize();

		RadianceCacheReprojectionPass::Get().Initialize();
		RadianceCacheRaytracingPass::Get().Initialize();
		RadianceCacheFilterHorizontalPass::Get().Initialize();
		RadianceCacheFilterVerticalPass::Get().Initialize();
		RadianceCacheIntegrationPass::Get().Initialize();
		GIDenoisePass::Get().Initialize();
		GIATrous1Pass::Get().Initialize();
		GIATrous2Pass::Get().Initialize();
		GIATrous4Pass::Get().Initialize();

		SSAOPass::Get().Initialize();

		TiledFrustumGenerationPass::Get().Initialize();
		LightCullingPass::Get().Initialize();

		LightPass::Get().Initialize();

		SkyPass::Get().Initialize();

		PreTAAPass::Get().Initialize();
		TAAPass::Get().Initialize();

		LuminanceHistogramPass::Get().Initialize();
		LuminanceAveragePass::Get().Initialize();

		FinalBlendPass::Get().Initialize();
		GPUPathTracerPass::Get().Initialize();

		m_ObjectStatus = ObjectStatus::Activated;

		return true;
	}

	bool ExampleRenderingClientImpl::Update()
	{
		RadianceCacheReprojectionPass::Get().Update();
		TiledFrustumGenerationPass::Get().Update();
		LightCullingPass::Get().Update();
		LuminanceAveragePass::Get().Update();

		if (m_GPUPathTracerActive)
			GPUPathTracerPass::Get().Update();

		return true;
	}

	bool ExampleRenderingClientImpl::PrepareCommands()
	{
		if (m_GPUPathTracerDesired != m_GPUPathTracerActive)
		{
			m_GPUPathTracerActive = m_GPUPathTracerDesired;
			if (m_GPUPathTracerActive)
				GPUPathTracerPass::Get().ResetAccumulation();
		}

		if (m_GPUPathTracerActive && GPUPathTracerPass::Get().GetStatus() == ObjectStatus::Activated)
		{
			GPUPathTracerPass::Get().PrepareCommandList();
		}

		m_Canvas = FinalBlendPass::Get().GetResult();
		m_CanvasOwner = FinalBlendPass::Get().GetRenderPassComp();

		if (!m_GPUPathTracerActive)
		{
			if (m_ExecuteOneShotCommands)
			{
				BRDFLUTPass::Get().PrepareCommandList();
				BRDFLUTMSPass::Get().PrepareCommandList();
			}

			SunShadowCullingPass::Get().PrepareCommandList();
			SunShadowGeometryProcessPass::Get().PrepareCommandList();

			OpaqueCullingPass::Get().PrepareCommandList();
			OpaquePass::Get().PrepareCommandList();

			RadianceCacheReprojectionPass::Get().PrepareCommandList();
			RadianceCacheRaytracingPass::Get().PrepareCommandList();
			RadianceCacheFilterHorizontalPass::Get().PrepareCommandList();
			RadianceCacheFilterVerticalPass::Get().PrepareCommandList();
			RadianceCacheIntegrationPass::Get().PrepareCommandList();
			GIDenoisePass::Get().PrepareCommandList();
			GIATrous1Pass::Get().PrepareCommandList();
			GIATrous2Pass::Get().PrepareCommandList();
			GIATrous4Pass::Get().PrepareCommandList();

			SSAOPass::Get().PrepareCommandList();

			TiledFrustumGenerationPass::Get().PrepareCommandList();

			LightCullingPass::Get().PrepareCommandList();

			LightPass::Get().PrepareCommandList();

			SkyPass::Get().PrepareCommandList();

			PreTAAPass::Get().PrepareCommandList();

			TAAPassRenderingContext l_TAAPassRenderingContext;
			l_TAAPassRenderingContext.m_input = PreTAAPass::Get().GetResult();
			l_TAAPassRenderingContext.m_motionVector = OpaquePass::Get().GetRenderPassComp()->m_OutputMergerTarget->m_ColorOutputs[3];

			TAAPass::Get().PrepareCommandList(&l_TAAPassRenderingContext);
		}

		// Default viewport source: PT result if active, else TAA result.
		// ViewportSourceOverride lets a tooling client substitute any
		// pass's color RT for the default.
		GPUResourceComponent* l_hdrSource = nullptr;
		if (m_GPUPathTracerActive && GPUPathTracerPass::Get().GetStatus() == ObjectStatus::Activated)
		{
			l_hdrSource = GPUPathTracerPass::Get().GetResult();
		}
		else
		{
			l_hdrSource = TAAPass::Get().GetResult();
		}

		if (auto l_override = ViewportSourceOverride::Get())
		{
			auto* l_pass = g_Engine->Get<RenderPassResourceService>()->Find(l_override->m_PassName.c_str());
			if (l_pass && l_pass->m_OutputMergerTarget &&
				l_override->m_RTIndex < l_pass->m_OutputMergerTarget->m_ColorOutputs.size())
			{
				auto* l_chosen = l_pass->m_OutputMergerTarget->m_ColorOutputs[l_override->m_RTIndex];
				if (l_chosen)
					l_hdrSource = l_chosen;
			}
		}

		LuminanceHistogramPassRenderingContext l_LuminanceHistogramPassRenderingContext;
		l_LuminanceHistogramPassRenderingContext.m_input = l_hdrSource;
		LuminanceHistogramPass::Get().PrepareCommandList(&l_LuminanceHistogramPassRenderingContext);

		LuminanceAveragePass::Get().PrepareCommandList();

		FinalBlendPassRenderingContext l_FinalBlendPassRenderingContext;
		l_FinalBlendPassRenderingContext.m_input = l_hdrSource;
		FinalBlendPass::Get().PrepareCommandList(&l_FinalBlendPassRenderingContext);

		return true;
	}

	bool ExampleRenderingClientImpl::ExecuteCommands(IRenderingConfig* renderingConfig)
	{
		auto l_renderingConfig = g_Engine->Get<RenderingConfigurationService>()->GetRenderingConfig();
		auto l_hwService = g_Engine->Get<GraphicsHardwareService>();
		auto l_fmService = g_Engine->Get<FrameManagementService>();
		GPUResourceComponent* l_canvas;
		RenderPassComponent* l_canvasOwner;
		if (m_ExecuteOneShotCommands)
		{
			if (BRDFLUTPass::Get().GetStatus() == ObjectStatus::Activated
				&& BRDFLUTMSPass::Get().GetStatus() == ObjectStatus::Activated)
			{
				auto l_brdfRenderPass = BRDFLUTPass::Get().GetRenderPassComp();

				auto l_brdfPassComputeCommandList = BRDFLUTPass::Get().GetCommandListComp(GPUEngineType::Compute);
				l_hwService->Execute(l_brdfPassComputeCommandList, GPUEngineType::Compute);

				l_hwService->SignalOnGPU(l_brdfRenderPass, GPUEngineType::Compute);
				l_hwService->WaitOnGPU(l_brdfRenderPass, GPUEngineType::Compute, GPUEngineType::Compute);

				auto l_brdfMSCommandList = BRDFLUTMSPass::Get().GetCommandListComp(GPUEngineType::Compute);
				l_hwService->Execute(l_brdfMSCommandList, GPUEngineType::Compute);
				auto l_brdfMSRenderPass = BRDFLUTMSPass::Get().GetRenderPassComp();
				l_hwService->SignalOnGPU(l_brdfMSRenderPass, GPUEngineType::Compute);

				auto l_graphicsSemaphoreValue = l_hwService->GetSemaphoreValue(GPUEngineType::Graphics);
				auto l_computeSemaphoreValue = l_hwService->GetSemaphoreValue(GPUEngineType::Compute);
				l_hwService->WaitOnCPU(l_graphicsSemaphoreValue, GPUEngineType::Graphics);
				l_hwService->WaitOnCPU(l_computeSemaphoreValue, GPUEngineType::Compute);
				m_ExecuteOneShotCommands = false;
			}
		}

		if (m_GPUPathTracerActive && GPUPathTracerPass::Get().GetStatus() == ObjectStatus::Activated)
		{
			auto l_renderPass = GPUPathTracerPass::Get().GetRenderPassComp();

			// Graphics CL: transition accumulation buffer to UAV
			auto l_graphicsCL = GPUPathTracerPass::Get().GetCommandListComp(GPUEngineType::Graphics);
			l_hwService->Execute(l_graphicsCL, GPUEngineType::Graphics);
			l_hwService->SignalOnGPU(l_renderPass, GPUEngineType::Graphics);
			l_hwService->WaitOnGPU(l_renderPass, GPUEngineType::Compute, GPUEngineType::Graphics);

			// Compute CL: ray tracing dispatch (also transitions AccumBuffer to ReadOnly at end)
			auto l_computeCL = GPUPathTracerPass::Get().GetCommandListComp(GPUEngineType::Compute);
			l_hwService->Execute(l_computeCL, GPUEngineType::Compute);
			l_hwService->SignalOnGPU(l_renderPass, GPUEngineType::Compute);
		}

		if (!m_GPUPathTracerActive)
		{

		if (SunShadowCullingPass::Get().GetStatus() == ObjectStatus::Activated)
		{
			auto l_commandList = SunShadowCullingPass::Get().GetCommandListComp(GPUEngineType::Compute);
			l_hwService->Execute(l_commandList, GPUEngineType::Compute);
			auto l_renderPass = SunShadowCullingPass::Get().GetRenderPassComp();
			l_hwService->SignalOnGPU(l_renderPass, GPUEngineType::Compute);
		}

		if (SunShadowGeometryProcessPass::Get().GetStatus() == ObjectStatus::Activated)
		{
			l_hwService->WaitOnGPU(SunShadowCullingPass::Get().GetRenderPassComp(), GPUEngineType::Graphics, GPUEngineType::Compute);
			auto l_commandList = SunShadowGeometryProcessPass::Get().GetCommandListComp(GPUEngineType::Graphics);
			l_hwService->Execute(l_commandList, GPUEngineType::Graphics);
			auto l_renderPass = SunShadowGeometryProcessPass::Get().GetRenderPassComp();
			l_hwService->SignalOnGPU(l_renderPass, GPUEngineType::Graphics);
		}

		if (OpaqueCullingPass::Get().GetStatus() == ObjectStatus::Activated)
		{
			auto l_commandList = OpaqueCullingPass::Get().GetCommandListComp(GPUEngineType::Compute);
			l_hwService->Execute(l_commandList, GPUEngineType::Compute);
			auto l_renderPass = OpaqueCullingPass::Get().GetRenderPassComp();
			l_hwService->SignalOnGPU(l_renderPass, GPUEngineType::Compute);
		}

		if (OpaquePass::Get().GetStatus() == ObjectStatus::Activated)
		{
			l_hwService->WaitOnGPU(OpaqueCullingPass::Get().GetRenderPassComp(), GPUEngineType::Graphics, GPUEngineType::Compute);
			auto l_commandList = OpaquePass::Get().GetCommandListComp(GPUEngineType::Graphics);
			l_hwService->Execute(l_commandList, GPUEngineType::Graphics);
			auto l_renderPass = OpaquePass::Get().GetRenderPassComp();
			l_hwService->SignalOnGPU(l_renderPass, GPUEngineType::Graphics);
		}

		if (RadianceCacheReprojectionPass::Get().GetStatus() == ObjectStatus::Activated)
		{
			l_hwService->WaitOnGPU(OpaquePass::Get().GetRenderPassComp(), GPUEngineType::Graphics, GPUEngineType::Graphics);

			auto l_renderPass = RadianceCacheReprojectionPass::Get().GetRenderPassComp();

			// Execute graphics command list for resource transitions
			auto l_graphicsCommandList = RadianceCacheReprojectionPass::Get().GetCommandListComp(GPUEngineType::Graphics);
			l_hwService->Execute(l_graphicsCommandList, GPUEngineType::Graphics);
			l_hwService->SignalOnGPU(l_renderPass, GPUEngineType::Graphics);
			l_hwService->WaitOnGPU(l_renderPass, GPUEngineType::Compute, GPUEngineType::Graphics);

			// Execute compute command list for actual work
			auto l_computeCommandList = RadianceCacheReprojectionPass::Get().GetCommandListComp(GPUEngineType::Compute);
			l_hwService->Execute(l_computeCommandList, GPUEngineType::Compute);
			l_hwService->SignalOnGPU(l_renderPass, GPUEngineType::Compute);
		}

		if (RadianceCacheRaytracingPass::Get().GetStatus() == ObjectStatus::Activated)
		{
			l_hwService->WaitOnGPU(RadianceCacheReprojectionPass::Get().GetRenderPassComp(), GPUEngineType::Graphics, GPUEngineType::Compute);

			auto l_renderPass = RadianceCacheRaytracingPass::Get().GetRenderPassComp();

			// Execute graphics command list for resource transitions
			auto l_graphicsCommandList = RadianceCacheRaytracingPass::Get().GetCommandListComp(GPUEngineType::Graphics);
			l_hwService->Execute(l_graphicsCommandList, GPUEngineType::Graphics);
			l_hwService->SignalOnGPU(l_renderPass, GPUEngineType::Graphics);
			l_hwService->WaitOnGPU(l_renderPass, GPUEngineType::Compute, GPUEngineType::Graphics);

			// Execute compute command list for actual work
			l_hwService->WaitOnGPU(OpaquePass::Get().GetRenderPassComp(), GPUEngineType::Graphics, GPUEngineType::Graphics);
			auto l_computeCommandList = RadianceCacheRaytracingPass::Get().GetCommandListComp(GPUEngineType::Compute);
			l_hwService->Execute(l_computeCommandList, GPUEngineType::Compute);
			l_hwService->SignalOnGPU(l_renderPass, GPUEngineType::Compute);
		}

		if (RadianceCacheFilterHorizontalPass::Get().GetStatus() == ObjectStatus::Activated)
		{
			l_hwService->WaitOnGPU(RadianceCacheRaytracingPass::Get().GetRenderPassComp(), GPUEngineType::Graphics, GPUEngineType::Compute);

			auto l_renderPass = RadianceCacheFilterHorizontalPass::Get().GetRenderPassComp();

			// Execute graphics command list for resource transitions
			auto l_graphicsCommandList = RadianceCacheFilterHorizontalPass::Get().GetCommandListComp(GPUEngineType::Graphics);
			l_hwService->Execute(l_graphicsCommandList, GPUEngineType::Graphics);
			l_hwService->SignalOnGPU(l_renderPass, GPUEngineType::Graphics);
			l_hwService->WaitOnGPU(l_renderPass, GPUEngineType::Compute, GPUEngineType::Graphics);

			// Execute compute command list for actual work
			auto l_computeCommandList = RadianceCacheFilterHorizontalPass::Get().GetCommandListComp(GPUEngineType::Compute);
			l_hwService->Execute(l_computeCommandList, GPUEngineType::Compute);
			l_hwService->SignalOnGPU(l_renderPass, GPUEngineType::Compute);
		}

		if (RadianceCacheFilterVerticalPass::Get().GetStatus() == ObjectStatus::Activated)
		{
			l_hwService->WaitOnGPU(RadianceCacheFilterHorizontalPass::Get().GetRenderPassComp(), GPUEngineType::Graphics, GPUEngineType::Compute);

			auto l_renderPass = RadianceCacheFilterVerticalPass::Get().GetRenderPassComp();

			// Execute graphics command list for resource transitions
			auto l_graphicsCommandList = RadianceCacheFilterVerticalPass::Get().GetCommandListComp(GPUEngineType::Graphics);
			l_hwService->Execute(l_graphicsCommandList, GPUEngineType::Graphics);
			l_hwService->SignalOnGPU(l_renderPass, GPUEngineType::Graphics);
			l_hwService->WaitOnGPU(l_renderPass, GPUEngineType::Compute, GPUEngineType::Graphics);

			// Execute compute command list for actual work
			auto l_computeCommandList = RadianceCacheFilterVerticalPass::Get().GetCommandListComp(GPUEngineType::Compute);
			l_hwService->Execute(l_computeCommandList, GPUEngineType::Compute);
			l_hwService->SignalOnGPU(l_renderPass, GPUEngineType::Compute);
		}

		if (RadianceCacheIntegrationPass::Get().GetStatus() == ObjectStatus::Activated)
		{
			l_hwService->WaitOnGPU(RadianceCacheFilterVerticalPass::Get().GetRenderPassComp(), GPUEngineType::Graphics, GPUEngineType::Compute);

			auto l_renderPass = RadianceCacheIntegrationPass::Get().GetRenderPassComp();

			// Execute graphics command list for resource transitions
			auto l_graphicsCommandList = RadianceCacheIntegrationPass::Get().GetCommandListComp(GPUEngineType::Graphics);
			l_hwService->Execute(l_graphicsCommandList, GPUEngineType::Graphics);
			l_hwService->SignalOnGPU(l_renderPass, GPUEngineType::Graphics);
			l_hwService->WaitOnGPU(l_renderPass, GPUEngineType::Compute, GPUEngineType::Graphics);

			// Execute compute command list for actual work
			auto l_computeCommandList = RadianceCacheIntegrationPass::Get().GetCommandListComp(GPUEngineType::Compute);
			l_hwService->Execute(l_computeCommandList, GPUEngineType::Compute);
			l_hwService->SignalOnGPU(l_renderPass, GPUEngineType::Compute);
		}

		if (GIDenoisePass::Get().GetStatus() == ObjectStatus::Activated)
		{
			l_hwService->WaitOnGPU(OpaquePass::Get().GetRenderPassComp(), GPUEngineType::Graphics, GPUEngineType::Graphics);
			if (RadianceCacheIntegrationPass::Get().GetStatus() == ObjectStatus::Activated)
				l_hwService->WaitOnGPU(RadianceCacheIntegrationPass::Get().GetRenderPassComp(), GPUEngineType::Graphics, GPUEngineType::Compute);

			auto l_renderPass = GIDenoisePass::Get().GetRenderPassComp();

			auto l_graphicsCommandList = GIDenoisePass::Get().GetCommandListComp(GPUEngineType::Graphics);
			l_hwService->Execute(l_graphicsCommandList, GPUEngineType::Graphics);
			l_hwService->SignalOnGPU(l_renderPass, GPUEngineType::Graphics);
			l_hwService->WaitOnGPU(l_renderPass, GPUEngineType::Compute, GPUEngineType::Graphics);

			auto l_computeCommandList = GIDenoisePass::Get().GetCommandListComp(GPUEngineType::Compute);
			l_hwService->Execute(l_computeCommandList, GPUEngineType::Compute);
			l_hwService->SignalOnGPU(l_renderPass, GPUEngineType::Compute);
		}

		if (GIATrous1Pass::Get().GetStatus() == ObjectStatus::Activated)
		{
			if (GIDenoisePass::Get().GetStatus() == ObjectStatus::Activated)
				l_hwService->WaitOnGPU(GIDenoisePass::Get().GetRenderPassComp(), GPUEngineType::Graphics, GPUEngineType::Compute);

			auto l_renderPass = GIATrous1Pass::Get().GetRenderPassComp();
			l_hwService->Execute(GIATrous1Pass::Get().GetCommandListComp(GPUEngineType::Graphics), GPUEngineType::Graphics);
			l_hwService->SignalOnGPU(l_renderPass, GPUEngineType::Graphics);
			l_hwService->WaitOnGPU(l_renderPass, GPUEngineType::Compute, GPUEngineType::Graphics);
			l_hwService->Execute(GIATrous1Pass::Get().GetCommandListComp(GPUEngineType::Compute), GPUEngineType::Compute);
			l_hwService->SignalOnGPU(l_renderPass, GPUEngineType::Compute);
		}

		if (GIATrous2Pass::Get().GetStatus() == ObjectStatus::Activated)
		{
			if (GIATrous1Pass::Get().GetStatus() == ObjectStatus::Activated)
				l_hwService->WaitOnGPU(GIATrous1Pass::Get().GetRenderPassComp(), GPUEngineType::Graphics, GPUEngineType::Compute);

			auto l_renderPass = GIATrous2Pass::Get().GetRenderPassComp();
			l_hwService->Execute(GIATrous2Pass::Get().GetCommandListComp(GPUEngineType::Graphics), GPUEngineType::Graphics);
			l_hwService->SignalOnGPU(l_renderPass, GPUEngineType::Graphics);
			l_hwService->WaitOnGPU(l_renderPass, GPUEngineType::Compute, GPUEngineType::Graphics);
			l_hwService->Execute(GIATrous2Pass::Get().GetCommandListComp(GPUEngineType::Compute), GPUEngineType::Compute);
			l_hwService->SignalOnGPU(l_renderPass, GPUEngineType::Compute);
		}

		if (GIATrous4Pass::Get().GetStatus() == ObjectStatus::Activated)
		{
			if (GIATrous2Pass::Get().GetStatus() == ObjectStatus::Activated)
				l_hwService->WaitOnGPU(GIATrous2Pass::Get().GetRenderPassComp(), GPUEngineType::Graphics, GPUEngineType::Compute);

			auto l_renderPass = GIATrous4Pass::Get().GetRenderPassComp();
			l_hwService->Execute(GIATrous4Pass::Get().GetCommandListComp(GPUEngineType::Graphics), GPUEngineType::Graphics);
			l_hwService->SignalOnGPU(l_renderPass, GPUEngineType::Graphics);
			l_hwService->WaitOnGPU(l_renderPass, GPUEngineType::Compute, GPUEngineType::Graphics);
			l_hwService->Execute(GIATrous4Pass::Get().GetCommandListComp(GPUEngineType::Compute), GPUEngineType::Compute);
			l_hwService->SignalOnGPU(l_renderPass, GPUEngineType::Compute);
		}

		if (SSAOPass::Get().GetStatus() == ObjectStatus::Activated)
		{
			l_hwService->WaitOnGPU(OpaquePass::Get().GetRenderPassComp(), GPUEngineType::Graphics, GPUEngineType::Graphics);

			auto l_renderPass = SSAOPass::Get().GetRenderPassComp();

			auto l_graphicsCommandList = SSAOPass::Get().GetCommandListComp(GPUEngineType::Graphics);
			l_hwService->Execute(l_graphicsCommandList, GPUEngineType::Graphics);
			l_hwService->SignalOnGPU(l_renderPass, GPUEngineType::Graphics);
			l_hwService->WaitOnGPU(l_renderPass, GPUEngineType::Compute, GPUEngineType::Graphics);

			auto l_computeCommandList = SSAOPass::Get().GetCommandListComp(GPUEngineType::Compute);
			l_hwService->Execute(l_computeCommandList, GPUEngineType::Compute);
			l_hwService->SignalOnGPU(l_renderPass, GPUEngineType::Compute);
		}

		if (TiledFrustumGenerationPass::Get().GetStatus() == ObjectStatus::Activated)
		{
			auto l_commandList = TiledFrustumGenerationPass::Get().GetCommandListComp(GPUEngineType::Compute);
			l_hwService->Execute(l_commandList, GPUEngineType::Compute);
			auto l_renderPass = TiledFrustumGenerationPass::Get().GetRenderPassComp();
			l_hwService->SignalOnGPU(l_renderPass, GPUEngineType::Compute);
		}

		if (LightCullingPass::Get().GetStatus() == ObjectStatus::Activated)
		{
			l_hwService->WaitOnGPU(TiledFrustumGenerationPass::Get().GetRenderPassComp(), GPUEngineType::Compute, GPUEngineType::Compute);
			l_hwService->WaitOnGPU(OpaquePass::Get().GetRenderPassComp(), GPUEngineType::Graphics, GPUEngineType::Graphics);

			auto l_renderPass = LightCullingPass::Get().GetRenderPassComp();

			// Execute graphics command list for resource transitions
			auto l_graphicsCommandList = LightCullingPass::Get().GetCommandListComp(GPUEngineType::Graphics);
			l_hwService->Execute(l_graphicsCommandList, GPUEngineType::Graphics);
			l_hwService->SignalOnGPU(l_renderPass, GPUEngineType::Graphics);
			l_hwService->WaitOnGPU(l_renderPass, GPUEngineType::Compute, GPUEngineType::Graphics);

			// Execute compute command list for actual work
			auto l_computeCommandList = LightCullingPass::Get().GetCommandListComp(GPUEngineType::Compute);
			l_hwService->Execute(l_computeCommandList, GPUEngineType::Compute);
			l_hwService->SignalOnGPU(l_renderPass, GPUEngineType::Compute);
		}

		if (LightPass::Get().GetStatus() == ObjectStatus::Activated)
		{
			l_hwService->WaitOnGPU(SunShadowGeometryProcessPass::Get().GetRenderPassComp(), GPUEngineType::Graphics, GPUEngineType::Graphics);
			l_hwService->WaitOnGPU(OpaquePass::Get().GetRenderPassComp(), GPUEngineType::Graphics, GPUEngineType::Graphics);
			l_hwService->WaitOnGPU(SSAOPass::Get().GetRenderPassComp(), GPUEngineType::Graphics, GPUEngineType::Compute);
			l_hwService->WaitOnGPU(LightCullingPass::Get().GetRenderPassComp(), GPUEngineType::Graphics, GPUEngineType::Compute);
			if (GIATrous4Pass::Get().GetStatus() == ObjectStatus::Activated)
				l_hwService->WaitOnGPU(GIATrous4Pass::Get().GetRenderPassComp(), GPUEngineType::Graphics, GPUEngineType::Compute);
			else if (GIDenoisePass::Get().GetStatus() == ObjectStatus::Activated)
				l_hwService->WaitOnGPU(GIDenoisePass::Get().GetRenderPassComp(), GPUEngineType::Graphics, GPUEngineType::Compute);
			
			auto l_renderPass = LightPass::Get().GetRenderPassComp();
			
			auto l_graphicsCommandList = LightPass::Get().GetCommandListComp(GPUEngineType::Graphics);
			l_hwService->Execute(l_graphicsCommandList, GPUEngineType::Graphics);
			l_hwService->SignalOnGPU(l_renderPass, GPUEngineType::Graphics);
			l_hwService->WaitOnGPU(l_renderPass, GPUEngineType::Compute, GPUEngineType::Graphics);
			
			auto l_computeCommandList = LightPass::Get().GetCommandListComp(GPUEngineType::Compute);
			l_hwService->Execute(l_computeCommandList, GPUEngineType::Compute);
			l_hwService->SignalOnGPU(l_renderPass, GPUEngineType::Compute);
		}

		if (SkyPass::Get().GetStatus() == ObjectStatus::Activated)
		{
			auto l_commandList = SkyPass::Get().GetCommandListComp(GPUEngineType::Compute);
			l_hwService->Execute(l_commandList, GPUEngineType::Compute);
			auto l_renderPass = SkyPass::Get().GetRenderPassComp();
			l_hwService->SignalOnGPU(l_renderPass, GPUEngineType::Compute);
		}

		if (PreTAAPass::Get().GetStatus() == ObjectStatus::Activated)
		{
			l_hwService->WaitOnGPU(LightPass::Get().GetRenderPassComp(), GPUEngineType::Graphics, GPUEngineType::Compute);
			l_hwService->WaitOnGPU(SkyPass::Get().GetRenderPassComp(), GPUEngineType::Graphics, GPUEngineType::Compute);

			auto l_renderPass = PreTAAPass::Get().GetRenderPassComp();

			auto l_graphicsCommandList = PreTAAPass::Get().GetCommandListComp(GPUEngineType::Graphics);
			l_hwService->Execute(l_graphicsCommandList, GPUEngineType::Graphics);
			l_hwService->SignalOnGPU(l_renderPass, GPUEngineType::Graphics);
			l_hwService->WaitOnGPU(l_renderPass, GPUEngineType::Compute, GPUEngineType::Graphics);

			auto l_computeCommandList = PreTAAPass::Get().GetCommandListComp(GPUEngineType::Compute);
			l_hwService->Execute(l_computeCommandList, GPUEngineType::Compute);
			l_hwService->SignalOnGPU(l_renderPass, GPUEngineType::Compute);
		}

		if (TAAPass::Get().GetStatus() == ObjectStatus::Activated)
		{
			l_hwService->WaitOnGPU(OpaquePass::Get().GetRenderPassComp(), GPUEngineType::Graphics, GPUEngineType::Graphics);
			l_hwService->WaitOnGPU(PreTAAPass::Get().GetRenderPassComp(), GPUEngineType::Graphics, GPUEngineType::Compute);

			auto l_renderPass = TAAPass::Get().GetRenderPassComp();

			auto l_graphicsCommandList = TAAPass::Get().GetCommandListComp(GPUEngineType::Graphics);
			l_hwService->Execute(l_graphicsCommandList, GPUEngineType::Graphics);
			l_hwService->SignalOnGPU(l_renderPass, GPUEngineType::Graphics);
			l_hwService->WaitOnGPU(l_renderPass, GPUEngineType::Compute, GPUEngineType::Graphics);

			auto l_computeCommandList = TAAPass::Get().GetCommandListComp(GPUEngineType::Compute);
			l_hwService->Execute(l_computeCommandList, GPUEngineType::Compute);
			l_hwService->SignalOnGPU(l_renderPass, GPUEngineType::Compute);
		}

		} // end if (!m_GPUPathTracerActive)

		if (LuminanceHistogramPass::Get().GetStatus() == ObjectStatus::Activated)
		{
			if (m_GPUPathTracerActive)
				l_hwService->WaitOnGPU(GPUPathTracerPass::Get().GetRenderPassComp(), GPUEngineType::Graphics, GPUEngineType::Compute);
			else
				l_hwService->WaitOnGPU(TAAPass::Get().GetRenderPassComp(), GPUEngineType::Graphics, GPUEngineType::Compute);

			auto l_renderPass = LuminanceHistogramPass::Get().GetRenderPassComp();

			auto l_graphicsCommandList = LuminanceHistogramPass::Get().GetCommandListComp(GPUEngineType::Graphics);
			l_hwService->Execute(l_graphicsCommandList, GPUEngineType::Graphics);
			l_hwService->SignalOnGPU(l_renderPass, GPUEngineType::Graphics);
			l_hwService->WaitOnGPU(l_renderPass, GPUEngineType::Compute, GPUEngineType::Graphics);

			auto l_computeCommandList = LuminanceHistogramPass::Get().GetCommandListComp(GPUEngineType::Compute);
			l_hwService->Execute(l_computeCommandList, GPUEngineType::Compute);
			l_hwService->SignalOnGPU(l_renderPass, GPUEngineType::Compute);
		}

		if (LuminanceAveragePass::Get().GetStatus() == ObjectStatus::Activated)
		{
			l_hwService->WaitOnGPU(LuminanceHistogramPass::Get().GetRenderPassComp(), GPUEngineType::Compute, GPUEngineType::Compute);
			auto l_commandList = LuminanceAveragePass::Get().GetCommandListComp(GPUEngineType::Compute);
			l_hwService->Execute(l_commandList, GPUEngineType::Compute);
			auto l_renderPass = LuminanceAveragePass::Get().GetRenderPassComp();
			l_hwService->SignalOnGPU(l_renderPass, GPUEngineType::Compute);
		}

		if (FinalBlendPass::Get().GetStatus() == ObjectStatus::Activated)
		{
			if (m_GPUPathTracerActive)
				l_hwService->WaitOnGPU(GPUPathTracerPass::Get().GetRenderPassComp(), GPUEngineType::Graphics, GPUEngineType::Compute);
			else
				l_hwService->WaitOnGPU(TAAPass::Get().GetRenderPassComp(), GPUEngineType::Graphics, GPUEngineType::Compute);
			l_hwService->WaitOnGPU(LuminanceAveragePass::Get().GetRenderPassComp(), GPUEngineType::Graphics, GPUEngineType::Compute);

			auto l_renderPass = FinalBlendPass::Get().GetRenderPassComp();

			auto l_graphicsCommandList = FinalBlendPass::Get().GetCommandListComp(GPUEngineType::Graphics);
			l_hwService->Execute(l_graphicsCommandList, GPUEngineType::Graphics);
			l_hwService->SignalOnGPU(l_renderPass, GPUEngineType::Graphics);
			l_hwService->WaitOnGPU(l_renderPass, GPUEngineType::Compute, GPUEngineType::Graphics);

			auto l_computeCommandList = FinalBlendPass::Get().GetCommandListComp(GPUEngineType::Compute);
			l_hwService->Execute(l_computeCommandList, GPUEngineType::Compute);
			l_hwService->SignalOnGPU(l_renderPass, GPUEngineType::Compute);
		}

		if (m_saveScreenCapture)
		{
			auto l_srcTextureComp = static_cast<TextureComponent*>(FinalBlendPass::Get().GetResult());
			auto l_textureData = g_Engine->Get<TextureResourceService>()->ReadTextureBackToCPU(FinalBlendPass::Get().GetRenderPassComp(), l_srcTextureComp);
			g_Engine->Get<AssetService>()->Save("ScreenCapture", l_srcTextureComp->m_TextureDesc, l_textureData.data());
			m_saveScreenCapture = false;
		}

		auto l_totalFrames = g_Engine->getInitConfig().totalFrames;
		const bool l_isPathTracerTestMode =
			strcmp(g_Engine->getInitConfig().testCase, "gpu_path_tracer") == 0 && m_GPUPathTracerActive;
		const uint32_t l_triggerAtFrame = l_totalFrames > 0
			? static_cast<uint32_t>(l_totalFrames)
			: (l_isPathTracerTestMode ? 30u : 0u);

		// Per-frame trigger: mid-session snapshot (e.g. path tracer frame 30).
		// The structural fallback is FinalizeGPUResults, which runs at shutdown
		// after WaitForGPUIdle and catches any case the per-frame trigger missed
		// (e.g. the user exited before the trigger frame). TASK-42.
		if (l_triggerAtFrame > 0 && !m_autoCaptureWritten)
		{
			m_autoCaptureFrameCount++;
			if (m_autoCaptureFrameCount >= l_triggerAtFrame)
				TryWriteAutoCapture();
		}

		if (g_Engine->getInitConfig().isAudit)
		{
			static uint32_t s_AuditFrame = 0;
			if (++s_AuditFrame == 5)
				AuditDump();
		}

		return true;
	}

	void ExampleRenderingClientImpl::TryWriteAutoCapture()
	{
		if (m_autoCaptureWritten)
			return;
		m_autoCaptureWritten = true;

		auto l_fmService = g_Engine->Get<FrameManagementService>();
		auto l_hwService = g_Engine->Get<GraphicsHardwareService>();
		l_hwService->SignalOnGPU(l_fmService->GetGlobalSemaphore(), GPUEngineType::Graphics);
		auto l_semVal = l_hwService->GetSemaphoreValue(GPUEngineType::Graphics);
		l_hwService->WaitOnCPU(l_semVal, GPUEngineType::Graphics);

		auto l_srcTex = static_cast<TextureComponent*>(FinalBlendPass::Get().GetResult());
		auto l_texFrameIndex = l_srcTex->m_TextureDesc.IsMultiBuffer ? l_fmService->GetCurrentFrame() : 0u;
		l_srcTex->SetCurrentState(l_texFrameIndex, l_srcTex->m_WriteState);
		auto l_floatPixels = g_Engine->Get<TextureResourceService>()->ReadTextureBackToCPU(
			FinalBlendPass::Get().GetRenderPassComp(), l_srcTex);

		if (l_floatPixels.empty())
		{
			Log(Warning, "Auto-capture: ReadTextureBackToCPU returned empty.");
			return;
		}

		size_t l_zeroCount = 0, l_nonZeroCount = 0;
		float l_sumR = 0, l_sumG = 0, l_sumB = 0, l_maxR = 0, l_maxG = 0, l_maxB = 0;
		for (const auto& px : l_floatPixels)
		{
			const bool l_isZero = (px.x == 0.0f && px.y == 0.0f && px.z == 0.0f);
			if (l_isZero) { l_zeroCount++; }
			else
			{
				l_nonZeroCount++;
				l_sumR += px.x; l_sumG += px.y; l_sumB += px.z;
				if (px.x > l_maxR) l_maxR = px.x;
				if (px.y > l_maxG) l_maxG = px.y;
				if (px.z > l_maxB) l_maxB = px.z;
			}
		}
		const float l_total = static_cast<float>(l_floatPixels.size());
		Log(Success, "PathTracerReadback: total=", l_floatPixels.size(),
			" zero=", l_zeroCount, " nonZero=", l_nonZeroCount,
			" mean=(", l_sumR / l_total, ",", l_sumG / l_total, ",", l_sumB / l_total, ")",
			" max=(", l_maxR, ",", l_maxG, ",", l_maxB, ")");

		std::vector<uint8_t> l_uint8Pixels;
		l_uint8Pixels.reserve(l_floatPixels.size() * 4);
		for (const auto& px : l_floatPixels)
		{
			l_uint8Pixels.push_back(uint8_t(255.99f * std::min(sqrtf(std::max(px.x, 0.0f)), 1.0f)));
			l_uint8Pixels.push_back(uint8_t(255.99f * std::min(sqrtf(std::max(px.y, 0.0f)), 1.0f)));
			l_uint8Pixels.push_back(uint8_t(255.99f * std::min(sqrtf(std::max(px.z, 0.0f)), 1.0f)));
			l_uint8Pixels.push_back(uint8_t(255));
		}

		TextureDesc l_desc = l_srcTex->m_TextureDesc;
		l_desc.PixelDataType = TexturePixelDataType::UByte;
		l_desc.PixelDataFormat = TexturePixelDataFormat::RGBA;
		l_desc.Sampler = TextureSampler::Sampler2D;

		if (g_Engine->Get<AssetService>()->Save("gpu_output.png", l_desc, l_uint8Pixels.data()))
			Log(Success, "Auto-capture: gpu_output.png written.");
		else
			Log(Warning, "Auto-capture: failed to write gpu_output.png.");
	}

	bool ExampleRenderingClientImpl::FinalizeGPUResults()
	{
		// Structural fallback for the per-frame auto-capture trigger in
		// ExecuteCommands. Engine::Terminate calls this AFTER WaitForGPUIdle
		// and BEFORE the LogicClient CPU path tracer, so the GPU is guaranteed
		// alive and a readback/PNG write here is safe. Per-frame trigger
		// usually wins (m_autoCaptureWritten is already set); this catches
		// the "user exited before the trigger frame fired" case.
		if (g_Engine->getInitConfig().totalFrames > 0 && !m_autoCaptureWritten)
			TryWriteAutoCapture();
		return true;
	}

	void ExampleRenderingClientImpl::AuditDump()
	{
		auto l_hwService = g_Engine->Get<GraphicsHardwareService>();
		
		l_hwService->WaitOnCPU(l_hwService->GetSemaphoreValue(GPUEngineType::Graphics), GPUEngineType::Graphics);
		l_hwService->WaitOnCPU(l_hwService->GetSemaphoreValue(GPUEngineType::Compute), GPUEngineType::Compute);

		auto Dump = [&](const char* filename, RenderPassComponent* rp, TextureComponent* tc)
		{
			if (!tc) { Log(Warning, "AuditDump: null TextureComponent for ", filename); return; }
			auto l_pixels = g_Engine->Get<TextureResourceService>()->ReadTextureBackToCPU(rp, tc);
			if (l_pixels.empty()) { Log(Error, "AuditDump: empty readback for ", filename); return; }
			for (size_t pi = 0; pi < l_pixels.size(); pi++) {
				if (l_pixels[pi].x != 0.0f || l_pixels[pi].y != 0.0f || l_pixels[pi].z != 0.0f) {
					uint32_t py = (uint32_t)(pi / tc->m_TextureDesc.Width);
					uint32_t px = (uint32_t)(pi % tc->m_TextureDesc.Width);
					Log(Verbose, "AuditDump: ", filename, " first-nonzero y=", py, " x=", px, " rgba=(", l_pixels[pi].x, ",", l_pixels[pi].y, ",", l_pixels[pi].z, ",", l_pixels[pi].w, ")");
					break;
				}
			}
			TextureDesc l_desc = tc->m_TextureDesc;
			l_desc.PixelDataType = TexturePixelDataType::Float32;
			g_Engine->Get<AssetService>()->Save(filename, l_desc, l_pixels.data());
			Log(Success, "AuditDump: saved ", filename);
		};

		auto DumpRP = [&](const char* filename, RenderPassComponent* rp, uint32_t colorIndex = 0)
		{
			if (!rp || !rp->m_OutputMergerTarget) { Log(Warning, "AuditDump: null RenderPassComp for ", filename); return; }
			if (colorIndex >= rp->m_OutputMergerTarget->m_ColorOutputs.size()) { Log(Warning, "AuditDump: RT index ", colorIndex, " out of range for ", filename); return; }
			Dump(filename, rp, rp->m_OutputMergerTarget->m_ColorOutputs[colorIndex]);
		};

		// 1-2: BRDF LUTs
		Dump("audit_01_BRDFLUTPass.hdr",   BRDFLUTPass::Get().GetRenderPassComp(),   static_cast<TextureComponent*>(BRDFLUTPass::Get().GetResult()));
		Dump("audit_02_BRDFLUTMSPass.hdr",  BRDFLUTMSPass::Get().GetRenderPassComp(), static_cast<TextureComponent*>(BRDFLUTMSPass::Get().GetResult()));

		// 3: Shadow map
		DumpRP("audit_03_SunShadow_RT0.hdr", SunShadowGeometryProcessPass::Get().GetRenderPassComp(), 0);

		// 4: Opaque G-buffer
		DumpRP("audit_04a_Opaque_RT0.hdr", OpaquePass::Get().GetRenderPassComp(), 0);
		DumpRP("audit_04b_Opaque_RT1.hdr", OpaquePass::Get().GetRenderPassComp(), 1);
		DumpRP("audit_04c_Opaque_RT2.hdr", OpaquePass::Get().GetRenderPassComp(), 2);

		// 5: SSAO
		Dump("audit_05_SSAO.hdr", SSAOPass::Get().GetRenderPassComp(), static_cast<TextureComponent*>(SSAOPass::Get().GetResult()));

		// 8: Light
		Dump("audit_08a_Light_Luminance.hdr",   LightPass::Get().GetRenderPassComp(), LightPass::Get().GetLuminanceResult());
		Dump("audit_08b_Light_Illuminance.hdr",  LightPass::Get().GetRenderPassComp(), LightPass::Get().GetIlluminanceResult());

		// 9: Sky
		Dump("audit_09_Sky.hdr",     SkyPass::Get().GetRenderPassComp(),  static_cast<TextureComponent*>(SkyPass::Get().GetResult()));

		// 10: TAA
		Dump("audit_10_TAAPass.hdr", TAAPass::Get().GetRenderPassComp(), static_cast<TextureComponent*>(TAAPass::Get().GetResult()));

		// 11: Final blend
		// PrepareSwapChainCommands (which runs before ExecuteCommands) speculatively
		// transitions FinalBlend result to SRV in the state tracker. At AuditDump time
		// the actual GPU state is UAV (compute wrote, swap chain hasn't executed yet).
		// Temporarily correct the tracker, dump, then restore so the swap chain CL works.
		{
			auto* l_fbTex = static_cast<TextureComponent*>(FinalBlendPass::Get().GetResult());
			auto l_fbIdx = l_fbTex->m_TextureDesc.IsMultiBuffer ? g_Engine->Get<FrameManagementService>()->GetCurrentFrame() : 0u;
			auto l_savedState = l_fbTex->GetCurrentState(l_fbIdx);
			l_fbTex->SetCurrentState(l_fbIdx, l_fbTex->m_WriteState);
			Dump("audit_11_FinalBlend.hdr", FinalBlendPass::Get().GetRenderPassComp(), l_fbTex);
			l_fbTex->SetCurrentState(l_fbIdx, l_savedState);
		}

		Log(Success, "AuditDump complete. Check Bin/*.hdr");
		std::exit(0);
	}

	bool ExampleRenderingClientImpl::Terminate()
	{
		// Registered callbacks capture `this`; clear the registry before this
		// instance starts to die so an in-flight WS message can't deref it.
		DevToggleRegistry::Clear();

		auto l_hwService = g_Engine->Get<GraphicsHardwareService>();
		auto l_graphicsSemaphoreValue = l_hwService->GetSemaphoreValue(GPUEngineType::Graphics);
		auto l_computeSemaphoreValue = l_hwService->GetSemaphoreValue(GPUEngineType::Compute);
		l_hwService->WaitOnCPU(l_computeSemaphoreValue, GPUEngineType::Compute);
		l_hwService->WaitOnCPU(l_graphicsSemaphoreValue, GPUEngineType::Graphics);

		// Auto-capture readback now happens in Update() on the last frame,
		// before the CPU path tracer runs and causes a GPU device timeout.

		FinalBlendPass::Get().Terminate();

		LuminanceAveragePass::Get().Terminate();
		LuminanceHistogramPass::Get().Terminate();

		TAAPass::Get().Terminate();
		PreTAAPass::Get().Terminate();

		SkyPass::Get().Terminate();

		LightPass::Get().Terminate();
		GIATrous4Pass::Get().Terminate();
		GIATrous2Pass::Get().Terminate();
		GIATrous1Pass::Get().Terminate();
		GIDenoisePass::Get().Terminate();

		LightCullingPass::Get().Terminate();
		TiledFrustumGenerationPass::Get().Terminate();

		SSAOPass::Get().Terminate();

		RadianceCacheIntegrationPass::Get().Terminate();
		RadianceCacheFilterHorizontalPass::Get().Terminate();
		RadianceCacheFilterVerticalPass::Get().Terminate();
		RadianceCacheRaytracingPass::Get().Terminate();
		RadianceCacheReprojectionPass::Get().Terminate();

		OpaqueCullingPass::Get().Terminate();
		OpaquePass::Get().Terminate();

		SunShadowGeometryProcessPass::Get().Terminate();

		BRDFLUTMSPass::Get().Terminate();
		BRDFLUTPass::Get().Terminate();

		m_ObjectStatus = ObjectStatus::Terminated;

		return true;
	}

	ObjectStatus ExampleRenderingClientImpl::GetStatus()
	{
		return m_ObjectStatus;
	}
}

bool ExampleRenderingClient::Setup(IServiceConfig* systemConfig)
{
	m_Impl = new ExampleRenderingClientImpl();
	return m_Impl->Setup(systemConfig);
}

bool ExampleRenderingClient::Initialize()
{
	return m_Impl->Initialize();
}

bool ExampleRenderingClient::Update()
{
	return m_Impl->Update();
}

bool ExampleRenderingClient::PrepareCommands()
{
	return m_Impl->PrepareCommands();
}

bool ExampleRenderingClient::ExecuteCommands(IRenderingConfig* renderingConfig)
{
	return m_Impl->ExecuteCommands(renderingConfig);
}

bool ExampleRenderingClient::FinalizeGPUResults()
{
	return m_Impl->FinalizeGPUResults();
}

bool ExampleRenderingClient::Terminate()
{
	if (m_Impl->Terminate())
	{
		delete m_Impl;
		return true;
	}

	return false;
}

ObjectStatus ExampleRenderingClient::GetStatus()
{
	return m_Impl->GetStatus();
}