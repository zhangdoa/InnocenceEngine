#pragma once
#include "../../Engine/Interface/IRenderPass.h"

namespace Inno
{
	class RadianceCacheRaytracingPass : public IRenderPass
	{
	public:
		INNO_CLASS_SINGLETON(RadianceCacheRaytracingPass)

		bool Setup(IServiceConfig* systemConfig = nullptr) override;
		bool Initialize() override;
		bool Terminate() override;
		ObjectStatus GetStatus() override;

		bool PrepareCommandList(IRenderingContext* renderingContext = nullptr) override;
		RenderPassComponent* GetRenderPassComp() override;

	private:
		const uint32_t TILE_SIZE = 8;
		// GI-1.0 §2.1.1 sparse spawn: ray budget reduced by ξ_x·ξ_y; these
		// match upscaleFactor in RayTracingTypes.hlsl. Spawn-tile size is
		// what the ray-gen dispatch grid actually iterates — one thread
		// per spawn tile, one probe spawned per frame, Halton-picked pixel.
		const uint32_t UPSCALE_X = 2;
		const uint32_t UPSCALE_Y = 2;
		const uint32_t SPAWN_TILE_SIZE_X = TILE_SIZE * UPSCALE_X;
		const uint32_t SPAWN_TILE_SIZE_Y = TILE_SIZE * UPSCALE_Y;

		ObjectStatus m_ObjectStatus;
		RenderPassComponent* m_RenderPassComp;
		ShaderProgramComponent* m_ShaderProgramComp;
		SamplerComponent* m_SamplerComp;

		ShaderStage m_ShaderStage;

		bool RenderTargetsCreationFunc();		
	};
} // namespace Inno
