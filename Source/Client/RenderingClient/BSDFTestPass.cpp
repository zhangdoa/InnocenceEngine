#include "BSDFTestPass.h"
#include "../DefaultGPUBuffers/DefaultGPUBuffers.h"
#include "../../Engine/Services/RenderingConfigurationService.h"
#include "../../Engine/Services/RenderingContextService.h"
#include "../../Engine/Services/TemplateAssetService.h"

#include "BRDFLUTPass.h"
#include "BRDFLUTMSPass.h"

#include "../../Engine/Engine.h"

using namespace Inno;


using namespace DefaultGPUBuffers;

bool BSDFTestPass::Setup(ISystemConfig *systemConfig)
{
	auto l_renderingServer = g_Engine->getRenderingServer();

	m_ShaderProgramComp = l_renderingServer->AddShaderProgramComponent("BSDFTestPass/");

	m_ShaderProgramComp->m_ShaderFilePaths.m_VSPath = "opaqueGeometryProcessPass.vert/";
	m_ShaderProgramComp->m_ShaderFilePaths.m_PSPath = "BSDFTestPass.frag/";

	m_RenderPassComp = l_renderingServer->AddRenderPassComponent("BSDFTestPass/");

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

	m_RenderPassComp->m_ResourceBindingLayoutDescs[4].m_GPUResourceType = GPUResourceType::Image;
	m_RenderPassComp->m_ResourceBindingLayoutDescs[4].m_DescriptorSetIndex = 1;
	m_RenderPassComp->m_ResourceBindingLayoutDescs[4].m_DescriptorIndex = 1;
	m_RenderPassComp->m_ResourceBindingLayoutDescs[4].m_IndirectBinding = true;

	m_RenderPassComp->m_ResourceBindingLayoutDescs[5].m_GPUResourceType = GPUResourceType::Sampler;
	m_RenderPassComp->m_ResourceBindingLayoutDescs[5].m_DescriptorSetIndex = 2;
	m_RenderPassComp->m_ResourceBindingLayoutDescs[5].m_DescriptorIndex = 0;
	m_RenderPassComp->m_ResourceBindingLayoutDescs[5].m_IndirectBinding = true;

	m_RenderPassComp->m_ShaderProgram = m_ShaderProgramComp;

	m_RenderPassComp->m_OnResize = std::bind(&BSDFTestPass::InitializeResourceBindingLayoutDescs, this);

	m_SamplerComp = l_renderingServer->AddSamplerComponent("BSDFTestPass/");

	//
	auto l_RenderingCapability = g_Engine->Get<RenderingConfigurationService>()->GetRenderingCapability();

	m_meshConstantBuffer.resize(l_RenderingCapability.maxMeshes);
	m_materialConstantBuffer.resize(l_RenderingCapability.maxMaterials);

	size_t l_index = 0;

	auto l_interval = 4.0f;

	for (size_t i = 0; i < m_shpereCount; i++)
	{
		for (size_t j = 0; j < m_shpereCount; j++)
		{
			PerObjectConstantBuffer l_meshConstantBuffer;
			l_meshConstantBuffer.m = Math::toTranslationMatrix(Vec4((float)i * l_interval, 0.0f, (float)j * l_interval, 1.0f));
			l_meshConstantBuffer.normalMat = Math::generateIdentityMatrix<float>();

			m_meshConstantBuffer[l_index] = l_meshConstantBuffer;

			MaterialConstantBuffer l_materialConstantBuffer;

			l_materialConstantBuffer.materialAttributes.Metallic = (float)i / (float)m_shpereCount;
			l_materialConstantBuffer.materialAttributes.Roughness = (float)j / (float)m_shpereCount;

			m_materialConstantBuffer[l_index] = l_materialConstantBuffer;

			l_index++;
		}
	}

	m_ObjectStatus = ObjectStatus::Created;
	
	return true;
}

