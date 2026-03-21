#include "JSONWrapper.h"
#include "../../Common/ThreadSafeQueue.h"
#include "../../Common/LogService.h"
#include "../../Common/IOService.h"
#include "../../Common/TaskScheduler.h"

#include "../../Services/ComponentManager.h"
#include "../../Services/AnimationService.h"
#include "../../Services/AssetService.h"
// TODO Phase2-migrate: EntityManager.h still needed for SaveScene entity enumeration
#include "../../Services/EntityManager.h"
#include "../../Services/EntityRegistry.h"

#include "../../Engine.h"
using namespace Inno;

namespace Inno
{
	namespace JSONWrapper
	{
		// TODO Phase2-migrate: SaveComponentAndAddReference uses m_Owner->m_UUID (Entity*) — remove when Entity is deleted
		template<typename T>
		inline bool SaveComponentAndAddReference(json& topLevel, T* component)
		{
			std::string componentFileName = std::string(component->m_InstanceName.c_str());
			AssetService::Save(*component);

			auto l_result = std::find_if(
				topLevel["Entities"].begin(),
				topLevel["Entities"].end(),
				[&](auto val) -> bool {
					return val["UUID"] == component->m_Owner->m_UUID;
				});

			if (l_result != topLevel["Entities"].end())
			{
				json componentRef;
				componentRef["Type"] = T::GetTypeID();
				componentRef["Name"] = componentFileName;
				l_result.value()["Components"].emplace_back(componentRef);
				return true;
			}
			else
			{
				Log(Warning, "Entity UUID ", component->m_Owner->m_UUID, " is invalid.");
				return false;
			}
		}
	}
}

bool JSONWrapper::Load(const char* fileName, json& data)
{
	std::ifstream i;

	i.open(g_Engine->Get<IOService>()->getWorkingDirectory() + fileName);

	if (!i.is_open())
	{
		Log(Error, "Can't open JSON file: ", fileName, "!");
		return false;
	}

	i >> data;
	i.close();

	return true;
}

bool JSONWrapper::Save(const char* fileName, const json& data)
{
	std::ofstream o;
	o.open(g_Engine->Get<IOService>()->getWorkingDirectory() + fileName, std::ios::out | std::ios::trunc);
	o << std::setw(4) << data << std::endl;
	o.close();

	Log(Verbose, "JSON file: ", fileName, " has been saved.");

	return true;
}

bool JSONWrapper::SaveScene(const char* fileName)
{
	json topLevel;

	std::string sceneName = g_Engine->Get<IOService>()->getFileName(fileName);
	
	topLevel["Name"] = sceneName;

	// TODO Phase2-migrate: SaveScene still uses EntityManager::GetEntities — replace with EntityRegistry iteration
	for (auto i : g_Engine->Get<EntityManager>()->GetEntities())
	{
		if (i->m_Serializable)
		{
			json entityJson;
			to_json(entityJson, *i);
			entityJson["Components"] = json::array();
			topLevel["Entities"].emplace_back(entityJson);
		}
	}

	// TODO Phase2-migrate: LightComponent/CameraComponent are plain structs — serialize via EntityRegistry when Entity migration is complete

	Save(fileName, topLevel);
	Log(Success, "Scene ", fileName, " has been saved.");
	return true;
}

bool JSONWrapper::LoadScene(const char* fileName)
{
	json j;
	if (!Load(fileName, j))
		return false;

	auto l_name = j["Name"];

	for (auto i : j["Entities"])
	{
		std::string l_EntityName = i["Name"];
		l_EntityName += "/";

		auto l_Entity = g_Engine->Get<EntityManager>()->Spawn(true, ObjectLifespan::Scene, l_EntityName.c_str());
		auto l_EntityID = g_Engine->Get<EntityRegistry>()->Spawn(ObjectLifespan::Scene, l_EntityName.c_str());

		for (auto k : i["Components"])
		{
			uint32_t l_ComponentTypeID = k["Type"];
			std::string l_ComponentName = k["Name"];

			if (l_ComponentTypeID == 2 || l_ComponentTypeID == 200)
			{
				// ModelComponent (2) and DrawCallComponent (200) are deleted — skip deprecated scene data
				Log(Warning, "Skipping deprecated component type ", l_ComponentTypeID, " in scene file.");
			}
			else if (l_ComponentTypeID == LightComponent::GetTypeID())
			{
				auto& l_Light = g_Engine->Get<EntityRegistry>()->Emplace<LightComponent>(l_EntityID);
				std::string l_FilePath = AssetService::GetAssetFilePath(l_ComponentName.c_str());
				AssetService::Load(l_FilePath.c_str(), l_Light);
			}
			else if (l_ComponentTypeID == CameraComponent::GetTypeID())
			{
				auto& l_Camera = g_Engine->Get<EntityRegistry>()->Emplace<CameraComponent>(l_EntityID);
				std::string l_FilePath = AssetService::GetAssetFilePath(l_ComponentName.c_str());
				AssetService::Load(l_FilePath.c_str(), l_Camera);
			}
			else
			{
				Log(Error, "Unknown ComponentTypeID: ", l_ComponentTypeID);
			}
		}
	}

	Log(Success, "Scene loading finished.");

	return true;
}