#include "TransparentGeometryProcessPass.h"
#include "../../Engine/Services/RenderingConfigurationService.h"
#include "../../Engine/Services/RenderingContextService.h"

#include "OpaquePass.h"
#include "PreTAAPass.h"

#include "../../Engine/Engine.h"

using namespace Inno;

bool TransparentGeometryProcessPass::Setup(ISystemConfig *systemConfig)
{	
	auto l_renderingServer = g_Engine->getRenderingServer();
	
	auto l_RenderPassDesc = g_Engine->Get<RenderingConfigurationService>()->GetDefaultRenderPassDesc();

	m_atomicCounterGPUBufferComp = l_renderingServer->AddGPUBufferComponent("TransparentPassAtomicCounter/");
	m_atomicCounterGPUBufferComp->m_GPUAccessibility = Accessibility::ReadWrite;
	m_atomicCounterGPUBufferComp->m_ElementSize = sizeof(uint32_t);
	m_atomicCounterGPUBufferComp->m_ElementCount = 1;
	m_atomicCounterGPUBufferComp->m_Usage = GPUBufferUsage::AtomicCounter;

	uint32_t l_averangeFragmentPerPixel = 4;
	m_RT0 = l_renderingServer->AddGPUBufferComponent("TransparentGeometryProcessPassRT0/");
	m_RT0->m_GPUAccessibility = Accessibility::ReadWrite;
	m_RT0->m_ElementSize = sizeof(TVec4<uint32_t>);
	m_RT0->m_ElementCount = l_RenderPassDesc.m_RenderTargetDesc.Width * l_RenderPassDesc.m_RenderTargetDesc.Height * l_averangeFragmentPerPixel;

	m_RT1 = l_renderingServer->AddGPUBufferComponent("TransparentGeometryProcessPassRT1/");
	m_RT1->m_GPUAccessibility = Accessibility::ReadWrite;
	m_RT1->m_ElementSize = sizeof(TVec4<uint32_t>);
	m_RT1->m_ElementCount = l_RenderPassDesc.m_RenderTargetDesc.Width * l_RenderPassDesc.m_RenderTargetDesc.Height * l_averangeFragmentPerPixel;

	m_HeadPtr = l_renderingServer->AddTextureComponent("TransparentGeometryProcessPassHeadPtr/");
	m_HeadPtr->m_TextureDesc = l_RenderPassDesc.m_RenderTargetDesc;
	m_HeadPtr->m_TextureDesc.PixelDataFormat = TexturePixelDataFormat::R;
	m_HeadPtr->m_TextureDesc.PixelDataType = TexturePixelDataType::UInt32;
	auto l_cleanValue = 0xFFFFFFFF;
	std::memcpy(&m_HeadPtr->m_TextureDesc.ClearColor[0], &l_cleanValue, sizeof(l_cleanValue));

	m_ShaderProgramComp = l_renderingServer->AddShaderProgramComponent("TransparentGeometryProcessPass/");

	m_ShaderProgramComp->m_ShaderFilePaths.m_VSPath = "transparentGeometryProcessPass.vert/";
	m_ShaderProgramComp->m_ShaderFilePaths.m_PSPath = "transparentGeometryProcessPass.frag/";

	m_RenderPassComp = l_renderingServer->AddRenderPassComponent("TransparentGeometryProcessPass/");

	l_RenderPassDesc.m_RenderTargetCount = 0;
	l_RenderPassDesc.m_UseOutputMerger = false;
	l_RenderPassDesc.m_DepthStencilRenderTargetsInitializationFunc = std::bind(&TransparentGeometryProcessPass::DepthStencilRenderTargetsCreationFunc, this);
	l_RenderPassDesc.m_DepthStencilRenderTargetsCreationFunc = std::bind(&TransparentGeometryProcessPass::DepthStencilRenderTargetsReservationFunc, this);

	l_RenderPassDesc.m_GraphicsPipelineDesc.m_DepthStencilDesc.m_DepthEnable = true;
	l_RenderPassDesc.m_GraphicsPipelineDesc.m_DepthStencilDesc.m_AllowDepthWrite = false;
	l_RenderPassDesc.m_GraphicsPipelineDesc.m_DepthStencilDesc.m_DepthComparisionFunction = ComparisionFunction::LessEqual;

	l_RenderPassDesc.m_GraphicsPipelineDesc.m_RasterizerDesc.m_UseCulling = false;

	m_RenderPassComp->m_RenderPassDesc = l_RenderPassDesc;

	m_RenderPassComp->m_ResourceBindingLayoutDescs.resize(7);
	m_RenderPassComp->m_ResourceBindingLayoutDescs[0].m_GPUResourceType = GPUResourceType::Buffer;
	m_RenderPassComp->m_ResourceBindingLayoutDescs[0].m_DescriptorSetIndex = 0;
	m_RenderPassComp->m_ResourceBindingLayoutDescs[0].m_DescriptorIndex = 0;

	m_RenderPassComp->m_ResourceBindingLayoutDescs[1].m_GPUResourceType = GPUResourceType::Buffer;
	m_RenderPassComp->m_ResourceBindingLayoutDescs[1].m_DescriptorSetIndex = 0;
	m_RenderPassComp->m_ResourceBindingLayoutDescs[1].m_DescriptorIndex = 1;

	m_RenderPassComp->m_ResourceBindingLayoutDescs[2].m_GPUResourceType = GPUResourceType::Buffer;
	m_RenderPassComp->m_ResourceBindingLayoutDescs[2].m_DescriptorSetIndex = 0;
	m_RenderPassComp->m_ResourceBindingLayoutDescs[2].m_DescriptorIndex = 2;

	m_RenderPassComp->m_ResourceBindingLayoutDescs[3].m_GPUResourceType = GPUResourceType::Image;
	m_RenderPassComp->m_ResourceBindingLayoutDescs[3].m_DescriptorSetIndex = 1;
	m_RenderPassComp->m_ResourceBindingLayoutDescs[3].m_DescriptorIndex = 0;
	m_RenderPassComp->m_ResourceBindingLayoutDescs[3].m_BindingAccessibility = Accessibility::ReadWrite;
	m_RenderPassComp->m_ResourceBindingLayoutDescs[3].m_ResourceAccessibility = Accessibility::ReadWrite;
	m_RenderPassComp->m_ResourceBindingLayoutDescs[3].m_IndirectBinding = true;

	m_RenderPassComp->m_ResourceBindingLayoutDescs[4].m_GPUResourceType = GPUResourceType::Buffer;
	m_RenderPassComp->m_ResourceBindingLayoutDescs[4].m_DescriptorSetIndex = 1;
	m_RenderPassComp->m_ResourceBindingLayoutDescs[4].m_DescriptorIndex = 1;
	m_RenderPassComp->m_ResourceBindingLayoutDescs[4].m_BindingAccessibility = Accessibility::ReadWrite;
	m_RenderPassComp->m_ResourceBindingLayoutDescs[4].m_ResourceAccessibility = Accessibility::ReadWrite;

	m_RenderPassComp->m_ResourceBindingLayoutDescs[5].m_GPUResourceType = GPUResourceType::Buffer;
	m_RenderPassComp->m_ResourceBindingLayoutDescs[5].m_DescriptorSetIndex = 1;
	m_RenderPassComp->m_ResourceBindingLayoutDescs[5].m_DescriptorIndex = 2;
	m_RenderPassComp->m_ResourceBindingLayoutDescs[5].m_BindingAccessibility = Accessibility::ReadWrite;
	m_RenderPassComp->m_ResourceBindingLayoutDescs[5].m_ResourceAccessibility = Accessibility::ReadWrite;

	m_RenderPassComp->m_ResourceBindingLayoutDescs[6].m_GPUResourceType = GPUResourceType::Buffer;
	m_RenderPassComp->m_ResourceBindingLayoutDescs[6].m_DescriptorSetIndex = 1;
	// dxc would generate [[vk::binding(3, 1)]] to set = 1, binding = 6
	m_RenderPassComp->m_ResourceBindingLayoutDescs[6].m_DescriptorIndex = 3;
	m_RenderPassComp->m_ResourceBindingLayoutDescs[6].m_BindingAccessibility = Accessibility::ReadWrite;
	m_RenderPassComp->m_ResourceBindingLayoutDescs[6].m_ResourceAccessibility = Accessibility::ReadWrite;
	m_RenderPassComp->m_ResourceBindingLayoutDescs[6].m_IndirectBinding = true;

	m_RenderPassComp->m_ShaderProgram = m_ShaderProgramComp;

	m_ObjectStatus = ObjectStatus::Created;
	
	return true;
}

