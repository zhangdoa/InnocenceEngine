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

// Called by FrameManagementService::PostResize after the GPU has been fully
// drained and RenderingConfigurationService holds the new resolution. The
// accumulation buffer must be re-sized (its dimensions drive DispatchRays,
// which already reads the current resolution each frame — a stale buffer
// there causes out-of-bounds UAV writes). Accumulation history is scrapped
// because sample counts across two resolutions can't be combined.
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

// PT screen-space denoiser GBuffer-equivalent + per-lobe radiance textures.
// RGBA16F to mirror the rasterizer GBuffer's float16 RGBA format
// (RenderingConfigurationService::m_DefaultRenderPassDesc) so DecodeGBuffer
// in common/lightPassCommon.hlsl reads them with the same precision in PT
// mode as in raster mode. Position carries 16F precision floor; same as the
// rasterizer ships, so the format-convert pass downstream gets a contract-
// equivalent input.
//
// CL-2 (TASK-77.4) collapsed the Even/Odd ping-pong introduced by TASK-77.2:
// NRD ReBLUR owns prev-frame reconstruction via motion vectors, so the
// engine never re-reads last frame's GBuffer-equivalent textures. 6 single-
// buffered RGBA16F textures (RT0..RT3 + per-lobe diffuse / specular).
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
