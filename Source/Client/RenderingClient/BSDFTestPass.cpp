#include "BSDFTestPass.h"
#include "../DefaultGPUBuffers/DefaultGPUBuffers.h"

#include "BRDFLUTPass.h"
#include "BRDFLUTMSPass.h"

#include "../../Engine/Interface/IEngine.h"

using namespace Inno;
extern INNO_ENGINE_API IEngine *g_Engine;

using namespace DefaultGPUBuffers;

bool BSDFTestPass::Setup(ISystemConfig *systemConfig)
{
	m_ShaderProgramComp = g_Engine->getRenderingServer()->AddShaderProgramComponent("BSDFTestPass/");

	m_ShaderProgramComp->m_ShaderFilePaths.m_VSPath = "opaqueGeometryProcessPass.vert/";
	m_ShaderProgramComp->m_ShaderFilePaths.m_PSPath = "BSDFTestPass.frag/";

	m_RenderPassComp = g_Engine->getRenderingServer()->AddRenderPassComponent("BSDFTestPass/");

	auto l_RenderPassDesc = g_Engine->getRenderingFrontend()->GetDefaultRenderPassDesc();

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

	m_SamplerComp = g_Engine->getRenderingServer()->AddSamplerComponent("BSDFTestPass/");

	//
	auto l_RenderingCapability = g_Engine->getRenderingFrontend()->GetRenderingCapability();

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
	g_Engine->getRenderingServer()->InitializeShaderProgramComponent(m_ShaderProgramComp);
	g_Engine->getRenderingServer()->InitializeRenderPassComponent(m_RenderPassComp);
	g_Engine->getRenderingServer()->InitializeSamplerComponent(m_SamplerComp);

	m_ObjectStatus = ObjectStatus::Activated;

	return true;
}

bool BSDFTestPass::Terminate()
{
	g_Engine->getRenderingServer()->DeleteRenderPassComponent(m_RenderPassComp);

	m_ObjectStatus = ObjectStatus::Terminated;

	return true;
}

ObjectStatus BSDFTestPass::GetStatus()
{
	return m_ObjectStatus;
}

bool BSDFTestPass::PrepareCommandList(IRenderingContext* renderingContext)
{
	auto l_PerFrameCBufferGPUBufferComp = GetGPUBufferComponent(GPUBufferUsageType::PerFrame);
	auto l_MeshGPUBufferComp = GetGPUBufferComponent(GPUBufferUsageType::Mesh);
	auto l_MaterialGPUBufferComp = GetGPUBufferComponent(GPUBufferUsageType::Material);

	g_Engine->getRenderingServer()->UploadGPUBufferComponent(l_MeshGPUBufferComp, m_meshConstantBuffer);
	g_Engine->getRenderingServer()->UploadGPUBufferComponent(l_MaterialGPUBufferComp, m_materialConstantBuffer);

	g_Engine->getRenderingServer()->CommandListBegin(m_RenderPassComp, 0);
	g_Engine->getRenderingServer()->BindRenderPassComponent(m_RenderPassComp);
	g_Engine->getRenderingServer()->CleanRenderTargets(m_RenderPassComp);
	g_Engine->getRenderingServer()->BindGPUResource(m_RenderPassComp, ShaderStage::Pixel, m_SamplerComp, 5);

	g_Engine->getRenderingServer()->BindGPUResource(m_RenderPassComp, ShaderStage::Vertex, l_PerFrameCBufferGPUBufferComp, 0, Accessibility::ReadOnly);
	g_Engine->getRenderingServer()->BindGPUResource(m_RenderPassComp, ShaderStage::Pixel, l_PerFrameCBufferGPUBufferComp, 0, Accessibility::ReadOnly);

	g_Engine->getRenderingServer()->BindGPUResource(m_RenderPassComp, ShaderStage::Pixel, BRDFLUTPass::Get().GetResult(), 3);
	g_Engine->getRenderingServer()->BindGPUResource(m_RenderPassComp, ShaderStage::Pixel, BRDFLUTMSPass::Get().GetResult(), 4);

	auto l_mesh = g_Engine->getRenderingFrontend()->GetMeshComponent(ProceduralMeshShape::Sphere);

	uint32_t l_offset = 0;

	for (size_t i = 0; i < m_shpereCount * m_shpereCount; i++)
	{
		g_Engine->getRenderingServer()->BindGPUResource(m_RenderPassComp, ShaderStage::Vertex, l_MeshGPUBufferComp, 1, Accessibility::ReadOnly, l_offset, 1);
		g_Engine->getRenderingServer()->BindGPUResource(m_RenderPassComp, ShaderStage::Pixel, l_MaterialGPUBufferComp, 2, Accessibility::ReadOnly, l_offset, 1);
		g_Engine->getRenderingServer()->DrawIndexedInstanced(m_RenderPassComp, l_mesh);

		l_offset++;
	}

	g_Engine->getRenderingServer()->UnbindGPUResource(m_RenderPassComp, ShaderStage::Pixel, BRDFLUTPass::Get().GetResult(), 3);
	g_Engine->getRenderingServer()->UnbindGPUResource(m_RenderPassComp, ShaderStage::Pixel, BRDFLUTMSPass::Get().GetResult(), 4);

	g_Engine->getRenderingServer()->CommandListEnd(m_RenderPassComp);

	return true;
}

RenderPassComponent* BSDFTestPass::GetRenderPassComp()
{
	return m_RenderPassComp;
}