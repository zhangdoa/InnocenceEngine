#include "VXGIRenderer.h"
#include "../DefaultGPUBuffers/DefaultGPUBuffers.h"
#include "../../Engine/Services/RenderingContextService.h"
#include "../../Engine/Services/SceneSystem.h"
#include "../../Engine/Services/PhysicsSystem.h"

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


using namespace DefaultGPUBuffers;

bool VXGIRenderer::Setup(ISystemConfig* systemConfig)
{
	auto l_renderingServer = g_Engine->getRenderingServer();
	
	f_sceneLoadingFinishCallback = [&]() {
		m_isInitialLoadScene = true;
	};

	g_Engine->Get<SceneSystem>()->addSceneLoadingFinishCallback(&f_sceneLoadingFinishCallback);
	
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

	l_renderingServer->InitializeGPUBufferComponent(m_VXGICBuffer);
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

bool VXGIRenderer::Render(IRenderingConfig* renderingConfig)
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

		l_renderingServer->ExecuteCommandList(VXGIGeometryProcessPass::Get().GetRenderPassComp(), GPUEngineType::Graphics);
		l_renderingServer->WaitCommandQueue(VXGIGeometryProcessPass::Get().GetRenderPassComp(), GPUEngineType::Graphics, GPUEngineType::Graphics);

		l_renderingServer->ExecuteCommandList(VXGIConvertPass::Get().GetRenderPassComp(), GPUEngineType::Graphics);
		l_renderingServer->WaitCommandQueue(VXGIConvertPass::Get().GetRenderPassComp(), GPUEngineType::Compute, GPUEngineType::Graphics);
		l_renderingServer->ExecuteCommandList(VXGIConvertPass::Get().GetRenderPassComp(), GPUEngineType::Compute);
		l_renderingServer->WaitCommandQueue(VXGIConvertPass::Get().GetRenderPassComp(), GPUEngineType::Graphics, GPUEngineType::Compute);

		l_renderingServer->ExecuteCommandList(VXGILightPass::Get().GetRenderPassComp(), GPUEngineType::Graphics);
		l_renderingServer->WaitCommandQueue(VXGILightPass::Get().GetRenderPassComp(), GPUEngineType::Compute, GPUEngineType::Graphics);
		l_renderingServer->ExecuteCommandList(VXGILightPass::Get().GetRenderPassComp(), GPUEngineType::Compute);
		l_renderingServer->WaitCommandQueue(VXGILightPass::Get().GetRenderPassComp(), GPUEngineType::Compute, GPUEngineType::Compute);

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

		l_renderingServer->UploadGPUBufferComponent(m_VXGICBuffer, &l_voxelPassCB);

		if (m_isInitialLoadScene)
		{
			m_isInitialLoadScene = false;

			f_renderGeometryPasses();
			l_renderingServer->CopyTextureComponent(reinterpret_cast<TextureComponent*>(VXGILightPass::Get().GetIlluminanceVolume()), reinterpret_cast<TextureComponent*>(VXGIScreenSpaceFeedbackPass::Get().GetResult()));
		}
		else
		{
			l_renderingServer->WaitCommandQueue(VXGIScreenSpaceFeedbackPass::Get().GetRenderPassComp(), GPUEngineType::Graphics, GPUEngineType::Compute);
			l_renderingServer->ClearTextureComponent(reinterpret_cast<TextureComponent*>(VXGIScreenSpaceFeedbackPass::Get().GetResult()));
			
			VXGIScreenSpaceFeedbackPassRenderingContext l_VXGIScreenSpaceFeedbackPassRenderingContext;
			l_VXGIScreenSpaceFeedbackPassRenderingContext.m_output = VXGIScreenSpaceFeedbackPass::Get().GetResult();
			VXGIScreenSpaceFeedbackPass::Get().PrepareCommandList(&l_VXGIScreenSpaceFeedbackPassRenderingContext);

			l_renderingServer->ExecuteCommandList(VXGIScreenSpaceFeedbackPass::Get().GetRenderPassComp(), GPUEngineType::Graphics);
			l_renderingServer->WaitCommandQueue(VXGIScreenSpaceFeedbackPass::Get().GetRenderPassComp(), GPUEngineType::Compute, GPUEngineType::Graphics);
			l_renderingServer->ExecuteCommandList(VXGIScreenSpaceFeedbackPass::Get().GetRenderPassComp(), GPUEngineType::Compute);		
			l_renderingServer->WaitCommandQueue(VXGIScreenSpaceFeedbackPass::Get().GetRenderPassComp(), GPUEngineType::Graphics, GPUEngineType::Compute);
		}

		VXGIRayTracingPassRenderingContext l_VXGIRayTracingPassRenderingContext;
		l_VXGIRayTracingPassRenderingContext.m_input = VXGIScreenSpaceFeedbackPass::Get().GetResult();
		l_VXGIRayTracingPassRenderingContext.m_output = VXGIRayTracingPass::Get().GetResult();

		VXGIRayTracingPass::Get().PrepareCommandList(&l_VXGIRayTracingPassRenderingContext);

		l_renderingServer->ExecuteCommandList(VXGIRayTracingPass::Get().GetRenderPassComp(), GPUEngineType::Graphics);
		l_renderingServer->WaitCommandQueue(VXGIRayTracingPass::Get().GetRenderPassComp(), GPUEngineType::Compute, GPUEngineType::Graphics);
		l_renderingServer->ExecuteCommandList(VXGIRayTracingPass::Get().GetRenderPassComp(), GPUEngineType::Compute);
		l_renderingServer->WaitCommandQueue(VXGIRayTracingPass::Get().GetRenderPassComp(), GPUEngineType::Graphics, GPUEngineType::Compute);

		m_result = VXGIRayTracingPass::Get().GetResult();
	}
	else
	{
		auto l_cameraPos = g_Engine->Get<RenderingContextService>()->GetPerFrameConstantBuffer().camera_posWS;
		auto l_sceneAABB = g_Engine->Get<PhysicsSystem>()->GetStaticSceneAABB();
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

		l_renderingServer->UploadGPUBufferComponent(m_VXGICBuffer, &l_voxelPassCB);

		////	
		l_renderingServer->WaitCommandQueue(VXGIGeometryProcessPass::Get().GetRenderPassComp(), GPUEngineType::Graphics, GPUEngineType::Graphics);
		l_renderingServer->ClearGPUBufferComponent(reinterpret_cast<GPUBufferComponent*>(VXGIGeometryProcessPass::Get().GetResult()));

		f_renderGeometryPasses();

		if (l_VXGIRenderingConfig->m_multiBounceCount)
		{			
			VXGIMultiBouncePassRenderingContext l_VXGIMultiBouncePassRenderingContext;
			l_VXGIMultiBouncePassRenderingContext.m_input = VXGILightPass::Get().GetIlluminanceVolume();
			// @TODO: Investigate if we really needs a dynamic output target or not
			l_VXGIMultiBouncePassRenderingContext.m_output = VXGIMultiBouncePass::Get().GetResult();	

			for (uint32_t i = 0; i < l_VXGIRenderingConfig->m_multiBounceCount; i++)
			{
				VXGIMultiBouncePass::Get().PrepareCommandList(&l_VXGIMultiBouncePassRenderingContext);
				l_renderingServer->ExecuteCommandList(VXGIMultiBouncePass::Get().GetRenderPassComp(), GPUEngineType::Graphics);
				l_renderingServer->WaitCommandQueue(VXGIMultiBouncePass::Get().GetRenderPassComp(), GPUEngineType::Compute, GPUEngineType::Graphics);
				l_renderingServer->ExecuteCommandList(VXGIMultiBouncePass::Get().GetRenderPassComp(), GPUEngineType::Compute);
				l_renderingServer->WaitCommandQueue(VXGIMultiBouncePass::Get().GetRenderPassComp(), GPUEngineType::Compute, GPUEngineType::Compute);

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

		VXGIVisualizationPass::Get().PrepareCommandList(&l_VXGIVisualizationPassRenderingContext);
		l_renderingServer->ExecuteCommandList(VXGIVisualizationPass::Get().GetRenderPassComp(), GPUEngineType::Graphics);
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