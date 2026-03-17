#include "VXGIRenderer.h"

#include "../../Engine/Services/RenderingContextService.h"
#include "../../Engine/Services/SceneService.h"
#include "../../Engine/Services/PhysicsSimulationService.h"

#include "VXGIGeometryProcessPass.h"
#include "VXGIConvertPass.h"
#include "VXGILightPass.h"
#include "VXGIMultiBouncePass.h"
#include "VXGIScreenSpaceFeedbackPass.h"
#include "VXGIRayTracingPass.h"
#include "VXGIVisualizationPass.h"
#include "OpaquePass.h"
#include "LightPass.h"

#include "../../Engine/Engine.h"

using namespace Inno;

bool VXGIRenderer::Setup(ISystemConfig* systemConfig)
{
	auto l_renderingServer = g_Engine->getRenderingServer();
	
	f_sceneLoadingFinishedCallback = [&]() {
		m_isInitialLoadScene = true;
	};

	g_Engine->Get<SceneService>()->AddSceneLoadingFinishedCallback(&f_sceneLoadingFinishedCallback, 0);
	
	m_VXGICBuffer = l_renderingServer->AddGPUBufferComponent("VXGIPassCBuffer/");
	m_VXGICBuffer->m_ElementCount = 1;
	m_VXGICBuffer->m_ElementSize = sizeof(VoxelizationConstantBuffer);
	
	VXGIGeometryProcessPass::Get().Setup(systemConfig);
	VXGIConvertPass::Get().Setup(systemConfig);
	VXGILightPass::Get().Setup(systemConfig);
	VXGIMultiBouncePass::Get().Setup(systemConfig);
	VXGIScreenSpaceFeedbackPass::Get().Setup(systemConfig);
	VXGIRayTracingPass::Get().Setup(systemConfig);
	VXGIVisualizationPass::Get().Setup(systemConfig);

	m_ObjectStatus = ObjectStatus::Created;

	return true;
}

bool VXGIRenderer::Initialize()
{
	auto l_renderingServer = g_Engine->getRenderingServer();

	l_renderingServer->Initialize(m_VXGICBuffer);
	VXGIGeometryProcessPass::Get().Initialize();
	VXGIConvertPass::Get().Initialize();
	VXGILightPass::Get().Initialize();
	VXGIMultiBouncePass::Get().Initialize();
	VXGIScreenSpaceFeedbackPass::Get().Initialize();
	VXGIRayTracingPass::Get().Initialize();
	VXGIVisualizationPass::Get().Initialize();

	m_ObjectStatus = ObjectStatus::Activated;
	
	return true;
}

