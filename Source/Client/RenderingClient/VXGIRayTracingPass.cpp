#include "VXGIRayTracingPass.h"
#include "../DefaultGPUBuffers/DefaultGPUBuffers.h"

#include "VXGIRenderer.h"
#include "VXGIConvertPass.h"

#include "../../Engine/Interface/IEngine.h"

using namespace Inno;
extern INNO_ENGINE_API IEngine* g_Engine;

using namespace DefaultGPUBuffers;

bool VXGIRayTracingPass::Setup(ISystemConfig *systemConfig)
{	
	auto l_RenderPassDesc = g_Engine->getRenderingFrontend()->getDefaultRenderPassDesc();
	auto l_VXGIRenderingConfig = VXGIRenderer::Get().GetVXGIRenderingConfig();

	m_TDC = g_Engine->getRenderingServer()->AddTextureDataComponent("VoxelRayTracingVolume/");
	m_TDC->m_TextureDesc = l_RenderPassDesc.m_RenderTargetDesc;

	m_TDC->m_TextureDesc.Width = l_VXGIRenderingConfig.m_voxelizationResolution;
	m_TDC->m_TextureDesc.Height = l_VXGIRenderingConfig.m_voxelizationResolution;
	m_TDC->m_TextureDesc.DepthOrArraySize = l_VXGIRenderingConfig.m_voxelizationResolution;
	m_TDC->m_TextureDesc.Usage = TextureUsage::Sample;
	m_TDC->m_TextureDesc.Sampler = TextureSampler::Sampler3D;
	m_TDC->m_TextureDesc.UseMipMap = true;

	m_SPC = g_Engine->getRenderingServer()->AddShaderProgramComponent("VoxelRayTracingPass/");

	m_SPC->m_ShaderFilePaths.m_CSPath = "voxelRayTracingPass.comp/";

	m_RPDC = g_Engine->getRenderingServer()->AddRenderPassDataComponent("VoxelRayTracingPass/");

	l_RenderPassDesc.m_RenderTargetCount = 0;
	l_RenderPassDesc.m_GPUEngineType = GPUEngineType::Compute;
	l_RenderPassDesc.m_IsOffScreen = true;

	m_RPDC->m_RenderPassDesc = l_RenderPassDesc;

	m_RPDC->m_ResourceBindingLayoutDescs.resize(7);

	m_RPDC->m_ResourceBindingLayoutDescs[0].m_GPUResourceType = GPUResourceType::Image;
	m_RPDC->m_ResourceBindingLayoutDescs[0].m_BindingAccessibility = Accessibility::ReadOnly;
	m_RPDC->m_ResourceBindingLayoutDescs[0].m_ResourceAccessibility = Accessibility::ReadWrite;
	m_RPDC->m_ResourceBindingLayoutDescs[0].m_DescriptorSetIndex = 1;
	m_RPDC->m_ResourceBindingLayoutDescs[0].m_DescriptorIndex = 0;
	m_RPDC->m_ResourceBindingLayoutDescs[0].m_IndirectBinding = true;

	m_RPDC->m_ResourceBindingLayoutDescs[1].m_GPUResourceType = GPUResourceType::Image;
	m_RPDC->m_ResourceBindingLayoutDescs[1].m_BindingAccessibility = Accessibility::ReadOnly;
	m_RPDC->m_ResourceBindingLayoutDescs[1].m_ResourceAccessibility = Accessibility::ReadWrite;
	m_RPDC->m_ResourceBindingLayoutDescs[1].m_DescriptorSetIndex = 1;
	m_RPDC->m_ResourceBindingLayoutDescs[1].m_DescriptorIndex = 1;
	m_RPDC->m_ResourceBindingLayoutDescs[1].m_IndirectBinding = true;

	m_RPDC->m_ResourceBindingLayoutDescs[2].m_GPUResourceType = GPUResourceType::Image;
	m_RPDC->m_ResourceBindingLayoutDescs[2].m_BindingAccessibility = Accessibility::ReadWrite;
	m_RPDC->m_ResourceBindingLayoutDescs[2].m_ResourceAccessibility = Accessibility::ReadWrite;
	m_RPDC->m_ResourceBindingLayoutDescs[2].m_DescriptorSetIndex = 2;
	m_RPDC->m_ResourceBindingLayoutDescs[2].m_DescriptorIndex = 0;
	m_RPDC->m_ResourceBindingLayoutDescs[2].m_IndirectBinding = true;

	m_RPDC->m_ResourceBindingLayoutDescs[3].m_GPUResourceType = GPUResourceType::Sampler;
	m_RPDC->m_ResourceBindingLayoutDescs[3].m_DescriptorSetIndex = 3;
	m_RPDC->m_ResourceBindingLayoutDescs[3].m_DescriptorIndex = 0;
	m_RPDC->m_ResourceBindingLayoutDescs[3].m_IndirectBinding = true;

	m_RPDC->m_ResourceBindingLayoutDescs[4].m_GPUResourceType = GPUResourceType::Buffer;
	m_RPDC->m_ResourceBindingLayoutDescs[4].m_DescriptorSetIndex = 0;
	m_RPDC->m_ResourceBindingLayoutDescs[4].m_DescriptorIndex = 9;

	m_RPDC->m_ResourceBindingLayoutDescs[5].m_GPUResourceType = GPUResourceType::Buffer;
	m_RPDC->m_ResourceBindingLayoutDescs[5].m_BindingAccessibility = Accessibility::ReadWrite;
	m_RPDC->m_ResourceBindingLayoutDescs[5].m_ResourceAccessibility = Accessibility::ReadWrite;
	m_RPDC->m_ResourceBindingLayoutDescs[5].m_DescriptorSetIndex = 2;
	m_RPDC->m_ResourceBindingLayoutDescs[5].m_DescriptorIndex = 1;

	m_RPDC->m_ResourceBindingLayoutDescs[6].m_GPUResourceType = GPUResourceType::Buffer;
	m_RPDC->m_ResourceBindingLayoutDescs[6].m_BindingAccessibility = Accessibility::ReadWrite;
	m_RPDC->m_ResourceBindingLayoutDescs[6].m_ResourceAccessibility = Accessibility::ReadWrite;
	m_RPDC->m_ResourceBindingLayoutDescs[6].m_DescriptorSetIndex = 2;
	m_RPDC->m_ResourceBindingLayoutDescs[6].m_DescriptorIndex = 2;

	m_RPDC->m_ShaderProgram = m_SPC;

	m_SDC = g_Engine->getRenderingServer()->AddSamplerDataComponent("VoxelRayTracingPass/");

	m_SDC->m_SamplerDesc.m_WrapMethodU = TextureWrapMethod::Repeat;
	m_SDC->m_SamplerDesc.m_WrapMethodV = TextureWrapMethod::Repeat;
	m_SDC->m_SamplerDesc.m_WrapMethodW = TextureWrapMethod::Repeat;

	m_Ray.reserve(l_VXGIRenderingConfig.m_maxRay * l_VXGIRenderingConfig.m_maxRay);
	m_ProbeIndex.reserve(l_VXGIRenderingConfig.m_maxProbe * l_VXGIRenderingConfig.m_maxProbe * l_VXGIRenderingConfig.m_maxProbe);
	m_randomInt = std::uniform_int_distribution<uint32_t>(0, l_VXGIRenderingConfig.m_voxelizationResolution);

	auto radius = 1.0f;
	auto sectorCount = l_VXGIRenderingConfig.m_maxRay;
	auto stackCount = l_VXGIRenderingConfig.m_maxRay;

	float x, y, z, xy;

	float sectorStep = 2 * PI<float> / sectorCount;
	float stackStep = PI<float> / stackCount;
	float sectorAngle, stackAngle;

	for (uint32_t i = 0; i < stackCount; ++i)
	{
		stackAngle = PI<float> / 2 - i * stackStep;
		xy = radius * cosf(stackAngle);
		z = radius * sinf(stackAngle);

		for (uint32_t j = 0; j < sectorCount; ++j)
		{
			sectorAngle = j * sectorStep;

			x = xy * cosf(sectorAngle);
			y = xy * sinf(sectorAngle);
			auto l_dir = Vec4(x, y, z, 0.0f);
			l_dir = l_dir.normalize();

			m_Ray.emplace_back(l_dir);
		}
	}

	m_RaySBufferGBDC = g_Engine->getRenderingServer()->AddGPUBufferDataComponent("VoxelRayTracingRaySBuffer/");
	m_RaySBufferGBDC->m_ElementCount = l_VXGIRenderingConfig.m_maxRay * l_VXGIRenderingConfig.m_maxRay;
	m_RaySBufferGBDC->m_ElementSize = sizeof(Vec4);
	m_RaySBufferGBDC->m_GPUAccessibility = Accessibility::ReadWrite;
	m_RaySBufferGBDC->m_InitialData = &m_Ray[0];

	////
	m_ProbeIndexSBufferGBDC = g_Engine->getRenderingServer()->AddGPUBufferDataComponent("VoxelRayTracingProbeIndexSBuffer/");
	m_ProbeIndexSBufferGBDC->m_ElementCount = l_VXGIRenderingConfig.m_maxProbe * l_VXGIRenderingConfig.m_maxProbe * l_VXGIRenderingConfig.m_maxProbe;
	m_ProbeIndexSBufferGBDC->m_ElementSize = sizeof(TVec4<uint32_t>);
	m_ProbeIndexSBufferGBDC->m_GPUAccessibility = Accessibility::ReadWrite;

	m_ObjectStatus = ObjectStatus::Created;
	
	return true;
}

