#include "GPUPathTracerPass.h"
#include "HashGridCacheConstants.h"
#include "PTDenoiseConstants.h"

#include "../../Engine/Services/RenderingConfigurationService.h"
#include "../../Engine/Services/FrameManagementService.h"
#include "../../Engine/Services/ShaderProgramResourceService.h"
#include "../../Engine/Services/RenderPassResourceService.h"
#include "../../Engine/Services/TextureResourceService.h"
#include "../../Engine/Services/GPUBufferResourceService.h"
#include "../../Engine/Services/SamplerResourceService.h"
#include "../../Engine/Services/CommandListResourceService.h"
#include "../../Engine/Engine.h"

using namespace Inno;

bool GPUPathTracerPass::Terminate()
{
	auto l_fmService = g_Engine->Get<FrameManagementService>();

	if (m_MaterialBuffer)
		g_Engine->Get<GPUBufferResourceService>()->Delete(m_MaterialBuffer);

	if (m_FrameCountCB)
		g_Engine->Get<GPUBufferResourceService>()->Delete(m_FrameCountCB);
	if (m_LightCountCB)
		g_Engine->Get<GPUBufferResourceService>()->Delete(m_LightCountCB);
	if (m_AccumulationBuffer)
		g_Engine->Get<TextureResourceService>()->Delete(m_AccumulationBuffer);

	if constexpr (Inno::PTHashGridCache::ENABLED)
	{
		auto l_bufService = g_Engine->Get<GPUBufferResourceService>();
		if (m_HashGridCache_ValueIndirectBuffer)        l_bufService->Delete(m_HashGridCache_ValueIndirectBuffer);
		if (m_HashGridCache_UpdateCellValueIndirectBuffer) l_bufService->Delete(m_HashGridCache_UpdateCellValueIndirectBuffer);
		if (m_HashGridCache_ValueBuffer)            l_bufService->Delete(m_HashGridCache_ValueBuffer);
		if (m_HashGridCache_UpdateCellValueBuffer)  l_bufService->Delete(m_HashGridCache_UpdateCellValueBuffer);
		if (m_HashGridCache_DecayTileBuffer)        l_bufService->Delete(m_HashGridCache_DecayTileBuffer);
		if (m_HashGridCache_HashBuffer)             l_bufService->Delete(m_HashGridCache_HashBuffer);
		if (m_HashGridCacheCB)                      l_bufService->Delete(m_HashGridCacheCB);
	}

	if constexpr (Inno::PTDenoise::ENABLED)
	{
		DeletePTGBufferTextures();
	}

	g_Engine->Get<CommandListResourceService>()->Delete(m_CommandListComp_Compute);
	g_Engine->Get<CommandListResourceService>()->Delete(m_CommandListComp_Graphics);
	g_Engine->Get<RenderPassResourceService>()->Delete(m_RayTracingRenderPassComp);
	g_Engine->Get<ShaderProgramResourceService>()->Delete(m_RayTracingSPC);
	if (m_MaterialSampler)
		g_Engine->Get<SamplerResourceService>()->Delete(m_MaterialSampler);

	m_ObjectStatus = ObjectStatus::Terminated;

	return true;
}

ObjectStatus GPUPathTracerPass::GetStatus()
{
	return m_ObjectStatus;
}

RenderPassComponent* GPUPathTracerPass::GetRenderPassComp()
{
	return m_RayTracingRenderPassComp;
}

GPUResourceComponent* GPUPathTracerPass::GetResult()
{
	return m_AccumulationBuffer;
}

void GPUPathTracerPass::ResetAccumulation()
{
	m_FrameCount = 1;
}

void GPUPathTracerPass::CreateAccumulationBuffer()
{
	auto l_resolution = g_Engine->Get<RenderingConfigurationService>()->GetScreenResolution();
	auto l_texService = g_Engine->Get<TextureResourceService>();

	m_AccumulationBuffer = l_texService->Add("GPUPathTracerAccumBuffer");
	m_AccumulationBuffer->m_TextureDesc.Sampler          = TextureSampler::Sampler2D;
	m_AccumulationBuffer->m_TextureDesc.Usage            = TextureUsage::ComputeOnly;
	m_AccumulationBuffer->m_TextureDesc.PixelDataFormat  = TexturePixelDataFormat::RGBA;
	m_AccumulationBuffer->m_TextureDesc.PixelDataType    = TexturePixelDataType::Float32;
	m_AccumulationBuffer->m_TextureDesc.Width            = l_resolution.x;
	m_AccumulationBuffer->m_TextureDesc.Height           = l_resolution.y;
	m_AccumulationBuffer->m_TextureDesc.DepthOrArraySize = 1;
	m_AccumulationBuffer->m_CPUAccessibility             = Accessibility::Immutable;
	m_AccumulationBuffer->m_GPUAccessibility             = Accessibility::ReadWrite;
	l_texService->Initialize(m_AccumulationBuffer);
}

// Accumulation history is dropped because sample counts across two resolutions can't be combined.
void GPUPathTracerPass::OnResize()
{
	auto l_texService = g_Engine->Get<TextureResourceService>();
	if (m_AccumulationBuffer)
	{
		l_texService->Delete(m_AccumulationBuffer);
		m_AccumulationBuffer = nullptr;
	}
	CreateAccumulationBuffer();

	if constexpr (Inno::PTDenoise::ENABLED)
	{
		DeletePTGBufferTextures();
		CreatePTGBufferTextures();
	}

	ResetAccumulation();
}

// RGBA16F matches the rasterizer GBuffer precision so DecodeGBuffer reads PT-mode textures with
// the same contract as raster mode.
void GPUPathTracerPass::CreatePTGBufferTextures()
{
	auto l_resolution = g_Engine->Get<RenderingConfigurationService>()->GetScreenResolution();
	auto l_texService = g_Engine->Get<TextureResourceService>();

	auto l_create = [&](const char* in_Name, TextureComponent*& out_Tex)
	{
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

	l_create("PTDenoise_GBuffer_Position",        m_PTGBuffer_Position);
	l_create("PTDenoise_GBuffer_NormalMetalness", m_PTGBuffer_NormalMetalness);
	l_create("PTDenoise_GBuffer_AlbedoRoughness", m_PTGBuffer_AlbedoRoughness);
	l_create("PTDenoise_GBuffer_MotionHitDist",   m_PTGBuffer_MotionHitDist);
	l_create("PTDenoise_RadianceDiffuse",         m_PTRadianceDiffuse);
	l_create("PTDenoise_RadianceSpecular",        m_PTRadianceSpecular);
}

void GPUPathTracerPass::DeletePTGBufferTextures()
{
	auto l_texService = g_Engine->Get<TextureResourceService>();
	auto l_drop = [&](TextureComponent*& tex) { if (tex) { l_texService->Delete(tex); tex = nullptr; } };
	l_drop(m_PTRadianceSpecular);
	l_drop(m_PTRadianceDiffuse);
	l_drop(m_PTGBuffer_MotionHitDist);
	l_drop(m_PTGBuffer_AlbedoRoughness);
	l_drop(m_PTGBuffer_NormalMetalness);
	l_drop(m_PTGBuffer_Position);
}
