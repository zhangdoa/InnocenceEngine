#include "JSONWrapper.h"
#include "../../Common/ThreadSafeQueue.h"
#include "../../Common/LogService.h"
#include "../../Common/IOService.h"
#include "../../Common/TaskScheduler.h"

#include "../../Services/ComponentManager.h"
#include "../../Services/RenderingContextService.h"
#include "../../Services/AnimationService.h"
#include "../../Services/AssetService.h"
#include "../../Services/EntityManager.h"

#include "../../Engine.h"
using namespace Inno;

namespace Inno
{
	namespace JSONWrapper
	{
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
	
	// Use IOService to extract just the filename without path and extension
	std::string sceneName = g_Engine->Get<IOService>()->getFileName(fileName);
	
	topLevel["Name"] = sceneName;

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

	for (auto i : g_Engine->Get<ComponentManager>()->GetAll<ModelComponent>())
	{
		if (i->m_Serializable)
		{
			SaveComponentAndAddReference(topLevel, i);
		}
	}
	
	for (auto i : g_Engine->Get<ComponentManager>()->GetAll<LightComponent>())
	{
		if (i->m_Serializable)
		{
			SaveComponentAndAddReference(topLevel, i);
		}
	}
	
	for (auto i : g_Engine->Get<ComponentManager>()->GetAll<CameraComponent>())
	{
		if (i->m_Serializable)
		{
			SaveComponentAndAddReference(topLevel, i);
		}
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
		std::string l_entityName = i["Name"];
		l_entityName += "/";
		auto l_entity = g_Engine->Get<EntityManager>()->Spawn(true, ObjectLifespan::Scene, l_entityName.c_str());

		for (auto k : i["Components"])
		{
			uint32_t componentTypeID = k["Type"];
			std::string l_componentName = k["Name"];

			if (componentTypeID == ModelComponent::GetTypeID())
			{
				g_Engine->Get<ComponentManager>()->Load<ModelComponent>(l_componentName.c_str(), l_entity);
			}
			else if (componentTypeID == LightComponent::GetTypeID())
			{
				g_Engine->Get<ComponentManager>()->Load<LightComponent>(l_componentName.c_str(), l_entity);
			}
			else if (componentTypeID == CameraComponent::GetTypeID())
			{
				g_Engine->Get<ComponentManager>()->Load<CameraComponent>(l_componentName.c_str(), l_entity);
			}
			else
			{
				Log(Error, "Unknown ComponentTypeID: ", componentTypeID);
			}
		}
	}

	Log(Success, "Scene loading finished.");

	return true;
}