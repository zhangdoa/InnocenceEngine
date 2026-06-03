#include "SSAOPass.h"

#include "../../Engine/Services/RenderingConfigurationService.h"

#include "../../Engine/Engine.h"
#include "../../Engine/Services/ShaderProgramResourceService.h"
#include "../../Engine/Services/RenderPassResourceService.h"
#include "../../Engine/Services/TextureResourceService.h"
#include "../../Engine/Services/SamplerResourceService.h"
#include "../../Engine/Services/GPUBufferResourceService.h"
#include "../../Engine/Services/CommandListResourceService.h"
#include "../../Engine/Services/FrameManagementService.h"

#include <random>

using namespace Inno;

// Imperative binding-layout construction for SSAOPass. SetupImperative is the
// g_UseRenderGraph==false fallback (verbatim layout the graph node mirrors);
// SetupOwnedResources creates the kernel buffer / noise texture / samplers that
// are imported into the graph by name, so both setup paths call it.

bool SSAOPass::SetupImperative()
{
	auto l_fmService = g_Engine->Get<FrameManagementService>();

	m_ShaderProgramComp = g_Engine->Get<ShaderProgramResourceService>()->Add("SSAONoisePass");

	m_ShaderProgramComp->m_ShaderFilePaths.m_CSPath = "SSAONoisePass.comp";

	m_RenderPassComp = g_Engine->Get<RenderPassResourceService>()->Add("SSAONoisePass");

	auto l_RenderPassDesc = g_Engine->Get<RenderingConfigurationService>()->GetDefaultRenderPassDesc();

	l_RenderPassDesc.m_RenderTargetCount = 0;
	l_RenderPassDesc.m_GPUEngineType = GPUEngineType::Compute;
	l_RenderPassDesc.m_UseOutputMerger = false;
	l_RenderPassDesc.m_RenderTargetsInitializationFunc = std::bind(&SSAOPass::RenderTargetsCreationFunc, this);

	m_RenderPassComp->m_RenderPassDesc = l_RenderPassDesc;

	m_RenderPassComp->m_ResourceBindingLayoutDescs.resize(8);

	// b0 - PerFrameCBuffer
	m_RenderPassComp->m_ResourceBindingLayoutDescs[0].m_GPUResourceType = GPUResourceType::Buffer;
	m_RenderPassComp->m_ResourceBindingLayoutDescs[0].m_DescriptorSetIndex = 0;
	m_RenderPassComp->m_ResourceBindingLayoutDescs[0].m_DescriptorIndex = 0;

	// b1 - Kernel
	m_RenderPassComp->m_ResourceBindingLayoutDescs[1].m_GPUResourceType = GPUResourceType::Buffer;
	m_RenderPassComp->m_ResourceBindingLayoutDescs[1].m_DescriptorSetIndex = 0;
	m_RenderPassComp->m_ResourceBindingLayoutDescs[1].m_DescriptorIndex = 1;

	// t0 - World space position
	m_RenderPassComp->m_ResourceBindingLayoutDescs[2].m_GPUResourceType = GPUResourceType::Image;
	m_RenderPassComp->m_ResourceBindingLayoutDescs[2].m_DescriptorSetIndex = 1;
	m_RenderPassComp->m_ResourceBindingLayoutDescs[2].m_DescriptorIndex = 0;
	m_RenderPassComp->m_ResourceBindingLayoutDescs[2].m_TextureUsage = TextureUsage::ColorAttachment;

	// t1 - World space normal
	m_RenderPassComp->m_ResourceBindingLayoutDescs[3].m_GPUResourceType = GPUResourceType::Image;
	m_RenderPassComp->m_ResourceBindingLayoutDescs[3].m_DescriptorSetIndex = 1;
	m_RenderPassComp->m_ResourceBindingLayoutDescs[3].m_DescriptorIndex = 1;
	m_RenderPassComp->m_ResourceBindingLayoutDescs[3].m_TextureUsage = TextureUsage::ColorAttachment;	

	// t2 - Noise texture
	m_RenderPassComp->m_ResourceBindingLayoutDescs[4].m_GPUResourceType = GPUResourceType::Image;
	m_RenderPassComp->m_ResourceBindingLayoutDescs[4].m_DescriptorSetIndex = 1;
	m_RenderPassComp->m_ResourceBindingLayoutDescs[4].m_DescriptorIndex = 2;
	m_RenderPassComp->m_ResourceBindingLayoutDescs[4].m_TextureUsage = TextureUsage::ComputeOnly;

	// s0 - Sampler
	m_RenderPassComp->m_ResourceBindingLayoutDescs[5].m_GPUResourceType = GPUResourceType::Sampler;
	m_RenderPassComp->m_ResourceBindingLayoutDescs[5].m_DescriptorSetIndex = 2;
	m_RenderPassComp->m_ResourceBindingLayoutDescs[5].m_DescriptorIndex = 0;

	// s1 - Sampler for the random rotation texture
	m_RenderPassComp->m_ResourceBindingLayoutDescs[6].m_GPUResourceType = GPUResourceType::Sampler;
	m_RenderPassComp->m_ResourceBindingLayoutDescs[6].m_DescriptorSetIndex = 2;
	m_RenderPassComp->m_ResourceBindingLayoutDescs[6].m_DescriptorIndex = 1;

	// u0 - Result
	m_RenderPassComp->m_ResourceBindingLayoutDescs[7].m_GPUResourceType = GPUResourceType::Image;
	m_RenderPassComp->m_ResourceBindingLayoutDescs[7].m_DescriptorSetIndex = 3;
	m_RenderPassComp->m_ResourceBindingLayoutDescs[7].m_DescriptorIndex = 0;
	m_RenderPassComp->m_ResourceBindingLayoutDescs[7].m_BindingAccessibility = Accessibility::ReadWrite;
	m_RenderPassComp->m_ResourceBindingLayoutDescs[7].m_ResourceAccessibility = Accessibility::ReadWrite;
	m_RenderPassComp->m_ResourceBindingLayoutDescs[7].m_TextureUsage = TextureUsage::ComputeOnly;

	m_RenderPassComp->m_ShaderProgram = m_ShaderProgramComp;

	m_CommandListComp_Graphics = g_Engine->Get<CommandListResourceService>()->Add("SSAOPass/Graphics");
	m_CommandListComp_Graphics->m_Type = GPUEngineType::Graphics;

	m_CommandListComp_Compute = g_Engine->Get<CommandListResourceService>()->Add("SSAOPass/Compute");
	m_CommandListComp_Compute->m_Type = GPUEngineType::Compute;

	if (!SetupOwnedResources())
		return false;

	m_ObjectStatus = ObjectStatus::Created;

	return true;
}

