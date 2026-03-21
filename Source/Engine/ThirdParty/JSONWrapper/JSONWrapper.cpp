#include "JSONWrapper.h"
#include "../../Common/ThreadSafeQueue.h"
#include "../../Common/LogService.h"
#include "../../Common/IOService.h"
#include "../../Common/TaskScheduler.h"

#include "../../Services/ComponentManager.h"
#include "../../Services/AnimationService.h"
#include "../../Services/AssetService.h"
#include "../../Services/EntityRegistry.h"

#include "../../Engine.h"
using namespace Inno;


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

	auto l_EntityIDs = g_Engine->Get<EntityRegistry>()->GetAllEntityIDs(ObjectLifespan::Scene);
	for (auto l_EntityID : l_EntityIDs)
	{
		json entityJson;
		entityJson["ID"] = l_EntityID;
		entityJson["Name"] = g_Engine->Get<EntityRegistry>()->GetName(l_EntityID);
		entityJson["Components"] = json::array();
		topLevel["Entities"].emplace_back(entityJson);
	}

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