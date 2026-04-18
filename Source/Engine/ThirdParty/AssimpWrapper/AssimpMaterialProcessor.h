#pragma once
#include "../../Common/ComponentHeaders.h"
#include "../../Common/AssetData.h"

struct aiMaterial;

namespace Inno
{
	namespace AssimpMaterialProcessor
	{
		bool CreateMaterialComponent(const aiMaterial* Material, uint32_t MaterialIndex, const char* BaseName, const char* ModelBaseDir, MaterialComponent& OutMaterial);

		void ProcessMaterialProperties(const aiMaterial* Material, MaterialAssetData* AssetData);

		void ProcessMaterialTextures(const aiMaterial* Material, const char* BaseName, const char* ModelBaseDir, MaterialAssetData* AssetData);
	}
}
