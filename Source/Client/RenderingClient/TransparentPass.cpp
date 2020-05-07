#include "TransparentPass.h"
#include "../DefaultGPUBuffers/DefaultGPUBuffers.h"

#include "OpaquePass.h"
#include "PreTAAPass.h"

#include "../../Engine/Interface/IModuleManager.h"

INNO_ENGINE_API extern IModuleManager* g_pModuleManager;

using namespace DefaultGPUBuffers;

namespace TransparentPass
{
	bool setupGeometryProcessPass();
	bool setupBlendPass();

	bool geometryProcess();
	bool blend();

	RenderPassDataComponent* m_geometryProcessRPDC;
	ShaderProgramComponent* m_geometryProcessSPC;
	RenderPassDataComponent* m_blendRPDC;
	ShaderProgramComponent* m_blendSPC;

	GPUBufferDataComponent* m_atomicCounterGBDC;
	GPUBufferDataComponent* m_geometryProcessRT0GBDC;

	TextureDataComponent* m_blendPassRT0TDC;
}

bool TransparentPass::setupGeometryProcessPass()
{
	m_geometryProcessSPC = g_pModuleManager->getRenderingServer()->AddShaderProgramComponent("TransparentGeometryProcessPass/");

	m_geometryProcessSPC->m_ShaderFilePaths.m_VSPath = "transparentGeometryProcessPass.vert/";
	m_geometryProcessSPC->m_ShaderFilePaths.m_PSPath = "transparentGeometryProcessPass.frag/";

	m_geometryProcessRPDC = g_pModuleManager->getRenderingServer()->AddRenderPassDataComponent("TransparentGeometryProcessPass/");

	auto l_RenderPassDesc = g_pModuleManager->getRenderingFrontend()->getDefaultRenderPassDesc();

	l_RenderPassDesc.m_RenderTargetCount = 1;

	l_RenderPassDesc.m_RenderTargetDesc.PixelDataFormat = TexturePixelDataFormat::R;
	l_RenderPassDesc.m_RenderTargetDesc.PixelDataType = TexturePixelDataType::UInt32;

	auto l_cleanValue = 0xFFFFFFFF;
	std::memcpy(&l_RenderPassDesc.m_GraphicsPipelineDesc.CleanColor[0], &l_cleanValue, sizeof(l_cleanValue));

	l_RenderPassDesc.m_GraphicsPipelineDesc.m_DepthStencilDesc.m_DepthEnable = true;
	l_RenderPassDesc.m_GraphicsPipelineDesc.m_DepthStencilDesc.m_AllowDepthWrite = false;
	l_RenderPassDesc.m_GraphicsPipelineDesc.m_DepthStencilDesc.m_AllowDepthClamp = true;
	l_RenderPassDesc.m_GraphicsPipelineDesc.m_DepthStencilDesc.m_DepthComparisionFunction = ComparisionFunction::LessEqual;

	l_RenderPassDesc.m_GraphicsPipelineDesc.m_DepthStencilDesc.m_StencilEnable = true;
	l_RenderPassDesc.m_GraphicsPipelineDesc.m_DepthStencilDesc.m_AllowStencilWrite = false;
	l_RenderPassDesc.m_GraphicsPipelineDesc.m_DepthStencilDesc.m_FrontFaceStencilComparisionFunction = ComparisionFunction::Always;
	l_RenderPassDesc.m_GraphicsPipelineDesc.m_DepthStencilDesc.m_BackFaceStencilComparisionFunction = ComparisionFunction::Always;
	l_RenderPassDesc.m_UseOutputMerger = false;
	l_RenderPassDesc.m_GraphicsPipelineDesc.m_RasterizerDesc.m_UseCulling = false;

	m_geometryProcessRPDC->m_RenderPassDesc = l_RenderPassDesc;

	m_geometryProcessRPDC->m_ResourceBinderLayoutDescs.resize(7);
	m_geometryProcessRPDC->m_ResourceBinderLayoutDescs[0].m_ResourceBinderType = ResourceBinderType::Buffer;
	m_geometryProcessRPDC->m_ResourceBinderLayoutDescs[0].m_DescriptorSetIndex = 0;
	m_geometryProcessRPDC->m_ResourceBinderLayoutDescs[0].m_DescriptorIndex = 0;

	m_geometryProcessRPDC->m_ResourceBinderLayoutDescs[1].m_ResourceBinderType = ResourceBinderType::Buffer;
	m_geometryProcessRPDC->m_ResourceBinderLayoutDescs[1].m_DescriptorSetIndex = 0;
	m_geometryProcessRPDC->m_ResourceBinderLayoutDescs[1].m_DescriptorIndex = 1;

	m_geometryProcessRPDC->m_ResourceBinderLayoutDescs[2].m_ResourceBinderType = ResourceBinderType::Buffer;
	m_geometryProcessRPDC->m_ResourceBinderLayoutDescs[2].m_DescriptorSetIndex = 0;
	m_geometryProcessRPDC->m_ResourceBinderLayoutDescs[2].m_DescriptorIndex = 2;

	m_geometryProcessRPDC->m_ResourceBinderLayoutDescs[3].m_ResourceBinderType = ResourceBinderType::Buffer;
	m_geometryProcessRPDC->m_ResourceBinderLayoutDescs[3].m_DescriptorSetIndex = 1;
	m_geometryProcessRPDC->m_ResourceBinderLayoutDescs[3].m_DescriptorIndex = 0;
	m_geometryProcessRPDC->m_ResourceBinderLayoutDescs[3].m_BinderAccessibility = Accessibility::ReadWrite;
	m_geometryProcessRPDC->m_ResourceBinderLayoutDescs[3].m_ResourceAccessibility = Accessibility::ReadWrite;
	m_geometryProcessRPDC->m_ResourceBinderLayoutDescs[3].m_IndirectBinding = true;

	m_geometryProcessRPDC->m_ResourceBinderLayoutDescs[4].m_ResourceBinderType = ResourceBinderType::Image;
	m_geometryProcessRPDC->m_ResourceBinderLayoutDescs[4].m_DescriptorSetIndex = 1;
	m_geometryProcessRPDC->m_ResourceBinderLayoutDescs[4].m_DescriptorIndex = 1;
	m_geometryProcessRPDC->m_ResourceBinderLayoutDescs[4].m_BinderAccessibility = Accessibility::ReadWrite;
	m_geometryProcessRPDC->m_ResourceBinderLayoutDescs[4].m_ResourceAccessibility = Accessibility::ReadWrite;
	m_geometryProcessRPDC->m_ResourceBinderLayoutDescs[4].m_IndirectBinding = true;

	m_geometryProcessRPDC->m_ResourceBinderLayoutDescs[5].m_ResourceBinderType = ResourceBinderType::Buffer;
	m_geometryProcessRPDC->m_ResourceBinderLayoutDescs[5].m_DescriptorSetIndex = 1;
	m_geometryProcessRPDC->m_ResourceBinderLayoutDescs[5].m_DescriptorIndex = 2;
	m_geometryProcessRPDC->m_ResourceBinderLayoutDescs[5].m_BinderAccessibility = Accessibility::ReadWrite;
	m_geometryProcessRPDC->m_ResourceBinderLayoutDescs[5].m_ResourceAccessibility = Accessibility::ReadWrite;

	m_geometryProcessRPDC->m_ResourceBinderLayoutDescs[6].m_ResourceBinderType = ResourceBinderType::Buffer;
	m_geometryProcessRPDC->m_ResourceBinderLayoutDescs[6].m_DescriptorSetIndex = 1;
	m_geometryProcessRPDC->m_ResourceBinderLayoutDescs[6].m_DescriptorIndex = 3;
	m_geometryProcessRPDC->m_ResourceBinderLayoutDescs[6].m_BinderAccessibility = Accessibility::ReadWrite;
	m_geometryProcessRPDC->m_ResourceBinderLayoutDescs[6].m_ResourceAccessibility = Accessibility::ReadWrite;

	m_geometryProcessRPDC->m_ShaderProgram = m_geometryProcessSPC;

	return true;
}

