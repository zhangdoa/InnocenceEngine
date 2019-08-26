#pragma once
#include "../Common/InnoComponent.h"
#include "MeshDataComponent.h"
#include "MaterialDataComponent.h"

using ModelPair = std::pair<MeshDataComponent*, MaterialDataComponent*>;
using ModelMap = std::unordered_map<MeshDataComponent*, MaterialDataComponent*>;

class PhysicsDataComponent;
class VisibleComponent : public InnoComponent
{
public:
	VisiblilityType m_visiblilityType = VisiblilityType::Invisible;
	MeshUsageType m_meshUsageType = MeshUsageType::Static;
	MeshShapeType m_meshShapeType = MeshShapeType::Line;
	MeshPrimitiveTopology m_meshPrimitiveTopology = MeshPrimitiveTopology::Triangle;
	TextureWrapMethod m_textureWrapMethod = TextureWrapMethod::Repeat;

	std::string m_modelFileName;
	bool m_simulatePhysics = false;

	ModelMap m_modelMap;
	std::vector<PhysicsDataComponent*> m_PDCs;
};
