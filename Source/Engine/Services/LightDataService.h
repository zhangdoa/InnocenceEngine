#pragma once
#include "../Interface/IService.h"
#include "../Component/GPUBufferComponent.h"
#include "../Component/TextureComponent.h"

namespace Inno
{
	struct LightDataServiceImpl;
	class LightDataService : public IService
	{
	public:
		INNO_CLASS_CONCRETE_NON_COPYABLE(LightDataService);

		bool Setup(IServiceConfig* systemConfig) override;
		bool Initialize() override;
		bool Update() override;
		bool Terminate() override;

		ObjectStatus GetStatus() override;

		GPUBufferComponent* GetPointLightBuffer();
		GPUBufferComponent* GetSphereLightBuffer();
		GPUBufferComponent* GetCSMBuffer();
		GPUBufferComponent* GetGIBuffer();
		// Per shadow-casting point/sphere light, packed densely up to
		// RenderingCapability::maxPointShadows (TASK-66 / TASK-150). Resolver
		// indexes by the atlas slot stored in m_*LightAtlasSlot. May be empty
		// when no light has m_CastShadow=true; callers must guard size().
		GPUBufferComponent* GetPointShadowBuffer();
		// Texture2DArray, R32G32B32A32_FLOAT, DepthOrArraySize = maxPointShadows*6.
		// TASK-148's PointShadowGeometryProcessPass binds this as render target via
		// RenderTargetsCreationFunc; LightPass binds it as SRV for the resolver.
		TextureComponent* GetPointShadowAtlas();

		uint32_t GetPointLightCount();
		uint32_t GetSphereLightCount();
		uint32_t GetPointShadowCount();

		// Per-frame atlas-slot sidecar (TASK-149). Slot vectors are parallel-
		// indexed to the point/sphere CB vectors uploaded each Update(); a
		// returned INVALID_ATLAS_SLOT means the light is non-shadow-casting
		// (m_CastShadow == false) or was rejected by the slot allocator.
		// The slot allocator (TASK-147) writes valid indices over the
		// sentinel during UpdateLightData(); read-only thereafter.
		uint32_t GetPointLightAtlasSlot(uint32_t in_Index);
		uint32_t GetSphereLightAtlasSlot(uint32_t in_Index);

	private:
		LightDataServiceImpl* m_Impl;
	};
}
