#pragma once
#include "../Common/InnoComponent.h"
#include "MeshDataComponent.h"
#include "MaterialDataComponent.h"

struct MeshMaterialPair
{
	MeshDataComponent* mesh;
	MaterialDataComponent* material;
};

struct Model
{
	ArrayRangeInfo meshMaterialPairs;
};

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

	Model* m_model;
	ArrayRangeInfo m_PDCIndex;
};
