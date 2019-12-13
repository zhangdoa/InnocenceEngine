#include "VolumetricFogPass.h"
#include "../DefaultGPUBuffers/DefaultGPUBuffers.h"

#include "OpaquePass.h"
#include "PreTAAPass.h"

#include "../../Engine/ModuleManager/IModuleManager.h"

INNO_ENGINE_API extern IModuleManager* g_pModuleManager;

using namespace DefaultGPUBuffers;

struct VolumetricPassConstantBuffer
{
	Mat4 VP[3];
	Mat4 VP_inv[3];
	Vec4 posWSOffset;
	Vec4 volumeDim;
	Vec4 voxelSize;
	Vec4 padding[5];
};

namespace VolumetricFogPass
{
	bool setupFroxelizationPass();
	bool setupIrradianceInjectionPass();
	bool setupRayMarchingPass();

	bool froxelization();
	bool irraidanceInjection();
	bool rayMarching();

	GPUBufferDataComponent* m_volumetricPassGBDC;

	RenderPassDataComponent* m_froxelizationRPDC;
	ShaderProgramComponent* m_froxelizationSPC;

	RenderPassDataComponent* m_irraidanceInjectionRPDC;
	ShaderProgramComponent* m_irraidanceInjectionSPC;

	RenderPassDataComponent* m_rayMarchingRPDC;
	ShaderProgramComponent* m_rayMarchingSPC;
	SamplerDataComponent* m_rayMarchingSDC;

	TextureDataComponent* m_irraidanceInjectionResult;
	TextureDataComponent* m_rayMarchingResult;

	const uint32_t m_volumeDimension = 128;
	const uint32_t m_voxelCount = m_volumeDimension * m_volumeDimension * m_volumeDimension;
}

