#include "LuminanceAveragePass.h"
#include "../DefaultGPUBuffers/DefaultGPUBuffers.h"

#include "LuminanceHistogramPass.h"

#include "../../Engine/Interface/IEngine.h"

using namespace Inno;
extern INNO_ENGINE_API IEngine* g_Engine;

using namespace DefaultGPUBuffers;

bool LuminanceAveragePass::Setup(ISystemConfig *systemConfig)
{	
	auto l_RenderPassDesc = g_Engine->getRenderingFrontend()->GetDefaultRenderPassDesc();

	l_RenderPassDesc.m_RenderTargetCount = 0;
	l_RenderPassDesc.m_GPUEngineType = GPUEngineType::Compute;
	l_RenderPassDesc.m_Resizable = false;

	m_ShaderProgramComp = g_Engine->getRenderingServer()->AddShaderProgramComponent("LuminanceAveragePass/");

	m_ShaderProgramComp->m_ShaderFilePaths.m_CSPath = "luminanceAveragePass.comp/";

	m_RenderPassComp = g_Engine->getRenderingServer()->AddRenderPassComponent("LuminanceAveragePass/");

	m_RenderPassComp->m_RenderPassDesc = l_RenderPassDesc;

	m_RenderPassComp->m_ResourceBindingLayoutDescs.resize(3);
	m_RenderPassComp->m_ResourceBindingLayoutDescs[0].m_GPUResourceType = GPUResourceType::Buffer;
	m_RenderPassComp->m_ResourceBindingLayoutDescs[0].m_DescriptorSetIndex = 1;
	m_RenderPassComp->m_ResourceBindingLayoutDescs[0].m_DescriptorIndex = 0;
	m_RenderPassComp->m_ResourceBindingLayoutDescs[0].m_BindingAccessibility = Accessibility::ReadWrite;
	m_RenderPassComp->m_ResourceBindingLayoutDescs[0].m_ResourceAccessibility = Accessibility::ReadWrite;

	m_RenderPassComp->m_ResourceBindingLayoutDescs[1].m_GPUResourceType = GPUResourceType::Buffer;
	m_RenderPassComp->m_ResourceBindingLayoutDescs[1].m_DescriptorSetIndex = 1;
	m_RenderPassComp->m_ResourceBindingLayoutDescs[1].m_DescriptorIndex = 1;
	m_RenderPassComp->m_ResourceBindingLayoutDescs[1].m_BindingAccessibility = Accessibility::ReadWrite;
	m_RenderPassComp->m_ResourceBindingLayoutDescs[1].m_ResourceAccessibility = Accessibility::ReadWrite;

	m_RenderPassComp->m_ResourceBindingLayoutDescs[2].m_GPUResourceType = GPUResourceType::Buffer;
	m_RenderPassComp->m_ResourceBindingLayoutDescs[2].m_DescriptorSetIndex = 0;
	m_RenderPassComp->m_ResourceBindingLayoutDescs[2].m_DescriptorIndex = 0;

	m_RenderPassComp->m_ShaderProgram = m_ShaderProgramComp;

	m_luminanceAverage = g_Engine->getRenderingServer()->AddGPUBufferComponent("LuminanceAverageGPUBuffer/");
	m_luminanceAverage->m_CPUAccessibility = Accessibility::Immutable;
	m_luminanceAverage->m_GPUAccessibility = Accessibility::ReadWrite;
	m_luminanceAverage->m_ElementCount = 1;
	m_luminanceAverage->m_ElementSize = sizeof(float);

	m_ObjectStatus = ObjectStatus::Created;
	
	return true;
}

bool LuminanceAveragePass::Initialize()
{	
	g_Engine->getRenderingServer()->InitializeShaderProgramComponent(m_ShaderProgramComp);
	g_Engine->getRenderingServer()->InitializeRenderPassComponent(m_RenderPassComp);

	g_Engine->getRenderingServer()->InitializeGPUBufferComponent(m_luminanceAverage);

	m_ObjectStatus = ObjectStatus::Activated;

	return true;
}

bool LuminanceAveragePass::Terminate()
{
	g_Engine->getRenderingServer()->DeleteRenderPassComponent(m_RenderPassComp);

	m_ObjectStatus = ObjectStatus::Terminated;

	return true;
}

ObjectStatus LuminanceAveragePass::GetStatus()
{
	return m_ObjectStatus;
}

bool LuminanceAveragePass::PrepareCommandList(IRenderingContext* renderingContext)
{	
	auto l_PerFrameCBufferGPUBufferComp = GetGPUBufferComponent(GPUBufferUsageType::PerFrame);

	g_Engine->getRenderingServer()->CommandListBegin(m_RenderPassComp, 0);
	g_Engine->getRenderingServer()->BindRenderPassComponent(m_RenderPassComp);
	g_Engine->getRenderingServer()->CleanRenderTargets(m_RenderPassComp);

	g_Engine->getRenderingServer()->BindGPUResource(m_RenderPassComp, ShaderStage::Compute, LuminanceHistogramPass::Get().GetResult(), 0, Accessibility::ReadWrite);
	g_Engine->getRenderingServer()->BindGPUResource(m_RenderPassComp, ShaderStage::Compute, m_luminanceAverage, 1, Accessibility::ReadWrite);
	g_Engine->getRenderingServer()->BindGPUResource(m_RenderPassComp, ShaderStage::Compute, l_PerFrameCBufferGPUBufferComp, 2, Accessibility::ReadOnly);

	g_Engine->getRenderingServer()->Dispatch(m_RenderPassComp, 1, 1, 1);

	g_Engine->getRenderingServer()->UnbindGPUResource(m_RenderPassComp, ShaderStage::Compute, LuminanceHistogramPass::Get().GetResult(), 0, Accessibility::ReadWrite);
	g_Engine->getRenderingServer()->UnbindGPUResource(m_RenderPassComp, ShaderStage::Compute, m_luminanceAverage, 1, Accessibility::ReadWrite);

	g_Engine->getRenderingServer()->CommandListEnd(m_RenderPassComp);
	
	return true;
}

RenderPassComponent* LuminanceAveragePass::GetRenderPassComp()
{
	return m_RenderPassComp;
}

GPUResourceComponent* LuminanceAveragePass::GetResult()
{
	return m_luminanceAverage;
}