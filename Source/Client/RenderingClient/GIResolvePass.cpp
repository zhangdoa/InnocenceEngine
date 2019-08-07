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

	RenderPassDataComponent* m_RPDC = 0;
	ShaderProgramComponent* m_SPC = 0;
	SamplerDataComponent* m_SDC = 0;

	GPUBufferDataComponent* m_surfelGBDC = 0;
	GPUBufferDataComponent* m_brickGBDC = 0;
	GPUBufferDataComponent* m_brickFactorGBDC = 0;
	GPUBufferDataComponent* m_probeGBDC = 0;

	std::function<void()> f_sceneLoadingFinishCallback;
	std::function<void()> f_sceneLoadingStartCallback;
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

		g_pModuleManager->getRenderingServer()->InitializeGPUBufferDataComponent(m_surfelGBDC);
	}

	return true;
}

bool GIResolvePass::deleteGPUBuffers()
{
	if (m_surfelGBDC)
	{
		g_pModuleManager->getRenderingServer()->DeleteGPUBufferDataComponent(m_surfelGBDC);
	}

	return true;
}

bool GIResolvePass::Setup()
{
	m_SPC = g_pModuleManager->getRenderingServer()->AddShaderProgramComponent("GIBrickResolvePass/");

	m_SPC->m_ShaderFilePaths.m_CSPath = "GIBrickResolvePass.comp/";

	g_pModuleManager->getRenderingServer()->InitializeShaderProgramComponent(m_SPC);

	m_RPDC = g_pModuleManager->getRenderingServer()->AddRenderPassDataComponent("GIBrickResolvePass/");

	auto l_RenderPassDesc = g_pModuleManager->getRenderingFrontend()->getDefaultRenderPassDesc();
	l_RenderPassDesc.m_RenderTargetCount = 0;
	l_RenderPassDesc.m_RenderPassUsageType = RenderPassUsageType::Compute;

	m_RPDC->m_RenderPassDesc = l_RenderPassDesc;

	m_RPDC->m_ResourceBinderLayoutDescs.resize(6);
	m_RPDC->m_ResourceBinderLayoutDescs[0].m_ResourceBinderType = ResourceBinderType::Buffer;
	m_RPDC->m_ResourceBinderLayoutDescs[0].m_GlobalSlot = 0;
	m_RPDC->m_ResourceBinderLayoutDescs[0].m_LocalSlot = 0;

	m_RPDC->m_ResourceBinderLayoutDescs[1].m_ResourceBinderType = ResourceBinderType::Buffer;
	m_RPDC->m_ResourceBinderLayoutDescs[1].m_GlobalSlot = 1;
	m_RPDC->m_ResourceBinderLayoutDescs[1].m_LocalSlot = 3;

	m_RPDC->m_ResourceBinderLayoutDescs[2].m_ResourceBinderType = ResourceBinderType::Buffer;
	m_RPDC->m_ResourceBinderLayoutDescs[2].m_BinderAccessibility = Accessibility::ReadWrite;
	m_RPDC->m_ResourceBinderLayoutDescs[2].m_ResourceAccessibility = Accessibility::ReadWrite;
	m_RPDC->m_ResourceBinderLayoutDescs[2].m_GlobalSlot = 2;
	m_RPDC->m_ResourceBinderLayoutDescs[2].m_LocalSlot = 0;

	m_RPDC->m_ResourceBinderLayoutDescs[3].m_ResourceBinderType = ResourceBinderType::Image;
	m_RPDC->m_ResourceBinderLayoutDescs[3].m_GlobalSlot = 3;
	m_RPDC->m_ResourceBinderLayoutDescs[3].m_LocalSlot = 0;
	m_RPDC->m_ResourceBinderLayoutDescs[3].m_IsRanged = true;

	m_RPDC->m_ResourceBinderLayoutDescs[4].m_ResourceBinderType = ResourceBinderType::Image;
	m_RPDC->m_ResourceBinderLayoutDescs[4].m_GlobalSlot = 4;
	m_RPDC->m_ResourceBinderLayoutDescs[4].m_LocalSlot = 1;
	m_RPDC->m_ResourceBinderLayoutDescs[4].m_IsRanged = true;

	m_RPDC->m_ResourceBinderLayoutDescs[5].m_ResourceBinderType = ResourceBinderType::Sampler;
	m_RPDC->m_ResourceBinderLayoutDescs[5].m_GlobalSlot = 5;
	m_RPDC->m_ResourceBinderLayoutDescs[5].m_LocalSlot = 0;
	m_RPDC->m_ResourceBinderLayoutDescs[5].m_IsRanged = true;

	m_RPDC->m_ShaderProgram = m_SPC;

	g_pModuleManager->getRenderingServer()->InitializeRenderPassDataComponent(m_RPDC);

	m_SDC = g_pModuleManager->getRenderingServer()->AddSamplerDataComponent("GIBrickResolvePass/");

	g_pModuleManager->getRenderingServer()->InitializeSamplerDataComponent(m_SDC);

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

bool GIResolvePass::PrepareCommandList()
{
	g_pModuleManager->getRenderingServer()->CommandListBegin(m_RPDC, 0);
	g_pModuleManager->getRenderingServer()->BindRenderPassDataComponent(m_RPDC);
	g_pModuleManager->getRenderingServer()->CleanRenderTargets(m_RPDC);

	g_pModuleManager->getRenderingServer()->CommandListEnd(m_RPDC);

	return true;
}

bool GIResolvePass::ExecuteCommandList()
{
	g_pModuleManager->getRenderingServer()->ExecuteCommandList(m_RPDC);

	g_pModuleManager->getRenderingServer()->WaitForFrame(m_RPDC);

	return true;
}

bool GIResolvePass::Terminate()
{
	g_pModuleManager->getRenderingServer()->DeleteRenderPassDataComponent(m_RPDC);

	return true;
}

RenderPassDataComponent * GIResolvePass::GetRPDC()
{
	return m_RPDC;
}

ShaderProgramComponent * GIResolvePass::GetSPC()
{
	return m_SPC;
}