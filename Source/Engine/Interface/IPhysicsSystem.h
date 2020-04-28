#pragma once
#include "../Common/InnoType.h"

#include "../Common/InnoClassTemplate.h"

#include "../Component/PhysicsDataComponent.h"
#include "../Common/GPUDataStructure.h"

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
	BVHNode* parentNode = 0;
	BVHNode* leftChildNode = 0;
	BVHNode* rightChildNode = 0;

	size_t depth = 0;

	PhysicsDataComponent* intermediatePDC;
	std::vector<PhysicsDataComponent*> childrenPDCs;
};

class IPhysicsSystem
{
public:
	INNO_CLASS_INTERFACE_NON_COPYABLE(IPhysicsSystem);

	virtual bool setup() = 0;
	virtual bool initialize() = 0;
	virtual bool update() = 0;
	virtual bool terminate() = 0;

	virtual ObjectStatus getStatus() = 0;

	virtual bool generatePhysicsProxy(VisibleComponent* VC) = 0;
	virtual void updateBVH() = 0;
	virtual void updateCulling() = 0;
	virtual const std::vector<CullingData>& getCullingData() = 0;
	virtual AABB getVisibleSceneAABB() = 0;
	virtual AABB getStaticSceneAABB() = 0;
	virtual AABB getTotalSceneAABB() = 0;
	virtual BVHNode* getRootBVHNode() = 0;

	virtual bool addForce(VisibleComponent* VC, Vec4 force) = 0;
};
