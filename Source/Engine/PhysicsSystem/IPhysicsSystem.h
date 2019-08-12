#pragma once
#include "../Common/InnoType.h"
#include "../Common/STL17.h"

#include "../Common/InnoClassTemplate.h"
#include "../Component/VisibleComponent.h"
#include "../Component/MeshDataComponent.h"

struct CullingDataPack
{
	mat4 m;
	mat4 m_prev;
	mat4 normalMat;
	MeshDataComponent* mesh;
	MaterialDataComponent* material;
	VisiblilityType visiblilityType;
	MeshUsageType meshUsageType;
	unsigned int UUID;
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

	virtual bool generatePhysicsDataComponent(MeshDataComponent* MDC) = 0;
	virtual bool generateAABBInWorldSpace(PhysicsDataComponent* PDC, const mat4& m) = 0;
	virtual bool generatePhysicsProxy(VisibleComponent* VC) = 0;
	virtual void updateCulling() = 0;
	virtual std::optional<std::vector<CullingDataPack>> getCullingDataPack() = 0;
	virtual AABB getVisibleSceneAABB() = 0;
	virtual AABB getTotalSceneAABB() = 0;
};
