#pragma once
#include "IPhysicsSystem.h"

class InnoPhysicsSystem : INNO_IMPLEMENT IPhysicsSystem
{
public:
	INNO_CLASS_CONCRETE_NON_COPYABLE(InnoPhysicsSystem);

	INNO_SYSTEM_EXPORT bool setup() override;
	INNO_SYSTEM_EXPORT bool initialize() override;
	INNO_SYSTEM_EXPORT bool update() override;
	INNO_SYSTEM_EXPORT bool terminate() override;

	INNO_SYSTEM_EXPORT ObjectStatus getStatus() override;

	INNO_SYSTEM_EXPORT PhysicsDataComponent* generatePhysicsDataComponent(const ModelMap& modelMap, const EntityID& entityID) override;
	INNO_SYSTEM_EXPORT std::optional<std::vector<CullingDataPack>> getCullingDataPack() override;
	INNO_SYSTEM_EXPORT AABB getSceneAABB() override;
};
