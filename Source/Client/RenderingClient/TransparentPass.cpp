#include "TransparentPass.h"
#include "../DefaultGPUBuffers/DefaultGPUBuffers.h"

#include "OpaquePass.h"
#include "PreTAAPass.h"

#include "../../Engine/Interface/IEngine.h"

using namespace Inno;
extern INNO_ENGINE_API IEngine* g_Engine;

using namespace DefaultGPUBuffers;

namespace TransparentPass
{
	bool setupGeometryProcessPass();
	bool setupBlendPass();

	bool geometryProcess();
	bool blend(IResourceBinder* canvas);

	RenderPassDataComponent* m_geometryProcessRPDC;
	ShaderProgramComponent* m_geometryProcessSPC;
	RenderPassDataComponent* m_blendRPDC;
	ShaderProgramComponent* m_blendSPC;

	GPUBufferDataComponent* m_atomicCounterGBDC;
	GPUBufferDataComponent* m_geometryProcessRT0GBDC;
	GPUBufferDataComponent* m_geometryProcessRT1GBDC;

	TextureDataComponent* m_geometryProcessHeadPtrTDC;
	TextureDataComponent* m_blendPassRT0TDC;
}

bool TransparentPass::setupGeometryProcessPass()
{
	m_geometryProcessSPC = g_Engine->getRenderingServer()->AddShaderProgramComponent("TransparentGeometryProcessPass/");

	m_geometryProcessSPC->m_ShaderFilePaths.m_VSPath = "transparentGeometryProcessPass.vert/";
	m_geometryProcessSPC->m_ShaderFilePaths.m_PSPath = "transparentGeometryProcessPass.frag/";

	m_geometryProcessRPDC = g_Engine->getRenderingServer()->AddRenderPassDataComponent("TransparentGeometryProcessPass/");

	auto l_RenderPassDesc = g_Engine->getRenderingFrontend()->getDefaultRenderPassDesc();

	l_RenderPassDesc.m_RenderTargetCount = 0;
	l_RenderPassDesc.m_UseOutputMerger = false;

	l_RenderPassDesc.m_GraphicsPipelineDesc.m_DepthStencilDesc.m_DepthEnable = true;
	l_RenderPassDesc.m_GraphicsPipelineDesc.m_DepthStencilDesc.m_AllowDepthWrite = false;
	l_RenderPassDesc.m_GraphicsPipelineDesc.m_DepthStencilDesc.m_DepthComparisionFunction = ComparisionFunction::LessEqual;

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

	m_geometryProcessRPDC->m_ResourceBinderLayoutDescs[3].m_ResourceBinderType = ResourceBinderType::Image;
	m_geometryProcessRPDC->m_ResourceBinderLayoutDescs[3].m_DescriptorSetIndex = 1;
	m_geometryProcessRPDC->m_ResourceBinderLayoutDescs[3].m_DescriptorIndex = 0;
	m_geometryProcessRPDC->m_ResourceBinderLayoutDescs[3].m_BinderAccessibility = Accessibility::ReadWrite;
	m_geometryProcessRPDC->m_ResourceBinderLayoutDescs[3].m_ResourceAccessibility = Accessibility::ReadWrite;
	m_geometryProcessRPDC->m_ResourceBinderLayoutDescs[3].m_IndirectBinding = true;

	m_geometryProcessRPDC->m_ResourceBinderLayoutDescs[4].m_ResourceBinderType = ResourceBinderType::Buffer;
	m_geometryProcessRPDC->m_ResourceBinderLayoutDescs[4].m_DescriptorSetIndex = 1;
	m_geometryProcessRPDC->m_ResourceBinderLayoutDescs[4].m_DescriptorIndex = 1;
	m_geometryProcessRPDC->m_ResourceBinderLayoutDescs[4].m_BinderAccessibility = Accessibility::ReadWrite;
	m_geometryProcessRPDC->m_ResourceBinderLayoutDescs[4].m_ResourceAccessibility = Accessibility::ReadWrite;

	m_geometryProcessRPDC->m_ResourceBinderLayoutDescs[5].m_ResourceBinderType = ResourceBinderType::Buffer;
	m_geometryProcessRPDC->m_ResourceBinderLayoutDescs[5].m_DescriptorSetIndex = 1;
	m_geometryProcessRPDC->m_ResourceBinderLayoutDescs[5].m_DescriptorIndex = 2;
	m_geometryProcessRPDC->m_ResourceBinderLayoutDescs[5].m_BinderAccessibility = Accessibility::ReadWrite;
	m_geometryProcessRPDC->m_ResourceBinderLayoutDescs[5].m_ResourceAccessibility = Accessibility::ReadWrite;

	m_geometryProcessRPDC->m_ResourceBinderLayoutDescs[6].m_ResourceBinderType = ResourceBinderType::Buffer;
	m_geometryProcessRPDC->m_ResourceBinderLayoutDescs[6].m_DescriptorSetIndex = 1;
	// dxc would generate [[vk::binding(3, 1)]] to set = 1, binding = 6
	m_geometryProcessRPDC->m_ResourceBinderLayoutDescs[6].m_DescriptorIndex = 3;
	m_geometryProcessRPDC->m_ResourceBinderLayoutDescs[6].m_BinderAccessibility = Accessibility::ReadWrite;
	m_geometryProcessRPDC->m_ResourceBinderLayoutDescs[6].m_ResourceAccessibility = Accessibility::ReadWrite;
	m_geometryProcessRPDC->m_ResourceBinderLayoutDescs[6].m_IndirectBinding = true;

	m_geometryProcessRPDC->m_ShaderProgram = m_geometryProcessSPC;

	return true;
}

