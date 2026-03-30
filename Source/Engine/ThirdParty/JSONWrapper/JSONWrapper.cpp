#include "JSONWrapper.h"
#include "../../Common/ThreadSafeQueue.h"
#include "../../Common/LogService.h"
#include "../../Common/IOService.h"
#include "../../Common/TaskScheduler.h"

#include "../../Services/AssetService.h"
#include "../../Services/EntityRegistry.h"
#include "../../Component/TransformComponent.h"

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
	auto l_registry = g_Engine->Get<EntityRegistry>();
	auto l_EntityIDs = l_registry->GetAllEntityIDs(ObjectLifespan::Scene);

	json topLevel;
	topLevel["Name"] = g_Engine->Get<IOService>()->getFileName(fileName);
	topLevel["Entities"] = json::array();

	for (auto l_EntityID : l_EntityIDs)
	{
		// Strip the trailing "/" that Spawn appends
		std::string l_FullName = l_registry->GetName(l_EntityID);
		std::string l_Name = (!l_FullName.empty() && l_FullName.back() == '/')
			? l_FullName.substr(0, l_FullName.size() - 1)
			: l_FullName;

		json entityJson;
		entityJson["Name"] = l_Name;
		entityJson["Components"] = json::array();

		// TransformComponent
		auto* l_xf = l_registry->Get<TransformComponent>(l_EntityID);
		if (l_xf)
		{
			std::string l_CompName = l_Name + ".TransformComponent";
			json j;
			to_json(j, *l_xf);
			Save(AssetService::GetAssetFilePath(l_CompName.c_str()).c_str(), j);
			entityJson["Components"].push_back({{"Type", TransformComponent::GetTypeID()}, {"Name", l_CompName}});
		}

		// LightComponent — name derived from entity name
		auto* l_light = l_registry->Get<LightComponent>(l_EntityID);
		if (l_light)
		{
			std::string l_CompName = l_Name + ".LightComponent";
			json j;
			to_json(j, *l_light);
			Save(AssetService::GetAssetFilePath(l_CompName.c_str()).c_str(), j);
			entityJson["Components"].push_back({{"Type", LightComponent::GetTypeID()}, {"Name", l_CompName}});
		}

		// CameraComponent — name derived from entity name
		auto* l_camera = l_registry->Get<CameraComponent>(l_EntityID);
		if (l_camera)
		{
			std::string l_CompName = l_Name + ".CameraComponent";
			json j;
			to_json(j, *l_camera);
			Save(AssetService::GetAssetFilePath(l_CompName.c_str()).c_str(), j);
			entityJson["Components"].push_back({{"Type", CameraComponent::GetTypeID()}, {"Name", l_CompName}});
		}

		// MeshComponent — reference only, asset file is source of truth
		auto* l_mesh = l_registry->Get<MeshComponent>(l_EntityID);
		if (l_mesh && l_mesh->m_InstanceName.c_str()[0] != '\0')
		{
			entityJson["Components"].push_back({{"Type", MeshComponent::GetTypeID()}, {"Name", l_mesh->m_InstanceName.c_str()}});
		}

		// MaterialComponent — regenerate file (attributes may have changed)
		auto* l_material = l_registry->Get<MaterialComponent>(l_EntityID);
		if (l_material && l_material->m_InstanceName.c_str()[0] != '\0')
		{
			std::string l_CompName = l_material->m_InstanceName.c_str();
			json j;
			to_json(j, *l_material);
			Save(AssetService::GetAssetFilePath(l_CompName.c_str()).c_str(), j);
			entityJson["Components"].push_back({{"Type", MaterialComponent::GetTypeID()}, {"Name", l_CompName}});
		}

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

	auto l_registry = g_Engine->Get<EntityRegistry>();

	for (auto& entityJson : j["Entities"])
	{
		std::string l_EntityName = entityJson["Name"];
		l_EntityName += "/";
		auto l_EntityID = l_registry->Spawn(ObjectLifespan::Scene, l_EntityName.c_str());

		for (auto& compJson : entityJson["Components"])
		{
			uint32_t    l_TypeID   = compJson["Type"];
			std::string l_CompName = compJson["Name"];
			std::string l_FilePath = AssetService::GetAssetFilePath(l_CompName.c_str());

			if (l_TypeID == TransformComponent::GetTypeID())
			{
				auto& l_Transform = l_registry->Emplace<TransformComponent>(l_EntityID);
				AssetService::Load(l_FilePath.c_str(), l_Transform);
			}
			else if (l_TypeID == LightComponent::GetTypeID())
			{
				auto& l_Light = l_registry->Emplace<LightComponent>(l_EntityID);
				AssetService::Load(l_FilePath.c_str(), l_Light);
			}
			else if (l_TypeID == CameraComponent::GetTypeID())
			{
				auto& l_Camera = l_registry->Emplace<CameraComponent>(l_EntityID);
				AssetService::Load(l_FilePath.c_str(), l_Camera);
			}
			else if (l_TypeID == MeshComponent::GetTypeID())
			{
				auto& l_Mesh = l_registry->Emplace<MeshComponent>(l_EntityID);
				AssetService::Load(l_FilePath.c_str(), l_Mesh);
				l_Mesh.m_InstanceName = l_CompName.c_str(); // restore after template copy
			}
			else if (l_TypeID == MaterialComponent::GetTypeID())
			{
				auto& l_Material = l_registry->Emplace<MaterialComponent>(l_EntityID);
				AssetService::Load(l_FilePath.c_str(), l_Material);
				l_Material.m_InstanceName = l_CompName.c_str();
			}
			else
			{
				Log(Warning, "LoadScene: skipping unknown component type ", l_TypeID,
					" (", l_CompName.c_str(), ")");
			}
		}
	}

	Log(Success, "Scene loading finished.");

	return true;
}