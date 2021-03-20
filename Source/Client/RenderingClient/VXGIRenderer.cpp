#include "VXGIRenderer.h"
#include "../DefaultGPUBuffers/DefaultGPUBuffers.h"

#include "VXGIGeometryProcessPass.h"
#include "VXGIConvertPass.h"
#include "VXGIMultiBouncePass.h"
#include "VXGIScreenSpaceFeedbackPass.h"
#include "VXGIRayTracingPass.h"
#include "VXGIVisualizationPass.h"
#include "OpaquePass.h"
#include "LightPass.h"

#include "../../Engine/Interface/IEngine.h"

using namespace Inno;
extern INNO_ENGINE_API IEngine *g_Engine;

using namespace DefaultGPUBuffers;

bool VXGIRenderer::Setup(ISystemConfig* systemConfig)
{
	f_sceneLoadingFinishCallback = [&]() {
		m_isInitialLoadScene = true;
	};

	g_Engine->getSceneSystem()->addSceneLoadingFinishCallback(&f_sceneLoadingFinishCallback);
	
	m_VXGICBuffer = g_Engine->getRenderingServer()->AddGPUBufferDataComponent("VXGIPassCBuffer/");
	m_VXGICBuffer->m_ElementCount = 1;
	m_VXGICBuffer->m_ElementSize = sizeof(VoxelizationConstantBuffer);
	
	m_ObjectStatus = ObjectStatus::Created;

	return true;
}

bool VXGIRenderer::Initialize()
{
	g_Engine->getRenderingServer()->InitializeGPUBufferDataComponent(m_VXGICBuffer);

	m_ObjectStatus = ObjectStatus::Activated;
	
	return true;
}

