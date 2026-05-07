#include "PTDenoiseTemporalPass.h"

#include "../../Engine/Engine.h"
#include "../../Engine/Services/RenderingConfigurationService.h"
#include "../../Engine/Services/TextureResourceService.h"

using namespace Inno;

bool PTDenoiseTemporalPass::RenderTargetsCreationFunc()
{
	auto l_resolution = g_Engine->Get<RenderingConfigurationService>()->GetScreenResolution();
	auto l_texService = g_Engine->Get<TextureResourceService>();

	auto l_makeRGBA16F = [&](const char* in_Name, TextureComponent*& out_Tex)
	{
		if (out_Tex)
			l_texService->Delete(out_Tex);
		out_Tex = l_texService->Add(in_Name);
		out_Tex->m_TextureDesc.Sampler          = TextureSampler::Sampler2D;
		out_Tex->m_TextureDesc.Usage            = TextureUsage::ComputeOnly;
		out_Tex->m_TextureDesc.PixelDataFormat  = TexturePixelDataFormat::RGBA;
		out_Tex->m_TextureDesc.PixelDataType    = TexturePixelDataType::Float16;
		out_Tex->m_TextureDesc.Width            = l_resolution.x;
		out_Tex->m_TextureDesc.Height           = l_resolution.y;
		out_Tex->m_TextureDesc.DepthOrArraySize = 1;
		out_Tex->m_CPUAccessibility             = Accessibility::Immutable;
		out_Tex->m_GPUAccessibility             = Accessibility::ReadWrite;
		l_texService->Initialize(out_Tex);
	};

	auto l_makeRG16F = [&](const char* in_Name, TextureComponent*& out_Tex)
	{
		if (out_Tex)
			l_texService->Delete(out_Tex);
		out_Tex = l_texService->Add(in_Name);
		out_Tex->m_TextureDesc.Sampler          = TextureSampler::Sampler2D;
		out_Tex->m_TextureDesc.Usage            = TextureUsage::ComputeOnly;
		out_Tex->m_TextureDesc.PixelDataFormat  = TexturePixelDataFormat::RG;
		out_Tex->m_TextureDesc.PixelDataType    = TexturePixelDataType::Float16;
		out_Tex->m_TextureDesc.Width            = l_resolution.x;
		out_Tex->m_TextureDesc.Height           = l_resolution.y;
		out_Tex->m_TextureDesc.DepthOrArraySize = 1;
		out_Tex->m_CPUAccessibility             = Accessibility::Immutable;
		out_Tex->m_GPUAccessibility             = Accessibility::ReadWrite;
		l_texService->Initialize(out_Tex);
	};

	// Per-lobe current-frame radiance — single-buffered RGBA16F.
	l_makeRGBA16F("PTDenoiseTemporal_RadianceDiffuse",  m_RadianceDiffuse);
	l_makeRGBA16F("PTDenoiseTemporal_RadianceSpecular", m_RadianceSpecular);

	// Per-lobe history radiance — RGBA16F, ping-pong.
	l_makeRGBA16F("PTDenoiseTemporal_HistoryRadianceDiffuse_Even",  m_HistoryRadianceDiffuse_Even);
	l_makeRGBA16F("PTDenoiseTemporal_HistoryRadianceDiffuse_Odd",   m_HistoryRadianceDiffuse_Odd);
	l_makeRGBA16F("PTDenoiseTemporal_HistoryRadianceSpecular_Even", m_HistoryRadianceSpecular_Even);
	l_makeRGBA16F("PTDenoiseTemporal_HistoryRadianceSpecular_Odd",  m_HistoryRadianceSpecular_Odd);

	// Per-lobe history moments — RG16F, ping-pong.
	l_makeRG16F("PTDenoiseTemporal_HistoryMomentsDiffuse_Even",   m_HistoryMomentsDiffuse_Even);
	l_makeRG16F("PTDenoiseTemporal_HistoryMomentsDiffuse_Odd",    m_HistoryMomentsDiffuse_Odd);
	l_makeRG16F("PTDenoiseTemporal_HistoryMomentsSpecular_Even",  m_HistoryMomentsSpecular_Even);
	l_makeRG16F("PTDenoiseTemporal_HistoryMomentsSpecular_Odd",   m_HistoryMomentsSpecular_Odd);

	return true;
}