bool SSAOPass::SetupOwnedResources()
{
	m_SamplerComp = g_Engine->Get<SamplerResourceService>()->Add("SSAONoisePass");

	m_SamplerComp_RandomRot = g_Engine->Get<SamplerResourceService>()->Add("SSAONoisePass_RandomRot");

	m_SamplerComp_RandomRot->m_SamplerDesc.m_MinFilterMethod = TextureFilterMethod::Nearest;
	m_SamplerComp_RandomRot->m_SamplerDesc.m_MagFilterMethod = TextureFilterMethod::Nearest;
	m_SamplerComp_RandomRot->m_SamplerDesc.m_WrapMethodU = TextureWrapMethod::Repeat;
	m_SamplerComp_RandomRot->m_SamplerDesc.m_WrapMethodV = TextureWrapMethod::Repeat;

	// Kernel
	std::uniform_real_distribution<float> l_randomFloats(0.0f, 1.0f);
	std::default_random_engine l_generator;

	m_Kernel.reserve(m_kernelSize);

	for (uint32_t i = 0; i < m_kernelSize; ++i)
	{
		auto l_sample = Vec4(l_randomFloats(l_generator) * 2.0f - 1.0f, l_randomFloats(l_generator) * 2.0f - 1.0f, l_randomFloats(l_generator), 0.0f);
		l_sample = l_sample.normalize();
		l_sample = l_sample * l_randomFloats(l_generator);
		float l_scale = float(i) / float(m_kernelSize);

		// scale samples s.t. they're more aligned to center of kernel
		auto l_alpha = l_scale * l_scale;
		l_scale = 0.1f + 0.9f * l_alpha;
		l_sample.x = l_sample.x * l_scale;
		l_sample.y = l_sample.y * l_scale;
		m_Kernel.emplace_back(l_sample);
	}

	m_KernelGPUBuffer = g_Engine->Get<GPUBufferResourceService>()->Add("SSAO_Kernel");
	m_KernelGPUBuffer->m_GPUResourceType = GPUResourceType::Buffer;
	m_KernelGPUBuffer->m_ElementSize = sizeof(Vec4);
	m_KernelGPUBuffer->m_ElementCount = m_kernelSize;
	m_KernelGPUBuffer->m_InitialData = &m_Kernel[0];

	// Noise
	auto l_textureSize = 4;

	m_Noise.reserve(l_textureSize * l_textureSize);

	for (size_t i = 0; i < m_Noise.capacity(); i++)
	{
		// rotate around z-axis (in tangent space)
		auto noise = Vec4(l_randomFloats(l_generator) * 2.0f - 1.0f, l_randomFloats(l_generator) * 2.0f - 1.0f, 0.0f, 0.0f);
		noise = noise.normalize();
		m_Noise.push_back(noise);
	}

	m_NoiseTexture = g_Engine->Get<TextureResourceService>()->Add("SSAO_Noise");

	m_NoiseTexture->m_TextureDesc.Sampler = TextureSampler::Sampler2D;
	m_NoiseTexture->m_TextureDesc.Usage = TextureUsage::ComputeOnly;
	m_NoiseTexture->m_TextureDesc.PixelDataFormat = TexturePixelDataFormat::RGBA;
	m_NoiseTexture->m_TextureDesc.Width = l_textureSize;
	m_NoiseTexture->m_TextureDesc.Height = l_textureSize;
	m_NoiseTexture->m_TextureDesc.PixelDataType = TexturePixelDataType::Float32;

	return true;
}

bool SSAOPass::RenderTargetsCreationFunc()
{
	auto l_fmService = g_Engine->Get<FrameManagementService>();

	if (m_Result)
		g_Engine->Get<TextureResourceService>()->Delete(m_Result);

	auto l_RenderPassDesc = g_Engine->Get<RenderingConfigurationService>()->GetDefaultRenderPassDesc();
	auto l_viewportSize = g_Engine->Get<RenderingConfigurationService>()->GetScreenResolution();

	m_Result = g_Engine->Get<TextureResourceService>()->Add("SSAO_Result");
	m_Result->m_TextureDesc = l_RenderPassDesc.m_RenderTargetDesc;
	m_Result->m_TextureDesc.Usage = TextureUsage::ComputeOnly;

	g_Engine->Get<TextureResourceService>()->Initialize(m_Result);

	return true;
}
