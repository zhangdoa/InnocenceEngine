#include "GIResolvePass.h"
#include "DefaultGPUBuffers.h"

#include "GIBakePass.h"

#include "../../Engine/ModuleManager/IModuleManager.h"

INNO_ENGINE_API extern IModuleManager* g_pModuleManager;

using namespace DefaultGPUBuffers;

namespace GIResolvePass
{
	bool initializeGPUBuffers();
	bool deleteGPUBuffers();
	bool litSurfels();
	bool litBricks();
	bool litProbes();

	RenderPassDataComponent* m_surfelRPDC = 0;
	ShaderProgramComponent* m_surfelSPC = 0;

	RenderPassDataComponent* m_brickRPDC = 0;
	ShaderProgramComponent* m_brickSPC = 0;

	RenderPassDataComponent* m_probeRPDC = 0;
	ShaderProgramComponent* m_probeSPC = 0;

	GPUBufferDataComponent* m_surfelGBDC = 0;
	GPUBufferDataComponent* m_surfelIrradianceGBDC = 0;
	GPUBufferDataComponent* m_brickGBDC = 0;
	GPUBufferDataComponent* m_brickIrradianceGBDC = 0;
	GPUBufferDataComponent* m_brickFactorGBDC = 0;
	GPUBufferDataComponent* m_probeGBDC = 0;
	TextureDataComponent* m_irradianceVolume = 0;

	std::function<void()> f_sceneLoadingFinishCallback;
	std::function<void()> f_sceneLoadingStartCallback;

	bool l_GIDataLoaded = false;
}