bool TransparentPass::setupBlendPass()
{
	m_blendSPC = g_Engine->getRenderingServer()->AddShaderProgramComponent("TransparentBlendPass/");

	m_blendSPC->m_ShaderFilePaths.m_CSPath = "transparentBlendPass.comp/";

	m_blendRPDC = g_Engine->getRenderingServer()->AddRenderPassDataComponent("TransparentBlendPass/");

	auto l_RenderPassDesc = g_Engine->getRenderingFrontend()->getDefaultRenderPassDesc();

	l_RenderPassDesc.m_RenderTargetCount = 0;
	l_RenderPassDesc.m_RenderPassUsage = RenderPassUsage::Compute;

	m_blendRPDC->m_RenderPassDesc = l_RenderPassDesc;

	m_blendRPDC->m_ResourceBinderLayoutDescs.resize(5);

	m_blendRPDC->m_ResourceBinderLayoutDescs[0].m_ResourceBinderType = ResourceBinderType::Image;
	m_blendRPDC->m_ResourceBinderLayoutDescs[0].m_DescriptorSetIndex = 1;
	m_blendRPDC->m_ResourceBinderLayoutDescs[0].m_DescriptorIndex = 0;
	m_blendRPDC->m_ResourceBinderLayoutDescs[0].m_BinderAccessibility = Accessibility::ReadWrite;
	m_blendRPDC->m_ResourceBinderLayoutDescs[0].m_ResourceAccessibility = Accessibility::ReadWrite;
	m_blendRPDC->m_ResourceBinderLayoutDescs[0].m_IndirectBinding = true;

	m_blendRPDC->m_ResourceBinderLayoutDescs[1].m_ResourceBinderType = ResourceBinderType::Buffer;
	m_blendRPDC->m_ResourceBinderLayoutDescs[1].m_DescriptorSetIndex = 1;
	m_blendRPDC->m_ResourceBinderLayoutDescs[1].m_DescriptorIndex = 1;
	m_blendRPDC->m_ResourceBinderLayoutDescs[1].m_BinderAccessibility = Accessibility::ReadWrite;
	m_blendRPDC->m_ResourceBinderLayoutDescs[1].m_ResourceAccessibility = Accessibility::ReadWrite;

	m_blendRPDC->m_ResourceBinderLayoutDescs[2].m_ResourceBinderType = ResourceBinderType::Buffer;
	m_blendRPDC->m_ResourceBinderLayoutDescs[2].m_DescriptorSetIndex = 1;
	m_blendRPDC->m_ResourceBinderLayoutDescs[2].m_DescriptorIndex = 2;
	m_blendRPDC->m_ResourceBinderLayoutDescs[2].m_BinderAccessibility = Accessibility::ReadWrite;
	m_blendRPDC->m_ResourceBinderLayoutDescs[2].m_ResourceAccessibility = Accessibility::ReadWrite;

	m_blendRPDC->m_ResourceBinderLayoutDescs[3].m_ResourceBinderType = ResourceBinderType::Image;
	m_blendRPDC->m_ResourceBinderLayoutDescs[3].m_DescriptorSetIndex = 1;
	m_blendRPDC->m_ResourceBinderLayoutDescs[3].m_DescriptorIndex = 3;
	m_blendRPDC->m_ResourceBinderLayoutDescs[3].m_BinderAccessibility = Accessibility::ReadWrite;
	m_blendRPDC->m_ResourceBinderLayoutDescs[3].m_ResourceAccessibility = Accessibility::ReadWrite;
	m_blendRPDC->m_ResourceBinderLayoutDescs[3].m_IndirectBinding = true;

	m_blendRPDC->m_ResourceBinderLayoutDescs[4].m_ResourceBinderType = ResourceBinderType::Buffer;
	m_blendRPDC->m_ResourceBinderLayoutDescs[4].m_DescriptorSetIndex = 0;
	m_blendRPDC->m_ResourceBinderLayoutDescs[4].m_DescriptorIndex = 0;

	m_blendRPDC->m_ShaderProgram = m_blendSPC;

	return true;
}

