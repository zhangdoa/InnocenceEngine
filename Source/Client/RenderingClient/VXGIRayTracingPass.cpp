#include "VXGIRayTracingPass.h"

#include "../../Engine/Common/Timer.h"
#include "../../Engine/Services/RenderingConfigurationService.h"
#include "../../Engine/Services/RenderingContextService.h"
#include "VXGIRenderer.h"
#include "VXGIConvertPass.h"

#include "../../Engine/Engine.h"

using namespace Inno;




bool VXGIRayTracingPass::Setup(ISystemConfig *systemConfig)
{	
	auto l_renderingServer = g_Engine->getRenderingServer();

	auto l_RenderPassDesc = g_Engine->Get<RenderingConfigurationService>()->GetDefaultRenderPassDesc();
	auto l_VXGIRenderingConfig = &reinterpret_cast<VXGIRendererSystemConfig*>(systemConfig)->m_VXGIRenderingConfig;

	m_TextureComp = l_renderingServer->AddTextureComponent("VoxelRayTracingVolume/");
	m_TextureComp->m_TextureDesc = l_RenderPassDesc.m_RenderTargetDesc;

	m_TextureComp->m_TextureDesc.Width = l_VXGIRenderingConfig->m_voxelizationResolution;
	m_TextureComp->m_TextureDesc.Height = l_VXGIRenderingConfig->m_voxelizationResolution;
	m_TextureComp->m_TextureDesc.DepthOrArraySize = l_VXGIRenderingConfig->m_voxelizationResolution;
	m_TextureComp->m_TextureDesc.Usage = TextureUsage::Sample;
	m_TextureComp->m_TextureDesc.Sampler = TextureSampler::Sampler3D;
	m_TextureComp->m_TextureDesc.UseMipMap = true;

	m_ShaderProgramComp = l_renderingServer->AddShaderProgramComponent("VoxelRayTracingPass/");

	m_ShaderProgramComp->m_ShaderFilePaths.m_CSPath = "voxelRayTracingPass.comp/";

	m_RenderPassComp = l_renderingServer->AddRenderPassComponent("VoxelRayTracingPass/");

	l_RenderPassDesc.m_RenderTargetCount = 0;
	l_RenderPassDesc.m_GPUEngineType = GPUEngineType::Compute;
	l_RenderPassDesc.m_Resizable = false;

	m_RenderPassComp->m_RenderPassDesc = l_RenderPassDesc;

	m_RenderPassComp->m_ResourceBindingLayoutDescs.resize(7);

	m_RenderPassComp->m_ResourceBindingLayoutDescs[0].m_GPUResourceType = GPUResourceType::Image;
	m_RenderPassComp->m_ResourceBindingLayoutDescs[0].m_BindingAccessibility = Accessibility::ReadOnly;
	m_RenderPassComp->m_ResourceBindingLayoutDescs[0].m_ResourceAccessibility = Accessibility::ReadWrite;
	m_RenderPassComp->m_ResourceBindingLayoutDescs[0].m_DescriptorSetIndex = 1;
	m_RenderPassComp->m_ResourceBindingLayoutDescs[0].m_DescriptorIndex = 0;
	m_RenderPassComp->m_ResourceBindingLayoutDescs[0].m_IndirectBinding = true;

	m_RenderPassComp->m_ResourceBindingLayoutDescs[1].m_GPUResourceType = GPUResourceType::Image;
	m_RenderPassComp->m_ResourceBindingLayoutDescs[1].m_BindingAccessibility = Accessibility::ReadOnly;
	m_RenderPassComp->m_ResourceBindingLayoutDescs[1].m_ResourceAccessibility = Accessibility::ReadWrite;
	m_RenderPassComp->m_ResourceBindingLayoutDescs[1].m_DescriptorSetIndex = 1;
	m_RenderPassComp->m_ResourceBindingLayoutDescs[1].m_DescriptorIndex = 1;
	m_RenderPassComp->m_ResourceBindingLayoutDescs[1].m_IndirectBinding = true;

	m_RenderPassComp->m_ResourceBindingLayoutDescs[2].m_GPUResourceType = GPUResourceType::Image;
	m_RenderPassComp->m_ResourceBindingLayoutDescs[2].m_BindingAccessibility = Accessibility::ReadWrite;
	m_RenderPassComp->m_ResourceBindingLayoutDescs[2].m_ResourceAccessibility = Accessibility::ReadWrite;
	m_RenderPassComp->m_ResourceBindingLayoutDescs[2].m_DescriptorSetIndex = 2;
	m_RenderPassComp->m_ResourceBindingLayoutDescs[2].m_DescriptorIndex = 0;
	m_RenderPassComp->m_ResourceBindingLayoutDescs[2].m_IndirectBinding = true;

	m_RenderPassComp->m_ResourceBindingLayoutDescs[3].m_GPUResourceType = GPUResourceType::Sampler;
	m_RenderPassComp->m_ResourceBindingLayoutDescs[3].m_DescriptorSetIndex = 3;
	m_RenderPassComp->m_ResourceBindingLayoutDescs[3].m_DescriptorIndex = 0;
	m_RenderPassComp->m_ResourceBindingLayoutDescs[3].m_IndirectBinding = true;

	m_RenderPassComp->m_ResourceBindingLayoutDescs[4].m_GPUResourceType = GPUResourceType::Buffer;
	m_RenderPassComp->m_ResourceBindingLayoutDescs[4].m_DescriptorSetIndex = 0;
	m_RenderPassComp->m_ResourceBindingLayoutDescs[4].m_DescriptorIndex = 9;

	m_RenderPassComp->m_ResourceBindingLayoutDescs[5].m_GPUResourceType = GPUResourceType::Buffer;
	m_RenderPassComp->m_ResourceBindingLayoutDescs[5].m_BindingAccessibility = Accessibility::ReadWrite;
	m_RenderPassComp->m_ResourceBindingLayoutDescs[5].m_ResourceAccessibility = Accessibility::ReadWrite;
	m_RenderPassComp->m_ResourceBindingLayoutDescs[5].m_DescriptorSetIndex = 2;
	m_RenderPassComp->m_ResourceBindingLayoutDescs[5].m_DescriptorIndex = 1;

	m_RenderPassComp->m_ResourceBindingLayoutDescs[6].m_GPUResourceType = GPUResourceType::Buffer;
	m_RenderPassComp->m_ResourceBindingLayoutDescs[6].m_BindingAccessibility = Accessibility::ReadWrite;
	m_RenderPassComp->m_ResourceBindingLayoutDescs[6].m_ResourceAccessibility = Accessibility::ReadWrite;
	m_RenderPassComp->m_ResourceBindingLayoutDescs[6].m_DescriptorSetIndex = 2;
	m_RenderPassComp->m_ResourceBindingLayoutDescs[6].m_DescriptorIndex = 2;

	m_RenderPassComp->m_ShaderProgram = m_ShaderProgramComp;

	m_SamplerComp = l_renderingServer->AddSamplerComponent("VoxelRayTracingPass/");

	m_SamplerComp->m_SamplerDesc.m_WrapMethodU = TextureWrapMethod::Repeat;
	m_SamplerComp->m_SamplerDesc.m_WrapMethodV = TextureWrapMethod::Repeat;
	m_SamplerComp->m_SamplerDesc.m_WrapMethodW = TextureWrapMethod::Repeat;

	m_Ray.reserve(l_VXGIRenderingConfig->m_maxRay * l_VXGIRenderingConfig->m_maxRay);
	m_ProbeIndex.reserve(l_VXGIRenderingConfig->m_maxProbe * l_VXGIRenderingConfig->m_maxProbe * l_VXGIRenderingConfig->m_maxProbe);
	m_randomInt = std::uniform_int_distribution<uint32_t>(0, l_VXGIRenderingConfig->m_voxelizationResolution);

	auto radius = 1.0f;
	auto sectorCount = l_VXGIRenderingConfig->m_maxRay;
	auto stackCount = l_VXGIRenderingConfig->m_maxRay;

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

	m_RaySBufferGPUBufferComp = l_renderingServer->AddGPUBufferComponent("VoxelRayTracingRaySBuffer/");
	m_RaySBufferGPUBufferComp->m_ElementCount = l_VXGIRenderingConfig->m_maxRay * l_VXGIRenderingConfig->m_maxRay;
	m_RaySBufferGPUBufferComp->m_ElementSize = sizeof(Vec4);
	m_RaySBufferGPUBufferComp->m_GPUAccessibility = Accessibility::ReadWrite;
	m_RaySBufferGPUBufferComp->m_InitialData = &m_Ray[0];

	////
	m_ProbeIndexSBufferGPUBufferComp = l_renderingServer->AddGPUBufferComponent("VoxelRayTracingProbeIndexSBuffer/");
	m_ProbeIndexSBufferGPUBufferComp->m_ElementCount = l_VXGIRenderingConfig->m_maxProbe * l_VXGIRenderingConfig->m_maxProbe * l_VXGIRenderingConfig->m_maxProbe;
	m_ProbeIndexSBufferGPUBufferComp->m_ElementSize = sizeof(TVec4<uint32_t>);
	m_ProbeIndexSBufferGPUBufferComp->m_GPUAccessibility = Accessibility::ReadWrite;

	m_ObjectStatus = ObjectStatus::Created;
	
	return true;
}

