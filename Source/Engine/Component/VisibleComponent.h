#pragma once
#include "../Common/InnoComponent.h"
#include "MeshDataComponent.h"
#include "MaterialDataComponent.h"

struct MeshMaterialPair
{
	MeshDataComponent* mesh;
	MaterialDataComponent* material;
};

using ModelIndex = ArrayRangeInfo;

class VisibleComponent : public InnoComponent
{
public:
	VisibilityType m_visibilityType = VisibilityType::Invisible;
	MeshUsageType m_meshUsageType = MeshUsageType::Static;
	MeshShapeType m_meshShapeType = MeshShapeType::Line;
	MeshPrimitiveTopology m_meshPrimitiveTopology = MeshPrimitiveTopology::Triangle;
	TextureWrapMethod m_textureWrapMethod = TextureWrapMethod::Repeat;

	std::string m_modelFileName;
	bool m_simulatePhysics = false;

	ModelIndex m_modelIndex;
	ArrayRangeInfo m_PDCIndex;
};