bool TransparentPass::Setup()
{
	auto l_RenderPassDesc = g_Engine->getRenderingFrontend()->getDefaultRenderPassDesc();

	m_atomicCounterGBDC = g_Engine->getRenderingServer()->AddGPUBufferDataComponent("TransparentPassAtomicCounter/");
	m_atomicCounterGBDC->m_GPUAccessibility = Accessibility::ReadWrite;
	m_atomicCounterGBDC->m_ElementSize = sizeof(uint32_t);
	m_atomicCounterGBDC->m_ElementCount = 1;
	m_atomicCounterGBDC->m_isAtomicCounter = true;

	uint32_t l_averangeFragmentPerPixel = 4;
	m_geometryProcessRT0GBDC = g_Engine->getRenderingServer()->AddGPUBufferDataComponent("TransparentGeometryProcessPassRT0/");
	m_geometryProcessRT0GBDC->m_GPUAccessibility = Accessibility::ReadWrite;
	m_geometryProcessRT0GBDC->m_ElementSize = sizeof(TVec4<uint32_t>);
	m_geometryProcessRT0GBDC->m_ElementCount = l_RenderPassDesc.m_RenderTargetDesc.Width * l_RenderPassDesc.m_RenderTargetDesc.Height * l_averangeFragmentPerPixel;

	m_geometryProcessRT1GBDC = g_Engine->getRenderingServer()->AddGPUBufferDataComponent("TransparentGeometryProcessPassRT1/");
	m_geometryProcessRT1GBDC->m_GPUAccessibility = Accessibility::ReadWrite;
	m_geometryProcessRT1GBDC->m_ElementSize = sizeof(TVec4<uint32_t>);
	m_geometryProcessRT1GBDC->m_ElementCount = l_RenderPassDesc.m_RenderTargetDesc.Width * l_RenderPassDesc.m_RenderTargetDesc.Height * l_averangeFragmentPerPixel;

	m_geometryProcessHeadPtrTDC = g_Engine->getRenderingServer()->AddTextureDataComponent("TransparentGeometryProcessPassHeadPtr/");
	m_geometryProcessHeadPtrTDC->m_TextureDesc = l_RenderPassDesc.m_RenderTargetDesc;
	m_geometryProcessHeadPtrTDC->m_TextureDesc.PixelDataFormat = TexturePixelDataFormat::R;
	m_geometryProcessHeadPtrTDC->m_TextureDesc.PixelDataType = TexturePixelDataType::UInt32;
	auto l_cleanValue = 0xFFFFFFFF;
	std::memcpy(&m_geometryProcessHeadPtrTDC->m_TextureDesc.ClearColor[0], &l_cleanValue, sizeof(l_cleanValue));

	m_blendPassRT0TDC = g_Engine->getRenderingServer()->AddTextureDataComponent("TransparentBlendPassRT0/");
	m_blendPassRT0TDC->m_TextureDesc = l_RenderPassDesc.m_RenderTargetDesc;

	setupGeometryProcessPass();
	setupBlendPass();

	return true;
}