bool VolumetricFogPass::setupFroxelizationPass()
{
	m_volumetricPassGBDC = g_pModuleManager->getRenderingServer()->AddGPUBufferDataComponent("VolumetricPassGPUBuffer/");
	m_volumetricPassGBDC->m_ElementCount = 1;
	m_volumetricPassGBDC->m_ElementSize = sizeof(VolumetricPassConstantBuffer);
	m_volumetricPassGBDC->m_BindingPoint = 12;

	////
	m_froxelizationSPC = g_pModuleManager->getRenderingServer()->AddShaderProgramComponent("VolumetricFogFroxelizationPass/");

	m_froxelizationSPC->m_ShaderFilePaths.m_VSPath = "volumetricFogFroxelizationPass.vert/";
	m_froxelizationSPC->m_ShaderFilePaths.m_GSPath = "volumetricFogFroxelizationPass.geom/";
	m_froxelizationSPC->m_ShaderFilePaths.m_PSPath = "volumetricFogFroxelizationPass.frag/";

	m_froxelizationRPDC = g_pModuleManager->getRenderingServer()->AddRenderPassDataComponent("VolumetricFogFroxelizationPass/");

	auto l_RenderPassDesc = g_pModuleManager->getRenderingFrontend()->getDefaultRenderPassDesc();

	l_RenderPassDesc.m_RenderTargetCount = 1;
	l_RenderPassDesc.m_IsOffScreen = true;

	l_RenderPassDesc.m_GraphicsPipelineDesc.m_RasterizerDesc.m_UseCulling = false;

	l_RenderPassDesc.m_RenderTargetDesc.SamplerType = TextureSamplerType::Sampler3D;
	l_RenderPassDesc.m_RenderTargetDesc.UsageType = TextureUsageType::RawImage;
	l_RenderPassDesc.m_RenderTargetDesc.GPUAccessibility = Accessibility::ReadWrite;
	l_RenderPassDesc.m_RenderTargetDesc.Width = 160;
	l_RenderPassDesc.m_RenderTargetDesc.Height = 90;
	l_RenderPassDesc.m_RenderTargetDesc.DepthOrArraySize = 64;
	l_RenderPassDesc.m_GraphicsPipelineDesc.m_ViewportDesc.m_Width = 160;
	l_RenderPassDesc.m_GraphicsPipelineDesc.m_ViewportDesc.m_Height = 90;
	l_RenderPassDesc.m_GraphicsPipelineDesc.m_ViewportDesc.m_MaxDepth = 64;

	m_froxelizationRPDC->m_RenderPassDesc = l_RenderPassDesc;

	m_froxelizationRPDC->m_ResourceBinderLayoutDescs.resize(5);
	m_froxelizationRPDC->m_ResourceBinderLayoutDescs[0].m_ResourceBinderType = ResourceBinderType::Buffer;
	m_froxelizationRPDC->m_ResourceBinderLayoutDescs[0].m_DescriptorSetIndex = 0;
	m_froxelizationRPDC->m_ResourceBinderLayoutDescs[0].m_DescriptorIndex = 0;

	m_froxelizationRPDC->m_ResourceBinderLayoutDescs[1].m_ResourceBinderType = ResourceBinderType::Buffer;
	m_froxelizationRPDC->m_ResourceBinderLayoutDescs[1].m_DescriptorSetIndex = 1;
	m_froxelizationRPDC->m_ResourceBinderLayoutDescs[1].m_DescriptorIndex = 1;

	m_froxelizationRPDC->m_ResourceBinderLayoutDescs[2].m_ResourceBinderType = ResourceBinderType::Buffer;
	m_froxelizationRPDC->m_ResourceBinderLayoutDescs[2].m_DescriptorSetIndex = 2;
	m_froxelizationRPDC->m_ResourceBinderLayoutDescs[2].m_DescriptorIndex = 2;

	m_froxelizationRPDC->m_ResourceBinderLayoutDescs[3].m_ResourceBinderType = ResourceBinderType::Buffer;
	m_froxelizationRPDC->m_ResourceBinderLayoutDescs[3].m_DescriptorSetIndex = 3;
	m_froxelizationRPDC->m_ResourceBinderLayoutDescs[3].m_DescriptorIndex = 9;

	m_froxelizationRPDC->m_ResourceBinderLayoutDescs[4].m_ResourceBinderType = ResourceBinderType::Image;
	m_froxelizationRPDC->m_ResourceBinderLayoutDescs[4].m_BinderAccessibility = Accessibility::ReadWrite;
	m_froxelizationRPDC->m_ResourceBinderLayoutDescs[4].m_ResourceAccessibility = Accessibility::ReadWrite;
	m_froxelizationRPDC->m_ResourceBinderLayoutDescs[4].m_DescriptorSetIndex = 4;
	m_froxelizationRPDC->m_ResourceBinderLayoutDescs[4].m_DescriptorIndex = 0;
	m_froxelizationRPDC->m_ResourceBinderLayoutDescs[4].m_IndirectBinding = true;

	m_froxelizationRPDC->m_ShaderProgram = m_froxelizationSPC;

	return true;
}

bool VolumetricFogPass::setupIrradianceInjectionPass()
{
	m_irraidanceInjectionSPC = g_pModuleManager->getRenderingServer()->AddShaderProgramComponent("VolumetricFogIrraidanceInjectionPass/");

	m_irraidanceInjectionSPC->m_ShaderFilePaths.m_CSPath = "volumetricFogIrraidanceInjectionPass.comp/";

	auto l_RenderPassDesc = g_pModuleManager->getRenderingFrontend()->getDefaultRenderPassDesc();
	l_RenderPassDesc.m_RenderTargetCount = 0;
	l_RenderPassDesc.m_RenderPassUsageType = RenderPassUsageType::Compute;
	l_RenderPassDesc.m_IsOffScreen = true;

	m_irraidanceInjectionRPDC = g_pModuleManager->getRenderingServer()->AddRenderPassDataComponent("VolumetricFogIrraidanceInjectionPass/");

	m_irraidanceInjectionRPDC->m_RenderPassDesc = l_RenderPassDesc;

	m_irraidanceInjectionRPDC->m_ResourceBinderLayoutDescs.resize(3);
	m_irraidanceInjectionRPDC->m_ResourceBinderLayoutDescs[0].m_ResourceBinderType = ResourceBinderType::Buffer;
	m_irraidanceInjectionRPDC->m_ResourceBinderLayoutDescs[0].m_DescriptorSetIndex = 0;
	m_irraidanceInjectionRPDC->m_ResourceBinderLayoutDescs[0].m_DescriptorIndex = 0;

	m_irraidanceInjectionRPDC->m_ResourceBinderLayoutDescs[1].m_ResourceBinderType = ResourceBinderType::Buffer;
	m_irraidanceInjectionRPDC->m_ResourceBinderLayoutDescs[1].m_DescriptorSetIndex = 0;
	m_irraidanceInjectionRPDC->m_ResourceBinderLayoutDescs[1].m_DescriptorIndex = 6;

	m_irraidanceInjectionRPDC->m_ResourceBinderLayoutDescs[2].m_ResourceBinderType = ResourceBinderType::Image;
	m_irraidanceInjectionRPDC->m_ResourceBinderLayoutDescs[2].m_BinderAccessibility = Accessibility::ReadWrite;
	m_irraidanceInjectionRPDC->m_ResourceBinderLayoutDescs[2].m_ResourceAccessibility = Accessibility::ReadWrite;
	m_irraidanceInjectionRPDC->m_ResourceBinderLayoutDescs[2].m_DescriptorSetIndex = 1;
	m_irraidanceInjectionRPDC->m_ResourceBinderLayoutDescs[2].m_DescriptorIndex = 0;
	m_irraidanceInjectionRPDC->m_ResourceBinderLayoutDescs[2].m_IndirectBinding = true;

	m_irraidanceInjectionRPDC->m_ShaderProgram = m_irraidanceInjectionSPC;

	return true;
}

