#include "TransparentGeometryProcessPass.h"

#include "OpaquePass.h"
#include "PreTAAPass.h"

#include "../DefaultGPUBuffers/DefaultGPUBuffers.h"

#include "../../Engine/Interface/IEngine.h"

using namespace Inno;
extern INNO_ENGINE_API IEngine* g_Engine;

using namespace DefaultGPUBuffers;

bool TransparentGeometryProcessPass::Setup(ISystemConfig *systemConfig)
{	
	auto l_RenderPassDesc = g_Engine->getRenderingFrontend()->getDefaultRenderPassDesc();

	m_atomicCounterGBDC = g_Engine->getRenderingServer()->AddGPUBufferDataComponent("TransparentPassAtomicCounter/");
	m_atomicCounterGBDC->m_GPUAccessibility = Accessibility::ReadWrite;
	m_atomicCounterGBDC->m_ElementSize = sizeof(uint32_t);
	m_atomicCounterGBDC->m_ElementCount = 1;
	m_atomicCounterGBDC->m_isAtomicCounter = true;

	uint32_t l_averangeFragmentPerPixel = 4;
	m_RT0 = g_Engine->getRenderingServer()->AddGPUBufferDataComponent("TransparentGeometryProcessPassRT0/");
	m_RT0->m_GPUAccessibility = Accessibility::ReadWrite;
	m_RT0->m_ElementSize = sizeof(TVec4<uint32_t>);
	m_RT0->m_ElementCount = l_RenderPassDesc.m_RenderTargetDesc.Width * l_RenderPassDesc.m_RenderTargetDesc.Height * l_averangeFragmentPerPixel;

	m_RT1 = g_Engine->getRenderingServer()->AddGPUBufferDataComponent("TransparentGeometryProcessPassRT1/");
	m_RT1->m_GPUAccessibility = Accessibility::ReadWrite;
	m_RT1->m_ElementSize = sizeof(TVec4<uint32_t>);
	m_RT1->m_ElementCount = l_RenderPassDesc.m_RenderTargetDesc.Width * l_RenderPassDesc.m_RenderTargetDesc.Height * l_averangeFragmentPerPixel;

	m_HeadPtr = g_Engine->getRenderingServer()->AddTextureDataComponent("TransparentGeometryProcessPassHeadPtr/");
	m_HeadPtr->m_TextureDesc = l_RenderPassDesc.m_RenderTargetDesc;
	m_HeadPtr->m_TextureDesc.PixelDataFormat = TexturePixelDataFormat::R;
	m_HeadPtr->m_TextureDesc.PixelDataType = TexturePixelDataType::UInt32;
	auto l_cleanValue = 0xFFFFFFFF;
	std::memcpy(&m_HeadPtr->m_TextureDesc.ClearColor[0], &l_cleanValue, sizeof(l_cleanValue));

	m_SPC = g_Engine->getRenderingServer()->AddShaderProgramComponent("TransparentGeometryProcessPass/");

	m_SPC->m_ShaderFilePaths.m_VSPath = "transparentGeometryProcessPass.vert/";
	m_SPC->m_ShaderFilePaths.m_PSPath = "transparentGeometryProcessPass.frag/";

	m_RPDC = g_Engine->getRenderingServer()->AddRenderPassDataComponent("TransparentGeometryProcessPass/");

	l_RenderPassDesc.m_RenderTargetCount = 0;
	l_RenderPassDesc.m_UseOutputMerger = false;

	l_RenderPassDesc.m_GraphicsPipelineDesc.m_DepthStencilDesc.m_DepthEnable = true;
	l_RenderPassDesc.m_GraphicsPipelineDesc.m_DepthStencilDesc.m_AllowDepthWrite = false;
	l_RenderPassDesc.m_GraphicsPipelineDesc.m_DepthStencilDesc.m_DepthComparisionFunction = ComparisionFunction::LessEqual;

	l_RenderPassDesc.m_GraphicsPipelineDesc.m_RasterizerDesc.m_UseCulling = false;

	m_RPDC->m_RenderPassDesc = l_RenderPassDesc;

	m_RPDC->m_ResourceBindingLayoutDescs.resize(7);
	m_RPDC->m_ResourceBindingLayoutDescs[0].m_GPUResourceType = GPUResourceType::Buffer;
	m_RPDC->m_ResourceBindingLayoutDescs[0].m_DescriptorSetIndex = 0;
	m_RPDC->m_ResourceBindingLayoutDescs[0].m_DescriptorIndex = 0;

	m_RPDC->m_ResourceBindingLayoutDescs[1].m_GPUResourceType = GPUResourceType::Buffer;
	m_RPDC->m_ResourceBindingLayoutDescs[1].m_DescriptorSetIndex = 0;
	m_RPDC->m_ResourceBindingLayoutDescs[1].m_DescriptorIndex = 1;

	m_RPDC->m_ResourceBindingLayoutDescs[2].m_GPUResourceType = GPUResourceType::Buffer;
	m_RPDC->m_ResourceBindingLayoutDescs[2].m_DescriptorSetIndex = 0;
	m_RPDC->m_ResourceBindingLayoutDescs[2].m_DescriptorIndex = 2;

	m_RPDC->m_ResourceBindingLayoutDescs[3].m_GPUResourceType = GPUResourceType::Image;
	m_RPDC->m_ResourceBindingLayoutDescs[3].m_DescriptorSetIndex = 1;
	m_RPDC->m_ResourceBindingLayoutDescs[3].m_DescriptorIndex = 0;
	m_RPDC->m_ResourceBindingLayoutDescs[3].m_BindingAccessibility = Accessibility::ReadWrite;
	m_RPDC->m_ResourceBindingLayoutDescs[3].m_ResourceAccessibility = Accessibility::ReadWrite;
	m_RPDC->m_ResourceBindingLayoutDescs[3].m_IndirectBinding = true;

	m_RPDC->m_ResourceBindingLayoutDescs[4].m_GPUResourceType = GPUResourceType::Buffer;
	m_RPDC->m_ResourceBindingLayoutDescs[4].m_DescriptorSetIndex = 1;
	m_RPDC->m_ResourceBindingLayoutDescs[4].m_DescriptorIndex = 1;
	m_RPDC->m_ResourceBindingLayoutDescs[4].m_BindingAccessibility = Accessibility::ReadWrite;
	m_RPDC->m_ResourceBindingLayoutDescs[4].m_ResourceAccessibility = Accessibility::ReadWrite;

	m_RPDC->m_ResourceBindingLayoutDescs[5].m_GPUResourceType = GPUResourceType::Buffer;
	m_RPDC->m_ResourceBindingLayoutDescs[5].m_DescriptorSetIndex = 1;
	m_RPDC->m_ResourceBindingLayoutDescs[5].m_DescriptorIndex = 2;
	m_RPDC->m_ResourceBindingLayoutDescs[5].m_BindingAccessibility = Accessibility::ReadWrite;
	m_RPDC->m_ResourceBindingLayoutDescs[5].m_ResourceAccessibility = Accessibility::ReadWrite;

	m_RPDC->m_ResourceBindingLayoutDescs[6].m_GPUResourceType = GPUResourceType::Buffer;
	m_RPDC->m_ResourceBindingLayoutDescs[6].m_DescriptorSetIndex = 1;
	// dxc would generate [[vk::binding(3, 1)]] to set = 1, binding = 6
	m_RPDC->m_ResourceBindingLayoutDescs[6].m_DescriptorIndex = 3;
	m_RPDC->m_ResourceBindingLayoutDescs[6].m_BindingAccessibility = Accessibility::ReadWrite;
	m_RPDC->m_ResourceBindingLayoutDescs[6].m_ResourceAccessibility = Accessibility::ReadWrite;
	m_RPDC->m_ResourceBindingLayoutDescs[6].m_IndirectBinding = true;

	m_RPDC->m_ShaderProgram = m_SPC;

	m_ObjectStatus = ObjectStatus::Created;
	
	return true;
}