bool VXGIRayTracingPass::Initialize()
{	
	g_Engine->getRenderingServer()->InitializeShaderProgramComponent(m_SPC);
	g_Engine->getRenderingServer()->InitializeRenderPassDataComponent(m_RPDC);
	g_Engine->getRenderingServer()->InitializeSamplerDataComponent(m_SDC);

	g_Engine->getRenderingServer()->InitializeGPUBufferDataComponent(m_RaySBufferGBDC);
	g_Engine->getRenderingServer()->InitializeGPUBufferDataComponent(m_ProbeIndexSBufferGBDC);
	g_Engine->getRenderingServer()->InitializeTextureDataComponent(m_TDC);
		
	m_ObjectStatus = ObjectStatus::Activated;

	return true;
}

bool VXGIRayTracingPass::Terminate()
{
	g_Engine->getRenderingServer()->DeleteRenderPassDataComponent(m_RPDC);

	m_ObjectStatus = ObjectStatus::Terminated;

	return true;
}

ObjectStatus VXGIRayTracingPass::GetStatus()
{
	return m_ObjectStatus;
}

bool VXGIRayTracingPass::PrepareCommandList(IRenderingContext* renderingContext)
{	
	auto l_renderingContext = reinterpret_cast<VXGIRayTracingPassRenderingContext*>(renderingContext);
	
	auto l_tick = g_Engine->getTimeSystem()->getCurrentTimeFromEpoch();
	m_generator.seed((uint32_t)l_tick);

	m_ProbeIndex.clear();
	auto l_size = m_ProbeIndex.capacity();

	for (uint32_t i = 0; i < l_size; i++)
	{
		auto l_sample = TVec4(m_randomInt(m_generator), m_randomInt(m_generator), m_randomInt(m_generator), (uint32_t)0);

		m_ProbeIndex.emplace_back(l_sample);
	}

	g_Engine->getRenderingServer()->UploadGPUBufferDataComponent(m_ProbeIndexSBufferGBDC, m_ProbeIndex);


	g_Engine->getRenderingServer()->CommandListBegin(m_RPDC, 0);
	g_Engine->getRenderingServer()->BindRenderPassDataComponent(m_RPDC);
	g_Engine->getRenderingServer()->CleanRenderTargets(m_RPDC);

	g_Engine->getRenderingServer()->BindGPUResource(m_RPDC, ShaderStage::Compute, VXGIRenderer::Get().GetVoxelizationCBuffer(), 4, Accessibility::ReadOnly);
	g_Engine->getRenderingServer()->BindGPUResource(m_RPDC, ShaderStage::Compute, l_renderingContext->m_input, 0, Accessibility::ReadOnly);
	g_Engine->getRenderingServer()->BindGPUResource(m_RPDC, ShaderStage::Compute, VXGIConvertPass::Get().GetNormalVolume(), 1, Accessibility::ReadOnly);
	g_Engine->getRenderingServer()->BindGPUResource(m_RPDC, ShaderStage::Compute, l_renderingContext->m_output, 2, Accessibility::ReadWrite);
	g_Engine->getRenderingServer()->BindGPUResource(m_RPDC, ShaderStage::Compute, m_SDC, 3);
	g_Engine->getRenderingServer()->BindGPUResource(m_RPDC, ShaderStage::Compute, m_RaySBufferGBDC, 5, Accessibility::ReadWrite);
	g_Engine->getRenderingServer()->BindGPUResource(m_RPDC, ShaderStage::Compute, m_ProbeIndexSBufferGBDC, 6, Accessibility::ReadWrite);

	g_Engine->getRenderingServer()->Dispatch(m_RPDC, 8, 8, 8);

	g_Engine->getRenderingServer()->UnbindGPUResource(m_RPDC, ShaderStage::Compute, l_renderingContext->m_input, 0, Accessibility::ReadOnly);
	g_Engine->getRenderingServer()->UnbindGPUResource(m_RPDC, ShaderStage::Compute, VXGIConvertPass::Get().GetNormalVolume(), 1, Accessibility::ReadOnly);
	g_Engine->getRenderingServer()->UnbindGPUResource(m_RPDC, ShaderStage::Compute, l_renderingContext->m_output, 2, Accessibility::ReadWrite);
	g_Engine->getRenderingServer()->UnbindGPUResource(m_RPDC, ShaderStage::Compute, m_RaySBufferGBDC, 5, Accessibility::ReadWrite);
	g_Engine->getRenderingServer()->UnbindGPUResource(m_RPDC, ShaderStage::Compute, m_ProbeIndexSBufferGBDC, 6, Accessibility::ReadWrite);

	g_Engine->getRenderingServer()->CommandListEnd(m_RPDC);

	return true;
}

RenderPassDataComponent* VXGIRayTracingPass::GetRPDC()
{
	return m_RPDC;
}

GPUResourceComponent* VXGIRayTracingPass::GetResult()
{
	return m_TDC;
}