bool VolumetricFogPass::setupRayMarchingPass()
{
	m_rayMarchingSPC = g_pModuleManager->getRenderingServer()->AddShaderProgramComponent("VolumetricFogRayMarchingPass/");

	m_rayMarchingSPC->m_ShaderFilePaths.m_CSPath = "volumetricFogRayMarchingPass.comp/";

	auto l_RenderPassDesc = g_pModuleManager->getRenderingFrontend()->getDefaultRenderPassDesc();
	l_RenderPassDesc.m_RenderTargetCount = 0;
	l_RenderPassDesc.m_RenderPassUsageType = RenderPassUsageType::Compute;
	l_RenderPassDesc.m_IsOffScreen = true;

	m_rayMarchingRPDC = g_pModuleManager->getRenderingServer()->AddRenderPassDataComponent("VolumetricFogRayMarchingPass/");

	m_rayMarchingRPDC->m_RenderPassDesc = l_RenderPassDesc;

	m_rayMarchingRPDC->m_ResourceBinderLayoutDescs.resize(4);
	m_rayMarchingRPDC->m_ResourceBinderLayoutDescs[0].m_ResourceBinderType = ResourceBinderType::Buffer;
	m_rayMarchingRPDC->m_ResourceBinderLayoutDescs[0].m_DescriptorSetIndex = 0;
	m_rayMarchingRPDC->m_ResourceBinderLayoutDescs[0].m_DescriptorIndex = 6;

	m_rayMarchingRPDC->m_ResourceBinderLayoutDescs[1].m_ResourceBinderType = ResourceBinderType::Image;
	m_rayMarchingRPDC->m_ResourceBinderLayoutDescs[1].m_BinderAccessibility = Accessibility::ReadOnly;
	m_rayMarchingRPDC->m_ResourceBinderLayoutDescs[1].m_ResourceAccessibility = Accessibility::ReadWrite;
	m_rayMarchingRPDC->m_ResourceBinderLayoutDescs[1].m_DescriptorSetIndex = 1;
	m_rayMarchingRPDC->m_ResourceBinderLayoutDescs[1].m_DescriptorIndex = 0;
	m_rayMarchingRPDC->m_ResourceBinderLayoutDescs[1].m_IndirectBinding = true;

	m_rayMarchingRPDC->m_ResourceBinderLayoutDescs[2].m_ResourceBinderType = ResourceBinderType::Image;
	m_rayMarchingRPDC->m_ResourceBinderLayoutDescs[2].m_BinderAccessibility = Accessibility::ReadWrite;
	m_rayMarchingRPDC->m_ResourceBinderLayoutDescs[2].m_ResourceAccessibility = Accessibility::ReadWrite;
	m_rayMarchingRPDC->m_ResourceBinderLayoutDescs[2].m_DescriptorSetIndex = 2;
	m_rayMarchingRPDC->m_ResourceBinderLayoutDescs[2].m_DescriptorIndex = 0;
	m_rayMarchingRPDC->m_ResourceBinderLayoutDescs[2].m_IndirectBinding = true;

	m_rayMarchingRPDC->m_ResourceBinderLayoutDescs[3].m_ResourceBinderType = ResourceBinderType::Sampler;
	m_rayMarchingRPDC->m_ResourceBinderLayoutDescs[3].m_DescriptorSetIndex = 3;
	m_rayMarchingRPDC->m_ResourceBinderLayoutDescs[3].m_DescriptorIndex = 0;
	m_rayMarchingRPDC->m_ResourceBinderLayoutDescs[3].m_IndirectBinding = true;

	m_rayMarchingRPDC->m_ShaderProgram = m_rayMarchingSPC;

	m_rayMarchingSDC = g_pModuleManager->getRenderingServer()->AddSamplerDataComponent("VolumetricFogRayMarchingPass/");

	return true;
}