bool BSDFTestPass::Initialize()
{
	auto l_renderingServer = g_Engine->getRenderingServer();

	l_renderingServer->InitializeShaderProgramComponent(m_ShaderProgramComp);
	l_renderingServer->InitializeRenderPassComponent(m_RenderPassComp);
	l_renderingServer->InitializeSamplerComponent(m_SamplerComp);

    InitializeResourceBindingLayoutDescs();

	m_ObjectStatus = ObjectStatus::Activated;

	return true;
}

void BSDFTestPass::InitializeResourceBindingLayoutDescs()
{
    m_RenderPassComp->m_ResourceBindingLayoutDescs[0].m_GPUResource = GetGPUBufferComponent(GPUBufferUsageType::PerFrame);
    m_RenderPassComp->m_ResourceBindingLayoutDescs[0].m_ShaderStage = ShaderStage::Vertex | ShaderStage::Pixel;
    m_RenderPassComp->m_ResourceBindingLayoutDescs[3].m_GPUResource = BRDFLUTPass::Get().GetResult();
    m_RenderPassComp->m_ResourceBindingLayoutDescs[3].m_ShaderStage = ShaderStage::Pixel;
    m_RenderPassComp->m_ResourceBindingLayoutDescs[4].m_GPUResource = BRDFLUTMSPass::Get().GetResult();
    m_RenderPassComp->m_ResourceBindingLayoutDescs[4].m_ShaderStage = ShaderStage::Pixel;
    m_RenderPassComp->m_ResourceBindingLayoutDescs[5].m_GPUResource = m_SamplerComp;
    m_RenderPassComp->m_ResourceBindingLayoutDescs[5].m_ShaderStage = ShaderStage::Pixel;
}

bool BSDFTestPass::Terminate()
{
	auto l_renderingServer = g_Engine->getRenderingServer();

	l_renderingServer->DeleteRenderPassComponent(m_RenderPassComp);

	m_ObjectStatus = ObjectStatus::Terminated;

	return true;
}

ObjectStatus BSDFTestPass::GetStatus()
{
	return m_ObjectStatus;
}

bool BSDFTestPass::PrepareCommandList(IRenderingContext* renderingContext)
{
	auto l_renderingServer = g_Engine->getRenderingServer();
	
	auto l_PerFrameCBufferGPUBufferComp = GetGPUBufferComponent(GPUBufferUsageType::PerFrame);
	auto l_MeshGPUBufferComp = GetGPUBufferComponent(GPUBufferUsageType::Mesh);
	auto l_MaterialGPUBufferComp = GetGPUBufferComponent(GPUBufferUsageType::Material);

	l_renderingServer->UploadGPUBufferComponent(l_MeshGPUBufferComp, m_meshConstantBuffer);
	l_renderingServer->UploadGPUBufferComponent(l_MaterialGPUBufferComp, m_materialConstantBuffer);

	l_renderingServer->CommandListBegin(m_RenderPassComp, 0);
	l_renderingServer->BindRenderPassComponent(m_RenderPassComp);
	l_renderingServer->ClearRenderTargets(m_RenderPassComp);

	auto l_mesh = g_Engine->Get<TemplateAssetService>()->GetMeshComponent(MeshShape::Sphere);

	uint32_t l_offset = 0;

	for (size_t i = 0; i < m_shpereCount * m_shpereCount; i++)
	{
		l_renderingServer->BindGPUResource(m_RenderPassComp, ShaderStage::Vertex, l_MeshGPUBufferComp, 1, l_offset, 1);
		l_renderingServer->BindGPUResource(m_RenderPassComp, ShaderStage::Pixel, l_MaterialGPUBufferComp, 2, l_offset, 1);
		l_renderingServer->DrawIndexedInstanced(m_RenderPassComp, l_mesh);

		l_offset++;
	}

	l_renderingServer->CommandListEnd(m_RenderPassComp);

	return true;
}

RenderPassComponent* BSDFTestPass::GetRenderPassComp()
{
	return m_RenderPassComp;
}