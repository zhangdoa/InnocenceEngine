#pragma once
#include "../Common/InnoType.h"

#include "json/json.hpp"
using json = nlohmann::json;

#include "../Common/ComponentHeaders.h"

namespace InnoFileSystemNS
{
	namespace JSONParser
	{
		bool loadJsonDataFromDisk(const std::string & fileName, json & data);
		bool saveJsonDataToDisk(const std::string & fileName, const json & data);

		void to_json(json& j, const InnoEntity& p);

		void to_json(json& j, const TransformComponent& p);
		void to_json(json& j, const TransformVector& p);
		void to_json(json& j, const VisibleComponent& p);
		void to_json(json& j, const vec4& p);
		void to_json(json& j, const DirectionalLightComponent& p);
		void to_json(json& j, const PointLightComponent& p);
		void to_json(json& j, const SpotLightComponent& p);
		void to_json(json& j, const SphereLightComponent& p);
		void to_json(json& j, const CameraComponent& p);

		void from_json(const json& j, TransformComponent& p);
		void from_json(const json& j, TransformVector& p);
		void from_json(const json& j, VisibleComponent& p);
		void from_json(const json& j, vec4& p);
		void from_json(const json& j, DirectionalLightComponent& p);
		void from_json(const json& j, PointLightComponent& p);
		void from_json(const json& j, SpotLightComponent& p);
		void from_json(const json& j, SphereLightComponent& p);
		void from_json(const json& j, CameraComponent& p);

		ModelMap loadModelFromDisk(const std::string & fileName, bool AsyncUploadGPUResource = true);

		bool saveScene(const std::string& fileName);
		bool loadScene(const std::string& fileName);
	}
}
