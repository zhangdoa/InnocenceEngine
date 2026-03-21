#pragma once
#include "../../Common/ComponentHeaders.h"

struct aiMaterial;

namespace Inno
{
	namespace AssimpMaterialProcessor
	{
		bool CreateMaterialComponent(const aiMaterial* Material, const char* BaseName, MaterialComponent& OutMaterial);

		void ProcessMaterialProperties(const aiMaterial* Material, MaterialComponent* MaterialComp);

		void ProcessMaterialTextures(const aiMaterial* Material, const char* BaseName, MaterialComponent* MaterialComp);
	}
}