bool VolumetricFogPass::Setup()
{
	setupFroxelizationPass();
	setupIrradianceInjectionPass();
	setupRayMarchingPass();

	////
	auto l_RenderPassDesc = g_pModuleManager->getRenderingFrontend()->getDefaultRenderPassDesc();
	l_RenderPassDesc.m_RenderTargetDesc.SamplerType = TextureSamplerType::Sampler3D;
	l_RenderPassDesc.m_RenderTargetDesc.UsageType = TextureUsageType::RawImage;
	l_RenderPassDesc.m_RenderTargetDesc.GPUAccessibility = Accessibility::ReadWrite;
	l_RenderPassDesc.m_RenderTargetDesc.Width = 160;
	l_RenderPassDesc.m_RenderTargetDesc.Height = 90;
	l_RenderPassDesc.m_RenderTargetDesc.DepthOrArraySize = 64;

	m_irraidanceInjectionResult = g_pModuleManager->getRenderingServer()->AddTextureDataComponent("VolumetricFogIrraidanceInjectionResult/");
	m_irraidanceInjectionResult->m_TextureDesc = l_RenderPassDesc.m_RenderTargetDesc;

	m_rayMarchingResult = g_pModuleManager->getRenderingServer()->AddTextureDataComponent("VolumetricFogRayMarchingResult/");
	m_rayMarchingResult->m_TextureDesc = l_RenderPassDesc.m_RenderTargetDesc;

	return true;
}

bool VolumetricFogPass::Initialize()
{
	g_pModuleManager->getRenderingServer()->InitializeGPUBufferDataComponent(m_volumetricPassGBDC);

	g_pModuleManager->getRenderingServer()->InitializeShaderProgramComponent(m_froxelizationSPC);
	g_pModuleManager->getRenderingServer()->InitializeRenderPassDataComponent(m_froxelizationRPDC);

	g_pModuleManager->getRenderingServer()->InitializeShaderProgramComponent(m_irraidanceInjectionSPC);
	g_pModuleManager->getRenderingServer()->InitializeRenderPassDataComponent(m_irraidanceInjectionRPDC);

	g_pModuleManager->getRenderingServer()->InitializeShaderProgramComponent(m_rayMarchingSPC);
	g_pModuleManager->getRenderingServer()->InitializeRenderPassDataComponent(m_rayMarchingRPDC);
	g_pModuleManager->getRenderingServer()->InitializeSamplerDataComponent(m_rayMarchingSDC);

	g_pModuleManager->getRenderingServer()->InitializeTextureDataComponent(m_irraidanceInjectionResult);
	g_pModuleManager->getRenderingServer()->InitializeTextureDataComponent(m_rayMarchingResult);

	return true;
}

