#pragma once
#include "ISystem.h"

#include "../Component/PhysicsDataComponent.h"
#include "../Common/GPUDataStructure.h"

namespace Inno
{
	struct CullingData
	{
		Mat4 m = Mat4();
		Mat4 m_prev = Mat4();
		Mat4 normalMat = Mat4();
		MeshDataComponent* mesh = 0;
		MaterialDataComponent* material = 0;
		MeshUsage meshUsage = MeshUsage::Invalid;
		VisibilityMask visibilityMask = VisibilityMask::Invalid;
		uint64_t UUID = 0;
	};

	struct BVHNode
	{
		AABB m_AABB;

		std::vector<BVHNode>::iterator parentNode;
		std::vector<BVHNode>::iterator leftChildNode;
		std::vector<BVHNode>::iterator rightChildNode;
		size_t depth = 0;

		PhysicsDataComponent* PDC = 0;

		bool operator==(const BVHNode& other) const
		{
			return (
				parentNode == other.parentNode
				&& leftChildNode == other.leftChildNode
				&& rightChildNode == other.rightChildNode
				);
		}
	};

	class IPhysicsSystem : public IComponentSystem
	{
	public:
		INNO_CLASS_INTERFACE_NON_COPYABLE(IPhysicsSystem);

		virtual bool generatePhysicsProxy(VisibleComponent* VC) = 0;
		virtual void updateBVH() = 0;
		virtual void updateCulling() = 0;
		virtual const std::vector<CullingData>& getCullingData() = 0;
		virtual AABB getVisibleSceneAABB() = 0;
		virtual AABB getStaticSceneAABB() = 0;
		virtual AABB getTotalSceneAABB() = 0;
		virtual const std::vector<BVHNode>& getBVHNodes() = 0;

		virtual bool addForce(VisibleComponent* VC, Vec4 force) = 0;
	};
}