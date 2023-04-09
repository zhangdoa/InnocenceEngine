#pragma once
#include "../Common/InnoObject.h"
#include "MeshComponent.h"
#include "MaterialComponent.h"

namespace Inno
{
	struct MeshMaterialPair
	{
		MeshComponent* mesh;
		MaterialComponent* material;
	};

	struct Model
	{
		ArrayRangeInfo meshMaterialPairs;
	};

	class VisibleComponent : public InnoComponent
	{
	public:
		static uint32_t GetTypeID() { return 2; };
		static const char* GetTypeName() { return "VisibleComponent"; };

		MeshPrimitiveTopology m_meshPrimitiveTopology = MeshPrimitiveTopology::Triangle;
		TextureWrapMethod m_textureWrapMethod = TextureWrapMethod::Repeat;

		MeshUsage m_meshUsage = MeshUsage::Static;
		MeshSource m_meshSource = MeshSource::Procedural;
		ProceduralMeshShape m_proceduralMeshShape = ProceduralMeshShape::Triangle;

		std::string m_modelFileName;
		bool m_simulatePhysics = false;

		Model* m_model;
	};
}