#include "SSAOPass.h"
#include "../DefaultGPUBuffers/DefaultGPUBuffers.h"

#include "OpaquePass.h"

#include "../../Engine/Interface/IEngine.h"

using namespace Inno;
extern INNO_ENGINE_API IEngine* g_Engine;

using namespace DefaultGPUBuffers;

bool SSAOPass::Setup(ISystemConfig *systemConfig)
{
	m_SPC = g_Engine->getRenderingServer()->AddShaderProgramComponent("SSAONoisePass/");

	m_SPC->m_ShaderFilePaths.m_CSPath = "SSAONoisePass.comp/";

	m_RenderPassComp = g_Engine->getRenderingServer()->AddRenderPassComponent("SSAONoisePass/");

	auto l_RenderPassDesc = g_Engine->getRenderingFrontend()->GetDefaultRenderPassDesc();

	l_RenderPassDesc.m_RenderTargetCount = 0;
	l_RenderPassDesc.m_GPUEngineType = GPUEngineType::Compute;

	m_RenderPassComp->m_RenderPassDesc = l_RenderPassDesc;

	m_RenderPassComp->m_ResourceBindingLayoutDescs.resize(8);
	m_RenderPassComp->m_ResourceBindingLayoutDescs[0].m_GPUResourceType = GPUResourceType::Buffer;
	m_RenderPassComp->m_ResourceBindingLayoutDescs[0].m_DescriptorSetIndex = 0;
	m_RenderPassComp->m_ResourceBindingLayoutDescs[0].m_DescriptorIndex = 0;

	m_RenderPassComp->m_ResourceBindingLayoutDescs[1].m_GPUResourceType = GPUResourceType::Buffer;
	m_RenderPassComp->m_ResourceBindingLayoutDescs[1].m_DescriptorSetIndex = 0;
	m_RenderPassComp->m_ResourceBindingLayoutDescs[1].m_DescriptorIndex = 7;

	m_RenderPassComp->m_ResourceBindingLayoutDescs[2].m_GPUResourceType = GPUResourceType::Image;
	m_RenderPassComp->m_ResourceBindingLayoutDescs[2].m_DescriptorSetIndex = 1;
	m_RenderPassComp->m_ResourceBindingLayoutDescs[2].m_DescriptorIndex = 0;
	m_RenderPassComp->m_ResourceBindingLayoutDescs[2].m_IndirectBinding = true;

	m_RenderPassComp->m_ResourceBindingLayoutDescs[3].m_GPUResourceType = GPUResourceType::Image;
	m_RenderPassComp->m_ResourceBindingLayoutDescs[3].m_DescriptorSetIndex = 1;
	m_RenderPassComp->m_ResourceBindingLayoutDescs[3].m_DescriptorIndex = 1;
	m_RenderPassComp->m_ResourceBindingLayoutDescs[3].m_IndirectBinding = true;

	m_RenderPassComp->m_ResourceBindingLayoutDescs[4].m_GPUResourceType = GPUResourceType::Image;
	m_RenderPassComp->m_ResourceBindingLayoutDescs[4].m_DescriptorSetIndex = 1;
	m_RenderPassComp->m_ResourceBindingLayoutDescs[4].m_DescriptorIndex = 2;
	m_RenderPassComp->m_ResourceBindingLayoutDescs[4].m_IndirectBinding = true;

	m_RenderPassComp->m_ResourceBindingLayoutDescs[5].m_GPUResourceType = GPUResourceType::Sampler;
	m_RenderPassComp->m_ResourceBindingLayoutDescs[5].m_DescriptorSetIndex = 2;
	m_RenderPassComp->m_ResourceBindingLayoutDescs[5].m_DescriptorIndex = 0;
	m_RenderPassComp->m_ResourceBindingLayoutDescs[5].m_IndirectBinding = true;

	m_RenderPassComp->m_ResourceBindingLayoutDescs[6].m_GPUResourceType = GPUResourceType::Sampler;
	m_RenderPassComp->m_ResourceBindingLayoutDescs[6].m_DescriptorSetIndex = 2;
	m_RenderPassComp->m_ResourceBindingLayoutDescs[6].m_DescriptorIndex = 1;
	m_RenderPassComp->m_ResourceBindingLayoutDescs[6].m_IndirectBinding = true;

	m_RenderPassComp->m_ResourceBindingLayoutDescs[7].m_GPUResourceType = GPUResourceType::Image;
	m_RenderPassComp->m_ResourceBindingLayoutDescs[7].m_DescriptorSetIndex = 3;
	m_RenderPassComp->m_ResourceBindingLayoutDescs[7].m_DescriptorIndex = 0;
	m_RenderPassComp->m_ResourceBindingLayoutDescs[7].m_BindingAccessibility = Accessibility::ReadWrite;
	m_RenderPassComp->m_ResourceBindingLayoutDescs[7].m_ResourceAccessibility = Accessibility::ReadWrite;
	m_RenderPassComp->m_ResourceBindingLayoutDescs[7].m_IndirectBinding = true;

	m_RenderPassComp->m_ShaderProgram = m_SPC;

	m_SamplerComp = g_Engine->getRenderingServer()->AddSamplerComponent("SSAONoisePass/");

	m_SamplerComp_RandomRot = g_Engine->getRenderingServer()->AddSamplerComponent("SSAONoisePass_RandomRot/");

	m_SamplerComp_RandomRot->m_SamplerDesc.m_MinFilterMethod = TextureFilterMethod::Nearest;
	m_SamplerComp_RandomRot->m_SamplerDesc.m_MagFilterMethod = TextureFilterMethod::Nearest;
	m_SamplerComp_RandomRot->m_SamplerDesc.m_WrapMethodU = TextureWrapMethod::Repeat;
	m_SamplerComp_RandomRot->m_SamplerDesc.m_WrapMethodV = TextureWrapMethod::Repeat;

	m_TextureComp = g_Engine->getRenderingServer()->AddTextureComponent("SSAONoisePass/");
	m_TextureComp->m_TextureDesc = l_RenderPassDesc.m_RenderTargetDesc;

	// Kernel
	std::uniform_real_distribution<float> l_randomFloats(0.0f, 1.0f);
	std::default_random_engine l_generator;

	m_SSAOKernel.reserve(m_kernelSize);

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
		m_SSAOKernel.emplace_back(l_sample);
	}

	m_SSAOKernelGPUBuffer = g_Engine->getRenderingServer()->AddGPUBufferComponent("SSAOKernel/");
	m_SSAOKernelGPUBuffer->m_ElementSize = sizeof(Vec4);
	m_SSAOKernelGPUBuffer->m_ElementCount = m_kernelSize;
	m_SSAOKernelGPUBuffer->m_InitialData = &m_SSAOKernel[0];

	// Noise
	auto l_textureSize = 4;

	m_SSAONoise.reserve(l_textureSize * l_textureSize);

	for (size_t i = 0; i < m_SSAONoise.capacity(); i++)
	{
		// rotate around z-axis (in tangent space)
		auto noise = Vec4(l_randomFloats(l_generator) * 2.0f - 1.0f, l_randomFloats(l_generator) * 2.0f - 1.0f, 0.0f, 0.0f);
		noise = noise.normalize();
		m_SSAONoise.push_back(noise);
	}

	m_SSAONoiseTextureComp = g_Engine->getRenderingServer()->AddTextureComponent("SSAONoise/");

	m_SSAONoiseTextureComp->m_TextureDesc.Sampler = TextureSampler::Sampler2D;
	m_SSAONoiseTextureComp->m_TextureDesc.Usage = TextureUsage::Sample;
	m_SSAONoiseTextureComp->m_TextureDesc.PixelDataFormat = TexturePixelDataFormat::RGBA;

	m_SSAONoiseTextureComp->m_TextureDesc.Width = l_textureSize;
	m_SSAONoiseTextureComp->m_TextureDesc.Height = l_textureSize;
	m_SSAONoiseTextureComp->m_TextureDesc.PixelDataType = TexturePixelDataType::Float32;

	m_SSAONoiseTextureComp->m_TextureData = &m_SSAONoise[0];

	m_ObjectStatus = ObjectStatus::Created;
	
	return true;
}

