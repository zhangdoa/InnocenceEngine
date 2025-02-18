#include "SSAOPass.h"

#include "../../Engine/Services/RenderingConfigurationService.h"
#include "../../Engine/Services/RenderingContextService.h"

#include "OpaquePass.h"

#include "../../Engine/Engine.h"

using namespace Inno;

bool SSAOPass::Setup(ISystemConfig* systemConfig)
{
	auto l_renderingServer = g_Engine->getRenderingServer();

	m_ShaderProgramComp = l_renderingServer->AddShaderProgramComponent("SSAONoisePass/");

	m_ShaderProgramComp->m_ShaderFilePaths.m_CSPath = "SSAONoisePass.comp/";

	m_RenderPassComp = l_renderingServer->AddRenderPassComponent("SSAONoisePass/");

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
	m_RenderPassComp->m_ResourceBindingLayoutDescs[4].m_TextureUsage = TextureUsage::ColorAttachment;

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
	m_RenderPassComp->m_ResourceBindingLayoutDescs[7].m_TextureUsage = TextureUsage::ColorAttachment;

	m_RenderPassComp->m_ShaderProgram = m_ShaderProgramComp;

	m_SamplerComp = l_renderingServer->AddSamplerComponent("SSAONoisePass/");

	m_SamplerComp_RandomRot = l_renderingServer->AddSamplerComponent("SSAONoisePass_RandomRot/");

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

	m_KernelGPUBuffer = l_renderingServer->AddGPUBufferComponent("SSAO_Kernel/");
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

	m_NoiseTexture = l_renderingServer->AddTextureComponent("SSAO_Noise/");

	m_NoiseTexture->m_TextureDesc.Sampler = TextureSampler::Sampler2D;
	m_NoiseTexture->m_TextureDesc.Usage = TextureUsage::ColorAttachment;
	m_NoiseTexture->m_TextureDesc.PixelDataFormat = TexturePixelDataFormat::RGBA;

	m_NoiseTexture->m_TextureDesc.Width = l_textureSize;
	m_NoiseTexture->m_TextureDesc.Height = l_textureSize;
	m_NoiseTexture->m_TextureDesc.PixelDataType = TexturePixelDataType::Float32;

	m_NoiseTexture->m_InitialData = &m_Noise[0];

	m_ObjectStatus = ObjectStatus::Created;

	return true;
}

bool SSAOPass::Initialize()
{
	auto l_renderingServer = g_Engine->getRenderingServer();

	l_renderingServer->Initialize(m_ShaderProgramComp);
	l_renderingServer->Initialize(m_RenderPassComp);
	l_renderingServer->Initialize(m_SamplerComp);
	l_renderingServer->Initialize(m_SamplerComp_RandomRot);
	l_renderingServer->Initialize(m_KernelGPUBuffer);
	l_renderingServer->Initialize(m_NoiseTexture);

	m_ObjectStatus = ObjectStatus::Suspended;

	return true;
}

bool SSAOPass::Terminate()
{
	auto l_renderingServer = g_Engine->getRenderingServer();

	l_renderingServer->Delete(m_Result);
	l_renderingServer->Delete(m_NoiseTexture);
	l_renderingServer->Delete(m_KernelGPUBuffer);
	l_renderingServer->Delete(m_SamplerComp_RandomRot);
	l_renderingServer->Delete(m_SamplerComp);
	l_renderingServer->Delete(m_RenderPassComp);
	l_renderingServer->Delete(m_ShaderProgramComp);
	
	m_ObjectStatus = ObjectStatus::Terminated;

	return true;
}

ObjectStatus SSAOPass::GetStatus()
{
	return m_ObjectStatus;
}

bool SSAOPass::PrepareCommandList(IRenderingContext* renderingContext)
{
	if (m_RenderPassComp->m_ObjectStatus != ObjectStatus::Activated)
		return false;

	if (m_Result->m_ObjectStatus != ObjectStatus::Activated)
		return false;

	auto l_renderingServer = g_Engine->getRenderingServer();
	auto l_currentFrame = l_renderingServer->GetCurrentFrame();

	auto l_viewportSize = g_Engine->Get<RenderingConfigurationService>()->GetScreenResolution();
	auto l_PerFrameCBufferGPUBufferComp = g_Engine->Get<RenderingContextService>()->GetGPUBufferComponent(GPUBufferUsageType::PerFrame);

	l_renderingServer->CommandListBegin(m_RenderPassComp, 0);
	l_renderingServer->BindRenderPassComponent(m_RenderPassComp);

	l_renderingServer->BindGPUResource(m_RenderPassComp, ShaderStage::Compute, l_PerFrameCBufferGPUBufferComp, 0);
	l_renderingServer->BindGPUResource(m_RenderPassComp, ShaderStage::Compute, m_KernelGPUBuffer, 1);
	l_renderingServer->BindGPUResource(m_RenderPassComp, ShaderStage::Compute, OpaquePass::Get().GetRenderPassComp()->m_OutputMergerTarget->m_ColorOutputs[0], 2);
	l_renderingServer->BindGPUResource(m_RenderPassComp, ShaderStage::Compute, OpaquePass::Get().GetRenderPassComp()->m_OutputMergerTarget->m_ColorOutputs[1], 3);
	l_renderingServer->BindGPUResource(m_RenderPassComp, ShaderStage::Compute, m_NoiseTexture, 4);
	l_renderingServer->BindGPUResource(m_RenderPassComp, ShaderStage::Compute, m_SamplerComp, 5);
	l_renderingServer->BindGPUResource(m_RenderPassComp, ShaderStage::Compute, m_SamplerComp_RandomRot, 6);
	l_renderingServer->BindGPUResource(m_RenderPassComp, ShaderStage::Compute, m_Result, 7);

	l_renderingServer->Dispatch(m_RenderPassComp, uint32_t(l_viewportSize.x / 8.0f), uint32_t(l_viewportSize.y / 8.0f), 1);

	l_renderingServer->CommandListEnd(m_RenderPassComp);

	m_ObjectStatus = ObjectStatus::Activated;

	return true;
}

RenderPassComponent* SSAOPass::GetRenderPassComp()
{
	return m_RenderPassComp;
}

GPUResourceComponent* SSAOPass::GetResult()
{
	return m_Result;
}

bool SSAOPass::RenderTargetsCreationFunc()
{
	auto l_renderingServer = g_Engine->getRenderingServer();

	if (m_Result)
		l_renderingServer->Delete(m_Result);

	auto l_RenderPassDesc = g_Engine->Get<RenderingConfigurationService>()->GetDefaultRenderPassDesc();
	auto l_viewportSize = g_Engine->Get<RenderingConfigurationService>()->GetScreenResolution();

	m_Result = l_renderingServer->AddTextureComponent("SSAO_Result/");
	m_Result->m_TextureDesc = l_RenderPassDesc.m_RenderTargetDesc;
	m_Result->m_TextureDesc.Usage = TextureUsage::ColorAttachment;

	l_renderingServer->Initialize(m_Result);

	return true;
}