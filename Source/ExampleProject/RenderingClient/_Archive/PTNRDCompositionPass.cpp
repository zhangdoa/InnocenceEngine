#include "PTNRDCompositionPass.h"

#include "PTPass.h"
#include "PTNRDDenoisePass.h"
#include "NRDConstants.h"

#include "../../Engine/Engine.h"
#include "../../Engine/Services/RenderingConfigurationService.h"
#include "../../Engine/Services/ShaderProgramResourceService.h"
#include "../../Engine/Services/RenderPassResourceService.h"
#include "../../Engine/Services/TextureResourceService.h"
#include "../../Engine/Services/CommandListResourceService.h"

using namespace Inno;

bool PTNRDCompositionPass::Setup(IServiceConfig* systemConfig)
{
	if constexpr (!Inno::NRD::ENABLED)
	{
		m_ObjectStatus = ObjectStatus::Terminated;
		return true;
	}

	m_ShaderProgramComp = g_Engine->Get<ShaderProgramResourceService>()->Add("PTNRDCompositionPass");
	m_ShaderProgramComp->m_ShaderFilePaths.m_CSPath = "PTNRDComposition.comp";

	m_RenderPassComp = g_Engine->Get<RenderPassResourceService>()->Add("PTNRDCompositionPass");

	auto l_RenderPassDesc = g_Engine->Get<RenderingConfigurationService>()->GetDefaultRenderPassDesc();
	l_RenderPassDesc.m_GPUEngineType                   = GPUEngineType::Compute;
	l_RenderPassDesc.m_RenderTargetCount               = 0;
	l_RenderPassDesc.m_UseOutputMerger                 = false;
	l_RenderPassDesc.m_RenderTargetsInitializationFunc = std::bind(&PTNRDCompositionPass::RenderTargetsCreationFunc, this);

	m_RenderPassComp->m_RenderPassDesc = l_RenderPassDesc;
	m_RenderPassComp->m_OnResize       = [this]() { RenderTargetsCreationFunc(); };

	// Mirrors PTNRDComposition.comp register block: 1 CB + 5 SRVs + 1 UAV = 7 descs.
	m_RenderPassComp->m_ResourceBindingLayoutDescs.resize(7);

	// b0: PerFrameConstantBuffer (set 0, binding 0).
	m_RenderPassComp->m_ResourceBindingLayoutDescs[0].m_GPUResourceType    = GPUResourceType::Buffer;
	m_RenderPassComp->m_ResourceBindingLayoutDescs[0].m_DescriptorSetIndex = 0;
	m_RenderPassComp->m_ResourceBindingLayoutDescs[0].m_DescriptorIndex    = 0;
	m_RenderPassComp->m_ResourceBindingLayoutDescs[0].m_ShaderStage        = ShaderStage::Compute;

	// t0..t4: 5 SRVs (set 1, binding 0..4).
	for (uint32_t i = 0u; i < 5u; ++i)
	{
		auto& l_desc = m_RenderPassComp->m_ResourceBindingLayoutDescs[1u + i];
		l_desc.m_GPUResourceType       = GPUResourceType::Image;
		l_desc.m_DescriptorSetIndex    = 1;
		l_desc.m_DescriptorIndex       = static_cast<int32_t>(i);
		l_desc.m_TextureUsage          = TextureUsage::ComputeOnly;
		l_desc.m_BindingAccessibility  = Accessibility::ReadOnly;
		l_desc.m_ResourceAccessibility = Accessibility::ReadWrite;
		l_desc.m_ShaderStage           = ShaderStage::Compute;
	}

	// u0: 1 UAV (set 2, binding 0) — out_FinalRadiance.
	{
		auto& l_desc = m_RenderPassComp->m_ResourceBindingLayoutDescs[6];
		l_desc.m_GPUResourceType       = GPUResourceType::Image;
		l_desc.m_DescriptorSetIndex    = 2;
		l_desc.m_DescriptorIndex       = 0;
		l_desc.m_TextureUsage          = TextureUsage::ComputeOnly;
		l_desc.m_BindingAccessibility  = Accessibility::ReadWrite;
		l_desc.m_ResourceAccessibility = Accessibility::ReadWrite;
		l_desc.m_ShaderStage           = ShaderStage::Compute;
	}

	m_RenderPassComp->m_ShaderProgram = m_ShaderProgramComp;

	m_CommandListComp_Graphics = g_Engine->Get<CommandListResourceService>()->Add("PTNRDCompositionPass/Graphics");
	m_CommandListComp_Graphics->m_Type = GPUEngineType::Graphics;
	m_CommandListComp_Compute = g_Engine->Get<CommandListResourceService>()->Add("PTNRDCompositionPass/Compute");
	m_CommandListComp_Compute->m_Type = GPUEngineType::Compute;

	m_ObjectStatus = ObjectStatus::Created;
	return true;
}

