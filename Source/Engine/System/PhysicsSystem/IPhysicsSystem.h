#pragma once
#include "../../Common/InnoType.h"
#include "../../Common/stl17.h"

#include "../../Common/InnoClassTemplate.h"
#include "../../Component/VisibleComponent.h"
#include "../../Component/MeshDataComponent.h"

struct CullingDataPack
{
	mat4 m;
	mat4 m_prev;
	mat4 normalMat;
	MeshDataComponent* mesh;
	MaterialDataComponent* material;
	VisiblilityType visiblilityType;
	unsigned int UUID;
};

INNO_INTERFACE IPhysicsSystem
{
public:
	INNO_CLASS_INTERFACE_NON_COPYABLE(IPhysicsSystem);

	virtual bool setup() = 0;
	virtual bool initialize() = 0;
	virtual bool update() = 0;
	virtual bool terminate() = 0;

	virtual ObjectStatus getStatus() = 0;

	virtual bool generatePhysicsDataComponent(MeshDataComponent* MDC) = 0;
	virtual bool generatePhysicsDataComponent(VisibleComponent* VC) = 0;
	virtual std::optional<std::vector<CullingDataPack>> getCullingDataPack() = 0;
	virtual AABB getSceneAABB() = 0;
};
