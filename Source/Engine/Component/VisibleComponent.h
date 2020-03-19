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
	MeshPrimitiveTopology m_meshPrimitiveTopology = MeshPrimitiveTopology::Triangle;
	TextureWrapMethod m_textureWrapMethod = TextureWrapMethod::Repeat;

	MeshUsage m_meshUsage = MeshUsage::Static;
	MeshSource m_meshSource = MeshSource::Procedural;
	ProceduralMeshShape m_proceduralMeshShape = ProceduralMeshShape::Triangle;

	std::string m_modelFileName;
	bool m_simulatePhysics = false;

	Model* m_model;
};