bool GIResolvePass::initializeGPUBuffers()
{
	auto l_surfels = GIBakePass::GetSurfels();

	if (l_surfels.size())
	{
		m_surfelGBDC = g_pModuleManager->getRenderingServer()->AddGPUBufferDataComponent("SurfelGPUBuffer/");
		m_surfelGBDC->m_GPUAccessibility = Accessibility::ReadWrite;
		m_surfelGBDC->m_ElementCount = l_surfels.size();
		m_surfelGBDC->m_ElementSize = sizeof(Surfel);
		m_surfelGBDC->m_BindingPoint = 0;
		m_surfelGBDC->m_InitialData = &l_surfels[0];

		g_pModuleManager->getRenderingServer()->InitializeGPUBufferDataComponent(m_surfelGBDC);

		m_surfelIrradianceGBDC = g_pModuleManager->getRenderingServer()->AddGPUBufferDataComponent("SurfelIrradianceGPUBuffer/");
		m_surfelIrradianceGBDC->m_GPUAccessibility = Accessibility::ReadWrite;
		m_surfelIrradianceGBDC->m_ElementCount = l_surfels.size();
		m_surfelIrradianceGBDC->m_ElementSize = sizeof(vec4);
		m_surfelIrradianceGBDC->m_BindingPoint = 1;

		g_pModuleManager->getRenderingServer()->InitializeGPUBufferDataComponent(m_surfelIrradianceGBDC);

		auto l_bricks = GIBakePass::GetBricks();

		std::vector<unsigned int> l_brickGPUData;

		l_brickGPUData.resize(l_bricks.size() * 2);
		for (size_t i = 0; i < l_bricks.size(); i++)
		{
			l_brickGPUData[2 * i] = l_bricks[i].surfelRangeBegin;
			l_brickGPUData[2 * i + 1] = l_bricks[i].surfelRangeEnd;
		}

		m_brickGBDC = g_pModuleManager->getRenderingServer()->AddGPUBufferDataComponent("BrickGPUBuffer/");
		m_brickGBDC->m_GPUAccessibility = Accessibility::ReadWrite;
		m_brickGBDC->m_ElementCount = l_bricks.size();
		m_brickGBDC->m_ElementSize = sizeof(unsigned int) * 2;
		m_brickGBDC->m_BindingPoint = 0;
		m_brickGBDC->m_InitialData = &l_brickGPUData[0];

		g_pModuleManager->getRenderingServer()->InitializeGPUBufferDataComponent(m_brickGBDC);

		m_brickIrradianceGBDC = g_pModuleManager->getRenderingServer()->AddGPUBufferDataComponent("BrickIrradianceGPUBuffer/");
		m_brickIrradianceGBDC->m_GPUAccessibility = Accessibility::ReadWrite;
		m_brickIrradianceGBDC->m_ElementCount = l_bricks.size();
		m_brickIrradianceGBDC->m_ElementSize = sizeof(vec4);
		m_brickIrradianceGBDC->m_BindingPoint = 1;

		g_pModuleManager->getRenderingServer()->InitializeGPUBufferDataComponent(m_brickIrradianceGBDC);

		auto l_brickFactors = GIBakePass::GetBrickFactors();

		m_brickFactorGBDC = g_pModuleManager->getRenderingServer()->AddGPUBufferDataComponent("BrickFactorGPUBuffer/");
		m_brickFactorGBDC->m_GPUAccessibility = Accessibility::ReadWrite;
		m_brickFactorGBDC->m_ElementCount = l_brickFactors.size();
		m_brickFactorGBDC->m_ElementSize = sizeof(BrickFactor);
		m_brickFactorGBDC->m_BindingPoint = 2;
		m_brickFactorGBDC->m_InitialData = &l_brickFactors[0];

		g_pModuleManager->getRenderingServer()->InitializeGPUBufferDataComponent(m_brickFactorGBDC);

		auto l_probes = GIBakePass::GetProbes();

		m_probeGBDC = g_pModuleManager->getRenderingServer()->AddGPUBufferDataComponent("ProbeGPUBuffer/");
		m_probeGBDC->m_GPUAccessibility = Accessibility::ReadWrite;
		m_probeGBDC->m_ElementCount = l_probes.size();
		m_probeGBDC->m_ElementSize = sizeof(Probe);
		m_probeGBDC->m_BindingPoint = 3;
		m_probeGBDC->m_InitialData = &l_probes[0];

		g_pModuleManager->getRenderingServer()->InitializeGPUBufferDataComponent(m_probeGBDC);

		auto l_RenderPassDesc = g_pModuleManager->getRenderingFrontend()->getDefaultRenderPassDesc();

		m_irradianceVolume = g_pModuleManager->getRenderingServer()->AddTextureDataComponent("IrradianceVolume/");
		m_irradianceVolume->m_textureDataDesc = l_RenderPassDesc.m_RenderTargetDesc;

		m_irradianceVolume->m_textureDataDesc.Width = GIBakePass::GetProbeDimension();
		m_irradianceVolume->m_textureDataDesc.Height = GIBakePass::GetProbeDimension();
		m_irradianceVolume->m_textureDataDesc.DepthOrArraySize = GIBakePass::GetProbeDimension();
		m_irradianceVolume->m_textureDataDesc.UsageType = TextureUsageType::RawImage;
		m_irradianceVolume->m_textureDataDesc.SamplerType = TextureSamplerType::Sampler3D;
		m_irradianceVolume->m_textureDataDesc.PixelDataFormat = TexturePixelDataFormat::RGBA;
		m_irradianceVolume->m_textureDataDesc.PixelDataType = TexturePixelDataType::FLOAT16;

		g_pModuleManager->getRenderingServer()->InitializeTextureDataComponent(m_irradianceVolume);

		l_GIDataLoaded = true;
	}

	return true;
}

bool GIResolvePass::deleteGPUBuffers()
{
	if (m_surfelGBDC)
	{
		g_pModuleManager->getRenderingServer()->DeleteGPUBufferDataComponent(m_surfelGBDC);
	}
	if (m_surfelIrradianceGBDC)
	{
		g_pModuleManager->getRenderingServer()->DeleteGPUBufferDataComponent(m_surfelIrradianceGBDC);
	}
	if (m_brickGBDC)
	{
		g_pModuleManager->getRenderingServer()->DeleteGPUBufferDataComponent(m_brickGBDC);
	}
	if (m_brickIrradianceGBDC)
	{
		g_pModuleManager->getRenderingServer()->DeleteGPUBufferDataComponent(m_brickIrradianceGBDC);
	}
	if (m_brickFactorGBDC)
	{
		g_pModuleManager->getRenderingServer()->DeleteGPUBufferDataComponent(m_brickFactorGBDC);
	}
	if (m_probeGBDC)
	{
		g_pModuleManager->getRenderingServer()->DeleteGPUBufferDataComponent(m_probeGBDC);
	}

	l_GIDataLoaded = false;

	return true;
}