bool VolumetricFogPass::froxelization()
{
	auto l_PerFrameCBufferGBDC = GetGPUBufferDataComponent(GPUBufferUsageType::PerFrame);
	//auto l_MeshGBDC = GetGPUBufferDataComponent(GPUBufferUsageType::VolumetricFogPassMesh);
	//auto l_MaterialGBDC = GetGPUBufferDataComponent(GPUBufferUsageType::VolumetricFogPassMaterial);
	auto l_MeshGBDC = GetGPUBufferDataComponent(GPUBufferUsageType::Mesh);
	auto l_MaterialGBDC = GetGPUBufferDataComponent(GPUBufferUsageType::Material);

	g_pModuleManager->getRenderingServer()->CommandListBegin(m_froxelizationRPDC, 0);
	g_pModuleManager->getRenderingServer()->BindRenderPassDataComponent(m_froxelizationRPDC);
	g_pModuleManager->getRenderingServer()->CleanRenderTargets(m_froxelizationRPDC);

	g_pModuleManager->getRenderingServer()->ActivateResourceBinder(m_froxelizationRPDC, ShaderStage::Vertex, l_PerFrameCBufferGBDC->m_ResourceBinder, 0, 0, Accessibility::ReadOnly);
	g_pModuleManager->getRenderingServer()->ActivateResourceBinder(m_froxelizationRPDC, ShaderStage::Geometry, m_volumetricPassGBDC->m_ResourceBinder, 3, 9, Accessibility::ReadOnly);
	g_pModuleManager->getRenderingServer()->ActivateResourceBinder(m_froxelizationRPDC, ShaderStage::Pixel, m_froxelizationRPDC->m_RenderTargetsResourceBinders[0], 4, 0, Accessibility::ReadWrite);

	//g_pModuleManager->getRenderingServer()->ActivateResourceBinder(m_froxelizationRPDC, ShaderStage::Vertex, l_MeshGBDC->m_ResourceBinder, 1, 1, Accessibility::ReadOnly, l_drawCallData.meshConstantBufferIndex, 1);
	//g_pModuleManager->getRenderingServer()->ActivateResourceBinder(m_froxelizationRPDC, ShaderStage::Pixel, l_MaterialGBDC->m_ResourceBinder, 2, 2, Accessibility::ReadOnly, l_drawCallData.materialConstantBufferIndex, 1);

	//auto l_mesh = g_pModuleManager->getRenderingFrontend()->getMeshDataComponent(MeshShapeType::Cube);

	//g_pModuleManager->getRenderingServer()->DispatchDrawCall(m_froxelizationRPDC, l_mesh);

	auto& l_drawCallInfo = g_pModuleManager->getRenderingFrontend()->getDrawCallInfo();
	auto l_drawCallCount = l_drawCallInfo.size();

	for (uint32_t i = 0; i < l_drawCallCount; i++)
	{
		auto l_drawCallData = l_drawCallInfo[i];
		if (l_drawCallData.visibilityType == VisibilityType::Opaque)
		{
			if (l_drawCallData.mesh->m_ObjectStatus == ObjectStatus::Activated)
			{
				g_pModuleManager->getRenderingServer()->ActivateResourceBinder(m_froxelizationRPDC, ShaderStage::Vertex, l_MeshGBDC->m_ResourceBinder, 1, 1, Accessibility::ReadOnly, l_drawCallData.meshConstantBufferIndex, 1);
				g_pModuleManager->getRenderingServer()->ActivateResourceBinder(m_froxelizationRPDC, ShaderStage::Pixel, l_MaterialGBDC->m_ResourceBinder, 2, 2, Accessibility::ReadOnly, l_drawCallData.materialConstantBufferIndex, 1);

				g_pModuleManager->getRenderingServer()->DispatchDrawCall(m_froxelizationRPDC, l_drawCallData.mesh);
			}
		}
	}

	g_pModuleManager->getRenderingServer()->DeactivateResourceBinder(m_froxelizationRPDC, ShaderStage::Pixel, m_froxelizationRPDC->m_RenderTargetsResourceBinders[0], 4, 0, Accessibility::ReadWrite);

	g_pModuleManager->getRenderingServer()->CommandListEnd(m_froxelizationRPDC);

	return true;
}

