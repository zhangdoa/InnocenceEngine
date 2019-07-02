#pragma once
#include "IPhysicsSystem.h"

INNO_CONCRETE InnoPhysicsSystem : INNO_IMPLEMENT IPhysicsSystem
{
public:
	INNO_CLASS_CONCRETE_NON_COPYABLE(InnoPhysicsSystem);

	bool setup() override;
	bool initialize() override;
	bool update() override;
	bool terminate() override;

	ObjectStatus getStatus() override;

	bool generatePhysicsDataComponent(MeshDataComponent* MDC) override;
	bool generatePhysicsDataComponent(VisibleComponent* VC) override;
	std::optional<std::vector<CullingDataPack>> getCullingDataPack() override;
	AABB getVisibleSceneAABB() override;
	AABB getTotalSceneAABB() override;
};
