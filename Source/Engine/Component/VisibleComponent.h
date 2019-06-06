#pragma once
#include "../common/InnoComponent.h"
#include "MeshDataComponent.h"
#include "MaterialDataComponent.h"
#include "PhysicsDataComponent.h"

using ModelPair = std::pair<MeshDataComponent*, MaterialDataComponent*>;
using ModelMap = std::unordered_map<MeshDataComponent*, MaterialDataComponent*>;

class VisibleComponent : public InnoComponent
{
public:
	VisibleComponent() {};
	~VisibleComponent() {};

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