bool TransparentGeometryProcessPass::Initialize()
{	
	m_RPDC->m_DepthStencilRenderTarget = OpaquePass::Get().GetRPDC()->m_DepthStencilRenderTarget;

	g_Engine->getRenderingServer()->InitializeGPUBufferDataComponent(m_atomicCounterGBDC);
	g_Engine->getRenderingServer()->InitializeGPUBufferDataComponent(m_RT0);
	g_Engine->getRenderingServer()->InitializeGPUBufferDataComponent(m_RT1);
	g_Engine->getRenderingServer()->InitializeTextureDataComponent(m_HeadPtr);

	g_Engine->getRenderingServer()->InitializeShaderProgramComponent(m_SPC);
	g_Engine->getRenderingServer()->InitializeRenderPassDataComponent(m_RPDC);

	m_ObjectStatus = ObjectStatus::Activated;

	return true;
}

bool TransparentGeometryProcessPass::Terminate()
{
	g_Engine->getRenderingServer()->DeleteRenderPassDataComponent(m_RPDC);

	m_ObjectStatus = ObjectStatus::Terminated;

	return true;
}

ObjectStatus TransparentGeometryProcessPass::GetStatus()
{
	return m_ObjectStatus;
}

bool TransparentGeometryProcessPass::PrepareCommandList(IRenderingContext* renderingContext)
{
	static uint32_t zero = 0;
	g_Engine->getRenderingServer()->UploadGPUBufferDataComponent(m_atomicCounterGBDC, &zero);
	g_Engine->getRenderingServer()->ClearGPUBufferDataComponent(m_RT0);
	g_Engine->getRenderingServer()->ClearGPUBufferDataComponent(m_RT1);
	g_Engine->getRenderingServer()->ClearTextureDataComponent(m_HeadPtr);

	auto l_PerFrameCBufferGBDC = GetGPUBufferDataComponent(GPUBufferUsageType::PerFrame);
	auto l_MeshGBDC = GetGPUBufferDataComponent(GPUBufferUsageType::Mesh);
	auto l_MaterialGBDC = GetGPUBufferDataComponent(GPUBufferUsageType::Material);

	g_Engine->getRenderingServer()->CommandListBegin(m_RPDC, 0);
	g_Engine->getRenderingServer()->BindRenderPassDataComponent(m_RPDC);
	g_Engine->getRenderingServer()->CleanRenderTargets(m_RPDC);

	g_Engine->getRenderingServer()->BindGPUResource(m_RPDC, ShaderStage::Vertex, l_PerFrameCBufferGBDC, 0, Accessibility::ReadOnly);
	g_Engine->getRenderingServer()->BindGPUResource(m_RPDC, ShaderStage::Pixel, l_PerFrameCBufferGBDC, 0, Accessibility::ReadOnly);
	g_Engine->getRenderingServer()->BindGPUResource(m_RPDC, ShaderStage::Pixel, m_HeadPtr, 3, Accessibility::ReadWrite);
	g_Engine->getRenderingServer()->BindGPUResource(m_RPDC, ShaderStage::Pixel, m_RT0, 4, Accessibility::ReadWrite);
	g_Engine->getRenderingServer()->BindGPUResource(m_RPDC, ShaderStage::Pixel, m_RT1, 5, Accessibility::ReadWrite);
	g_Engine->getRenderingServer()->BindGPUResource(m_RPDC, ShaderStage::Pixel, m_atomicCounterGBDC, 6, Accessibility::ReadWrite);

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
					g_Engine->getRenderingServer()->BindGPUResource(m_RPDC, ShaderStage::Vertex, l_MeshGBDC, 1, Accessibility::ReadOnly, l_drawCallData.meshConstantBufferIndex, 1);
					g_Engine->getRenderingServer()->BindGPUResource(m_RPDC, ShaderStage::Pixel, l_MaterialGBDC, 2, Accessibility::ReadOnly, l_drawCallData.materialConstantBufferIndex, 1);

					g_Engine->getRenderingServer()->DrawIndexedInstanced(m_RPDC, l_drawCallData.mesh);
				}
			}
		}
	}

	g_Engine->getRenderingServer()->UnbindGPUResource(m_RPDC, ShaderStage::Pixel, m_HeadPtr, 3, Accessibility::ReadWrite);
	g_Engine->getRenderingServer()->UnbindGPUResource(m_RPDC, ShaderStage::Pixel, m_RT0, 4, Accessibility::ReadWrite);
	g_Engine->getRenderingServer()->UnbindGPUResource(m_RPDC, ShaderStage::Pixel, m_RT1, 5, Accessibility::ReadWrite);
	g_Engine->getRenderingServer()->UnbindGPUResource(m_RPDC, ShaderStage::Pixel, m_atomicCounterGBDC, 6, Accessibility::ReadWrite);

	g_Engine->getRenderingServer()->CommandListEnd(m_RPDC);

	return true;
}

RenderPassDataComponent* TransparentGeometryProcessPass::GetRPDC()
{
	return m_RPDC;
}

GPUBufferDataComponent* TransparentGeometryProcessPass::GetResultChannel0()
{
	return m_RT0;
}

GPUBufferDataComponent* TransparentGeometryProcessPass::GetResultChannel1()
{
	return m_RT1;
}

TextureDataComponent* TransparentGeometryProcessPass::GetHeadPtrTexture()
{
	return m_HeadPtr;
}