bool TransparentPass::setupBlendPass()
{
	m_blendSPC = g_pModuleManager->getRenderingServer()->AddShaderProgramComponent("TransparentBlendPass/");

	m_blendSPC->m_ShaderFilePaths.m_CSPath = "transparentBlendPass.comp/";

	m_blendRPDC = g_pModuleManager->getRenderingServer()->AddRenderPassDataComponent("TransparentBlendPass/");

	auto l_RenderPassDesc = g_pModuleManager->getRenderingFrontend()->getDefaultRenderPassDesc();

	l_RenderPassDesc.m_RenderTargetCount = 0;
	l_RenderPassDesc.m_RenderPassUsage = RenderPassUsage::Compute;

	m_blendRPDC->m_RenderPassDesc = l_RenderPassDesc;

	m_blendRPDC->m_ResourceBinderLayoutDescs.resize(3);

	m_blendRPDC->m_ResourceBinderLayoutDescs[0].m_ResourceBinderType = ResourceBinderType::Image;
	m_blendRPDC->m_ResourceBinderLayoutDescs[0].m_DescriptorSetIndex = 0;
	m_blendRPDC->m_ResourceBinderLayoutDescs[0].m_DescriptorIndex = 0;
	m_blendRPDC->m_ResourceBinderLayoutDescs[0].m_BinderAccessibility = Accessibility::ReadWrite;
	m_blendRPDC->m_ResourceBinderLayoutDescs[0].m_ResourceAccessibility = Accessibility::ReadWrite;
	m_blendRPDC->m_ResourceBinderLayoutDescs[0].m_IndirectBinding = true;

	m_blendRPDC->m_ResourceBinderLayoutDescs[1].m_ResourceBinderType = ResourceBinderType::Buffer;
	m_blendRPDC->m_ResourceBinderLayoutDescs[1].m_DescriptorSetIndex = 0;
	m_blendRPDC->m_ResourceBinderLayoutDescs[1].m_DescriptorIndex = 1;
	m_blendRPDC->m_ResourceBinderLayoutDescs[1].m_BinderAccessibility = Accessibility::ReadWrite;
	m_blendRPDC->m_ResourceBinderLayoutDescs[1].m_ResourceAccessibility = Accessibility::ReadWrite;

	m_blendRPDC->m_ResourceBinderLayoutDescs[2].m_ResourceBinderType = ResourceBinderType::Image;
	m_blendRPDC->m_ResourceBinderLayoutDescs[2].m_DescriptorSetIndex = 0;
	m_blendRPDC->m_ResourceBinderLayoutDescs[2].m_DescriptorIndex = 2;
	m_blendRPDC->m_ResourceBinderLayoutDescs[2].m_BinderAccessibility = Accessibility::ReadWrite;
	m_blendRPDC->m_ResourceBinderLayoutDescs[2].m_ResourceAccessibility = Accessibility::ReadWrite;
	m_blendRPDC->m_ResourceBinderLayoutDescs[2].m_IndirectBinding = true;

	m_blendRPDC->m_ShaderProgram = m_blendSPC;

	return true;
}