bool TransparentPass::Initialize()
{
	m_geometryProcessRPDC->m_DepthStencilRenderTarget = OpaquePass::GetRPDC()->m_DepthStencilRenderTarget;

	g_Engine->getRenderingServer()->InitializeGPUBufferDataComponent(m_atomicCounterGBDC);
	g_Engine->getRenderingServer()->InitializeGPUBufferDataComponent(m_geometryProcessRT0GBDC);
	g_Engine->getRenderingServer()->InitializeGPUBufferDataComponent(m_geometryProcessRT1GBDC);
	g_Engine->getRenderingServer()->InitializeTextureDataComponent(m_geometryProcessHeadPtrTDC);
	g_Engine->getRenderingServer()->InitializeTextureDataComponent(m_blendPassRT0TDC);

	g_Engine->getRenderingServer()->InitializeShaderProgramComponent(m_geometryProcessSPC);
	g_Engine->getRenderingServer()->InitializeRenderPassDataComponent(m_geometryProcessRPDC);
	g_Engine->getRenderingServer()->InitializeShaderProgramComponent(m_blendSPC);
	g_Engine->getRenderingServer()->InitializeRenderPassDataComponent(m_blendRPDC);

	return true;
}

bool TransparentPass::geometryProcess()
{
	static uint32_t zero = 0;
	g_Engine->getRenderingServer()->UploadGPUBufferDataComponent(m_atomicCounterGBDC, &zero);
	g_Engine->getRenderingServer()->ClearGPUBufferDataComponent(m_geometryProcessRT0GBDC);
	g_Engine->getRenderingServer()->ClearGPUBufferDataComponent(m_geometryProcessRT1GBDC);
	g_Engine->getRenderingServer()->ClearTextureDataComponent(m_geometryProcessHeadPtrTDC);

	auto l_PerFrameCBufferGBDC = GetGPUBufferDataComponent(GPUBufferUsageType::PerFrame);
	auto l_MeshGBDC = GetGPUBufferDataComponent(GPUBufferUsageType::Mesh);
	auto l_MaterialGBDC = GetGPUBufferDataComponent(GPUBufferUsageType::Material);

	g_Engine->getRenderingServer()->CommandListBegin(m_geometryProcessRPDC, 0);
	g_Engine->getRenderingServer()->BindRenderPassDataComponent(m_geometryProcessRPDC);
	g_Engine->getRenderingServer()->CleanRenderTargets(m_geometryProcessRPDC);

	g_Engine->getRenderingServer()->ActivateResourceBinder(m_geometryProcessRPDC, ShaderStage::Vertex, l_PerFrameCBufferGBDC->m_ResourceBinder, 0, 0, Accessibility::ReadOnly);
	g_Engine->getRenderingServer()->ActivateResourceBinder(m_geometryProcessRPDC, ShaderStage::Pixel, l_PerFrameCBufferGBDC->m_ResourceBinder, 0, 0, Accessibility::ReadOnly);
	g_Engine->getRenderingServer()->ActivateResourceBinder(m_geometryProcessRPDC, ShaderStage::Pixel, m_geometryProcessHeadPtrTDC->m_ResourceBinder, 3, 0, Accessibility::ReadWrite);
	g_Engine->getRenderingServer()->ActivateResourceBinder(m_geometryProcessRPDC, ShaderStage::Pixel, m_geometryProcessRT0GBDC->m_ResourceBinder, 4, 1, Accessibility::ReadWrite);
	g_Engine->getRenderingServer()->ActivateResourceBinder(m_geometryProcessRPDC, ShaderStage::Pixel, m_geometryProcessRT1GBDC->m_ResourceBinder, 5, 2, Accessibility::ReadWrite);
	g_Engine->getRenderingServer()->ActivateResourceBinder(m_geometryProcessRPDC, ShaderStage::Pixel, m_atomicCounterGBDC->m_ResourceBinder, 6, 3, Accessibility::ReadWrite);

	auto& l_drawCallInfo = g_Engine->getRenderingFrontend()->getDrawCallInfo();
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
					g_Engine->getRenderingServer()->ActivateResourceBinder(m_geometryProcessRPDC, ShaderStage::Vertex, l_MeshGBDC->m_ResourceBinder, 1, 1, Accessibility::ReadOnly, l_drawCallData.meshConstantBufferIndex, 1);
					g_Engine->getRenderingServer()->ActivateResourceBinder(m_geometryProcessRPDC, ShaderStage::Pixel, l_MaterialGBDC->m_ResourceBinder, 2, 2, Accessibility::ReadOnly, l_drawCallData.materialConstantBufferIndex, 1);

					g_Engine->getRenderingServer()->DispatchDrawCall(m_geometryProcessRPDC, l_drawCallData.mesh);
				}
			}
		}
	}

	g_Engine->getRenderingServer()->DeactivateResourceBinder(m_geometryProcessRPDC, ShaderStage::Pixel, m_geometryProcessHeadPtrTDC->m_ResourceBinder, 3, 0, Accessibility::ReadWrite);
	g_Engine->getRenderingServer()->DeactivateResourceBinder(m_geometryProcessRPDC, ShaderStage::Pixel, m_geometryProcessRT0GBDC->m_ResourceBinder, 4, 1, Accessibility::ReadWrite);
	g_Engine->getRenderingServer()->DeactivateResourceBinder(m_geometryProcessRPDC, ShaderStage::Pixel, m_geometryProcessRT1GBDC->m_ResourceBinder, 5, 2, Accessibility::ReadWrite);
	g_Engine->getRenderingServer()->DeactivateResourceBinder(m_geometryProcessRPDC, ShaderStage::Pixel, m_atomicCounterGBDC->m_ResourceBinder, 6, 3, Accessibility::ReadWrite);

	g_Engine->getRenderingServer()->CommandListEnd(m_geometryProcessRPDC);

	return true;
}

