#pragma once
#include "../Common/InnoComponent.h"
#include "MeshDataComponent.h"
#include "MaterialDataComponent.h"
#include "PhysicsDataComponent.h"

using ModelPair = std::pair<MeshDataComponent*, MaterialDataComponent*>;
using ModelMap = std::unordered_map<MeshDataComponent*, MaterialDataComponent*>;

class VisibleComponent : public InnoComponent
{
public:
	VisiblilityType m_visiblilityType = VisiblilityType::Invisible;
	MeshUsageType m_meshUsageType = MeshUsageType::Static;
	MeshShapeType m_meshShapeType = MeshShapeType::Line;
	MeshPrimitiveTopology m_meshPrimitiveTopology = MeshPrimitiveTopology::Triangle;
	TextureWrapMethod m_textureWrapMethod = TextureWrapMethod::Repeat;

	std::string m_modelFileName;

	ModelMap m_modelMap;

	bool m_simulatePhysics = false;
	PhysicsDataComponent* m_PDC = 0;
};