bool TransparentPass::Setup()
{
	auto l_RenderPassDesc = g_pModuleManager->getRenderingFrontend()->getDefaultRenderPassDesc();

	m_atomicCounterGBDC = g_pModuleManager->getRenderingServer()->AddGPUBufferDataComponent("TransparentPassAtomicCounter/");
	m_atomicCounterGBDC->m_GPUAccessibility = Accessibility::ReadWrite;
	m_atomicCounterGBDC->m_ElementSize = sizeof(uint32_t);
	m_atomicCounterGBDC->m_ElementCount = 1;
	m_atomicCounterGBDC->m_isAtomicCounter = true;

	uint32_t l_averangeFragmentPerPixel = 2;
	m_geometryProcessRT0GBDC = g_pModuleManager->getRenderingServer()->AddGPUBufferDataComponent("TransparentGeometryProcessPassRT0/");
	m_geometryProcessRT0GBDC->m_GPUAccessibility = Accessibility::ReadWrite;
	m_geometryProcessRT0GBDC->m_ElementSize = sizeof(TVec4<uint32_t>);
	m_geometryProcessRT0GBDC->m_ElementCount = l_RenderPassDesc.m_RenderTargetDesc.Width * l_RenderPassDesc.m_RenderTargetDesc.Height * l_averangeFragmentPerPixel;

	m_blendPassRT0TDC = g_pModuleManager->getRenderingServer()->AddTextureDataComponent("TransparentBlendPassRT0/");
	m_blendPassRT0TDC->m_TextureDesc = l_RenderPassDesc.m_RenderTargetDesc;

	setupGeometryProcessPass();
	setupBlendPass();

	return true;
}