bool GIResolvePass::Setup()
{
	m_surfelSPC = g_pModuleManager->getRenderingServer()->AddShaderProgramComponent("GISurfelResolvePass/");
	m_surfelSPC->m_ShaderFilePaths.m_CSPath = "GISurfelResolvePass.comp/";
	g_pModuleManager->getRenderingServer()->InitializeShaderProgramComponent(m_surfelSPC);

	m_surfelRPDC = g_pModuleManager->getRenderingServer()->AddRenderPassDataComponent("GISurfelResolvePass/");

	auto l_RenderPassDesc = g_pModuleManager->getRenderingFrontend()->getDefaultRenderPassDesc();
	l_RenderPassDesc.m_RenderTargetCount = 0;
	l_RenderPassDesc.m_RenderPassUsageType = RenderPassUsageType::Compute;

	m_surfelRPDC->m_RenderPassDesc = l_RenderPassDesc;

	m_surfelRPDC->m_ResourceBinderLayoutDescs.resize(5);
	m_surfelRPDC->m_ResourceBinderLayoutDescs[0].m_ResourceBinderType = ResourceBinderType::Buffer;
	m_surfelRPDC->m_ResourceBinderLayoutDescs[0].m_GlobalSlot = 0;
	m_surfelRPDC->m_ResourceBinderLayoutDescs[0].m_LocalSlot = 0;

	m_surfelRPDC->m_ResourceBinderLayoutDescs[1].m_ResourceBinderType = ResourceBinderType::Buffer;
	m_surfelRPDC->m_ResourceBinderLayoutDescs[1].m_GlobalSlot = 1;
	m_surfelRPDC->m_ResourceBinderLayoutDescs[1].m_LocalSlot = 3;

	m_surfelRPDC->m_ResourceBinderLayoutDescs[2].m_ResourceBinderType = ResourceBinderType::Buffer;
	m_surfelRPDC->m_ResourceBinderLayoutDescs[2].m_GlobalSlot = 2;
	m_surfelRPDC->m_ResourceBinderLayoutDescs[2].m_LocalSlot = 8;

	m_surfelRPDC->m_ResourceBinderLayoutDescs[3].m_ResourceBinderType = ResourceBinderType::Buffer;
	m_surfelRPDC->m_ResourceBinderLayoutDescs[3].m_BinderAccessibility = Accessibility::ReadWrite;
	m_surfelRPDC->m_ResourceBinderLayoutDescs[3].m_ResourceAccessibility = Accessibility::ReadWrite;
	m_surfelRPDC->m_ResourceBinderLayoutDescs[3].m_GlobalSlot = 3;
	m_surfelRPDC->m_ResourceBinderLayoutDescs[3].m_LocalSlot = 0;

	m_surfelRPDC->m_ResourceBinderLayoutDescs[4].m_ResourceBinderType = ResourceBinderType::Buffer;
	m_surfelRPDC->m_ResourceBinderLayoutDescs[4].m_BinderAccessibility = Accessibility::ReadWrite;
	m_surfelRPDC->m_ResourceBinderLayoutDescs[4].m_ResourceAccessibility = Accessibility::ReadWrite;
	m_surfelRPDC->m_ResourceBinderLayoutDescs[4].m_GlobalSlot = 4;
	m_surfelRPDC->m_ResourceBinderLayoutDescs[4].m_LocalSlot = 1;

	m_surfelRPDC->m_ShaderProgram = m_surfelSPC;

	g_pModuleManager->getRenderingServer()->InitializeRenderPassDataComponent(m_surfelRPDC);

	////
	m_brickSPC = g_pModuleManager->getRenderingServer()->AddShaderProgramComponent("GIBrickResolvePass/");
	m_brickSPC->m_ShaderFilePaths.m_CSPath = "GIBrickResolvePass.comp/";
	g_pModuleManager->getRenderingServer()->InitializeShaderProgramComponent(m_brickSPC);

	m_brickRPDC = g_pModuleManager->getRenderingServer()->AddRenderPassDataComponent("GIBrickResolvePass/");

	m_brickRPDC->m_RenderPassDesc = l_RenderPassDesc;

	m_brickRPDC->m_ResourceBinderLayoutDescs.resize(4);
	m_brickRPDC->m_ResourceBinderLayoutDescs[0].m_ResourceBinderType = ResourceBinderType::Buffer;
	m_brickRPDC->m_ResourceBinderLayoutDescs[0].m_GlobalSlot = 0;
	m_brickRPDC->m_ResourceBinderLayoutDescs[0].m_LocalSlot = 8;

	m_brickRPDC->m_ResourceBinderLayoutDescs[1].m_ResourceBinderType = ResourceBinderType::Buffer;
	m_brickRPDC->m_ResourceBinderLayoutDescs[1].m_BinderAccessibility = Accessibility::ReadWrite;
	m_brickRPDC->m_ResourceBinderLayoutDescs[1].m_ResourceAccessibility = Accessibility::ReadWrite;
	m_brickRPDC->m_ResourceBinderLayoutDescs[1].m_GlobalSlot = 1;
	m_brickRPDC->m_ResourceBinderLayoutDescs[1].m_LocalSlot = 0;

	m_brickRPDC->m_ResourceBinderLayoutDescs[2].m_ResourceBinderType = ResourceBinderType::Buffer;
	m_brickRPDC->m_ResourceBinderLayoutDescs[2].m_BinderAccessibility = Accessibility::ReadWrite;
	m_brickRPDC->m_ResourceBinderLayoutDescs[2].m_ResourceAccessibility = Accessibility::ReadWrite;
	m_brickRPDC->m_ResourceBinderLayoutDescs[2].m_GlobalSlot = 2;
	m_brickRPDC->m_ResourceBinderLayoutDescs[2].m_LocalSlot = 1;

	m_brickRPDC->m_ResourceBinderLayoutDescs[3].m_ResourceBinderType = ResourceBinderType::Buffer;
	m_brickRPDC->m_ResourceBinderLayoutDescs[3].m_BinderAccessibility = Accessibility::ReadWrite;
	m_brickRPDC->m_ResourceBinderLayoutDescs[3].m_ResourceAccessibility = Accessibility::ReadWrite;
	m_brickRPDC->m_ResourceBinderLayoutDescs[3].m_GlobalSlot = 3;
	m_brickRPDC->m_ResourceBinderLayoutDescs[3].m_LocalSlot = 2;

	m_brickRPDC->m_ShaderProgram = m_brickSPC;

	g_pModuleManager->getRenderingServer()->InitializeRenderPassDataComponent(m_brickRPDC);

	////
	m_probeSPC = g_pModuleManager->getRenderingServer()->AddShaderProgramComponent("GIProbeResolvePass/");
	m_probeSPC->m_ShaderFilePaths.m_CSPath = "GIProbeResolvePass.comp/";
	g_pModuleManager->getRenderingServer()->InitializeShaderProgramComponent(m_probeSPC);

	m_probeRPDC = g_pModuleManager->getRenderingServer()->AddRenderPassDataComponent("GIProbeResolvePass/");

	m_probeRPDC->m_RenderPassDesc = l_RenderPassDesc;

	m_probeRPDC->m_ResourceBinderLayoutDescs.resize(5);
	m_probeRPDC->m_ResourceBinderLayoutDescs[0].m_ResourceBinderType = ResourceBinderType::Buffer;
	m_probeRPDC->m_ResourceBinderLayoutDescs[0].m_GlobalSlot = 0;
	m_probeRPDC->m_ResourceBinderLayoutDescs[0].m_LocalSlot = 8;

	m_probeRPDC->m_ResourceBinderLayoutDescs[1].m_ResourceBinderType = ResourceBinderType::Buffer;
	m_probeRPDC->m_ResourceBinderLayoutDescs[1].m_BinderAccessibility = Accessibility::ReadWrite;
	m_probeRPDC->m_ResourceBinderLayoutDescs[1].m_ResourceAccessibility = Accessibility::ReadWrite;
	m_probeRPDC->m_ResourceBinderLayoutDescs[1].m_GlobalSlot = 1;
	m_probeRPDC->m_ResourceBinderLayoutDescs[1].m_LocalSlot = 0;

	m_probeRPDC->m_ResourceBinderLayoutDescs[2].m_ResourceBinderType = ResourceBinderType::Buffer;
	m_probeRPDC->m_ResourceBinderLayoutDescs[2].m_BinderAccessibility = Accessibility::ReadWrite;
	m_probeRPDC->m_ResourceBinderLayoutDescs[2].m_ResourceAccessibility = Accessibility::ReadWrite;
	m_probeRPDC->m_ResourceBinderLayoutDescs[2].m_GlobalSlot = 2;
	m_probeRPDC->m_ResourceBinderLayoutDescs[2].m_LocalSlot = 1;

	m_probeRPDC->m_ResourceBinderLayoutDescs[3].m_ResourceBinderType = ResourceBinderType::Buffer;
	m_probeRPDC->m_ResourceBinderLayoutDescs[3].m_BinderAccessibility = Accessibility::ReadWrite;
	m_probeRPDC->m_ResourceBinderLayoutDescs[3].m_ResourceAccessibility = Accessibility::ReadWrite;
	m_probeRPDC->m_ResourceBinderLayoutDescs[3].m_GlobalSlot = 3;
	m_probeRPDC->m_ResourceBinderLayoutDescs[3].m_LocalSlot = 2;

	m_probeRPDC->m_ResourceBinderLayoutDescs[4].m_ResourceBinderType = ResourceBinderType::Image;
	m_probeRPDC->m_ResourceBinderLayoutDescs[4].m_BinderAccessibility = Accessibility::ReadWrite;
	m_probeRPDC->m_ResourceBinderLayoutDescs[4].m_ResourceAccessibility = Accessibility::ReadWrite;
	m_probeRPDC->m_ResourceBinderLayoutDescs[4].m_GlobalSlot = 4;
	m_probeRPDC->m_ResourceBinderLayoutDescs[4].m_LocalSlot = 3;
	m_probeRPDC->m_ResourceBinderLayoutDescs[4].m_IsRanged = true;

	m_probeRPDC->m_ShaderProgram = m_probeSPC;

	g_pModuleManager->getRenderingServer()->InitializeRenderPassDataComponent(m_probeRPDC);

	f_sceneLoadingFinishCallback = []() { initializeGPUBuffers(); };
	f_sceneLoadingStartCallback = []() { deleteGPUBuffers(); };

	g_pModuleManager->getFileSystem()->addSceneLoadingFinishCallback(&f_sceneLoadingFinishCallback);
	g_pModuleManager->getFileSystem()->addSceneLoadingStartCallback(&f_sceneLoadingStartCallback);

	return true;
}

