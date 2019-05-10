#pragma once
#include "../common/InnoType.h"
#include "../common/stl17.h"
#include "../exports/InnoSystem_Export.h"
#include "../common/InnoClassTemplate.h"
#include "../component/VisibleComponent.h"
#include "../component/MeshDataComponent.h"

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

	INNO_SYSTEM_EXPORT virtual bool setup() = 0;
	INNO_SYSTEM_EXPORT virtual bool initialize() = 0;
	INNO_SYSTEM_EXPORT virtual bool update() = 0;
	INNO_SYSTEM_EXPORT virtual bool terminate() = 0;

	INNO_SYSTEM_EXPORT virtual ObjectStatus getStatus() = 0;

	INNO_SYSTEM_EXPORT virtual bool generatePhysicsDataComponent(MeshDataComponent* MDC) = 0;
	INNO_SYSTEM_EXPORT virtual bool generatePhysicsDataComponent(VisibleComponent* VC) = 0;
	INNO_SYSTEM_EXPORT virtual std::optional<std::vector<CullingDataPack>> getCullingDataPack() = 0;
	INNO_SYSTEM_EXPORT virtual AABB getSceneAABB() = 0;
};
