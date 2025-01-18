#pragma once
#include "../Common/Object.h"
#include "MeshComponent.h"
#include "SkeletonComponent.h"
#include "MaterialComponent.h"

namespace Inno
{
	struct RenderableSet
	{
		MeshComponent* mesh;
		SkeletonComponent* skeleton;
		MaterialComponent* material;
	};

	struct Model
	{
		ArrayRangeInfo renderableSets;
	};

	class ModelComponent : public Component
	{
	public:
		static uint32_t GetTypeID() { return 2; };
		static const char* GetTypeName() { return "ModelComponent"; };

		MeshUsage m_meshUsage = MeshUsage::Static;
		MeshShape m_MeshShape = MeshShape::Customized;

		std::string m_modelFileName;
		bool m_simulatePhysics = false;

		Model* m_Model;
	};
}