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
	EntityID m_parentEntity = 0;

	VisiblilityType m_visiblilityType = VisiblilityType::INNO_INVISIBLE;
	MeshShapeType m_meshShapeType = MeshShapeType::QUAD;
	MeshPrimitiveTopology m_meshDrawMethod = MeshPrimitiveTopology::TRIANGLE;
	TextureWrapMethod m_textureWrapMethod = TextureWrapMethod::REPEAT;

	bool m_drawAABB = false;

	std::string m_modelFileName;

	ModelMap m_modelMap;

	PhysicsDataComponent* m_PhysicsDataComponent = 0;
};

