#pragma once
#include "IPhysicsSystem.h"

INNO_CONCRETE InnoPhysicsSystem : INNO_IMPLEMENT IPhysicsSystem
{
public:
	INNO_CLASS_CONCRETE_NON_COPYABLE(InnoPhysicsSystem);

	INNO_SYSTEM_EXPORT bool setup() override;
	INNO_SYSTEM_EXPORT bool initialize() override;
	INNO_SYSTEM_EXPORT bool update() override;
	INNO_SYSTEM_EXPORT bool terminate() override;

	INNO_SYSTEM_EXPORT ObjectStatus getStatus() override;

	INNO_SYSTEM_EXPORT bool generatePhysicsDataComponent(MeshDataComponent* MDC) override;
	INNO_SYSTEM_EXPORT bool generatePhysicsDataComponent(VisibleComponent* VC) override;
	INNO_SYSTEM_EXPORT std::optional<std::vector<CullingDataPack>> getCullingDataPack() override;
	INNO_SYSTEM_EXPORT AABB getSceneAABB() override;
};
