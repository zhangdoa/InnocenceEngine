#include "JSONWrapper.h"
#include "../../Common/Array.h"
#include "../../Common/ThreadSafeQueue.h"
#include "../../Common/LogService.h"
#include "../../Common/IOService.h"
#include "../../Common/TaskScheduler.h"

#include "../../Services/AssetService.h"
#include "../../Services/EntityRegistry.h"
#include "../../Component/TransformComponent.h"

#include "../../Engine.h"
using namespace Inno;

namespace
{
	std::mutex g_LoadedCompFilenamesMutex;
	// Key: (EntityID, component type id). Value: the filename the
	// component was loaded from (without trailing ".json"). Preserved so
	// Save writes back to the authored file instead of an entity-name-
	// based replacement.
	std::map<std::pair<EntityID, uint32_t>, std::string> g_LoadedCompFilenames;

	// Child-scene structural metadata — cleared on ClearLoadedCompFilenames().
	// These three maps are populated during load and consumed during save to
	// reconstruct the original parent–ChildScene hierarchy rather than
	// flattening all child-scene entities into the parent file.
	std::map<EntityID, std::string>    g_ChildSceneByParent;    // parent EntityID → relative path
	std::map<EntityID, EntityID>       g_ParentByChildEntity;   // child EntityID  → parent EntityID
	std::map<std::string, std::string> g_ChildSceneDefaultPath; // relative path   → DefaultComponentPath

	void RememberLoadedCompFilename(EntityID entityId, uint32_t typeId, const std::string& name)
	{
		std::lock_guard<std::mutex> lock(g_LoadedCompFilenamesMutex);
		g_LoadedCompFilenames[{entityId, typeId}] = name;
	}

	std::string LookupLoadedCompFilename(EntityID entityId, uint32_t typeId)
	{
		std::lock_guard<std::mutex> lock(g_LoadedCompFilenamesMutex);
		auto it = g_LoadedCompFilenames.find({entityId, typeId});
		return it == g_LoadedCompFilenames.end() ? std::string{} : it->second;
	}
}

void JSONWrapper::ClearLoadedCompFilenames()
{
	std::lock_guard<std::mutex> lock(g_LoadedCompFilenamesMutex);
	g_LoadedCompFilenames.clear();
	g_ChildSceneByParent.clear();
	g_ParentByChildEntity.clear();
	g_ChildSceneDefaultPath.clear();
}


bool JSONWrapper::Load(const char* fileName, json& data)
{
	std::ifstream i;

	i.open(g_Engine->Get<IOService>()->GetDataDirectory() + fileName);

	if (!i.is_open())
	{
		Log(Error, "Can't open JSON file: ", fileName, "!");
		return false;
	}

	i >> data;
	i.close();

	return true;
}

namespace
{
	// nlohmann/json serializes doubles at max-roundtrip precision
	// (%.17g → "0.7820000052452087"). Transforms, colours, and PBR
	// constants authored as single-precision floats should read back
	// as the same short decimals they were authored with. Pre-rounding
	// each double to 6 significant digits before dump preserves all
	// precision a float can carry (~7.2 digits) while eliminating the
	// trailing-zero noise that makes every saved JSON diff look like
	// a format change. Leaves integers and non-numeric values untouched.
	void TrimFloatPrecision(json& j)
	{
		if (j.is_object())
		{
			for (auto& [_, v] : j.items())
				TrimFloatPrecision(v);
			return;
		}
		if (j.is_array())
		{
			for (auto& v : j)
				TrimFloatPrecision(v);
			return;
		}
		if (j.is_number_float())
		{
			double d = j.get<double>();
			if (d == 0.0 || std::isnan(d) || std::isinf(d))
				return;
			// Round-trip through %.6g to drop non-significant trailing noise
			// while preserving all meaningful digits of a single-precision
			// float.
			char buf[32];
			std::snprintf(buf, sizeof(buf), "%.6g", d);
			j = std::strtod(buf, nullptr);
		}
	}
}

