#pragma once
#include "../Interface/IPhysicsSystem.h"

namespace Inno
{
	class PhysicsSystem : public IPhysicsSystem
	{
	public:
		INNO_CLASS_CONCRETE_NON_COPYABLE(PhysicsSystem);

		bool Setup(ISystemConfig* systemConfig) override;
		bool Initialize() override;
		bool Update() override;
		bool Terminate() override;

		ObjectStatus GetStatus() override;

		bool generatePhysicsProxy(VisibleComponent* VC) override;
		void updateBVH() override;
		void updateCulling() override;
		const std::vector<CullingData>& getCullingData() override;
		AABB getVisibleSceneAABB() override;
		AABB getStaticSceneAABB() override;
		AABB getTotalSceneAABB() override;
		const std::vector<BVHNode>& getBVHNodes() override;

		bool addForce(VisibleComponent* VC, Vec4 force) override;
	};
}