bool VolumetricFogPass::irraidanceInjection()
{
	auto l_PerFrameCBufferGBDC = GetGPUBufferDataComponent(GPUBufferUsageType::PerFrame);
	auto l_dispatchParamsGBDC = GetGPUBufferDataComponent(GPUBufferUsageType::ComputeDispatchParam);

	auto l_numThreadsX = 160;
	auto l_numThreadsY = 90;
	auto l_numThreadsZ = 64;

	DispatchParamsConstantBuffer l_irraidanceInjectionWorkload;
	l_irraidanceInjectionWorkload.numThreadGroups = TVec4<uint32_t>(20, 12, 8, 0);
	l_irraidanceInjectionWorkload.numThreads = TVec4<uint32_t>(l_numThreadsX, l_numThreadsY, l_numThreadsZ, 0);

	g_pModuleManager->getRenderingServer()->UploadGPUBufferDataComponent(l_dispatchParamsGBDC, &l_irraidanceInjectionWorkload, 6, 1);

	g_pModuleManager->getRenderingServer()->CommandListBegin(m_irraidanceInjectionRPDC, 0);
	g_pModuleManager->getRenderingServer()->BindRenderPassDataComponent(m_irraidanceInjectionRPDC);
	g_pModuleManager->getRenderingServer()->CleanRenderTargets(m_irraidanceInjectionRPDC);

	g_pModuleManager->getRenderingServer()->ActivateResourceBinder(m_irraidanceInjectionRPDC, ShaderStage::Compute, l_PerFrameCBufferGBDC->m_ResourceBinder, 0, 0, Accessibility::ReadOnly);
	g_pModuleManager->getRenderingServer()->ActivateResourceBinder(m_irraidanceInjectionRPDC, ShaderStage::Compute, l_dispatchParamsGBDC->m_ResourceBinder, 1, 6, Accessibility::ReadOnly);
	g_pModuleManager->getRenderingServer()->ActivateResourceBinder(m_irraidanceInjectionRPDC, ShaderStage::Compute, m_irraidanceInjectionResult->m_ResourceBinder, 2, 0, Accessibility::ReadWrite);

	g_pModuleManager->getRenderingServer()->DispatchCompute(m_irraidanceInjectionRPDC, 20, 12, 8);

	g_pModuleManager->getRenderingServer()->DeactivateResourceBinder(m_irraidanceInjectionRPDC, ShaderStage::Compute, m_irraidanceInjectionResult->m_ResourceBinder, 2, 0, Accessibility::ReadWrite);

	g_pModuleManager->getRenderingServer()->CommandListEnd(m_irraidanceInjectionRPDC);

	return true;
}

bool VolumetricFogPass::rayMarching()
{
	auto l_dispatchParamsGBDC = GetGPUBufferDataComponent(GPUBufferUsageType::ComputeDispatchParam);

	auto l_numThreadsX = 160;
	auto l_numThreadsY = 90;
	auto l_numThreadsZ = 64;

	DispatchParamsConstantBuffer l_rayMarchingWorkload;
	l_rayMarchingWorkload.numThreadGroups = TVec4<uint32_t>(20, 12, 8, 0);
	l_rayMarchingWorkload.numThreads = TVec4<uint32_t>(l_numThreadsX, l_numThreadsY, l_numThreadsZ, 0);

	g_pModuleManager->getRenderingServer()->UploadGPUBufferDataComponent(l_dispatchParamsGBDC, &l_rayMarchingWorkload, 7, 1);

	g_pModuleManager->getRenderingServer()->CommandListBegin(m_rayMarchingRPDC, 0);
	g_pModuleManager->getRenderingServer()->BindRenderPassDataComponent(m_rayMarchingRPDC);
	g_pModuleManager->getRenderingServer()->CleanRenderTargets(m_rayMarchingRPDC);

	g_pModuleManager->getRenderingServer()->ActivateResourceBinder(m_rayMarchingRPDC, ShaderStage::Compute, m_rayMarchingSDC->m_ResourceBinder, 3, 0, Accessibility::ReadOnly);

	g_pModuleManager->getRenderingServer()->ActivateResourceBinder(m_rayMarchingRPDC, ShaderStage::Compute, l_dispatchParamsGBDC->m_ResourceBinder, 0, 6, Accessibility::ReadOnly);
	g_pModuleManager->getRenderingServer()->ActivateResourceBinder(m_rayMarchingRPDC, ShaderStage::Compute, m_irraidanceInjectionResult->m_ResourceBinder, 1, 0, Accessibility::ReadOnly);
	g_pModuleManager->getRenderingServer()->ActivateResourceBinder(m_rayMarchingRPDC, ShaderStage::Compute, m_rayMarchingResult->m_ResourceBinder, 2, 0, Accessibility::ReadWrite);

	g_pModuleManager->getRenderingServer()->DispatchCompute(m_rayMarchingRPDC, 20, 12, 8);

	g_pModuleManager->getRenderingServer()->DeactivateResourceBinder(m_rayMarchingRPDC, ShaderStage::Compute, m_irraidanceInjectionResult->m_ResourceBinder, 1, 0, Accessibility::ReadOnly);
	g_pModuleManager->getRenderingServer()->DeactivateResourceBinder(m_rayMarchingRPDC, ShaderStage::Compute, m_rayMarchingResult->m_ResourceBinder, 2, 0, Accessibility::ReadWrite);

	g_pModuleManager->getRenderingServer()->CommandListEnd(m_rayMarchingRPDC);

	return true;
}

