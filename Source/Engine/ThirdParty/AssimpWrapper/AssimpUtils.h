#pragma once
#include "../../Common/ComponentHeaders.h"
#include "nlohmann/json.hpp"
#include <assimp/matrix4x4.h>
#include <assimp/scene.h>

namespace Inno
{
	namespace AssimpUtils
	{
		void to_json(nlohmann::json& j, const aiMatrix4x4& m);
		void from_json(const nlohmann::json& j, aiMatrix4x4& m);
		
		void MergeTransformation(nlohmann::json& j, const aiNode* node);
		void DecomposeTransformation(nlohmann::json& j, const aiMatrix4x4& m);
	}
}
