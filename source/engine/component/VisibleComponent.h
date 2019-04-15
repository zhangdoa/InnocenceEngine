#pragma once
#include "../common/InnoType.h"
#include "../common/InnoMath.h"
#include "MeshDataComponent.h"
#include "MaterialDataComponent.h"
#include "PhysicsDataComponent.h"

using ModelPair = std::pair<MeshDataComponent*, MaterialDataComponent*>;
using ModelMap = std::unordered_map<MeshDataComponent*, MaterialDataComponent*>;

class VisibleComponent
{
public:
	VisibleComponent() {};
	~VisibleComponent() {};

	ObjectStatus m_objectStatus = ObjectStatus::SHUTDOWN;
	EntityID m_parentEntity;
	unsigned int m_UUID = 0;

	VisiblilityType m_visiblilityType = VisiblilityType::INNO_INVISIBLE;
	MeshUsageType m_meshUsageType = MeshUsageType::STATIC;
	MeshShapeType m_meshShapeType = MeshShapeType::QUAD;
	MeshPrimitiveTopology m_meshPrimitiveTopology = MeshPrimitiveTopology::TRIANGLE;
	TextureWrapMethod m_textureWrapMethod = TextureWrapMethod::REPEAT;

	std::string m_modelFileName;

	ModelMap m_modelMap;

	bool m_simulatePhysics = false;
	PhysicsDataComponent* m_PDC = 0;
};
