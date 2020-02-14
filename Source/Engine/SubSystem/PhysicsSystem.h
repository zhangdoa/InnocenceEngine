#pragma once
#include "../Interface/IPhysicsSystem.h"

class InnoPhysicsSystem : public IPhysicsSystem
{
public:
	INNO_CLASS_CONCRETE_NON_COPYABLE(InnoPhysicsSystem);

	bool setup() override;
	bool initialize() override;
	bool update() override;
	bool terminate() override;

	ObjectStatus getStatus() override;

	uint64_t getCurrentPhysicsDataComponentOffset() override;
	PhysicsDataComponent* generatePhysicsDataComponent(const MeshMaterialPair& meshMaterialPair) override;
	bool generateAABBInWorldSpace(PhysicsDataComponent* PDC, const Mat4& m) override;
	bool generatePhysicsProxy(VisibleComponent* VC) override;
	void updateBVH() override;
	void updateCulling() override;
	const std::vector<CullingData>& getCullingData() override;
	AABB getVisibleSceneAABB() override;
	AABB getStaticSceneAABB() override;
	AABB getTotalSceneAABB() override;
	BVHNode* getRootBVHNode() override;
};
