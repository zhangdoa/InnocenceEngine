#pragma once
#include "../Component/PhysicsComponent.h"
#include "../Common/GPUDataStructure.h"
#include "CullingData.h"

namespace Inno
{
	struct BVHNode
	{
		AABB m_AABB;

		std::vector<BVHNode>::iterator parentNode;
		std::vector<BVHNode>::iterator leftChildNode;
		std::vector<BVHNode>::iterator rightChildNode;
		size_t depth = 0;

		PhysicsComponent* PDC = 0;

		bool operator==(const BVHNode& other) const
		{
			return (
				parentNode == other.parentNode
				&& leftChildNode == other.leftChildNode
				&& rightChildNode == other.rightChildNode
				);
		}
	};

	class PhysicsSystem : public ISystem
	{
	public:
		INNO_CLASS_CONCRETE_NON_COPYABLE(PhysicsSystem);

		bool Setup(ISystemConfig* systemConfig) override;
		bool Initialize() override;
		bool Update() override;
		bool Terminate() override;

		ObjectStatus GetStatus();

		bool generatePhysicsProxy(VisibleComponent* VC);
		void updateBVH();
		void updateCulling();
		const std::vector<CullingData>& getCullingData();
		AABB getVisibleSceneAABB();
		AABB getStaticSceneAABB();
		AABB getTotalSceneAABB();
		const std::vector<BVHNode>& getBVHNodes();

		bool addForce(VisibleComponent* VC, Vec4 force);
	};
}