bool TransparentGeometryProcessPass::Initialize()
{	
	auto l_renderingServer = g_Engine->getRenderingServer();

	l_renderingServer->Initialize(m_atomicCounterGPUBufferComp);
	l_renderingServer->Initialize(m_RT0);
	l_renderingServer->Initialize(m_RT1);
	l_renderingServer->Initialize(m_HeadPtr);

	l_renderingServer->Initialize(m_ShaderProgramComp);
	l_renderingServer->Initialize(m_RenderPassComp);

	m_ObjectStatus = ObjectStatus::Activated;

	return true;
}

bool TransparentGeometryProcessPass::Terminate()
{
	auto l_renderingServer = g_Engine->getRenderingServer();

	l_renderingServer->Delete(m_RenderPassComp);

	m_ObjectStatus = ObjectStatus::Terminated;

	return true;
}

ObjectStatus TransparentGeometryProcessPass::GetStatus()
{
	return m_ObjectStatus;
}

bool TransparentGeometryProcessPass::PrepareCommandList(IRenderingContext* renderingContext)
{
	auto l_renderingServer = g_Engine->getRenderingServer();

	static uint32_t zero = 0;
	l_renderingServer->Upload(m_atomicCounterGPUBufferComp, &zero);
	l_renderingServer->Clear(m_RT0);
	l_renderingServer->Clear(m_RT1);
	l_renderingServer->Clear(m_HeadPtr);

	auto l_PerFrameCBufferGPUBufferComp = g_Engine->Get<RenderingContextService>()->GetGPUBufferComponent(GPUBufferUsageType::PerFrame);
	auto l_MeshGPUBufferComp = g_Engine->Get<RenderingContextService>()->GetGPUBufferComponent(GPUBufferUsageType::GPUModelData);
	auto l_MaterialGPUBufferComp = g_Engine->Get<RenderingContextService>()->GetGPUBufferComponent(GPUBufferUsageType::Material);

	// l_renderingServer->CommandListBegin(m_RenderPassComp, 0);
	// l_renderingServer->BindRenderPassComponent(m_RenderPassComp);
	// l_renderingServer->ClearRenderTargets(m_RenderPassComp);

	// l_renderingServer->BindGPUResource(m_RenderPassComp, ShaderStage::Vertex, l_PerFrameCBufferGPUBufferComp, 0);
	// l_renderingServer->BindGPUResource(m_RenderPassComp, ShaderStage::Pixel, l_PerFrameCBufferGPUBufferComp, 0);
	// l_renderingServer->BindGPUResource(m_RenderPassComp, ShaderStage::Pixel, m_HeadPtr, 3);
	// l_renderingServer->BindGPUResource(m_RenderPassComp, ShaderStage::Pixel, m_RT0, 4);
	// l_renderingServer->BindGPUResource(m_RenderPassComp, ShaderStage::Pixel, m_RT1, 5);
	// l_renderingServer->BindGPUResource(m_RenderPassComp, ShaderStage::Pixel, m_atomicCounterGPUBufferComp, 6);

	// auto& l_drawCallInfo = g_Engine->Get<RenderingContextService>()->GetDrawCallInfo();
	// auto l_drawCallCount = l_drawCallInfo.size();

	// for (uint32_t i = 0; i < l_drawCallCount; i++)
	// {
	// 	auto l_drawCallData = l_drawCallInfo[i];
	// 	auto l_visible = static_cast<uint32_t>(l_drawCallData.m_VisibilityMask & VisibilityMask::MainCamera);
	// 	//if (l_visible && l_drawCallData.material->m_ObjectStatus == ObjectStatus::Activated)
	// 	{
	// 		//if (l_drawCallData.material->m_ShaderModel == ShaderModel::Transparent)
	// 		{
	// 			if (l_drawCallData.mesh->m_ObjectStatus == ObjectStatus::Activated)
	// 			{
	// 				l_renderingServer->BindGPUResource(m_RenderPassComp, ShaderStage::Vertex, l_MeshGPUBufferComp, 1, l_drawCallData.m_PerObjectConstantBufferIndex, 1);
	// 				l_renderingServer->BindGPUResource(m_RenderPassComp, ShaderStage::Pixel, l_MaterialGPUBufferComp, 2, l_drawCallData.m_PerObjectConstantBufferIndex, 1);

	// 				l_renderingServer->DrawIndexedInstanced(m_RenderPassComp, l_drawCallData.mesh);
	// 			}
	// 		}
	// 	}
	// }

	// l_renderingServer->UnbindGPUResource(m_RenderPassComp, ShaderStage::Pixel, m_HeadPtr, 3);
	// l_renderingServer->UnbindGPUResource(m_RenderPassComp, ShaderStage::Pixel, m_RT0, 4);
	// l_renderingServer->UnbindGPUResource(m_RenderPassComp, ShaderStage::Pixel, m_RT1, 5);
	// l_renderingServer->UnbindGPUResource(m_RenderPassComp, ShaderStage::Pixel, m_atomicCounterGPUBufferComp, 6);

	// l_renderingServer->CommandListEnd(m_RenderPassComp);

	return true;
}

RenderPassComponent* TransparentGeometryProcessPass::GetRenderPassComp()
{
	return m_RenderPassComp;
}

GPUBufferComponent* TransparentGeometryProcessPass::GetResultChannel0()
{
	return m_RT0;
}

GPUBufferComponent* TransparentGeometryProcessPass::GetResultChannel1()
{
	return m_RT1;
}

TextureComponent* TransparentGeometryProcessPass::GetHeadPtrTexture()
{
	return m_HeadPtr;
}

bool Inno::TransparentGeometryProcessPass::DepthStencilRenderTargetsReservationFunc()
{
    return true;
}

bool TransparentGeometryProcessPass::DepthStencilRenderTargetsCreationFunc()
{
	auto l_outputMergerTarget = m_RenderPassComp->m_OutputMergerTarget;
	l_outputMergerTarget->m_DepthStencilOutput = OpaquePass::Get().GetRenderPassComp()->m_OutputMergerTarget->m_DepthStencilOutput;

    return true;
}
