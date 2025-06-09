#pragma once
#include "../../Common/ComponentHeaders.h"

struct aiMaterial;

namespace Inno
{
	namespace AssimpMaterialProcessor
	{
		// Create and save MaterialComponent directly - returns pointer for linking
		MaterialComponent* CreateMaterialComponent(const aiMaterial* material, const char* exportName);
		
		// Process material properties from Assimp material
		void ProcessMaterialProperties(const aiMaterial* material, MaterialComponent* materialComponent);
		
		// Process textures and link to material
		void ProcessMaterialTextures(const aiMaterial* material, const char* exportName, MaterialComponent* materialComponent);
	}
}
