#include "GPUPathTracerPass.h"

#include "../../Engine/Services/RenderingConfigurationService.h"
#include "../../Engine/Services/SceneService.h"
#include "../../Engine/Services/ShaderProgramResourceService.h"
#include "../../Engine/Services/RenderPassResourceService.h"
#include "../../Engine/Services/GPUBufferResourceService.h"
#include "../../Engine/Services/SamplerResourceService.h"
#include "../../Engine/Services/CommandListResourceService.h"
#include "../../Engine/Services/FrameManagementService.h"
#include "../../Engine/Engine.h"

using namespace Inno;

bool GPUPathTracerPass::Setup(IServiceConfig* systemConfig)
{
	auto l_fmService = g_Engine->Get<FrameManagementService>();

	m_ShaderStage = ShaderStage::RayGen | ShaderStage::ClosestHit | ShaderStage::AnyHit | ShaderStage::Miss;

	// --- Ray Tracing SPC ---
	m_RayTracingSPC = g_Engine->Get<ShaderProgramResourceService>()->Add("GPUPathTracerPass");
	m_RayTracingSPC->m_ShaderFilePaths.m_RayGenPath     = "GPUPathTracerRayGen.hlsl";
	m_RayTracingSPC->m_ShaderFilePaths.m_ClosestHitPath = "GPUPathTracerClosestHit.hlsl";
	m_RayTracingSPC->m_ShaderFilePaths.m_AnyHitPath     = "GPUPathTracerAnyHit.hlsl";
	m_RayTracingSPC->m_ShaderFilePaths.m_MissPath       = "GPUPathTracerMiss.hlsl";
	m_RayTracingSPC->m_ShaderFilePaths.m_ShadowMissPath = "GPUPathTracerShadowMiss.hlsl";

	// --- Ray Tracing Render Pass ---
	m_RayTracingRenderPassComp = g_Engine->Get<RenderPassResourceService>()->Add("GPUPathTracerPass");

	auto l_rtDesc = g_Engine->Get<RenderingConfigurationService>()->GetDefaultRenderPassDesc();
	l_rtDesc.m_GPUEngineType    = GPUEngineType::Compute;
	l_rtDesc.m_RenderTargetCount = 0;
	l_rtDesc.m_UseRaytracing    = true;
	l_rtDesc.m_UseOutputMerger  = false;

	m_RayTracingRenderPassComp->m_RenderPassDesc = l_rtDesc;

	// Owned accumulation UAV isn't an output-merger target so the frame-
	// management resize path would otherwise skip it. Hook OnResize so the
	// buffer is recreated at the new resolution and accumulation history
	// is scrapped.
	m_RayTracingRenderPassComp->m_OnResize = [this]() { OnResize(); };

	// Resource-binding layout descriptors live in
	// GPUPathTracerPass_BindingLayout.cpp — same TU class, separated so
	// the ~250-line layout block plus its toggle-gated static_asserts
	// does not push this file past the file-size ratchet.
	ConfigureRaytracingBindings();

	m_MaterialSampler = g_Engine->Get<SamplerResourceService>()->Add("GPUPathTracerMaterialSampler");
	m_MaterialSampler->m_SamplerDesc.m_WrapMethodU = TextureWrapMethod::Repeat;
	m_MaterialSampler->m_SamplerDesc.m_WrapMethodV = TextureWrapMethod::Repeat;

	m_RayTracingRenderPassComp->m_ShaderProgram = m_RayTracingSPC;

	m_CommandListComp_Graphics = g_Engine->Get<CommandListResourceService>()->Add("GPUPathTracerPass/Graphics");
	m_CommandListComp_Graphics->m_Type = GPUEngineType::Graphics;

	m_CommandListComp_Compute = g_Engine->Get<CommandListResourceService>()->Add("GPUPathTracerPass/Compute");
	m_CommandListComp_Compute->m_Type = GPUEngineType::Compute;

	// --- Scene callbacks ---
	f_sceneLoadedCallback = [this]()
	{
		m_PendingMaterialRebuild = true;
		m_FrameCount = 1;
		m_HashGridCachePendingClear = true;
	};

	f_sceneUnloadingCallback = [this]()
	{
		auto l_bufService = g_Engine->Get<GPUBufferResourceService>();
		if (m_MaterialBuffer)
		{
			l_bufService->Delete(m_MaterialBuffer);
			m_MaterialBuffer = nullptr;
		}
		m_BuiltMeshCount = 0;
		m_FrameCount = 1;
		m_ObjectStatus = ObjectStatus::Suspended;
	};

	g_Engine->Get<SceneService>()->AddSceneLoadedCallback(&f_sceneLoadedCallback);
	g_Engine->Get<SceneService>()->AddSceneUnloadingCallback(&f_sceneUnloadingCallback);

	m_ObjectStatus = ObjectStatus::Created;

	return true;
}
