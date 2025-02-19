#include "RenderingConfigurationService.h"

#include "../Engine.h"
using namespace Inno;

RenderingConfigurationService::RenderingConfigurationService()
{
	m_renderingConfig.useMotionBlur = true;
	m_renderingConfig.useTAA = true;
	//m_renderingConfig.useBloom = true;
	m_renderingConfig.drawSky = true;
	//m_renderingConfig.drawTerrain = true;
	//m_renderingConfig.drawDebugObject = true;
	m_renderingConfig.CSMFitToScene = true;
	m_renderingConfig.CSMAdjustDrawDistance = true;
	m_renderingConfig.CSMAdjustSidePlane = false;

	m_renderingCapability.maxCSMSplits = 4;
	m_renderingCapability.maxPointLights = 1024;
	m_renderingCapability.maxSphereLights = 128;
	m_renderingCapability.maxMeshes = 1024;
	m_renderingCapability.maxTextures = 2048;
	m_renderingCapability.maxMaterials = 4096;
	m_renderingCapability.maxBuffers = 512;

	m_DefaultRenderPassDesc.m_UseMultiFrames = false;
	m_DefaultRenderPassDesc.m_RenderTargetCount = 1;
	m_DefaultRenderPassDesc.m_RenderTargetDesc.Sampler = TextureSampler::Sampler2D;
	m_DefaultRenderPassDesc.m_RenderTargetDesc.IsMultiBuffer = true;
	m_DefaultRenderPassDesc.m_RenderTargetDesc.Usage = TextureUsage::ColorAttachment;
	m_DefaultRenderPassDesc.m_RenderTargetDesc.PixelDataFormat = TexturePixelDataFormat::RGBA;
	m_DefaultRenderPassDesc.m_RenderTargetDesc.Width = m_screenResolution.x;
	m_DefaultRenderPassDesc.m_RenderTargetDesc.Height = m_screenResolution.y;
	m_DefaultRenderPassDesc.m_RenderTargetDesc.DepthOrArraySize = 1;
	m_DefaultRenderPassDesc.m_RenderTargetDesc.PixelDataType = TexturePixelDataType::Float16;

	m_DefaultRenderPassDesc.m_GraphicsPipelineDesc.m_ViewportDesc.m_Width = (float)m_screenResolution.x;
	m_DefaultRenderPassDesc.m_GraphicsPipelineDesc.m_ViewportDesc.m_Height = (float)m_screenResolution.y;
}

TVec2<uint32_t> RenderingConfigurationService::GetScreenResolution()
{
	return m_screenResolution;
}

void RenderingConfigurationService::SetScreenResolution(TVec2<uint32_t> screenResolution)
{
	if (screenResolution.x == 0 || screenResolution.y == 0)
	{
		Log(Warning, "Trying to set a screen resolution with 0 width or height.");
		return;
	}

	m_screenResolution = screenResolution;
	m_DefaultRenderPassDesc.m_RenderTargetDesc.Width = m_screenResolution.x;
	m_DefaultRenderPassDesc.m_RenderTargetDesc.Height = m_screenResolution.y;
	m_DefaultRenderPassDesc.m_GraphicsPipelineDesc.m_ViewportDesc.m_Width = (float)m_screenResolution.x;
	m_DefaultRenderPassDesc.m_GraphicsPipelineDesc.m_ViewportDesc.m_Height = (float)m_screenResolution.y;
}

RenderingConfig RenderingConfigurationService::GetRenderingConfig()
{
	return m_renderingConfig;
}

void RenderingConfigurationService::SetRenderingConfig(RenderingConfig renderingConfig)
{
	m_renderingConfig = renderingConfig;
}

RenderingCapability RenderingConfigurationService::GetRenderingCapability()
{
	return m_renderingCapability;
}

RenderPassDesc RenderingConfigurationService::GetDefaultRenderPassDesc()
{
	return m_DefaultRenderPassDesc;
}