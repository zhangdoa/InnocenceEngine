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

	i.open(g_Engine->Get<IOService>()->getDataDirectory() + fileName);

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
	o.open(g_Engine->Get<IOService>()->getDataDirectory() + fileName, std::ios::out | std::ios::trunc);
	o << std::setw(4) << data << std::endl;
	o.close();

	Log(Verbose, "JSON file: ", fileName, " has been saved.");

	return true;
}

bool JSONWrapper::SaveChildScene(const char* exportName, const std::vector<std::pair<std::string, std::string>>& drawCalls)
{
	json topLevel;
	topLevel["Name"] = exportName;
	topLevel["DefaultComponentPath"] = "Generated/Components/";
	topLevel["Entities"] = json::array();

	for (size_t i = 0; i < drawCalls.size(); i++)
	{
		auto& [meshName, materialName] = drawCalls[i];

		json entityJson;
		entityJson["Name"] = std::string(exportName) + "." + std::to_string(i);
		entityJson["Components"] = json::array();

		entityJson["Components"].push_back({
			{"Type", MeshComponent::GetTypeID()},
			{"Name", meshName}
		});

		if (!materialName.empty())
		{
			entityJson["Components"].push_back({
				{"Type", MaterialComponent::GetTypeID()},
				{"Name", materialName}
			});
		}

		topLevel["Entities"].emplace_back(entityJson);
	}

	auto l_scenePath = std::string("Generated/Scenes/") + exportName + ".InnoScene";
	return Save(l_scenePath.c_str(), topLevel);
}

bool JSONWrapper::SaveScene(const char* fileName)
{
	auto l_registry = g_Engine->Get<EntityRegistry>();
	auto l_EntityIDs = l_registry->GetAllEntityIDs(ObjectLifespan::Scene);

	json topLevel;
	topLevel["Name"] = g_Engine->Get<IOService>()->getFileName(fileName);
	topLevel["DefaultComponentPath"] = g_Engine->Get<IOService>()->getProjectName() + std::string("/Components/");
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

	std::string l_DefaultPath;
	if (j.find("DefaultComponentPath") != j.end())
		l_DefaultPath = j["DefaultComponentPath"].get<std::string>();

	for (auto& entityJson : j["Entities"])
	{
		std::string l_EntityName = entityJson["Name"];
		l_EntityName += "/";
		auto l_EntityID = l_registry->Spawn(ObjectLifespan::Scene, l_EntityName.c_str());

		for (auto& compJson : entityJson["Components"])
		{
			uint32_t    l_TypeID   = compJson["Type"];
			std::string l_CompName = compJson["Name"];

			std::string l_FilePath;
			if (compJson.find("Path") != compJson.end())
				l_FilePath = compJson["Path"].get<std::string>() + l_CompName + ".json";
			else if (!l_DefaultPath.empty())
				l_FilePath = l_DefaultPath + l_CompName + ".json";
			else
				l_FilePath = AssetService::GetAssetFilePath(l_CompName.c_str());

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
				l_Mesh.m_InstanceName = l_CompName.c_str();
				AssetService::Load(l_FilePath.c_str(), l_Mesh, l_EntityID);
				l_Mesh.m_InstanceName = l_CompName.c_str(); // restore after template copy
			}
			else if (l_TypeID == MaterialComponent::GetTypeID())
			{
				auto& l_Material = l_registry->Emplace<MaterialComponent>(l_EntityID);
				l_Material.m_InstanceName = l_CompName.c_str();
				AssetService::Load(l_FilePath.c_str(), l_Material, l_EntityID);
			}
			else
			{
				Log(Warning, "LoadScene: skipping unknown component type ", l_TypeID,
					" (", l_CompName.c_str(), ")");
			}
		}

		// Load child scene if referenced
		if (entityJson.find("ChildScene") != entityJson.end())
		{
			std::string l_childScenePath = entityJson["ChildScene"];
			LoadChildScene(l_childScenePath.c_str(), l_EntityID);
		}
	}

	Log(Success, "Scene loading finished.");

	return true;
}

bool JSONWrapper::LoadChildScene(const char* sceneFilePath, EntityID parentEntity)
{
	json j;
	if (!Load(sceneFilePath, j))
	{
		Log(Warning, "LoadChildScene: failed to load ", sceneFilePath);
		return false;
	}

	auto l_registry = g_Engine->Get<EntityRegistry>();
	auto* l_ParentTransform = l_registry->Get<TransformComponent>(parentEntity);
	std::string l_ParentName = l_registry->GetName(parentEntity);

	std::string l_DefaultPath;
	if (j.find("DefaultComponentPath") != j.end())
		l_DefaultPath = j["DefaultComponentPath"].get<std::string>();

	for (auto& entityJson : j["Entities"])
	{
		std::string l_EntityName = l_ParentName + entityJson["Name"].get<std::string>() + "/";
		auto l_EntityID = l_registry->Spawn(ObjectLifespan::Scene, l_EntityName.c_str());

		// Inherit parent transform
		auto& l_Transform = l_registry->Emplace<TransformComponent>(l_EntityID);
		if (l_ParentTransform)
		{
			l_Transform.m_LocalPos = l_ParentTransform->m_LocalPos;
			l_Transform.m_LocalRot = l_ParentTransform->m_LocalRot;
			l_Transform.m_LocalScale = l_ParentTransform->m_LocalScale;
		}

		for (auto& compJson : entityJson["Components"])
		{
			uint32_t    l_TypeID   = compJson["Type"];
			std::string l_CompName = compJson["Name"];

			std::string l_FilePath;
			if (compJson.find("Path") != compJson.end())
				l_FilePath = compJson["Path"].get<std::string>() + l_CompName + ".json";
			else if (!l_DefaultPath.empty())
				l_FilePath = l_DefaultPath + l_CompName + ".json";
			else
				l_FilePath = AssetService::GetAssetFilePath(l_CompName.c_str());

			if (l_TypeID == MeshComponent::GetTypeID())
			{
				auto& l_Mesh = l_registry->Emplace<MeshComponent>(l_EntityID);
				l_Mesh.m_InstanceName = l_CompName.c_str();
				AssetService::Load(l_FilePath.c_str(), l_Mesh, l_EntityID);
				l_Mesh.m_InstanceName = l_CompName.c_str();
			}
			else if (l_TypeID == MaterialComponent::GetTypeID())
			{
				auto& l_Material = l_registry->Emplace<MaterialComponent>(l_EntityID);
				l_Material.m_InstanceName = l_CompName.c_str();
				AssetService::Load(l_FilePath.c_str(), l_Material, l_EntityID);
			}
			else if (l_TypeID == TransformComponent::GetTypeID())
			{
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
		}

		// Recursive child scene support
		if (entityJson.find("ChildScene") != entityJson.end())
		{
			std::string l_childScenePath = entityJson["ChildScene"];
			LoadChildScene(l_childScenePath.c_str(), l_EntityID);
		}
	}

	Log(Verbose, "Loaded child scene: ", sceneFilePath);
	return true;
}