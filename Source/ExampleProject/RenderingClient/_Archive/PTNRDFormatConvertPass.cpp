#include "PTNRDFormatConvertPass.h"

#include "PTPass.h"
#include "NRDConstants.h"

#include "../../Engine/Engine.h"
#include "../../Engine/Services/RenderingConfigurationService.h"
#include "../../Engine/Services/ShaderProgramResourceService.h"
#include "../../Engine/Services/RenderPassResourceService.h"
#include "../../Engine/Services/TextureResourceService.h"
#include "../../Engine/Services/CommandListResourceService.h"

using namespace Inno;

bool PTNRDFormatConvertPass::Setup(IServiceConfig* systemConfig)
{
	if constexpr (!Inno::NRD::ENABLED)
	{
		// Toggle off (BUILD_WITH_NRD=OFF): pass stays Terminated, never
		// advertises Activated to the dispatcher, never allocates a shader
		// program or render pass. Mirrors the cache / PT-denoise sibling
		// passes' bypass shape.
		m_ObjectStatus = ObjectStatus::Terminated;
		return true;
	}

	m_ShaderProgramComp = g_Engine->Get<ShaderProgramResourceService>()->Add("PTNRDFormatConvertPass");
	m_ShaderProgramComp->m_ShaderFilePaths.m_CSPath = "PTNRDFormatConvert.comp";

	m_RenderPassComp = g_Engine->Get<RenderPassResourceService>()->Add("PTNRDFormatConvertPass");

	auto l_RenderPassDesc = g_Engine->Get<RenderingConfigurationService>()->GetDefaultRenderPassDesc();
	l_RenderPassDesc.m_GPUEngineType                   = GPUEngineType::Compute;
	l_RenderPassDesc.m_RenderTargetCount               = 0;
	l_RenderPassDesc.m_UseOutputMerger                 = false;
	l_RenderPassDesc.m_RenderTargetsInitializationFunc = std::bind(&PTNRDFormatConvertPass::RenderTargetsCreationFunc, this);

	m_RenderPassComp->m_RenderPassDesc = l_RenderPassDesc;

	// Re-allocate outputs at the new resolution after a swap-chain resize.
	// The five output UAVs are pure scratch; any resolution change scraps
	// them. Same hook PTPass uses for its accumulation and
	// GBuffer-equivalent textures.
	m_RenderPassComp->m_OnResize = [this]() { RenderTargetsCreationFunc(); };

	// 1 CB + 6 SRVs + 5 UAVs = 12 descs. Mirrors the HLSL register block at
	// PTNRDFormatConvert.comp (b0 in space 0, t0..t5 in space 1, u0..u4 in
	// space 2). Drift on either side is a PSO-create failure — same
	// invariant the cache passes' static_asserts protect.
	m_RenderPassComp->m_ResourceBindingLayoutDescs.resize(12);

	// b0 - PerFrameConstantBuffer (set 0, binding 0). Engine PerFrame_CB
	// uploaded by PerFrameDataService each frame. Provides v + viewportSize
	// for the linear view-Z math at the kernel entry.
	m_RenderPassComp->m_ResourceBindingLayoutDescs[0].m_GPUResourceType    = GPUResourceType::Buffer;
	m_RenderPassComp->m_ResourceBindingLayoutDescs[0].m_DescriptorSetIndex = 0;
	m_RenderPassComp->m_ResourceBindingLayoutDescs[0].m_DescriptorIndex    = 0;
	m_RenderPassComp->m_ResourceBindingLayoutDescs[0].m_ShaderStage        = ShaderStage::Compute;

	// t0..t5 — six SRV reads from compute-written textures. The path
	// tracer's GBuffer-equivalent textures (RT0..RT3) plus the per-lobe
	// radiance UAVs (raygen u11/u12). All allocated TextureUsage::ComputeOnly
	// by PTPass; the format-convert pass only reads them.
	for (uint32_t i = 0u; i < 6u; ++i)
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

	// u0..u4 — five UAV writes into the NRD-format outputs.
	for (uint32_t i = 0u; i < 5u; ++i)
	{
		auto& l_desc = m_RenderPassComp->m_ResourceBindingLayoutDescs[7u + i];
		l_desc.m_GPUResourceType       = GPUResourceType::Image;
		l_desc.m_DescriptorSetIndex    = 2;
		l_desc.m_DescriptorIndex       = static_cast<int32_t>(i);
		l_desc.m_TextureUsage          = TextureUsage::ComputeOnly;
		l_desc.m_BindingAccessibility  = Accessibility::ReadWrite;
		l_desc.m_ResourceAccessibility = Accessibility::ReadWrite;
		l_desc.m_ShaderStage           = ShaderStage::Compute;
	}

	m_RenderPassComp->m_ShaderProgram = m_ShaderProgramComp;

	m_CommandListComp_Graphics = g_Engine->Get<CommandListResourceService>()->Add("PTNRDFormatConvertPass/Graphics");
	m_CommandListComp_Graphics->m_Type = GPUEngineType::Graphics;

	m_CommandListComp_Compute = g_Engine->Get<CommandListResourceService>()->Add("PTNRDFormatConvertPass/Compute");
	m_CommandListComp_Compute->m_Type = GPUEngineType::Compute;

	m_ObjectStatus = ObjectStatus::Created;
	return true;
}