bool VXGIRayTracingPass::Initialize()
{	
	auto l_renderingServer = g_Engine->getRenderingServer();

	l_renderingServer->Initialize(m_ShaderProgramComp);
	l_renderingServer->Initialize(m_RenderPassComp);
	l_renderingServer->Initialize(m_SamplerComp);

	l_renderingServer->Initialize(m_RaySBufferGPUBufferComp);
	l_renderingServer->Initialize(m_ProbeIndexSBufferGPUBufferComp);
	l_renderingServer->Initialize(m_TextureComp);
		
	m_ObjectStatus = ObjectStatus::Activated;

	return true;
}

bool VXGIRayTracingPass::Terminate()
{
	auto l_renderingServer = g_Engine->getRenderingServer();

	l_renderingServer->Delete(m_RenderPassComp);

	m_ObjectStatus = ObjectStatus::Terminated;

	return true;
}

ObjectStatus VXGIRayTracingPass::GetStatus()
{
	return m_ObjectStatus;
}

bool VXGIRayTracingPass::PrepareCommandList(IRenderingContext* renderingContext)
{	
	auto l_renderingServer = g_Engine->getRenderingServer();
	auto l_renderingContext = reinterpret_cast<VXGIRayTracingPassRenderingContext*>(renderingContext);
	
	auto l_tick = g_Engine->Get<Timer>()->GetCurrentTimeFromEpoch(TimeUnit::Millisecond);
	m_generator.seed((uint32_t)l_tick);

	m_ProbeIndex.clear();
	auto l_size = m_ProbeIndex.capacity();

	for (uint32_t i = 0; i < l_size; i++)
	{
		auto l_sample = TVec4(m_randomInt(m_generator), m_randomInt(m_generator), m_randomInt(m_generator), (uint32_t)0);

		m_ProbeIndex.emplace_back(l_sample);
	}

	l_renderingServer->Upload(m_ProbeIndexSBufferGPUBufferComp, m_ProbeIndex);

	// l_renderingServer->CommandListBegin(m_RenderPassComp, 0);
	// l_renderingServer->BindRenderPassComponent(m_RenderPassComp);
	// l_renderingServer->ClearRenderTargets(m_RenderPassComp);

	// l_renderingServer->BindGPUResource(m_RenderPassComp, ShaderStage::Compute, VXGIRenderer::Get().GetVoxelizationCBuffer(), 4);
	// l_renderingServer->BindGPUResource(m_RenderPassComp, ShaderStage::Compute, l_renderingContext->m_input, 0);
	// l_renderingServer->BindGPUResource(m_RenderPassComp, ShaderStage::Compute, VXGIConvertPass::Get().GetNormalVolume(), 1);
	// l_renderingServer->BindGPUResource(m_RenderPassComp, ShaderStage::Compute, l_renderingContext->m_output, 2);
	// l_renderingServer->BindGPUResource(m_RenderPassComp, ShaderStage::Compute, m_SamplerComp, 3);
	// l_renderingServer->BindGPUResource(m_RenderPassComp, ShaderStage::Compute, m_RaySBufferGPUBufferComp, 5);
	// l_renderingServer->BindGPUResource(m_RenderPassComp, ShaderStage::Compute, m_ProbeIndexSBufferGPUBufferComp, 6);

	// l_renderingServer->Dispatch(m_RenderPassComp, 8, 8, 8);

	// l_renderingServer->UnbindGPUResource(m_RenderPassComp, ShaderStage::Compute, l_renderingContext->m_input, 0);
	// l_renderingServer->UnbindGPUResource(m_RenderPassComp, ShaderStage::Compute, VXGIConvertPass::Get().GetNormalVolume(), 1);
	// l_renderingServer->UnbindGPUResource(m_RenderPassComp, ShaderStage::Compute, l_renderingContext->m_output, 2);
	// l_renderingServer->UnbindGPUResource(m_RenderPassComp, ShaderStage::Compute, m_RaySBufferGPUBufferComp, 5);
	// l_renderingServer->UnbindGPUResource(m_RenderPassComp, ShaderStage::Compute, m_ProbeIndexSBufferGPUBufferComp, 6);

	// l_renderingServer->CommandListEnd(m_RenderPassComp);

	return true;
}

RenderPassComponent* VXGIRayTracingPass::GetRenderPassComp()
{
	return m_RenderPassComp;
}

GPUResourceComponent* VXGIRayTracingPass::GetResult()
{
	return m_TextureComp;
}