bool JSONWrapper::Save(const char* fileName, const json& data)
{
	json trimmed = data;
	TrimFloatPrecision(trimmed);
	std::ofstream o;
	o.open(g_Engine->Get<IOService>()->GetDataDirectory() + fileName, std::ios::out | std::ios::trunc | std::ios::binary);
	o << std::setw(4) << trimmed << std::endl;
	o.close();

	Log(Verbose, "JSON file: ", fileName, " has been saved.");

	return true;
}

bool JSONWrapper::SaveChildScene(const char* exportName, const Inno::Array<std::pair<std::string, std::string>>& drawCalls)
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
	auto l_io       = g_Engine->Get<IOService>();
	auto l_registry = g_Engine->Get<EntityRegistry>();
	auto l_EntityIDs = l_registry->GetAllEntityIDs(ObjectLifespan::Scene);

	// Snapshot child-scene maps under lock; release before heavy I/O.
	std::map<EntityID, std::string>    l_childSceneByParent;
	std::map<EntityID, EntityID>       l_parentByChild;
	std::map<std::string, std::string> l_childSceneDefaultPath;
	{
		std::lock_guard<std::mutex> lock(g_LoadedCompFilenamesMutex);
		l_childSceneByParent    = g_ChildSceneByParent;
		l_parentByChild         = g_ParentByChildEntity;
		l_childSceneDefaultPath = g_ChildSceneDefaultPath;
	}

	// Returns the directory portion of an asset-relative path.
	// e.g. "ExampleProject/Components/foo.json" → "ExampleProject/Components/"
	auto dirOf = [](const std::string& path) -> std::string {
		auto pos = path.rfind('/');
		return (pos != std::string::npos) ? path.substr(0, pos + 1) : std::string{};
	};

	// Builds a component JSON entry and adds "Path" when the component's
	// directory differs from the scene's DefaultComponentPath.
	auto makeCompEntry = [&](uint32_t typeId, const std::string& name,
	                         const std::string& defaultDir) -> json {
		json entry = {{"Type", typeId}, {"Name", name}};
		auto compDir = dirOf(AssetService::GetAssetFilePath(name.c_str()));
		if (!compDir.empty() && compDir != defaultDir)
			entry["Path"] = compDir;
		return entry;
	};

	std::string l_projectCompDir = l_io->GetProjectName() + std::string("/Components/");

	// --- Phase 1: Save the main scene file (child-scene entities excluded) ---
	json topLevel;
	topLevel["Name"]                 = l_io->GetFileName(fileName);
	topLevel["DefaultComponentPath"] = l_projectCompDir;
	topLevel["Entities"]             = json::array();

	for (auto l_EntityID : l_EntityIDs)
	{
		// Child-scene entities are saved in Phase 2; skip them here.
		if (l_parentByChild.count(l_EntityID))
			continue;

		std::string l_Name = l_registry->GetName(l_EntityID);
		json entityJson;
		entityJson["Name"]       = l_Name;
		entityJson["Components"] = json::array();

		// TransformComponent — filename: prefer the one it was loaded from
		// (captured by LoadScene / LoadChildScene), fall back to entity-name
		// pattern for entities the editor spawned mid-session.
		auto* l_xf = l_registry->Get<TransformComponent>(l_EntityID);
		if (l_xf)
		{
			std::string l_CompName = LookupLoadedCompFilename(l_EntityID, TransformComponent::GetTypeID());
			if (l_CompName.empty())
				l_CompName = l_Name + ".TransformComponent";
			json j;
			to_json(j, *l_xf);
			Save(AssetService::GetAssetFilePath(l_CompName.c_str()).c_str(), j);
			entityJson["Components"].push_back({{"Type", TransformComponent::GetTypeID()}, {"Name", l_CompName}});
		}

		auto* l_light = l_registry->Get<LightComponent>(l_EntityID);
		if (l_light)
		{
			std::string l_CompName = LookupLoadedCompFilename(l_EntityID, LightComponent::GetTypeID());
			if (l_CompName.empty())
				l_CompName = l_Name + ".LightComponent";
			json j;
			to_json(j, *l_light);
			Save(AssetService::GetAssetFilePath(l_CompName.c_str()).c_str(), j);
			entityJson["Components"].push_back({{"Type", LightComponent::GetTypeID()}, {"Name", l_CompName}});
		}

		auto* l_camera = l_registry->Get<CameraComponent>(l_EntityID);
		if (l_camera)
		{
			std::string l_CompName = LookupLoadedCompFilename(l_EntityID, CameraComponent::GetTypeID());
			if (l_CompName.empty())
				l_CompName = l_Name + ".CameraComponent";
			json j;
			to_json(j, *l_camera);
			Save(AssetService::GetAssetFilePath(l_CompName.c_str()).c_str(), j);
			entityJson["Components"].push_back({{"Type", CameraComponent::GetTypeID()}, {"Name", l_CompName}});
		}

		// MeshComponent — reference only, asset file is source of truth.
		auto* l_mesh = l_registry->Get<MeshComponent>(l_EntityID);
		if (l_mesh)
			entityJson["Components"].push_back({{"Type", MeshComponent::GetTypeID()}, {"Name", l_mesh->m_InstanceName.c_str()}});

		// MaterialComponent — regenerate file (attributes may have changed).
		auto* l_material = l_registry->Get<MaterialComponent>(l_EntityID);
		if (l_material)
		{
			std::string l_CompName = l_material->m_InstanceName.c_str();
			json j;
			to_json(j, *l_material);
			Save(AssetService::GetAssetFilePath(l_CompName.c_str()).c_str(), j);
			entityJson["Components"].push_back({{"Type", MaterialComponent::GetTypeID()}, {"Name", l_CompName}});
		}

		// Restore the ChildScene reference if this entity had one.
		auto childIt = l_childSceneByParent.find(l_EntityID);
		if (childIt != l_childSceneByParent.end())
			entityJson["ChildScene"] = childIt->second;

		topLevel["Entities"].emplace_back(entityJson);
	}

	Save(fileName, topLevel);
	Log(Success, "Scene ", fileName, " has been saved.");

	// --- Phase 2: Save each child scene file ---
	// Group child entities by child scene path (via parent→child-scene map).
	std::map<std::string, Inno::Array<EntityID>> l_byChildScene;
	for (auto& [childID, parentID] : l_parentByChild)
	{
		auto it = l_childSceneByParent.find(parentID);
		if (it != l_childSceneByParent.end())
			l_byChildScene[it->second].push_back(childID);
	}

	for (auto& [childScenePath, childIDs] : l_byChildScene)
	{
		auto defaultPathIt = l_childSceneDefaultPath.find(childScenePath);
		std::string l_defaultDir = (defaultPathIt != l_childSceneDefaultPath.end())
		                         ? defaultPathIt->second : std::string{};

		json childTop;
		childTop["Name"]                 = l_io->GetFileName(childScenePath.c_str());
		childTop["DefaultComponentPath"] = l_defaultDir;
		childTop["Entities"]             = json::array();

		for (auto l_ChildID : childIDs)
		{
			EntityID    l_ParentID   = l_parentByChild.at(l_ChildID);
			std::string l_ParentName = l_registry->GetName(l_ParentID);
			std::string l_FullName   = l_registry->GetName(l_ChildID);
			// Recover the name the entity had inside the child scene file.
			std::string l_OrigName   = l_FullName.substr(l_ParentName.size());

			json entityJson;
			entityJson["Name"]       = l_OrigName;
			entityJson["Components"] = json::array();

			// TransformComponent: only include if it was loaded from a file
			// (not the auto-created copy of the parent transform).
			auto* l_xf = l_registry->Get<TransformComponent>(l_ChildID);
			if (l_xf)
			{
				std::string l_CompName = LookupLoadedCompFilename(l_ChildID, TransformComponent::GetTypeID());
				if (!l_CompName.empty())
				{
					json j;
					to_json(j, *l_xf);
					Save(AssetService::GetAssetFilePath(l_CompName.c_str()).c_str(), j);
					entityJson["Components"].push_back(makeCompEntry(TransformComponent::GetTypeID(), l_CompName, l_defaultDir));
				}
			}

			auto* l_light = l_registry->Get<LightComponent>(l_ChildID);
			if (l_light)
			{
				std::string l_CompName = LookupLoadedCompFilename(l_ChildID, LightComponent::GetTypeID());
				if (l_CompName.empty())
					l_CompName = l_OrigName + ".LightComponent";
				json j;
				to_json(j, *l_light);
				Save(AssetService::GetAssetFilePath(l_CompName.c_str()).c_str(), j);
				entityJson["Components"].push_back(makeCompEntry(LightComponent::GetTypeID(), l_CompName, l_defaultDir));
			}

			auto* l_camera = l_registry->Get<CameraComponent>(l_ChildID);
			if (l_camera)
			{
				std::string l_CompName = LookupLoadedCompFilename(l_ChildID, CameraComponent::GetTypeID());
				if (l_CompName.empty())
					l_CompName = l_OrigName + ".CameraComponent";
				json j;
				to_json(j, *l_camera);
				Save(AssetService::GetAssetFilePath(l_CompName.c_str()).c_str(), j);
				entityJson["Components"].push_back(makeCompEntry(CameraComponent::GetTypeID(), l_CompName, l_defaultDir));
			}

			// MeshComponent — reference only.
			auto* l_mesh = l_registry->Get<MeshComponent>(l_ChildID);
			if (l_mesh)
				entityJson["Components"].push_back(makeCompEntry(MeshComponent::GetTypeID(), l_mesh->m_InstanceName.c_str(), l_defaultDir));

			// MaterialComponent — regenerate file.
			auto* l_material = l_registry->Get<MaterialComponent>(l_ChildID);
			if (l_material)
			{
				std::string l_CompName = l_material->m_InstanceName.c_str();
				json j;
				to_json(j, *l_material);
				Save(AssetService::GetAssetFilePath(l_CompName.c_str()).c_str(), j);
				entityJson["Components"].push_back(makeCompEntry(MaterialComponent::GetTypeID(), l_CompName, l_defaultDir));
			}

			childTop["Entities"].emplace_back(entityJson);
		}

		Save(childScenePath.c_str(), childTop);
		Log(Success, "Child scene ", childScenePath.c_str(), " has been saved.");
	}

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
				continue;
			}

			RememberLoadedCompFilename(l_EntityID, l_TypeID, l_CompName);
		}

		// Load child scene if referenced; record parent→path for round-trip save
		if (entityJson.find("ChildScene") != entityJson.end())
		{
			std::string l_childScenePath = entityJson["ChildScene"];
			{
				std::lock_guard<std::mutex> lock(g_LoadedCompFilenamesMutex);
				g_ChildSceneByParent[l_EntityID] = l_childScenePath;
			}
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
	// Snapshot parent transform by value. Emplace<TransformComponent> inside the
	// loop may reallocate the underlying component-storage vector, which would
	// dangle any pointer held across the call.
	bool l_hasParentXf = false;
	TransformComponent l_ParentXfCopy{};
	if (auto* p = l_registry->Get<TransformComponent>(parentEntity))
	{
		l_ParentXfCopy = *p;
		l_hasParentXf = true;
	}
	std::string l_ParentName = l_registry->GetName(parentEntity);

	std::string l_DefaultPath;
	if (j.find("DefaultComponentPath") != j.end())
	{
		l_DefaultPath = j["DefaultComponentPath"].get<std::string>();
		std::lock_guard<std::mutex> lock(g_LoadedCompFilenamesMutex);
		g_ChildSceneDefaultPath[std::string(sceneFilePath)] = l_DefaultPath;
	}

	for (auto& entityJson : j["Entities"])
	{
		std::string l_EntityName = l_ParentName + entityJson["Name"].get<std::string>();
		auto l_EntityID = l_registry->Spawn(ObjectLifespan::Scene, l_EntityName.c_str());
		{
			std::lock_guard<std::mutex> lock(g_LoadedCompFilenamesMutex);
			g_ParentByChildEntity[l_EntityID] = parentEntity;
		}

		// Inherit parent transform
		auto& l_Transform = l_registry->Emplace<TransformComponent>(l_EntityID);
		if (l_hasParentXf)
		{
			l_Transform.m_LocalPos = l_ParentXfCopy.m_LocalPos;
			l_Transform.m_LocalRot = l_ParentXfCopy.m_LocalRot;
			l_Transform.m_LocalScale = l_ParentXfCopy.m_LocalScale;
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
			else
			{
				continue;
			}

			RememberLoadedCompFilename(l_EntityID, l_TypeID, l_CompName);
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