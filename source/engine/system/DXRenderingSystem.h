#pragma once
#include "component/EnvironmentRenderPassSingletonComponent.h"
#include "component/ShadowRenderPassSingletonComponent.h"
#include "component/GeometryRenderPassSingletonComponent.h"
#include "component/LightRenderPassSingletonComponent.h"
#include "component/FinalRenderPassSingletonComponent.h"
#include "component/RenderingSystemSingletonComponent.h"
#include "component/AssetSystemSingletonComponent.h"

#include "interface/IRenderingSystem.h"
#include "interface/IMemorySystem.h"
#include "interface/IGameSystem.h"
#include "interface/IAssetSystem.h"

extern IMemorySystem* g_pMemorySystem;
extern IAssetSystem* g_pAssetSystem;
extern IGameSystem* g_pGameSystem;

class DXRenderingSystem : public IRenderingSystem
{
public:
	DXRenderingSystem() {};
	~DXRenderingSystem() {};

	void setup() override;
	void initialize() override;
	void update() override;
	void shutdown() override;

	const objectStatus& getStatus() const override;

private:
	objectStatus m_objectStatus = objectStatus::SHUTDOWN;
};