bool PTNRDCompositionPass::Initialize()
{
	if constexpr (!Inno::NRD::ENABLED)
		return true;

	g_Engine->Get<ShaderProgramResourceService>()->Initialize(m_ShaderProgramComp);
	g_Engine->Get<RenderPassResourceService>()->Initialize(m_RenderPassComp);
	g_Engine->Get<CommandListResourceService>()->Initialize(m_CommandListComp_Graphics);
	g_Engine->Get<CommandListResourceService>()->Initialize(m_CommandListComp_Compute);

	m_ObjectStatus = ObjectStatus::Suspended;
	return true;
}

bool PTNRDCompositionPass::Update()
{
	if constexpr (!Inno::NRD::ENABLED)
		return true;

	auto l_isActivated = [](TextureComponent* t)
	{
		return t && t->m_ObjectStatus == ObjectStatus::Activated;
	};
	auto* l_pt   = &PTPass::Get();
	auto* l_nrd  = &PTNRDDenoisePass::Get();
	const bool l_inputsReady =
		l_isActivated(l_pt->GetPTGBufferPosition()) &&
		l_isActivated(l_pt->GetPTGBufferAlbedoRoughness()) &&
		l_isActivated(static_cast<TextureComponent*>(l_pt->GetResult())) &&
		l_isActivated(l_nrd->GetOutDiffRadianceHitDist()) &&
		l_isActivated(l_nrd->GetOutSpecRadianceHitDist());
	const bool l_outputReady = l_isActivated(m_Result);
	m_ObjectStatus = (l_inputsReady && l_outputReady) ? ObjectStatus::Activated : ObjectStatus::Suspended;
	return true;
}

bool PTNRDCompositionPass::Terminate()
{
	if constexpr (!Inno::NRD::ENABLED)
	{
		m_ObjectStatus = ObjectStatus::Terminated;
		return true;
	}

	if (m_Result)
	{
		g_Engine->Get<TextureResourceService>()->Delete(m_Result);
		m_Result = nullptr;
	}
	g_Engine->Get<CommandListResourceService>()->Delete(m_CommandListComp_Compute);
	g_Engine->Get<CommandListResourceService>()->Delete(m_CommandListComp_Graphics);
	g_Engine->Get<RenderPassResourceService>()->Delete(m_RenderPassComp);
	g_Engine->Get<ShaderProgramResourceService>()->Delete(m_ShaderProgramComp);

	m_ObjectStatus = ObjectStatus::Terminated;
	return true;
}

ObjectStatus PTNRDCompositionPass::GetStatus()
{
	return m_ObjectStatus;
}

RenderPassComponent* PTNRDCompositionPass::GetRenderPassComp()
{
	return m_RenderPassComp;
}

bool PTNRDCompositionPass::RenderTargetsCreationFunc()
{
	auto l_resolution = g_Engine->Get<RenderingConfigurationService>()->GetScreenResolution();
	auto l_texService = g_Engine->Get<TextureResourceService>();
	if (m_Result)
		l_texService->Delete(m_Result);
	m_Result = l_texService->Add("PTNRDComposition_FinalRadiance");
	m_Result->m_TextureDesc.Sampler          = TextureSampler::Sampler2D;
	m_Result->m_TextureDesc.Usage            = TextureUsage::ComputeOnly;
	m_Result->m_TextureDesc.PixelDataFormat  = TexturePixelDataFormat::RGBA;
	m_Result->m_TextureDesc.PixelDataType    = TexturePixelDataType::Float16;
	m_Result->m_TextureDesc.Width            = l_resolution.x;
	m_Result->m_TextureDesc.Height           = l_resolution.y;
	m_Result->m_TextureDesc.DepthOrArraySize = 1;
	m_Result->m_CPUAccessibility             = Accessibility::Immutable;
	m_Result->m_GPUAccessibility             = Accessibility::ReadWrite;
	l_texService->Initialize(m_Result);
	return true;
}
