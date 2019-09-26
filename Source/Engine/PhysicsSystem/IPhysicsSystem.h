#pragma once
#include "../Common/InnoType.h"

#include "../Common/InnoClassTemplate.h"
#include "../Component/VisibleComponent.h"
#include "../Component/MeshDataComponent.h"

enum class CullingDataChannel {
	Shadow = 1, MainCamera = 2, All = Shadow | MainCamera
};

struct CullingData
{
	Mat4 m;
	Mat4 m_prev;
	Mat4 normalMat;
	MeshDataComponent* mesh;
	MaterialDataComponent* material;
	VisiblilityType visiblilityType;
	MeshUsageType meshUsageType;
	CullingDataChannel cullingDataChannel;
	uint64_t UUID;
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

	virtual PhysicsDataComponent* generatePhysicsDataComponent(const ModelPair& modelPair) = 0;
	virtual bool generateAABBInWorldSpace(PhysicsDataComponent* PDC, const Mat4& m) = 0;
	virtual bool generatePhysicsProxy(VisibleComponent* VC) = 0;
	virtual void updateBVH() = 0;
	virtual void updateCulling() = 0;
	virtual const std::vector<CullingData>& getCullingData() = 0;
	virtual AABB getVisibleSceneAABB() = 0;
	virtual AABB getStaticSceneAABB() = 0;
	virtual AABB getTotalSceneAABB() = 0;
	virtual BVHNode* getRootBVHNode() = 0;
};
