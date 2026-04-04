#include "BSDFTestPass.h"

#include "../../Engine/Services/RenderingConfigurationService.h"
#include "../../Engine/Services/PerFrameDataService.h"
#include "../../Engine/Services/DrawCallService.h"
#include "../../Engine/Services/TemplateAssetService.h"

#include "BRDFLUTPass.h"
#include "BRDFLUTMSPass.h"

#include "../../Engine/Engine.h"
#include "../../Engine/Services/GraphicsResourceService.h"
#include "../../Engine/Services/GraphicsHardwareService.h"

using namespace Inno;




bool BSDFTestPass::Setup(IServiceConfig *systemConfig)
{
	auto l_rsService = g_Engine->Get<GraphicsResourceService>();
	auto l_hwService = g_Engine->Get<GraphicsHardwareService>();

	m_ShaderProgramComp = l_rsService->AddShaderProgramComponent("BSDFTestPass/");

	m_ShaderProgramComp->m_ShaderFilePaths.m_VSPath = "opaqueGeometryProcessPass.vert/";
	m_ShaderProgramComp->m_ShaderFilePaths.m_PSPath = "BSDFTestPass.frag/";

	m_RenderPassComp = l_rsService->AddRenderPassComponent("BSDFTestPass/");

	auto l_RenderPassDesc = g_Engine->Get<RenderingConfigurationService>()->GetDefaultRenderPassDesc();

	l_RenderPassDesc.m_RenderTargetCount = 1;
	l_RenderPassDesc.m_UseDepthBuffer = true;

	l_RenderPassDesc.m_GraphicsPipelineDesc.m_DepthStencilDesc.m_DepthEnable = true;
	l_RenderPassDesc.m_GraphicsPipelineDesc.m_DepthStencilDesc.m_AllowDepthWrite = true;
	l_RenderPassDesc.m_GraphicsPipelineDesc.m_DepthStencilDesc.m_DepthComparisionFunction = ComparisionFunction::LessEqual;

	l_RenderPassDesc.m_RenderTargetDesc.ClearColor[0] = 1.0f;
	l_RenderPassDesc.m_RenderTargetDesc.ClearColor[1] = 1.0f;
	l_RenderPassDesc.m_RenderTargetDesc.ClearColor[2] = 1.0f;
	l_RenderPassDesc.m_RenderTargetDesc.ClearColor[3] = 1.0f;

	l_RenderPassDesc.m_GraphicsPipelineDesc.m_RasterizerDesc.m_UseCulling = false;

	m_RenderPassComp->m_RenderPassDesc = l_RenderPassDesc;

	m_RenderPassComp->m_ResourceBindingLayoutDescs.resize(6);
	m_RenderPassComp->m_ResourceBindingLayoutDescs[0].m_GPUResourceType = GPUResourceType::Buffer;
	m_RenderPassComp->m_ResourceBindingLayoutDescs[0].m_DescriptorSetIndex = 0;
	m_RenderPassComp->m_ResourceBindingLayoutDescs[0].m_DescriptorIndex = 0;
    m_RenderPassComp->m_ResourceBindingLayoutDescs[0].m_ShaderStage = ShaderStage::Vertex | ShaderStage::Pixel;

	m_RenderPassComp->m_ResourceBindingLayoutDescs[1].m_GPUResourceType = GPUResourceType::Buffer;
	m_RenderPassComp->m_ResourceBindingLayoutDescs[1].m_DescriptorSetIndex = 0;
	m_RenderPassComp->m_ResourceBindingLayoutDescs[1].m_DescriptorIndex = 1;
	
	m_RenderPassComp->m_ResourceBindingLayoutDescs[2].m_GPUResourceType = GPUResourceType::Buffer;
	m_RenderPassComp->m_ResourceBindingLayoutDescs[2].m_DescriptorSetIndex = 0;
	m_RenderPassComp->m_ResourceBindingLayoutDescs[2].m_DescriptorIndex = 2;

	m_RenderPassComp->m_ResourceBindingLayoutDescs[3].m_GPUResourceType = GPUResourceType::Image;
	m_RenderPassComp->m_ResourceBindingLayoutDescs[3].m_DescriptorSetIndex = 1;
	m_RenderPassComp->m_ResourceBindingLayoutDescs[3].m_DescriptorIndex = 0;
	m_RenderPassComp->m_ResourceBindingLayoutDescs[3].m_IndirectBinding = true;
    m_RenderPassComp->m_ResourceBindingLayoutDescs[3].m_ShaderStage = ShaderStage::Pixel;

	m_RenderPassComp->m_ResourceBindingLayoutDescs[4].m_GPUResourceType = GPUResourceType::Image;
	m_RenderPassComp->m_ResourceBindingLayoutDescs[4].m_DescriptorSetIndex = 1;
	m_RenderPassComp->m_ResourceBindingLayoutDescs[4].m_DescriptorIndex = 1;
	m_RenderPassComp->m_ResourceBindingLayoutDescs[4].m_IndirectBinding = true;
    m_RenderPassComp->m_ResourceBindingLayoutDescs[4].m_ShaderStage = ShaderStage::Pixel;

	m_RenderPassComp->m_ResourceBindingLayoutDescs[5].m_GPUResourceType = GPUResourceType::Sampler;
	m_RenderPassComp->m_ResourceBindingLayoutDescs[5].m_DescriptorSetIndex = 2;
	m_RenderPassComp->m_ResourceBindingLayoutDescs[5].m_DescriptorIndex = 0;
	m_RenderPassComp->m_ResourceBindingLayoutDescs[5].m_IndirectBinding = true;
    m_RenderPassComp->m_ResourceBindingLayoutDescs[5].m_ShaderStage = ShaderStage::Pixel;

	m_RenderPassComp->m_ShaderProgram = m_ShaderProgramComp;

	m_CommandListComp_Graphics = l_rsService->AddCommandListComponent("BSDFTestPass/Graphics/");
	m_CommandListComp_Graphics->m_Type = GPUEngineType::Graphics;

	m_SamplerComp = l_rsService->AddSamplerComponent("BSDFTestPass/");

	//
	auto l_RenderingCapability = g_Engine->Get<RenderingConfigurationService>()->GetRenderingCapability();

	m_transformConstantBuffer.resize(l_RenderingCapability.maxMeshes);
	m_materialConstantBuffer.resize(l_RenderingCapability.maxMaterials);

	size_t l_index = 0;

	auto l_interval = 4.0f;

	for (size_t i = 0; i < m_shpereCount; i++)
	{
		for (size_t j = 0; j < m_shpereCount; j++)
		{
			TransformConstantBuffer l_transformCBuffer;
			l_transformCBuffer.m = Math::toTranslationMatrix(Vec4((float)i * l_interval, 0.0f, (float)j * l_interval, 1.0f));
			l_transformCBuffer.normalMat = Math::generateIdentityMatrix<float>();

			m_transformConstantBuffer[l_index] = l_transformCBuffer;

			MaterialConstantBuffer l_materialConstantBuffer;

			l_materialConstantBuffer.m_MaterialAttributes.Metallic = (float)i / (float)m_shpereCount;
			l_materialConstantBuffer.m_MaterialAttributes.Roughness = (float)j / (float)m_shpereCount;

			m_materialConstantBuffer[l_index] = l_materialConstantBuffer;

			l_index++;
		}
	}

	m_ObjectStatus = ObjectStatus::Created;
	
	return true;
}