bool GIResolvePass::Initialize()
{
	return true;
}

bool GIResolvePass::litSurfels()
{
	auto l_CameraGBDC = GetGPUBufferDataComponent(GPUBufferUsageType::Camera);
	auto l_SunGBDC = GetGPUBufferDataComponent(GPUBufferUsageType::Sun);
	auto l_dispatchParamsGBDC = GetGPUBufferDataComponent(GPUBufferUsageType::Compute);

	auto l_threadCountPerGroup = 8;
	auto l_totalThreadGroupsCount = (double)m_surfelGBDC->m_ElementCount / (l_threadCountPerGroup * l_threadCountPerGroup * l_threadCountPerGroup);
	auto l_averangeThreadGroupsCountPerSide = std::pow(l_totalThreadGroupsCount, 1.0 / 3.0);

	auto l_numThreadsX = (unsigned int)std::ceil(l_averangeThreadGroupsCountPerSide * l_threadCountPerGroup);
	auto l_numThreadsY = (unsigned int)std::ceil(l_averangeThreadGroupsCountPerSide * l_threadCountPerGroup);
	auto l_numThreadsZ = (unsigned int)std::ceil((double)m_surfelGBDC->m_ElementCount / (l_numThreadsX * l_numThreadsY));

	auto l_numThreadGroupsX = (unsigned int)std::ceil((double)l_numThreadsX / (double)l_threadCountPerGroup);
	auto l_numThreadGroupsY = (unsigned int)std::ceil((double)l_numThreadsY / (double)l_threadCountPerGroup);
	auto l_numThreadGroupsZ = (unsigned int)std::ceil((double)l_numThreadsZ / (double)l_threadCountPerGroup);

	DispatchParamsGPUData l_surfelLitWorkload;
	l_surfelLitWorkload.numThreadGroups = TVec4<unsigned int>(l_numThreadGroupsX, l_numThreadGroupsY, l_numThreadGroupsZ, 0);
	l_surfelLitWorkload.numThreads = TVec4<unsigned int>(l_numThreadsX, l_numThreadsY, l_numThreadsZ, 0);

	g_pModuleManager->getRenderingServer()->UploadGPUBufferDataComponent(l_dispatchParamsGBDC, &l_surfelLitWorkload, 2, 1);

	g_pModuleManager->getRenderingServer()->CommandListBegin(m_surfelRPDC, 0);
	g_pModuleManager->getRenderingServer()->BindRenderPassDataComponent(m_surfelRPDC);
	g_pModuleManager->getRenderingServer()->CleanRenderTargets(m_surfelRPDC);

	g_pModuleManager->getRenderingServer()->ActivateResourceBinder(m_surfelRPDC, ShaderStage::Compute, l_CameraGBDC->m_ResourceBinder, 0, 0, Accessibility::ReadOnly, false, 0, l_CameraGBDC->m_TotalSize);
	g_pModuleManager->getRenderingServer()->ActivateResourceBinder(m_surfelRPDC, ShaderStage::Compute, l_SunGBDC->m_ResourceBinder, 1, 3, Accessibility::ReadOnly, false, 0, l_dispatchParamsGBDC->m_TotalSize);
	g_pModuleManager->getRenderingServer()->ActivateResourceBinder(m_surfelRPDC, ShaderStage::Compute, l_dispatchParamsGBDC->m_ResourceBinder, 2, 8, Accessibility::ReadOnly, false, 0, l_dispatchParamsGBDC->m_TotalSize);
	g_pModuleManager->getRenderingServer()->ActivateResourceBinder(m_surfelRPDC, ShaderStage::Compute, m_surfelGBDC->m_ResourceBinder, 3, 0, Accessibility::ReadWrite, false, 0, m_surfelGBDC->m_TotalSize);
	g_pModuleManager->getRenderingServer()->ActivateResourceBinder(m_surfelRPDC, ShaderStage::Compute, m_surfelIrradianceGBDC->m_ResourceBinder, 4, 1, Accessibility::ReadWrite, false, 0, m_surfelIrradianceGBDC->m_TotalSize);

	g_pModuleManager->getRenderingServer()->DispatchCompute(m_surfelRPDC, l_numThreadGroupsX, l_numThreadGroupsY, l_numThreadGroupsZ);

	g_pModuleManager->getRenderingServer()->DeactivateResourceBinder(m_surfelRPDC, ShaderStage::Compute, m_surfelGBDC->m_ResourceBinder, 3, 0, Accessibility::ReadWrite, false, 0, m_surfelGBDC->m_TotalSize);
	g_pModuleManager->getRenderingServer()->DeactivateResourceBinder(m_surfelRPDC, ShaderStage::Compute, m_surfelIrradianceGBDC->m_ResourceBinder, 4, 1, Accessibility::ReadWrite, false, 0, m_surfelIrradianceGBDC->m_TotalSize);

	g_pModuleManager->getRenderingServer()->CommandListEnd(m_surfelRPDC);

	return true;
}

