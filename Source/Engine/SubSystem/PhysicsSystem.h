#pragma once
#include "../Interface/IPhysicsSystem.h"

namespace Inno
{
	class InnoPhysicsSystem : public IPhysicsSystem
	{
	public:
		INNO_CLASS_CONCRETE_NON_COPYABLE(InnoPhysicsSystem);

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
		BVHNode* getRootBVHNode() override;

		bool addForce(VisibleComponent* VC, Vec4 force) override;
	};
}
