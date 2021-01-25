#include "BSDFTestPass.h"
#include "../DefaultGPUBuffers/DefaultGPUBuffers.h"

#include "BRDFLUTPass.h"

#include "../../Engine/Interface/IEngine.h"

using namespace Inno;
extern INNO_ENGINE_API IEngine* g_Engine;

using namespace DefaultGPUBuffers;

namespace BSDFTestPass
{
	RenderPassDataComponent* m_RPDC;
	ShaderProgramComponent* m_SPC;
	SamplerDataComponent* m_SDC;

	std::vector<PerObjectConstantBuffer> m_meshConstantBuffer;
	std::vector<MaterialConstantBuffer> m_materialConstantBuffer;

	const size_t m_shpereCount = 10;
}

bool BSDFTestPass::Setup()
{
	m_SPC = g_Engine->getRenderingServer()->AddShaderProgramComponent("BSDFTestPass/");

	m_SPC->m_ShaderFilePaths.m_VSPath = "opaqueGeometryProcessPass.vert/";
	m_SPC->m_ShaderFilePaths.m_PSPath = "BSDFTestPass.frag/";

	m_RPDC = g_Engine->getRenderingServer()->AddRenderPassDataComponent("BSDFTestPass/");

	auto l_RenderPassDesc = g_Engine->getRenderingFrontend()->getDefaultRenderPassDesc();

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

	m_RPDC->m_RenderPassDesc = l_RenderPassDesc;

	m_RPDC->m_ResourceBindingLayoutDescs.resize(6);
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
	m_RPDC->m_ResourceBindingLayoutDescs[3].m_IndirectBinding = true;

	m_RPDC->m_ResourceBindingLayoutDescs[4].m_GPUResourceType = GPUResourceType::Image;
	m_RPDC->m_ResourceBindingLayoutDescs[4].m_DescriptorSetIndex = 1;
	m_RPDC->m_ResourceBindingLayoutDescs[4].m_DescriptorIndex = 1;
	m_RPDC->m_ResourceBindingLayoutDescs[4].m_IndirectBinding = true;

	m_RPDC->m_ResourceBindingLayoutDescs[5].m_GPUResourceType = GPUResourceType::Sampler;
	m_RPDC->m_ResourceBindingLayoutDescs[5].m_DescriptorSetIndex = 2;
	m_RPDC->m_ResourceBindingLayoutDescs[5].m_DescriptorIndex = 0;
	m_RPDC->m_ResourceBindingLayoutDescs[5].m_IndirectBinding = true;

	m_RPDC->m_ShaderProgram = m_SPC;

	m_SDC = g_Engine->getRenderingServer()->AddSamplerDataComponent("BSDFTestPass/");

	//
	auto l_RenderingCapability = g_Engine->getRenderingFrontend()->getRenderingCapability();

	m_meshConstantBuffer.resize(l_RenderingCapability.maxMeshes);
	m_materialConstantBuffer.resize(l_RenderingCapability.maxMaterials);

	size_t l_index = 0;

	auto l_interval = 4.0f;

	for (size_t i = 0; i < m_shpereCount; i++)
	{
		for (size_t j = 0; j < m_shpereCount; j++)
		{
			PerObjectConstantBuffer l_meshConstantBuffer;
			l_meshConstantBuffer.m = InnoMath::toTranslationMatrix(Vec4((float)i * l_interval, 0.0f, (float)j * l_interval, 1.0f));
			l_meshConstantBuffer.normalMat = InnoMath::generateIdentityMatrix<float>();

			m_meshConstantBuffer[l_index] = l_meshConstantBuffer;

			MaterialConstantBuffer l_materialConstantBuffer;

			l_materialConstantBuffer.materialAttributes.Metallic = (float)i / (float)m_shpereCount;
			l_materialConstantBuffer.materialAttributes.Roughness = (float)j / (float)m_shpereCount;

			m_materialConstantBuffer[l_index] = l_materialConstantBuffer;

			l_index++;
		}
	}

	return true;
}

