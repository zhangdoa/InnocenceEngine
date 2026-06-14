#include "ExampleRenderingClient_Internal.h"

#include "../../Engine/RenderGraph/RenderGraphService.h"
#include "../../Engine/Services/TextureResourceService.h"
#include "../../Engine/Services/SamplerResourceService.h"
#include "../../Engine/Services/GPUBufferResourceService.h"
#include "../../Engine/Services/RenderingConfigurationService.h"
#include "../../Engine/Common/GPUDataStructure.h"
#include "LightCullingConstants.h"

#include "../../Engine/Engine.h"

#include <random>
#include <cmath>

using namespace Inno;

namespace
{
	// TiledFrustumGenerationPass state shared between its init hook (computes the
	// tiled dispatch extent + sizes the frustum buffer) and its per-frame update
	// hook (uploads the extent the shader reads). Resize would re-derive these;
	// not handled offscreen (degraded-dev limitation).
	Inno::GPUBufferComponent* g_TiledFrustumDispatchParams = nullptr;
	Inno::Math::TVec4<uint32_t> g_TiledFrustumNumThreads;
	Inno::Math::TVec4<uint32_t> g_TiledFrustumNumThreadGroups;
	// SSAO kernel + noise CPU data. Persisted (not lambda-locals) because the
	// resource Initialize is DEFERRED — the upload memcpy reads these pointers
	// from the frame loop, long after the init hook returns.
	Inno::Array<Inno::Math::Vec4> g_SSAOKernel;
	Inno::Array<Inno::Math::Vec4> g_SSAONoise;
	Inno::GPUBufferComponent* g_LightCullingDispatchParams = nullptr;
	Inno::GPUBufferComponent* g_LightListIndexCounter = nullptr;
	Inno::Math::TVec4<uint32_t> g_LightCullingNumThreads;
	Inno::Math::TVec4<uint32_t> g_LightCullingNumThreadGroups;
}