bool BSDFTestPass::Initialize()
{
	auto l_rsService = g_Engine->Get<GraphicsResourceService>();
	auto l_hwService = g_Engine->Get<GraphicsHardwareService>();

	l_rsService->Initialize(m_ShaderProgramComp);
	l_rsService->Initialize(m_RenderPassComp);
	l_rsService->Initialize(m_CommandListComp_Graphics);
	l_rsService->Initialize(m_SamplerComp);

	m_ObjectStatus = ObjectStatus::Activated;

	return true;
}

bool BSDFTestPass::Terminate()
{
	auto l_rsService = g_Engine->Get<GraphicsResourceService>();
	auto l_hwService = g_Engine->Get<GraphicsHardwareService>();

	l_rsService->Delete(m_RenderPassComp);

	m_ObjectStatus = ObjectStatus::Terminated;

	return true;
}

ObjectStatus BSDFTestPass::GetStatus()
{
	return m_ObjectStatus;
}

bool BSDFTestPass::PrepareCommandList(IRenderingContext* renderingContext)
{
	auto l_rsService = g_Engine->Get<GraphicsResourceService>();
	auto l_hwService = g_Engine->Get<GraphicsHardwareService>();
	
	auto l_PerFrameCBufferGPUBufferComp = g_Engine->Get<PerFrameDataService>()->GetCurrentFrameBuffer();
	auto l_MeshGPUBufferComp = g_Engine->Get<DrawCallService>()->GetGPUModelDataBuffer();
	auto l_MaterialGPUBufferComp = g_Engine->Get<DrawCallService>()->GetMaterialBuffer();

	l_rsService->Upload(l_MeshGPUBufferComp, m_transformConstantBuffer);
	l_rsService->Upload(l_MaterialGPUBufferComp, m_materialConstantBuffer);

    // m_RenderPassComp->m_ResourceBindingLayoutDescs[0].m_GPUResource = g_Engine->Get<PerFrameDataService>()->GetCurrentFrameBuffer();

    // m_RenderPassComp->m_ResourceBindingLayoutDescs[3].m_GPUResource = BRDFLUTPass::Get().GetResult();

    // m_RenderPassComp->m_ResourceBindingLayoutDescs[4].m_GPUResource = BRDFLUTMSPass::Get().GetResult();
    // m_RenderPassComp->m_ResourceBindingLayoutDescs[5].m_GPUResource = m_SamplerComp;

	l_hwService->CommandListBegin(m_RenderPassComp, m_CommandListComp_Graphics, 0);
	l_hwService->BindRenderPassComponent(m_RenderPassComp, m_CommandListComp_Graphics);
	l_hwService->ClearRenderTargets(m_RenderPassComp, m_CommandListComp_Graphics);

	auto l_mesh = g_Engine->Get<TemplateAssetService>()->GetMeshComponent(MeshShape::Sphere);

	uint32_t l_offset = 0;

	for (size_t i = 0; i < m_shpereCount * m_shpereCount; i++)
	{
		l_hwService->BindGPUResource(m_RenderPassComp, m_CommandListComp_Graphics, ShaderStage::Vertex, l_MeshGPUBufferComp, 1, l_offset, 1);
		l_hwService->BindGPUResource(m_RenderPassComp, m_CommandListComp_Graphics, ShaderStage::Pixel, l_MaterialGPUBufferComp, 2, l_offset, 1);
		l_hwService->DrawIndexedInstanced(m_RenderPassComp, m_CommandListComp_Graphics, l_mesh);

		l_offset++;
	}

	l_hwService->CommandListEnd(m_RenderPassComp, m_CommandListComp_Graphics);

	return true;
}

RenderPassComponent* BSDFTestPass::GetRenderPassComp()
{
	return m_RenderPassComp;
}