bool VXGIRenderer::Render(IRenderingConfig* renderingConfig)
{
	if (m_VXGIRenderingConfig.m_screenFeedback)
	{
		auto l_cameraPos = g_Engine->getRenderingFrontend()->getPerFrameConstantBuffer().camera_posWS;
		VoxelizationConstantBuffer l_voxelPassCB;

		l_voxelPassCB.volumeCenter = l_cameraPos;
		l_voxelPassCB.volumeExtend = 128.0f;
		l_voxelPassCB.volumeExtendRcp = 1.0f / l_voxelPassCB.volumeExtend;
		l_voxelPassCB.volumeResolution = (float)m_VXGIRenderingConfig.m_voxelizationResolution;
		l_voxelPassCB.volumeResolutionRcp = 1.0f / l_voxelPassCB.volumeResolution;
		l_voxelPassCB.voxelSize = l_voxelPassCB.volumeExtend / l_voxelPassCB.volumeResolution;
		l_voxelPassCB.voxelSizeRcp = 1.0f / l_voxelPassCB.voxelSize;
		l_voxelPassCB.numCones = (float)m_VXGIRenderingConfig.m_numCones;
		l_voxelPassCB.numConesRcp = 1.0f / l_voxelPassCB.numCones;
		l_voxelPassCB.coneTracingStep = (float)m_VXGIRenderingConfig.m_coneTracingStep;
		l_voxelPassCB.coneTracingMaxDistance = m_VXGIRenderingConfig.m_coneTracingMaxDistance;

		g_Engine->getRenderingServer()->UploadGPUBufferDataComponent(m_VXGICBuffer, &l_voxelPassCB);


		if (m_isInitialLoadScene)
		{
			m_isInitialLoadScene = false;

			VXGIGeometryProcessPass::Get().PrepareCommandList();
			g_Engine->getRenderingServer()->ExecuteCommandList(VXGIGeometryProcessPass::Get().GetRPDC(), RenderPassUsage::Graphics);

			VXGIConvertPass::Get().PrepareCommandList();
			g_Engine->getRenderingServer()->ExecuteCommandList(VXGIConvertPass::Get().GetRPDC(), RenderPassUsage::Graphics);

			g_Engine->getRenderingServer()->CopyTextureDataComponent(reinterpret_cast<TextureDataComponent*>(VXGIConvertPass::Get().GetLuminanceVolume()), reinterpret_cast<TextureDataComponent*>(VXGIScreenSpaceFeedbackPass::Get().GetResult()));
		}
		else
		{
			g_Engine->getRenderingServer()->ClearTextureDataComponent(reinterpret_cast<TextureDataComponent*>(VXGIScreenSpaceFeedbackPass::Get().GetResult()));

			VXGIScreenSpaceFeedbackPass::Get().PrepareCommandList();
			g_Engine->getRenderingServer()->ExecuteCommandList(VXGIScreenSpaceFeedbackPass::Get().GetRPDC(), RenderPassUsage::Graphics);
		}

		VXGIRayTracingPassRenderingContext l_VXGIRayTracingPassRenderingContext;
		l_VXGIRayTracingPassRenderingContext.m_input = VXGIScreenSpaceFeedbackPass::Get().GetResult();

		// @TODO: Investigate if we really needs a dynamic output target or not
		l_VXGIRayTracingPassRenderingContext.m_output = VXGIRayTracingPass::Get().GetResult();

		VXGIRayTracingPass::Get().PrepareCommandList(&l_VXGIRayTracingPassRenderingContext);
		g_Engine->getRenderingServer()->ExecuteCommandList(VXGIRayTracingPass::Get().GetRPDC(), RenderPassUsage::Graphics);

		m_result = VXGIRayTracingPass::Get().GetResult();
	}
	else
	{
		auto l_cameraPos = g_Engine->getRenderingFrontend()->getPerFrameConstantBuffer().camera_posWS;
		auto l_sceneAABB = g_Engine->getPhysicsSystem()->getStaticSceneAABB();
		auto l_maxExtend = std::max(std::max(l_sceneAABB.m_extend.x, l_sceneAABB.m_extend.y), l_sceneAABB.m_extend.z);
		auto l_adjustedBoundMax = l_sceneAABB.m_boundMin + Vec4(l_maxExtend, l_maxExtend, l_maxExtend, 0.0f);
		auto l_adjustedCenter = l_sceneAABB.m_boundMin + Vec4(l_maxExtend, l_maxExtend, l_maxExtend, 0.0f) / 2.0f;

		VoxelizationConstantBuffer l_voxelPassCB;

		l_voxelPassCB.volumeCenter = l_adjustedCenter;
		l_voxelPassCB.volumeExtend = l_maxExtend;
		l_voxelPassCB.volumeExtendRcp = 1.0f / l_voxelPassCB.volumeExtend;
		l_voxelPassCB.volumeResolution = (float)m_VXGIRenderingConfig.m_voxelizationResolution;
		l_voxelPassCB.volumeResolutionRcp = 1.0f / l_voxelPassCB.volumeResolution;
		l_voxelPassCB.voxelSize = l_voxelPassCB.volumeExtend / l_voxelPassCB.volumeResolution;
		l_voxelPassCB.voxelSizeRcp = 1.0f / l_voxelPassCB.voxelSize;
		l_voxelPassCB.numCones = (float)m_VXGIRenderingConfig.m_numCones;
		l_voxelPassCB.numConesRcp = 1.0f / l_voxelPassCB.numCones;
		l_voxelPassCB.coneTracingStep = (float)m_VXGIRenderingConfig.m_coneTracingStep;
		l_voxelPassCB.coneTracingMaxDistance = m_VXGIRenderingConfig.m_coneTracingMaxDistance;

		g_Engine->getRenderingServer()->UploadGPUBufferDataComponent(m_VXGICBuffer, &l_voxelPassCB);

		////
		g_Engine->getRenderingServer()->ClearGPUBufferDataComponent(reinterpret_cast<GPUBufferDataComponent*>(VXGIGeometryProcessPass::Get().GetResult()));

		VXGIGeometryProcessPass::Get().PrepareCommandList();
		g_Engine->getRenderingServer()->ExecuteCommandList(VXGIGeometryProcessPass::Get().GetRPDC(), RenderPassUsage::Graphics);

		VXGIConvertPass::Get().PrepareCommandList();
		g_Engine->getRenderingServer()->ExecuteCommandList(VXGIConvertPass::Get().GetRPDC(), RenderPassUsage::Graphics);

		if (m_VXGIRenderingConfig.m_multiBounceCount)
		{
			VXGIMultiBouncePassRenderingContext l_VXGIMultiBouncePassRenderingContext;
			l_VXGIMultiBouncePassRenderingContext.m_input = VXGIConvertPass::Get().GetLuminanceVolume();

			// @TODO: Investigate if we really needs a dynamic output target or not
			l_VXGIMultiBouncePassRenderingContext.m_output = VXGIMultiBouncePass::Get().GetResult();	

			for (uint32_t i = 0; i < m_VXGIRenderingConfig.m_multiBounceCount; i++)
			{
				VXGIMultiBouncePass::Get().PrepareCommandList(&l_VXGIMultiBouncePassRenderingContext);

				g_Engine->getRenderingServer()->ExecuteCommandList(VXGIMultiBouncePass::Get().GetRPDC(), RenderPassUsage::Graphics);

				g_Engine->getRenderingServer()->GenerateMipmap(reinterpret_cast<TextureDataComponent*>(l_VXGIMultiBouncePassRenderingContext.m_output));

				m_result = l_VXGIMultiBouncePassRenderingContext.m_output;

				std::swap(l_VXGIMultiBouncePassRenderingContext.m_input, l_VXGIMultiBouncePassRenderingContext.m_output);
			}
		}
		else
		{
			m_result = VXGIConvertPass::Get().GetLuminanceVolume();
		}
	}

	if (m_VXGIRenderingConfig.m_visualize)
	{
		VXGIVisualizationPassRenderingContext l_VXGIVisualizationPassRenderingContext;
		l_VXGIVisualizationPassRenderingContext.m_input = m_result;

		VXGIVisualizationPass::Get().PrepareCommandList(&l_VXGIVisualizationPassRenderingContext);
		g_Engine->getRenderingServer()->ExecuteCommandList(VXGIVisualizationPass::Get().GetRPDC(), RenderPassUsage::Graphics);
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

const VXGIRenderingConfig& VXGIRenderer::GetVXGIRenderingConfig()
{
	return m_VXGIRenderingConfig;
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