namespace Inno
{
	// Named graph hooks own the residual CPU work a pure-data node can't express:
	// creating + filling imported resources, and per-frame uploads. Registering
	// them here (before LoadGraph) is what lets the passes be pure JSON nodes with
	// no C++ class.
	void ExampleRenderingClientImpl::RegisterGraphHooks()
	{
		auto l_graph = g_Engine->Get<RenderGraphService>();

		// OpaquePass imports a single Repeat-wrap sampler by name (the imperative
		// pass created it in Setup); the graph binds it as the pass's s0.
		l_graph->RegisterInitHook("OpaquePass", []()
		{
			auto l_samplerService = g_Engine->Get<SamplerResourceService>();
			auto l_sampler = l_samplerService->Add("OpaquePass");
			l_sampler->m_SamplerDesc.m_WrapMethodU = TextureWrapMethod::Repeat;
			l_sampler->m_SamplerDesc.m_WrapMethodV = TextureWrapMethod::Repeat;
			l_samplerService->Initialize(l_sampler);
		});

		// LightPass imports a single point (nearest) sampler by name — used to
		// sample the BRDF LUTs in the BSDF accumulator. Bound as the pass's s0.
		l_graph->RegisterInitHook("LightPass", []()
		{
			auto l_samplerService = g_Engine->Get<SamplerResourceService>();
			auto l_sampler = l_samplerService->Add("LightPass");
			l_sampler->m_SamplerDesc.m_MinFilterMethod = TextureFilterMethod::Nearest;
			l_sampler->m_SamplerDesc.m_MagFilterMethod = TextureFilterMethod::Nearest;
			l_samplerService->Initialize(l_sampler);
		});

		// SSAONoisePass imports a sample kernel (hemisphere distribution), a 4x4
		// random-rotation noise texture, and two samplers — all by name. Created +
		// filled once after the graph loads.
		l_graph->RegisterInitHook("SSAONoisePass", []()
		{
			auto l_samplerService = g_Engine->Get<SamplerResourceService>();
			l_samplerService->Add("SSAONoisePass");

			auto l_randomRotSampler = l_samplerService->Add("SSAONoisePass_RandomRot");
			l_randomRotSampler->m_SamplerDesc.m_MinFilterMethod = TextureFilterMethod::Nearest;
			l_randomRotSampler->m_SamplerDesc.m_MagFilterMethod = TextureFilterMethod::Nearest;
			l_randomRotSampler->m_SamplerDesc.m_WrapMethodU = TextureWrapMethod::Repeat;
			l_randomRotSampler->m_SamplerDesc.m_WrapMethodV = TextureWrapMethod::Repeat;

			std::uniform_real_distribution<float> l_randomFloats(0.0f, 1.0f);
			std::default_random_engine l_generator;

			constexpr uint32_t l_kernelSize = 32;
			g_SSAOKernel.clear();
			g_SSAOKernel.reserve(l_kernelSize);
			for (uint32_t i = 0; i < l_kernelSize; ++i)
			{
				auto l_sample = Math::Vec4(l_randomFloats(l_generator) * 2.0f - 1.0f, l_randomFloats(l_generator) * 2.0f - 1.0f, l_randomFloats(l_generator), 0.0f);
				l_sample = l_sample.normalize() * l_randomFloats(l_generator);
				float l_scale = float(i) / float(l_kernelSize);
				l_scale = 0.1f + 0.9f * (l_scale * l_scale);
				l_sample.x *= l_scale;
				l_sample.y *= l_scale;
				g_SSAOKernel.emplace_back(l_sample);
			}

			auto l_kernelBuffer = g_Engine->Get<GPUBufferResourceService>()->Add("SSAO_Kernel");
			l_kernelBuffer->m_GPUResourceType = GPUResourceType::Buffer;
			l_kernelBuffer->m_ElementSize = sizeof(Math::Vec4);
			l_kernelBuffer->m_ElementCount = l_kernelSize;
			l_kernelBuffer->m_InitialData = &g_SSAOKernel[0];

			constexpr int l_textureSize = 4;
			g_SSAONoise.clear();
			g_SSAONoise.reserve(l_textureSize * l_textureSize);
			for (int i = 0; i < l_textureSize * l_textureSize; ++i)
			{
				auto l_n = Math::Vec4(l_randomFloats(l_generator) * 2.0f - 1.0f, l_randomFloats(l_generator) * 2.0f - 1.0f, 0.0f, 0.0f);
				g_SSAONoise.push_back(l_n.normalize());
			}

			auto l_noiseTexture = g_Engine->Get<TextureResourceService>()->Add("SSAO_Noise");
			l_noiseTexture->m_TextureDesc.Sampler = TextureSampler::Sampler2D;
			l_noiseTexture->m_TextureDesc.Usage = TextureUsage::ComputeOnly;
			l_noiseTexture->m_TextureDesc.PixelDataFormat = TexturePixelDataFormat::RGBA;
			l_noiseTexture->m_TextureDesc.Width = l_textureSize;
			l_noiseTexture->m_TextureDesc.Height = l_textureSize;
			l_noiseTexture->m_TextureDesc.PixelDataType = TexturePixelDataType::Float32;

			l_samplerService->Initialize(l_samplerService->Find("SSAONoisePass"));
			l_samplerService->Initialize(l_randomRotSampler);
			g_Engine->Get<GPUBufferResourceService>()->Initialize(l_kernelBuffer);
			g_Engine->Get<TextureResourceService>()->Initialize(l_noiseTexture, &g_SSAONoise[0]);
		});

		// TiledFrustumGenerationPass imports a dispatch-params cbuffer (per-frame
		// upload) and a viewport-sized frustum buffer, both by name.
		l_graph->RegisterInitHook("TiledFrustumGenerationPass", []()
		{
			auto l_bufferService = g_Engine->Get<GPUBufferResourceService>();

			g_TiledFrustumDispatchParams = l_bufferService->Add("TiledFrustumDispatchParams");
			g_TiledFrustumDispatchParams->m_ElementCount = 1;
			g_TiledFrustumDispatchParams->m_ElementSize = sizeof(DispatchParamsConstantBuffer);
			g_TiledFrustumDispatchParams->m_GPUAccessibility = Accessibility::ReadOnly;
			l_bufferService->Initialize(g_TiledFrustumDispatchParams);

			const float l_tile = static_cast<float>(LightCulling::TILE_SIZE);
			auto l_vp = g_Engine->Get<RenderingConfigurationService>()->GetScreenResolution();
			uint32_t l_threadsX = static_cast<uint32_t>(std::ceil(l_vp.x / l_tile));
			uint32_t l_threadsY = static_cast<uint32_t>(std::ceil(l_vp.y / l_tile));
			g_TiledFrustumNumThreads = Math::TVec4<uint32_t>(l_threadsX, l_threadsY, 1, 0);
			g_TiledFrustumNumThreadGroups = Math::TVec4<uint32_t>(
				static_cast<uint32_t>(std::ceil(l_threadsX / l_tile)),
				static_cast<uint32_t>(std::ceil(l_threadsY / l_tile)), 1, 0);

			auto l_frustum = l_bufferService->Add("TiledFrustumGPUBuffer");
			l_frustum->m_GPUAccessibility = Accessibility::ReadWrite;
			l_frustum->m_ElementCount = l_threadsX * l_threadsY;
			l_frustum->m_ElementSize = 64; // 4 planes (float3 normal + float dist) per frustum
			l_bufferService->Initialize(l_frustum);
		});

		l_graph->RegisterUpdateHook("TiledFrustumGenerationPass", []()
		{
			DispatchParamsConstantBuffer l_workload;
			l_workload.numThreadGroups = g_TiledFrustumNumThreadGroups;
			l_workload.numThreads = g_TiledFrustumNumThreads;
			g_Engine->Get<GPUBufferResourceService>()->Upload(g_TiledFrustumDispatchParams, &l_workload, 0, 1);
		});

		l_graph->RegisterInitHook("LightCullingPass", []()
		{
			auto l_bufferService = g_Engine->Get<GPUBufferResourceService>();

			g_LightCullingDispatchParams = l_bufferService->Add("LightCullingDispatchParams");
			g_LightCullingDispatchParams->m_ElementCount = 1;
			g_LightCullingDispatchParams->m_ElementSize = sizeof(DispatchParamsConstantBuffer);
			g_LightCullingDispatchParams->m_GPUAccessibility = Accessibility::ReadOnly;
			l_bufferService->Initialize(g_LightCullingDispatchParams);

			g_LightListIndexCounter = l_bufferService->Add("LightListIndexCounter");
			g_LightListIndexCounter->m_GPUAccessibility = Accessibility::ReadWrite;
			g_LightListIndexCounter->m_ElementCount = 1;
			g_LightListIndexCounter->m_ElementSize = sizeof(uint32_t);
			static uint32_t s_initialIndexCount = 1;
			g_LightListIndexCounter->m_InitialData = &s_initialIndexCount;
			l_bufferService->Initialize(g_LightListIndexCounter);

			auto l_samplerService = g_Engine->Get<SamplerResourceService>();
			auto l_sampler = l_samplerService->Add("LightCullingPass");
			l_samplerService->Initialize(l_sampler);

			const float l_tile = static_cast<float>(LightCulling::TILE_SIZE);
			auto l_vp = g_Engine->Get<RenderingConfigurationService>()->GetScreenResolution();
			uint32_t l_threadsX = static_cast<uint32_t>(std::ceil(l_vp.x / l_tile));
			uint32_t l_threadsY = static_cast<uint32_t>(std::ceil(l_vp.y / l_tile));
			g_LightCullingNumThreads = Math::TVec4<uint32_t>(l_threadsX, l_threadsY, 1, 0);
			g_LightCullingNumThreadGroups = Math::TVec4<uint32_t>(
				static_cast<uint32_t>(std::ceil(l_threadsX / l_tile)),
				static_cast<uint32_t>(std::ceil(l_threadsY / l_tile)), 1, 0);
		});

		l_graph->RegisterUpdateHook("LightCullingPass", []()
		{
			static uint32_t s_resetValue = 1;
			g_Engine->Get<GPUBufferResourceService>()->Upload(g_LightListIndexCounter, &s_resetValue);

			DispatchParamsConstantBuffer l_workload;
			l_workload.numThreadGroups = g_LightCullingNumThreadGroups;
			l_workload.numThreads = g_LightCullingNumThreads;
			g_Engine->Get<GPUBufferResourceService>()->Upload(g_LightCullingDispatchParams, &l_workload, 0, 1);
		});
	}
}