bool VolumetricFogPass::PrepareCommandList()
{
	auto l_p = InnoMath::generateOrthographicMatrix(-1.0f, 1.0f, -1.0f, 1.0f, 0.0f, 2.0f);
	auto l_center = Vec4(0.0f, 0.0f, 0.0f, 1.0f);

	VolumetricPassConstantBuffer l_volumetricPassConstantBuffer;

	l_volumetricPassConstantBuffer.VP[0] = InnoMath::lookAt(Vec4(0.0f, 0.0f, 0.0f, 1.0f), Vec4(1.0f, 0.0f, 0.0f, 1.0f), Vec4(0.0f, -1.0f, 0.0f, 0.0f));
	auto l_rNX = InnoMath::lookAt(Vec4(0.0f, 0.0f, 0.0f, 1.0f), Vec4(-1.0f, 0.0f, 0.0f, 1.0f), Vec4(0.0f, -1.0f, 0.0f, 0.0f));
	l_volumetricPassConstantBuffer.VP[1] = InnoMath::lookAt(Vec4(0.0f, 0.0f, 0.0f, 1.0f), Vec4(0.0f, 1.0f, 0.0f, 1.0f), Vec4(0.0f, 0.0f, 1.0f, 0.0f));
	auto l_rNY = InnoMath::lookAt(Vec4(0.0f, 0.0f, 0.0f, 1.0f), Vec4(0.0f, -1.0f, 0.0f, 1.0f), Vec4(0.0f, 0.0f, 1.0f, 0.0f));
	l_volumetricPassConstantBuffer.VP[2] = InnoMath::lookAt(Vec4(0.0f, 0.0f, 0.0f, 1.0f), Vec4(0.0f, 0.0f, 1.0f, 1.0f), Vec4(0.0f, -1.0f, 0.0f, 0.0f));
	auto l_rNZ = InnoMath::lookAt(Vec4(0.0f, 0.0f, 0.0f, 1.0f), Vec4(0.0f, 0.0f, -1.0f, 1.0f), Vec4(0.0f, -1.0f, 0.0f, 0.0f));

	for (size_t i = 0; i < 3; i++)
	{
		l_volumetricPassConstantBuffer.VP[i] = l_p * l_volumetricPassConstantBuffer.VP[i];
		l_volumetricPassConstantBuffer.VP_inv[i] = l_volumetricPassConstantBuffer.VP[i].inverse();
	}

	g_pModuleManager->getRenderingServer()->UploadGPUBufferDataComponent(m_volumetricPassGBDC, &l_volumetricPassConstantBuffer);

	froxelization();
	irraidanceInjection();
	rayMarching();

	return true;
}

bool VolumetricFogPass::ExecuteCommandList()
{
	g_pModuleManager->getRenderingServer()->ExecuteCommandList(m_froxelizationRPDC);

	g_pModuleManager->getRenderingServer()->WaitForFrame(m_froxelizationRPDC);

	g_pModuleManager->getRenderingServer()->ExecuteCommandList(m_irraidanceInjectionRPDC);

	g_pModuleManager->getRenderingServer()->WaitForFrame(m_irraidanceInjectionRPDC);

	g_pModuleManager->getRenderingServer()->ExecuteCommandList(m_rayMarchingRPDC);

	g_pModuleManager->getRenderingServer()->WaitForFrame(m_rayMarchingRPDC);

	return true;
}

bool VolumetricFogPass::Terminate()
{
	g_pModuleManager->getRenderingServer()->DeleteRenderPassDataComponent(m_froxelizationRPDC);
	g_pModuleManager->getRenderingServer()->DeleteRenderPassDataComponent(m_irraidanceInjectionRPDC);
	g_pModuleManager->getRenderingServer()->DeleteRenderPassDataComponent(m_rayMarchingRPDC);

	return true;
}

IResourceBinder * VolumetricFogPass::GetRayMarchingResult()
{
	return m_rayMarchingResult->m_ResourceBinder;
}