#pragma once
#include "../common/InnoType.h"
#include "../common/InnoMath.h"
#include "MeshDataComponent.h"
#include "MaterialDataComponent.h"

using modelPair = std::pair<MeshDataComponent*, MaterialDataComponent*>;
using modelMap = std::unordered_map<MeshDataComponent*, MaterialDataComponent*>;

class VisibleComponent
{
public:
	VisibleComponent() {};
	~VisibleComponent() {};

	objectStatus m_objectStatus = objectStatus::SHUTDOWN;
	EntityID m_parentEntity;

	visiblilityType m_visiblilityType = visiblilityType::INVISIBLE;
	meshShapeType m_meshShapeType = meshShapeType::QUAD;
	meshDrawMethod m_meshDrawMethod = meshDrawMethod::TRIANGLE;
	textureWrapMethod m_textureWrapMethod = textureWrapMethod::REPEAT;

	bool m_drawAABB = false;
	EntityID m_AABBMeshID = 0;
	AABB m_AABB;

	std::string m_modelFileName;

	modelMap m_modelMap;
};

