#pragma once
#include "../common/InnoType.h"
#include "../exports/InnoSystem_Export.h"
#include "../common/InnoClassTemplate.h"
#include "../component/VisibleComponent.h"
#include "../component/MeshDataComponent.h"

struct CullingDataPack
{
	mat4 m;
	mat4 m_prev;
	mat4 normalMat;
	VisibleComponent* visibleComponent;
	MeshDataComponent* MDC;
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

	INNO_SYSTEM_EXPORT virtual PhysicsDataComponent* generatePhysicsDataComponent(const ModelMap& modelMap, const EntityID& entityID) = 0;
	INNO_SYSTEM_EXPORT virtual std::optional<std::vector<CullingDataPack>> getCullingDataPack() = 0;
	INNO_SYSTEM_EXPORT virtual AABB getSceneAABB() = 0;
};