bool TransparentPass::blend(IResourceBinder* canvas)
{
	auto l_PerFrameCBufferGBDC = GetGPUBufferDataComponent(GPUBufferUsageType::PerFrame);

	g_Engine->getRenderingServer()->CommandListBegin(m_blendRPDC, 0);
	g_Engine->getRenderingServer()->BindRenderPassDataComponent(m_blendRPDC);
	g_Engine->getRenderingServer()->CleanRenderTargets(m_blendRPDC);

	g_Engine->getRenderingServer()->ActivateResourceBinder(m_blendRPDC, ShaderStage::Compute, m_geometryProcessHeadPtrTDC->m_ResourceBinder, 0, 0, Accessibility::ReadWrite);
	g_Engine->getRenderingServer()->ActivateResourceBinder(m_blendRPDC, ShaderStage::Compute, m_geometryProcessRT0GBDC->m_ResourceBinder, 1, 1, Accessibility::ReadWrite);
	g_Engine->getRenderingServer()->ActivateResourceBinder(m_blendRPDC, ShaderStage::Compute, m_geometryProcessRT1GBDC->m_ResourceBinder, 2, 2, Accessibility::ReadWrite);
	g_Engine->getRenderingServer()->ActivateResourceBinder(m_blendRPDC, ShaderStage::Compute, canvas, 3, 3, Accessibility::ReadWrite);

	g_Engine->getRenderingServer()->ActivateResourceBinder(m_blendRPDC, ShaderStage::Compute, l_PerFrameCBufferGBDC->m_ResourceBinder, 4, 0, Accessibility::ReadOnly);

	g_Engine->getRenderingServer()->DispatchCompute(m_blendRPDC, 160, 90, 1);

	g_Engine->getRenderingServer()->DeactivateResourceBinder(m_blendRPDC, ShaderStage::Compute, m_geometryProcessHeadPtrTDC->m_ResourceBinder, 0, 0, Accessibility::ReadWrite);
	g_Engine->getRenderingServer()->DeactivateResourceBinder(m_blendRPDC, ShaderStage::Compute, m_geometryProcessRT0GBDC->m_ResourceBinder, 1, 1, Accessibility::ReadWrite);
	g_Engine->getRenderingServer()->DeactivateResourceBinder(m_blendRPDC, ShaderStage::Compute, m_geometryProcessRT0GBDC->m_ResourceBinder, 2, 2, Accessibility::ReadWrite);
	g_Engine->getRenderingServer()->DeactivateResourceBinder(m_blendRPDC, ShaderStage::Compute, canvas, 3, 3, Accessibility::ReadWrite);

	g_Engine->getRenderingServer()->CommandListEnd(m_blendRPDC);

	return true;
}

bool TransparentPass::Render(IResourceBinder* canvas)
{
	geometryProcess();

	g_Engine->getRenderingServer()->ExecuteCommandList(m_geometryProcessRPDC);
	g_Engine->getRenderingServer()->WaitForFrame(m_geometryProcessRPDC);

	if (canvas != nullptr)
	{
		blend(canvas);
	}
	else
	{
		g_Engine->getRenderingServer()->ClearTextureDataComponent(m_blendPassRT0TDC);
		blend(m_blendPassRT0TDC->m_ResourceBinder);
	}

	g_Engine->getRenderingServer()->ExecuteCommandList(m_blendRPDC);
	g_Engine->getRenderingServer()->WaitForFrame(m_blendRPDC);

	return true;
}

bool TransparentPass::Terminate()
{
	g_Engine->getRenderingServer()->DeleteRenderPassDataComponent(m_geometryProcessRPDC);

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