bool PTNRDFormatConvertPass::Initialize()
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

bool PTNRDFormatConvertPass::Update()
{
	if constexpr (!Inno::NRD::ENABLED)
		return true;

	// Activation gates on the path tracer's PT-GBuffer + per-lobe radiance
	// textures being Activated (those are the SRV inputs we read) AND on
	// our own five UAV outputs being Activated. Mirrors the cache passes'
	// owner-buffer-status check shape.
	auto* l_owner = &PTPass::Get();
	auto l_isActivated = [](TextureComponent* in_Tex)
	{
		return in_Tex && in_Tex->m_ObjectStatus == ObjectStatus::Activated;
	};

	const bool l_inputsReady =
		l_isActivated(l_owner->GetPTGBufferPosition()) &&
		l_isActivated(l_owner->GetPTGBufferNormalMetalness()) &&
		l_isActivated(l_owner->GetPTGBufferAlbedoRoughness()) &&
		l_isActivated(l_owner->GetPTGBufferMotionHitDist()) &&
		l_isActivated(l_owner->GetPTRadianceDiffuse()) &&
		l_isActivated(l_owner->GetPTRadianceSpecular());

	const bool l_outputsReady =
		l_isActivated(m_NRD_ViewZ) &&
		l_isActivated(m_NRD_NormalRoughness) &&
		l_isActivated(m_NRD_MotionVector) &&
		l_isActivated(m_NRD_DiffRadianceHitDist) &&
		l_isActivated(m_NRD_SpecRadianceHitDist);

	m_ObjectStatus = (l_inputsReady && l_outputsReady) ? ObjectStatus::Activated : ObjectStatus::Suspended;
	return true;
}

bool PTNRDFormatConvertPass::Terminate()
{
	if constexpr (!Inno::NRD::ENABLED)
	{
		m_ObjectStatus = ObjectStatus::Terminated;
		return true;
	}

	auto l_texService = g_Engine->Get<TextureResourceService>();
	auto l_drop = [&](TextureComponent*& tex) { if (tex) { l_texService->Delete(tex); tex = nullptr; } };
	l_drop(m_NRD_SpecRadianceHitDist);
	l_drop(m_NRD_DiffRadianceHitDist);
	l_drop(m_NRD_MotionVector);
	l_drop(m_NRD_NormalRoughness);
	l_drop(m_NRD_ViewZ);

	g_Engine->Get<CommandListResourceService>()->Delete(m_CommandListComp_Compute);
	g_Engine->Get<CommandListResourceService>()->Delete(m_CommandListComp_Graphics);
	g_Engine->Get<RenderPassResourceService>()->Delete(m_RenderPassComp);
	g_Engine->Get<ShaderProgramResourceService>()->Delete(m_ShaderProgramComp);

	m_ObjectStatus = ObjectStatus::Terminated;
	return true;
}