bool GIResolvePass::litBricks()
{
	auto l_dispatchParamsGBDC = GetGPUBufferDataComponent(GPUBufferUsageType::Compute);

	auto l_threadCountPerGroup = 8;
	auto l_totalThreadGroupsCount = (double)m_brickGBDC->m_ElementCount / (l_threadCountPerGroup * l_threadCountPerGroup * l_threadCountPerGroup);
	auto l_averangeThreadGroupsCountPerSide = std::pow(l_totalThreadGroupsCount, 1.0 / 3.0);

	auto l_numThreadsX = (unsigned int)std::ceil(l_averangeThreadGroupsCountPerSide * l_threadCountPerGroup);
	auto l_numThreadsY = (unsigned int)std::ceil(l_averangeThreadGroupsCountPerSide * l_threadCountPerGroup);
	auto l_numThreadsZ = (unsigned int)std::ceil((double)m_brickGBDC->m_ElementCount / (l_numThreadsX * l_numThreadsY));

	auto l_numThreadGroupsX = (unsigned int)std::ceil((double)l_numThreadsX / (double)l_threadCountPerGroup);
	auto l_numThreadGroupsY = (unsigned int)std::ceil((double)l_numThreadsY / (double)l_threadCountPerGroup);
	auto l_numThreadGroupsZ = (unsigned int)std::ceil((double)l_numThreadsZ / (double)l_threadCountPerGroup);

	DispatchParamsGPUData l_brickLitWorkload;
	l_brickLitWorkload.numThreadGroups = TVec4<unsigned int>(l_numThreadGroupsX, l_numThreadGroupsY, l_numThreadGroupsZ, 0);
	l_brickLitWorkload.numThreads = TVec4<unsigned int>(l_numThreadsX, l_numThreadsY, l_numThreadsZ, 0);

	g_pModuleManager->getRenderingServer()->UploadGPUBufferDataComponent(l_dispatchParamsGBDC, &l_brickLitWorkload, 3, 1);

	g_pModuleManager->getRenderingServer()->CommandListBegin(m_brickRPDC, 0);
	g_pModuleManager->getRenderingServer()->BindRenderPassDataComponent(m_brickRPDC);
	g_pModuleManager->getRenderingServer()->CleanRenderTargets(m_brickRPDC);

	g_pModuleManager->getRenderingServer()->ActivateResourceBinder(m_brickRPDC, ShaderStage::Compute, l_dispatchParamsGBDC->m_ResourceBinder, 0, 8, Accessibility::ReadOnly, false, 0, l_dispatchParamsGBDC->m_TotalSize);
	g_pModuleManager->getRenderingServer()->ActivateResourceBinder(m_brickRPDC, ShaderStage::Compute, m_brickGBDC->m_ResourceBinder, 1, 0, Accessibility::ReadWrite, false, 0, m_brickGBDC->m_TotalSize);
	g_pModuleManager->getRenderingServer()->ActivateResourceBinder(m_brickRPDC, ShaderStage::Compute, m_surfelIrradianceGBDC->m_ResourceBinder, 2, 1, Accessibility::ReadWrite, false, 0, m_surfelIrradianceGBDC->m_TotalSize);
	g_pModuleManager->getRenderingServer()->ActivateResourceBinder(m_brickRPDC, ShaderStage::Compute, m_brickIrradianceGBDC->m_ResourceBinder, 3, 2, Accessibility::ReadWrite, false, 0, m_brickIrradianceGBDC->m_TotalSize);

	g_pModuleManager->getRenderingServer()->DispatchCompute(m_brickRPDC, l_numThreadGroupsX, l_numThreadGroupsY, l_numThreadGroupsZ);

	g_pModuleManager->getRenderingServer()->DeactivateResourceBinder(m_brickRPDC, ShaderStage::Compute, m_brickGBDC->m_ResourceBinder, 1, 0, Accessibility::ReadWrite, false, 0, m_brickGBDC->m_TotalSize);
	g_pModuleManager->getRenderingServer()->DeactivateResourceBinder(m_brickRPDC, ShaderStage::Compute, m_surfelIrradianceGBDC->m_ResourceBinder, 2, 1, Accessibility::ReadWrite, false, 0, m_surfelIrradianceGBDC->m_TotalSize);
	g_pModuleManager->getRenderingServer()->DeactivateResourceBinder(m_brickRPDC, ShaderStage::Compute, m_brickIrradianceGBDC->m_ResourceBinder, 3, 2, Accessibility::ReadWrite, false, 0, m_brickIrradianceGBDC->m_TotalSize);

	g_pModuleManager->getRenderingServer()->CommandListEnd(m_brickRPDC);

	return true;
}

