#pragma once
#include "../../common/InnoType.h"

#include "json/json.hpp"
using json = nlohmann::json;

#include "../../common/ComponentHeaders.h"

INNO_PRIVATE_SCOPE InnoFileSystemNS
{
	INNO_PRIVATE_SCOPE JSONParser
	{
		bool loadJsonDataFromDisk(const std::string & fileName, json & data);
		bool saveJsonDataToDisk(const std::string & fileName, const json & data);

		void to_json(json& j, const EntityNamePair& p);

		void to_json(json& j, const TransformComponent& p);
		void to_json(json& j, const TransformVector& p);
		void to_json(json& j, const VisibleComponent& p);
		void to_json(json& j, const vec4& p);
		void to_json(json& j, const DirectionalLightComponent& p);
		void to_json(json& j, const PointLightComponent& p);
		void to_json(json& j, const SphereLightComponent& p);
		void to_json(json& j, const CameraComponent& p);
		void to_json(json& j, const EnvironmentCaptureComponent& p);

		void from_json(const json& j, TransformComponent& p);
		void from_json(const json& j, TransformVector& p);
		void from_json(const json& j, VisibleComponent& p);
		void from_json(const json& j, vec4& p);
		void from_json(const json& j, DirectionalLightComponent& p);
		void from_json(const json& j, PointLightComponent& p);
		void from_json(const json& j, SphereLightComponent& p);
		void from_json(const json& j, CameraComponent& p);
		void from_json(const json& j, EnvironmentCaptureComponent& p);

		template<typename T>
		inline bool loadComponentData(const json& j, const EntityID& entityID)
		{
			auto l_result = g_pCoreSystem->getGameSystem()->spawn<T>(entityID);

			from_json(j, *l_result);

			return true;
		}

		template<typename T>
		inline bool saveComponentData(json& topLevel, T* rhs)
		{
			json j;
			to_json(j, *rhs);

			auto result = std::find_if(
				topLevel["SceneEntities"].begin(),
				topLevel["SceneEntities"].end(),
				[&](auto& val) -> bool {
				return val["EntityID"] == rhs->m_parentEntity.c_str();
			});

			if (result != topLevel["SceneEntities"].end())
			{
				result.value()["ChildrenComponents"].emplace_back(j);
				return true;
			}
			else
			{
				g_pCoreSystem->getLogSystem()->printLog(LogType::INNO_WARNING, "FileSystem: saveComponentData<T>: Entity ID " + std::string(rhs->m_parentEntity.c_str()) + " is invalid.");
				return false;
			}
		}

		ModelMap loadModelFromDisk(const std::string & fileName);

		bool saveScene(const std::string& fileName);
		bool loadScene(const std::string& fileName);
	}
}