bool SSAOPass::Initialize()
{
	g_Engine->getRenderingServer()->InitializeShaderProgramComponent(m_SPC);
	g_Engine->getRenderingServer()->InitializeRenderPassComponent(m_RenderPassComp);
	g_Engine->getRenderingServer()->InitializeTextureComponent(m_TextureComp);
	g_Engine->getRenderingServer()->InitializeSamplerComponent(m_SamplerComp);
	g_Engine->getRenderingServer()->InitializeSamplerComponent(m_SamplerComp_RandomRot);
	g_Engine->getRenderingServer()->InitializeGPUBufferComponent(m_SSAOKernelGPUBuffer);
	g_Engine->getRenderingServer()->InitializeTextureComponent(m_SSAONoiseTextureComp);

	m_ObjectStatus = ObjectStatus::Activated;

	return true;
}

bool SSAOPass::Terminate()
{
	g_Engine->getRenderingServer()->DeleteRenderPassComponent(m_RenderPassComp);

	m_ObjectStatus = ObjectStatus::Terminated;

	return true;
}

ObjectStatus SSAOPass::GetStatus()
{
	return m_ObjectStatus;
}

bool SSAOPass::PrepareCommandList(IRenderingContext* renderingContext)
{
	auto l_viewportSize = g_Engine->getRenderingFrontend()->GetScreenResolution();
	auto l_PerFrameCBufferGPUBufferComp = GetGPUBufferComponent(GPUBufferUsageType::PerFrame);

	g_Engine->getRenderingServer()->CommandListBegin(m_RenderPassComp, 0);
	g_Engine->getRenderingServer()->BindRenderPassComponent(m_RenderPassComp);
	g_Engine->getRenderingServer()->CleanRenderTargets(m_RenderPassComp);
	g_Engine->getRenderingServer()->BindGPUResource(m_RenderPassComp, ShaderStage::Compute, m_SamplerComp, 5);
	g_Engine->getRenderingServer()->BindGPUResource(m_RenderPassComp, ShaderStage::Compute, m_SamplerComp_RandomRot, 6);

	g_Engine->getRenderingServer()->BindGPUResource(m_RenderPassComp, ShaderStage::Compute, l_PerFrameCBufferGPUBufferComp, 0, Accessibility::ReadOnly);
	g_Engine->getRenderingServer()->BindGPUResource(m_RenderPassComp, ShaderStage::Compute, m_SSAOKernelGPUBuffer, 1, Accessibility::ReadOnly);
	g_Engine->getRenderingServer()->BindGPUResource(m_RenderPassComp, ShaderStage::Compute, OpaquePass::Get().GetRenderPassComp()->m_RenderTargets[0], 2);
	g_Engine->getRenderingServer()->BindGPUResource(m_RenderPassComp, ShaderStage::Compute, OpaquePass::Get().GetRenderPassComp()->m_RenderTargets[1], 3);
	g_Engine->getRenderingServer()->BindGPUResource(m_RenderPassComp, ShaderStage::Compute, m_SSAONoiseTextureComp, 4);
	g_Engine->getRenderingServer()->BindGPUResource(m_RenderPassComp, ShaderStage::Compute, m_TextureComp, 7, Accessibility::ReadWrite);

	g_Engine->getRenderingServer()->Dispatch(m_RenderPassComp, uint32_t(l_viewportSize.x / 8.0f), uint32_t(l_viewportSize.y / 8.0f), 1);

	g_Engine->getRenderingServer()->UnbindGPUResource(m_RenderPassComp, ShaderStage::Compute, OpaquePass::Get().GetRenderPassComp()->m_RenderTargets[0], 2);
	g_Engine->getRenderingServer()->UnbindGPUResource(m_RenderPassComp, ShaderStage::Compute, OpaquePass::Get().GetRenderPassComp()->m_RenderTargets[1], 3);
	g_Engine->getRenderingServer()->UnbindGPUResource(m_RenderPassComp, ShaderStage::Compute, m_SSAONoiseTextureComp, 4);
	g_Engine->getRenderingServer()->UnbindGPUResource(m_RenderPassComp, ShaderStage::Compute, m_TextureComp, 7, Accessibility::ReadWrite);

	g_Engine->getRenderingServer()->CommandListEnd(m_RenderPassComp);

	return true;
}

RenderPassComponent* SSAOPass::GetRenderPassComp()
{
	return m_RenderPassComp;
}

GPUResourceComponent *SSAOPass::GetResult()
{
	return m_TextureComp;
}