bool GIResolvePass::litProbes()
{
	auto l_dispatchParamsGBDC = GetGPUBufferDataComponent(GPUBufferUsageType::Compute);

	auto l_threadCountPerGroup = 8;
	auto l_totalThreadGroupsCount = (double)m_probeGBDC->m_ElementCount / (l_threadCountPerGroup * l_threadCountPerGroup * l_threadCountPerGroup);
	auto l_averangeThreadGroupsCountPerSide = std::pow(l_totalThreadGroupsCount, 1.0 / 3.0);

	auto l_numThreadsX = (unsigned int)std::ceil(l_averangeThreadGroupsCountPerSide * l_threadCountPerGroup);
	auto l_numThreadsY = (unsigned int)std::ceil(l_averangeThreadGroupsCountPerSide * l_threadCountPerGroup);
	auto l_numThreadsZ = (unsigned int)std::ceil((double)m_probeGBDC->m_ElementCount / (l_numThreadsX * l_numThreadsY));

	auto l_numThreadGroupsX = (unsigned int)std::ceil((double)l_numThreadsX / (double)l_threadCountPerGroup);
	auto l_numThreadGroupsY = (unsigned int)std::ceil((double)l_numThreadsY / (double)l_threadCountPerGroup);
	auto l_numThreadGroupsZ = (unsigned int)std::ceil((double)l_numThreadsZ / (double)l_threadCountPerGroup);

	DispatchParamsGPUData l_probeLitWorkload;
	l_probeLitWorkload.numThreadGroups = TVec4<unsigned int>(l_numThreadGroupsX, l_numThreadGroupsY, l_numThreadGroupsZ, 0);
	l_probeLitWorkload.numThreads = TVec4<unsigned int>(l_numThreadsX, l_numThreadsY, l_numThreadsZ, 0);

	g_pModuleManager->getRenderingServer()->UploadGPUBufferDataComponent(l_dispatchParamsGBDC, &l_probeLitWorkload, 4, 1);

	g_pModuleManager->getRenderingServer()->CommandListBegin(m_probeRPDC, 0);
	g_pModuleManager->getRenderingServer()->BindRenderPassDataComponent(m_probeRPDC);
	g_pModuleManager->getRenderingServer()->CleanRenderTargets(m_probeRPDC);

	g_pModuleManager->getRenderingServer()->ActivateResourceBinder(m_probeRPDC, ShaderStage::Compute, l_dispatchParamsGBDC->m_ResourceBinder, 0, 8, Accessibility::ReadOnly, false, 0, l_dispatchParamsGBDC->m_TotalSize);
	g_pModuleManager->getRenderingServer()->ActivateResourceBinder(m_probeRPDC, ShaderStage::Compute, m_probeGBDC->m_ResourceBinder, 1, 0, Accessibility::ReadWrite, false, 0, m_probeGBDC->m_TotalSize);
	g_pModuleManager->getRenderingServer()->ActivateResourceBinder(m_probeRPDC, ShaderStage::Compute, m_brickFactorGBDC->m_ResourceBinder, 2, 1, Accessibility::ReadWrite, false, 0, m_brickFactorGBDC->m_TotalSize);
	g_pModuleManager->getRenderingServer()->ActivateResourceBinder(m_probeRPDC, ShaderStage::Compute, m_brickIrradianceGBDC->m_ResourceBinder, 3, 2, Accessibility::ReadWrite, false, 0, m_brickIrradianceGBDC->m_TotalSize);
	g_pModuleManager->getRenderingServer()->ActivateResourceBinder(m_probeRPDC, ShaderStage::Compute, m_irradianceVolume->m_ResourceBinder, 4, 3, Accessibility::ReadWrite);

	g_pModuleManager->getRenderingServer()->DispatchCompute(m_probeRPDC, l_numThreadGroupsX, l_numThreadGroupsY, l_numThreadGroupsZ);

	g_pModuleManager->getRenderingServer()->DeactivateResourceBinder(m_probeRPDC, ShaderStage::Compute, m_probeGBDC->m_ResourceBinder, 1, 0, Accessibility::ReadWrite, false, 0, m_probeGBDC->m_TotalSize);
	g_pModuleManager->getRenderingServer()->DeactivateResourceBinder(m_probeRPDC, ShaderStage::Compute, m_brickFactorGBDC->m_ResourceBinder, 2, 1, Accessibility::ReadWrite, false, 0, m_brickFactorGBDC->m_TotalSize);
	g_pModuleManager->getRenderingServer()->DeactivateResourceBinder(m_probeRPDC, ShaderStage::Compute, m_brickIrradianceGBDC->m_ResourceBinder, 3, 2, Accessibility::ReadWrite, false, 0, m_brickIrradianceGBDC->m_TotalSize);
	g_pModuleManager->getRenderingServer()->DeactivateResourceBinder(m_probeRPDC, ShaderStage::Compute, m_irradianceVolume->m_ResourceBinder, 4, 3, Accessibility::ReadWrite);

	g_pModuleManager->getRenderingServer()->CommandListEnd(m_probeRPDC);

	return true;
}

bool GIResolvePass::PrepareCommandList()
{
	if (l_GIDataLoaded)
	{
		litSurfels();
		litBricks();
		litProbes();
	}

	return true;
}

bool GIResolvePass::ExecuteCommandList()
{
	if (l_GIDataLoaded)
	{
		g_pModuleManager->getRenderingServer()->ExecuteCommandList(m_surfelRPDC);

		g_pModuleManager->getRenderingServer()->WaitForFrame(m_surfelRPDC);

		g_pModuleManager->getRenderingServer()->ExecuteCommandList(m_brickRPDC);

		g_pModuleManager->getRenderingServer()->WaitForFrame(m_brickRPDC);
	}

	return true;
}

bool GIResolvePass::Terminate()
{
	g_pModuleManager->getRenderingServer()->DeleteRenderPassDataComponent(m_surfelRPDC);
	g_pModuleManager->getRenderingServer()->DeleteRenderPassDataComponent(m_brickRPDC);

	return true;
}

RenderPassDataComponent * GIResolvePass::GetRPDC()
{
	return m_surfelRPDC;
}

ShaderProgramComponent * GIResolvePass::GetSPC()
{
	return m_surfelSPC;
}