bool VXGIRenderer::ExecuteCommands(IRenderingConfig* renderingConfig)
{
	VXGIRenderingConfig* l_VXGIRenderingConfig = reinterpret_cast<VXGIRenderingConfig*>(renderingConfig);
	auto l_renderingServer = g_Engine->getRenderingServer();

	auto f_renderGeometryPasses = [&]() 
	{
		VXGIGeometryProcessPass::Get().PrepareCommandList();

		VXGIConvertPassRenderingContext l_VXGIConvertPassRenderingContext;
		l_VXGIConvertPassRenderingContext.m_input = VXGIGeometryProcessPass::Get().GetResult();
		l_VXGIConvertPassRenderingContext.m_resolution = l_VXGIRenderingConfig->m_voxelizationResolution;
		VXGIConvertPass::Get().PrepareCommandList(&l_VXGIConvertPassRenderingContext);

		VXGILightPassRenderingContext l_VXGILightPassRenderingContext;
		l_VXGILightPassRenderingContext.m_AlbedoVolume = VXGIConvertPass::Get().GetAlbedoVolume();
		l_VXGILightPassRenderingContext.m_NormalVolume = VXGIConvertPass::Get().GetNormalVolume();
		l_VXGILightPassRenderingContext.m_resolution = l_VXGIRenderingConfig->m_voxelizationResolution;
		VXGILightPass::Get().PrepareCommandList(&l_VXGILightPassRenderingContext);

		if (VXGIGeometryProcessPass::Get().PrepareCommandList(nullptr)) {
			auto l_commandList1 = VXGIGeometryProcessPass::Get().GetCommandListComp(GPUEngineType::Graphics);
			if (l_commandList1) { l_renderingServer->Execute(l_commandList1, GPUEngineType::Graphics); }
		}
		l_renderingServer->WaitOnGPU(VXGIGeometryProcessPass::Get().GetRenderPassComp(), GPUEngineType::Graphics, GPUEngineType::Graphics);

		if (VXGIConvertPass::Get().PrepareCommandList(&l_VXGIConvertPassRenderingContext)) {
			auto l_commandList2 = VXGIConvertPass::Get().GetCommandListComp(GPUEngineType::Graphics);
			if (l_commandList2) { l_renderingServer->Execute(l_commandList2, GPUEngineType::Graphics); }
		}
		l_renderingServer->WaitOnGPU(VXGIConvertPass::Get().GetRenderPassComp(), GPUEngineType::Compute, GPUEngineType::Graphics);
		{
			auto l_commandList2 = VXGIConvertPass::Get().GetCommandListComp(GPUEngineType::Compute);
			if (l_commandList2) { l_renderingServer->Execute(l_commandList2, GPUEngineType::Compute); }
		}
		l_renderingServer->WaitOnGPU(VXGIConvertPass::Get().GetRenderPassComp(), GPUEngineType::Graphics, GPUEngineType::Compute);

		if (VXGILightPass::Get().PrepareCommandList(&l_VXGILightPassRenderingContext)) {
			auto l_commandList3 = VXGILightPass::Get().GetCommandListComp(GPUEngineType::Graphics);
			if (l_commandList3) { l_renderingServer->Execute(l_commandList3, GPUEngineType::Graphics); }
		}
		l_renderingServer->WaitOnGPU(VXGILightPass::Get().GetRenderPassComp(), GPUEngineType::Compute, GPUEngineType::Graphics);
		{
			auto l_commandList3 = VXGILightPass::Get().GetCommandListComp(GPUEngineType::Compute);
			if (l_commandList3) { l_renderingServer->Execute(l_commandList3, GPUEngineType::Compute); }
		}
		l_renderingServer->WaitOnGPU(VXGILightPass::Get().GetRenderPassComp(), GPUEngineType::Compute, GPUEngineType::Compute);

		l_renderingServer->GenerateMipmap(reinterpret_cast<TextureComponent*>(VXGILightPass::Get().GetIlluminanceVolume()));
	};

	if (l_VXGIRenderingConfig->m_screenFeedback)
	{
		auto l_cameraPos = g_Engine->Get<RenderingContextService>()->GetPerFrameConstantBuffer().camera_posWS;
		VoxelizationConstantBuffer l_voxelPassCB;

		l_voxelPassCB.volumeCenter = l_cameraPos;
		l_voxelPassCB.volumeExtend = (float)l_VXGIRenderingConfig->m_voxelizationResolution;
		l_voxelPassCB.volumeExtendRcp = 1.0f / l_voxelPassCB.volumeExtend;
		l_voxelPassCB.volumeResolution = (float)l_VXGIRenderingConfig->m_voxelizationResolution;
		l_voxelPassCB.volumeResolutionRcp = 1.0f / l_voxelPassCB.volumeResolution;
		l_voxelPassCB.voxelSize = l_voxelPassCB.volumeExtend / l_voxelPassCB.volumeResolution;
		l_voxelPassCB.voxelSizeRcp = 1.0f / l_voxelPassCB.voxelSize;
		l_voxelPassCB.numCones = (float)l_VXGIRenderingConfig->m_numCones;
		l_voxelPassCB.numConesRcp = 1.0f / l_voxelPassCB.numCones;
		l_voxelPassCB.coneTracingStep = (float)l_VXGIRenderingConfig->m_coneTracingStep;
		l_voxelPassCB.coneTracingMaxDistance = l_VXGIRenderingConfig->m_coneTracingMaxDistance;

		l_renderingServer->Upload(m_VXGICBuffer, &l_voxelPassCB);

		if (m_isInitialLoadScene)
		{
			m_isInitialLoadScene = false;

			f_renderGeometryPasses();

			// @TODO: Fix it
			l_renderingServer->Copy(nullptr, reinterpret_cast<TextureComponent*>(VXGILightPass::Get().GetIlluminanceVolume()), reinterpret_cast<TextureComponent*>(VXGIScreenSpaceFeedbackPass::Get().GetResult()));
		}
		else
		{
			l_renderingServer->WaitOnGPU(VXGIScreenSpaceFeedbackPass::Get().GetRenderPassComp(), GPUEngineType::Graphics, GPUEngineType::Compute);
			
			// @TODO: Fix it
			l_renderingServer->Clear(nullptr, reinterpret_cast<TextureComponent*>(VXGIScreenSpaceFeedbackPass::Get().GetResult()));
			
			VXGIScreenSpaceFeedbackPassRenderingContext l_VXGIScreenSpaceFeedbackPassRenderingContext;
			l_VXGIScreenSpaceFeedbackPassRenderingContext.m_output = VXGIScreenSpaceFeedbackPass::Get().GetResult();
			
			if (VXGIScreenSpaceFeedbackPass::Get().PrepareCommandList(&l_VXGIScreenSpaceFeedbackPassRenderingContext)) {
				auto l_commandList4 = VXGIScreenSpaceFeedbackPass::Get().GetCommandListComp(GPUEngineType::Graphics);
				if (l_commandList4) { l_renderingServer->Execute(l_commandList4, GPUEngineType::Graphics); }
			}
			l_renderingServer->WaitOnGPU(VXGIScreenSpaceFeedbackPass::Get().GetRenderPassComp(), GPUEngineType::Compute, GPUEngineType::Graphics);
			{
				auto l_commandList4 = VXGIScreenSpaceFeedbackPass::Get().GetCommandListComp(GPUEngineType::Compute);
				if (l_commandList4) { l_renderingServer->Execute(l_commandList4, GPUEngineType::Compute); }
			}		
			l_renderingServer->WaitOnGPU(VXGIScreenSpaceFeedbackPass::Get().GetRenderPassComp(), GPUEngineType::Graphics, GPUEngineType::Compute);
		}

		VXGIRayTracingPassRenderingContext l_VXGIRayTracingPassRenderingContext;
		l_VXGIRayTracingPassRenderingContext.m_input = VXGIScreenSpaceFeedbackPass::Get().GetResult();
		l_VXGIRayTracingPassRenderingContext.m_output = VXGIRayTracingPass::Get().GetResult();

		if (VXGIRayTracingPass::Get().PrepareCommandList(&l_VXGIRayTracingPassRenderingContext)) {
			auto l_commandList5 = VXGIRayTracingPass::Get().GetCommandListComp(GPUEngineType::Graphics);
			if (l_commandList5) { l_renderingServer->Execute(l_commandList5, GPUEngineType::Graphics); }
		}
		l_renderingServer->WaitOnGPU(VXGIRayTracingPass::Get().GetRenderPassComp(), GPUEngineType::Compute, GPUEngineType::Graphics);
		{
			auto l_commandList5 = VXGIRayTracingPass::Get().GetCommandListComp(GPUEngineType::Compute);
			if (l_commandList5) { l_renderingServer->Execute(l_commandList5, GPUEngineType::Compute); }
		}
		l_renderingServer->WaitOnGPU(VXGIRayTracingPass::Get().GetRenderPassComp(), GPUEngineType::Graphics, GPUEngineType::Compute);

		m_result = VXGIRayTracingPass::Get().GetResult();
	}
	else
	{
		auto l_cameraPos = g_Engine->Get<RenderingContextService>()->GetPerFrameConstantBuffer().camera_posWS;
		auto l_sceneAABB = g_Engine->Get<PhysicsSimulationService>()->GetStaticSceneAABB();
		auto l_maxExtend = std::max(std::max(l_sceneAABB.m_extend.x, l_sceneAABB.m_extend.y), l_sceneAABB.m_extend.z);
		auto l_adjustedBoundMax = l_sceneAABB.m_boundMin + Vec4(l_maxExtend, l_maxExtend, l_maxExtend, 0.0f);
		auto l_adjustedCenter = l_sceneAABB.m_boundMin + Vec4(l_maxExtend, l_maxExtend, l_maxExtend, 0.0f) / 2.0f;

		VoxelizationConstantBuffer l_voxelPassCB;

		l_voxelPassCB.volumeCenter = l_adjustedCenter;
		l_voxelPassCB.volumeExtend = l_maxExtend;
		l_voxelPassCB.volumeExtendRcp = 1.0f / l_voxelPassCB.volumeExtend;
		l_voxelPassCB.volumeResolution = (float)l_VXGIRenderingConfig->m_voxelizationResolution;
		l_voxelPassCB.volumeResolutionRcp = 1.0f / l_voxelPassCB.volumeResolution;
		l_voxelPassCB.voxelSize = l_voxelPassCB.volumeExtend / l_voxelPassCB.volumeResolution;
		l_voxelPassCB.voxelSizeRcp = 1.0f / l_voxelPassCB.voxelSize;
		l_voxelPassCB.numCones = (float)l_VXGIRenderingConfig->m_numCones;
		l_voxelPassCB.numConesRcp = 1.0f / l_voxelPassCB.numCones;
		l_voxelPassCB.coneTracingStep = (float)l_VXGIRenderingConfig->m_coneTracingStep;
		l_voxelPassCB.coneTracingMaxDistance = l_VXGIRenderingConfig->m_coneTracingMaxDistance;

		l_renderingServer->Upload(m_VXGICBuffer, &l_voxelPassCB);

		////	
		l_renderingServer->WaitOnGPU(VXGIGeometryProcessPass::Get().GetRenderPassComp(), GPUEngineType::Graphics, GPUEngineType::Graphics);
		
		// @TODO: Fix it
		l_renderingServer->Clear(nullptr, reinterpret_cast<GPUBufferComponent*>(VXGIGeometryProcessPass::Get().GetResult()));

		f_renderGeometryPasses();

		if (l_VXGIRenderingConfig->m_multiBounceCount)
		{			
			VXGIMultiBouncePassRenderingContext l_VXGIMultiBouncePassRenderingContext;
			l_VXGIMultiBouncePassRenderingContext.m_input = VXGILightPass::Get().GetIlluminanceVolume();
			// @TODO: Investigate if we really needs a dynamic output target or not
			l_VXGIMultiBouncePassRenderingContext.m_output = VXGIMultiBouncePass::Get().GetResult();	

			for (uint32_t i = 0; i < l_VXGIRenderingConfig->m_multiBounceCount; i++)
			{
				if (VXGIMultiBouncePass::Get().PrepareCommandList(&l_VXGIMultiBouncePassRenderingContext)) {
					auto l_commandList6 = VXGIMultiBouncePass::Get().GetCommandListComp(GPUEngineType::Graphics);
					if (l_commandList6) { l_renderingServer->Execute(l_commandList6, GPUEngineType::Graphics); }
				}
				l_renderingServer->WaitOnGPU(VXGIMultiBouncePass::Get().GetRenderPassComp(), GPUEngineType::Compute, GPUEngineType::Graphics);
				{
					auto l_commandList6 = VXGIMultiBouncePass::Get().GetCommandListComp(GPUEngineType::Compute);
					if (l_commandList6) { l_renderingServer->Execute(l_commandList6, GPUEngineType::Compute); }
				}
				l_renderingServer->WaitOnGPU(VXGIMultiBouncePass::Get().GetRenderPassComp(), GPUEngineType::Compute, GPUEngineType::Compute);

				l_renderingServer->GenerateMipmap(reinterpret_cast<TextureComponent*>(l_VXGIMultiBouncePassRenderingContext.m_output));

				m_result = l_VXGIMultiBouncePassRenderingContext.m_output;

				std::swap(l_VXGIMultiBouncePassRenderingContext.m_input, l_VXGIMultiBouncePassRenderingContext.m_output);
			}
		}
		else
		{
			m_result = VXGILightPass::Get().GetIlluminanceVolume();
		}
	}

	if (l_VXGIRenderingConfig->m_visualize)
	{
		VXGIVisualizationPassRenderingContext l_VXGIVisualizationPassRenderingContext;
		l_VXGIVisualizationPassRenderingContext.m_input = m_result;
		l_VXGIVisualizationPassRenderingContext.m_resolution = l_VXGIRenderingConfig->m_voxelizationResolution;

		if (VXGIVisualizationPass::Get().PrepareCommandList(&l_VXGIVisualizationPassRenderingContext)) {
			auto l_commandList7 = VXGIVisualizationPass::Get().GetCommandListComp(GPUEngineType::Graphics);
			if (l_commandList7) { l_renderingServer->Execute(l_commandList7, GPUEngineType::Graphics); }
		}
	}

	return true;
}

bool VXGIRenderer::Terminate()
{
	m_ObjectStatus = ObjectStatus::Terminated;

	return true;
}

ObjectStatus VXGIRenderer::GetStatus()
{
	return m_ObjectStatus;
}

GPUResourceComponent *VXGIRenderer::GetVoxelizationCBuffer()
{
	return m_VXGICBuffer;
}

GPUResourceComponent *VXGIRenderer::GetResult()
{
	return m_result;
}

GPUResourceComponent *VXGIRenderer::GetVisualizationResult()
{
	return VXGIVisualizationPass::Get().GetResult();
}