bool BSDFTestPass::Initialize()
{
	g_Engine->getRenderingServer()->InitializeShaderProgramComponent(m_SPC);
	g_Engine->getRenderingServer()->InitializeRenderPassDataComponent(m_RPDC);
	g_Engine->getRenderingServer()->InitializeSamplerDataComponent(m_SDC);

	return true;
}

bool BSDFTestPass::Render()
{
	auto l_PerFrameCBufferGBDC = GetGPUBufferDataComponent(GPUBufferUsageType::PerFrame);
	auto l_MeshGBDC = GetGPUBufferDataComponent(GPUBufferUsageType::Mesh);
	auto l_MaterialGBDC = GetGPUBufferDataComponent(GPUBufferUsageType::Material);

	g_Engine->getRenderingServer()->UploadGPUBufferDataComponent(l_MeshGBDC, m_meshConstantBuffer);
	g_Engine->getRenderingServer()->UploadGPUBufferDataComponent(l_MaterialGBDC, m_materialConstantBuffer);

	g_Engine->getRenderingServer()->CommandListBegin(m_RPDC, 0);
	g_Engine->getRenderingServer()->BindRenderPassDataComponent(m_RPDC);
	g_Engine->getRenderingServer()->CleanRenderTargets(m_RPDC);
	g_Engine->getRenderingServer()->BindGPUResource(m_RPDC, ShaderStage::Pixel, m_SDC, 5);

	g_Engine->getRenderingServer()->BindGPUResource(m_RPDC, ShaderStage::Vertex, l_PerFrameCBufferGBDC, 0, Accessibility::ReadOnly);
	g_Engine->getRenderingServer()->BindGPUResource(m_RPDC, ShaderStage::Pixel, l_PerFrameCBufferGBDC, 0, Accessibility::ReadOnly);

	g_Engine->getRenderingServer()->BindGPUResource(m_RPDC, ShaderStage::Pixel, BRDFLUTPass::GetBRDFLUT(), 3);
	g_Engine->getRenderingServer()->BindGPUResource(m_RPDC, ShaderStage::Pixel, BRDFLUTPass::GetBRDFMSLUT(), 4);

	auto l_mesh = g_Engine->getRenderingFrontend()->getMeshDataComponent(ProceduralMeshShape::Sphere);

	uint32_t l_offset = 0;

	for (size_t i = 0; i < m_shpereCount * m_shpereCount; i++)
	{
		g_Engine->getRenderingServer()->BindGPUResource(m_RPDC, ShaderStage::Vertex, l_MeshGBDC, 1, Accessibility::ReadOnly, l_offset, 1);
		g_Engine->getRenderingServer()->BindGPUResource(m_RPDC, ShaderStage::Pixel, l_MaterialGBDC, 2, Accessibility::ReadOnly, l_offset, 1);
		g_Engine->getRenderingServer()->DrawIndexedInstanced(m_RPDC, l_mesh);

		l_offset++;
	}

	g_Engine->getRenderingServer()->UnbindGPUResource(m_RPDC, ShaderStage::Pixel, BRDFLUTPass::GetBRDFLUT(), 3);
	g_Engine->getRenderingServer()->UnbindGPUResource(m_RPDC, ShaderStage::Pixel, BRDFLUTPass::GetBRDFMSLUT(), 4);

	g_Engine->getRenderingServer()->CommandListEnd(m_RPDC);

	g_Engine->getRenderingServer()->ExecuteCommandList(m_RPDC);

	g_Engine->getRenderingServer()->WaitForFrame(m_RPDC);

	return true;
}

bool BSDFTestPass::Terminate()
{
	g_Engine->getRenderingServer()->DeleteRenderPassDataComponent(m_RPDC);

	return true;
}

RenderPassDataComponent* BSDFTestPass::GetRPDC()
{
	return m_RPDC;
}

ShaderProgramComponent* BSDFTestPass::GetSPC()
{
	return m_SPC;
}