bool TransparentPass::Initialize()
{
	m_geometryProcessRPDC->m_DepthStencilRenderTarget = OpaquePass::GetRPDC()->m_DepthStencilRenderTarget;

	g_pModuleManager->getRenderingServer()->InitializeGPUBufferDataComponent(m_atomicCounterGBDC);
	g_pModuleManager->getRenderingServer()->InitializeGPUBufferDataComponent(m_geometryProcessRT0GBDC);
	g_pModuleManager->getRenderingServer()->InitializeTextureDataComponent(m_blendPassRT0TDC);

	g_pModuleManager->getRenderingServer()->InitializeShaderProgramComponent(m_geometryProcessSPC);
	g_pModuleManager->getRenderingServer()->InitializeRenderPassDataComponent(m_geometryProcessRPDC);
	g_pModuleManager->getRenderingServer()->InitializeShaderProgramComponent(m_blendSPC);
	g_pModuleManager->getRenderingServer()->InitializeRenderPassDataComponent(m_blendRPDC);

	return true;
}

bool TransparentPass::geometryProcess()
{
	static uint32_t zero = 0;
	g_pModuleManager->getRenderingServer()->UploadGPUBufferDataComponent(m_atomicCounterGBDC, &zero);
	g_pModuleManager->getRenderingServer()->ClearGPUBufferDataComponent(m_geometryProcessRT0GBDC);

	auto l_PerFrameCBufferGBDC = GetGPUBufferDataComponent(GPUBufferUsageType::PerFrame);
	auto l_MeshGBDC = GetGPUBufferDataComponent(GPUBufferUsageType::Mesh);
	auto l_MaterialGBDC = GetGPUBufferDataComponent(GPUBufferUsageType::Material);

	g_pModuleManager->getRenderingServer()->CommandListBegin(m_geometryProcessRPDC, 0);
	g_pModuleManager->getRenderingServer()->BindRenderPassDataComponent(m_geometryProcessRPDC);
	g_pModuleManager->getRenderingServer()->CleanRenderTargets(m_geometryProcessRPDC);
	//g_pModuleManager->getRenderingServer()->CopyColorBuffer(PreTAAPass::GetRPDC(), 0, m_RPDC, 0);

	g_pModuleManager->getRenderingServer()->ActivateResourceBinder(m_geometryProcessRPDC, ShaderStage::Vertex, l_PerFrameCBufferGBDC->m_ResourceBinder, 0, 0, Accessibility::ReadOnly);
	g_pModuleManager->getRenderingServer()->ActivateResourceBinder(m_geometryProcessRPDC, ShaderStage::Pixel, l_PerFrameCBufferGBDC->m_ResourceBinder, 0, 0, Accessibility::ReadOnly);
	g_pModuleManager->getRenderingServer()->ActivateResourceBinder(m_geometryProcessRPDC, ShaderStage::Pixel, m_atomicCounterGBDC->m_ResourceBinder, 3, 0, Accessibility::ReadWrite);
	g_pModuleManager->getRenderingServer()->ActivateResourceBinder(m_geometryProcessRPDC, ShaderStage::Pixel, m_geometryProcessRPDC->m_RenderTargetsResourceBinders[0], 4, 1, Accessibility::ReadWrite);
	g_pModuleManager->getRenderingServer()->ActivateResourceBinder(m_geometryProcessRPDC, ShaderStage::Pixel, m_geometryProcessRT0GBDC->m_ResourceBinder, 5, 2, Accessibility::ReadWrite);

	auto& l_drawCallInfo = g_pModuleManager->getRenderingFrontend()->getDrawCallInfo();
	auto l_drawCallCount = l_drawCallInfo.size();

	for (uint32_t i = 0; i < l_drawCallCount; i++)
	{
		auto l_drawCallData = l_drawCallInfo[i];
		if (l_drawCallData.material->m_ObjectStatus == ObjectStatus::Activated)
		{
			if (l_drawCallData.material->m_ShaderModel == ShaderModel::Transparent)
			{
				if (l_drawCallData.mesh->m_ObjectStatus == ObjectStatus::Activated)
				{
					g_pModuleManager->getRenderingServer()->ActivateResourceBinder(m_geometryProcessRPDC, ShaderStage::Vertex, l_MeshGBDC->m_ResourceBinder, 1, 1, Accessibility::ReadOnly, l_drawCallData.meshConstantBufferIndex, 1);
					g_pModuleManager->getRenderingServer()->ActivateResourceBinder(m_geometryProcessRPDC, ShaderStage::Pixel, l_MaterialGBDC->m_ResourceBinder, 2, 2, Accessibility::ReadOnly, l_drawCallData.materialConstantBufferIndex, 1);

					g_pModuleManager->getRenderingServer()->DispatchDrawCall(m_geometryProcessRPDC, l_drawCallData.mesh);
				}
			}
		}
	}

	g_pModuleManager->getRenderingServer()->DeactivateResourceBinder(m_geometryProcessRPDC, ShaderStage::Pixel, m_atomicCounterGBDC->m_ResourceBinder, 3, 0, Accessibility::ReadWrite);
	g_pModuleManager->getRenderingServer()->DeactivateResourceBinder(m_geometryProcessRPDC, ShaderStage::Pixel, m_geometryProcessRPDC->m_RenderTargetsResourceBinders[0], 4, 1, Accessibility::ReadWrite);
	g_pModuleManager->getRenderingServer()->DeactivateResourceBinder(m_geometryProcessRPDC, ShaderStage::Pixel, m_geometryProcessRT0GBDC->m_ResourceBinder, 5, 2, Accessibility::ReadWrite);

	g_pModuleManager->getRenderingServer()->CommandListEnd(m_geometryProcessRPDC);

	return true;
}