ObjectStatus PTNRDFormatConvertPass::GetStatus()
{
	return m_ObjectStatus;
}

RenderPassComponent* PTNRDFormatConvertPass::GetRenderPassComp()
{
	return m_RenderPassComp;
}

bool PTNRDFormatConvertPass::RenderTargetsCreationFunc()
{
	auto l_resolution = g_Engine->Get<RenderingConfigurationService>()->GetScreenResolution();
	auto l_texService = g_Engine->Get<TextureResourceService>();

	// Common scratch shape: ComputeOnly UAV at screen resolution, zero-init,
	// GPU-RW. Per-output PixelDataFormat / PixelDataType fields differ; the
	// helper takes them as parameters.
	auto l_make = [&](const char*             in_Name,
	                  TexturePixelDataFormat  in_Format,
	                  TexturePixelDataType    in_Type,
	                  TextureComponent*&      out_Tex)
	{
		if (out_Tex)
			l_texService->Delete(out_Tex);
		out_Tex = l_texService->Add(in_Name);
		out_Tex->m_TextureDesc.Sampler          = TextureSampler::Sampler2D;
		out_Tex->m_TextureDesc.Usage            = TextureUsage::ComputeOnly;
		out_Tex->m_TextureDesc.PixelDataFormat  = in_Format;
		out_Tex->m_TextureDesc.PixelDataType    = in_Type;
		out_Tex->m_TextureDesc.Width            = l_resolution.x;
		out_Tex->m_TextureDesc.Height           = l_resolution.y;
		out_Tex->m_TextureDesc.DepthOrArraySize = 1;
		out_Tex->m_CPUAccessibility             = Accessibility::Immutable;
		out_Tex->m_GPUAccessibility             = Accessibility::ReadWrite;
		l_texService->Initialize(out_Tex);
	};

	// IN_VIEWZ — R32F (NRD ResourceType comment: "linear viewZ"; one float).
	l_make("PTNRDFormatConvert_ViewZ",
	       TexturePixelDataFormat::R, TexturePixelDataType::Float32,
	       m_NRD_ViewZ);

	// IN_NORMAL_ROUGHNESS — R10G10B10A2_UNORM, pinned by NRDConfig.hlsli
	// (NRD_NORMAL_ENCODING_R10G10B10A2_UNORM). PixelDataType is unused for
	// this packed tag; the format-mapper resolves on the tag alone.
	l_make("PTNRDFormatConvert_NormalRoughness",
	       TexturePixelDataFormat::RGB10A2, TexturePixelDataType::UByte,
	       m_NRD_NormalRoughness);

	// IN_MV — RG16F (2D-pixel motion). NRD CommonSettings::motionVectorScale
	// = {1, 1, 0} is the CL-3 wiring side; this pass writes RT3.xy verbatim.
	l_make("PTNRDFormatConvert_MotionVector",
	       TexturePixelDataFormat::RG, TexturePixelDataType::Float16,
	       m_NRD_MotionVector);

	// IN_DIFF_RADIANCE_HITDIST / IN_SPEC_RADIANCE_HITDIST — RGBA16F. The
	// front-end pack helper writes (YCoCg.xyz, normHitDist) into the
	// channels; sanitize=true scrubs NaN/inf at the pack site.
	l_make("PTNRDFormatConvert_DiffRadianceHitDist",
	       TexturePixelDataFormat::RGBA, TexturePixelDataType::Float16,
	       m_NRD_DiffRadianceHitDist);
	l_make("PTNRDFormatConvert_SpecRadianceHitDist",
	       TexturePixelDataFormat::RGBA, TexturePixelDataType::Float16,
	       m_NRD_SpecRadianceHitDist);

	return true;
}