bool TransparentPass::blend()
{
	g_pModuleManager->getRenderingServer()->CommandListBegin(m_blendRPDC, 0);
	g_pModuleManager->getRenderingServer()->BindRenderPassDataComponent(m_blendRPDC);
	g_pModuleManager->getRenderingServer()->CleanRenderTargets(m_blendRPDC);

	g_pModuleManager->getRenderingServer()->ActivateResourceBinder(m_blendRPDC, ShaderStage::Compute, m_geometryProcessRPDC->m_RenderTargetsResourceBinders[0], 0, 0, Accessibility::ReadWrite);
	g_pModuleManager->getRenderingServer()->ActivateResourceBinder(m_blendRPDC, ShaderStage::Compute, m_geometryProcessRT0GBDC->m_ResourceBinder, 1, 1, Accessibility::ReadWrite);
	g_pModuleManager->getRenderingServer()->ActivateResourceBinder(m_blendRPDC, ShaderStage::Compute, m_blendPassRT0TDC->m_ResourceBinder, 2, 2, Accessibility::ReadWrite);

	g_pModuleManager->getRenderingServer()->DispatchCompute(m_blendRPDC, 160, 90, 1);

	g_pModuleManager->getRenderingServer()->DeactivateResourceBinder(m_blendRPDC, ShaderStage::Compute, m_geometryProcessRPDC->m_RenderTargetsResourceBinders[0], 0, 0, Accessibility::ReadWrite);
	g_pModuleManager->getRenderingServer()->DeactivateResourceBinder(m_blendRPDC, ShaderStage::Compute, m_geometryProcessRT0GBDC->m_ResourceBinder, 1, 1, Accessibility::ReadWrite);
	g_pModuleManager->getRenderingServer()->DeactivateResourceBinder(m_blendRPDC, ShaderStage::Compute, m_blendPassRT0TDC->m_ResourceBinder, 2, 2, Accessibility::ReadWrite);

	g_pModuleManager->getRenderingServer()->CommandListEnd(m_blendRPDC);

	return true;
}

bool TransparentPass::Render()
{
	geometryProcess();

	g_pModuleManager->getRenderingServer()->ExecuteCommandList(m_geometryProcessRPDC);
	g_pModuleManager->getRenderingServer()->WaitForFrame(m_geometryProcessRPDC);

	blend();

	g_pModuleManager->getRenderingServer()->ExecuteCommandList(m_blendRPDC);
	g_pModuleManager->getRenderingServer()->WaitForFrame(m_blendRPDC);

	return true;
}

bool TransparentPass::Terminate()
{
	g_pModuleManager->getRenderingServer()->DeleteRenderPassDataComponent(m_geometryProcessRPDC);

	return true;
}

RenderPassDataComponent* TransparentPass::GetRPDC()
{
	return m_geometryProcessRPDC;
}

ShaderProgramComponent* TransparentPass::GetSPC()
{
	return m_geometryProcessSPC;
}

IResourceBinder* TransparentPass::GetResult()
{
	return m_blendPassRT0TDC->m_ResourceBinder;
}