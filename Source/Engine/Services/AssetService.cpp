#include "AssetService.h"
#include "../Common/ComponentHeaders.h"
#include "../Common/MathHelper.h"
#include "../Common/LogService.h"
#include "../Common/IOService.h"
#include "../Common/TaskScheduler.h"
#include "../Common/ObjectPool.h"
#include "../ThirdParty/JSONWrapper/JSONWrapper.h"
#include "../ThirdParty/STBWrapper/STBWrapper.h"
#include "../ThirdParty/AssimpWrapper/AssimpWrapper.h"
#include "TemplateAssetService.h"
#include "SceneService.h"
#include "PhysicsSimulationService.h"

#include "../Engine.h"
using namespace Inno;

namespace AssetServiceNS
{
	ObjectStatus m_ObjectStatus = ObjectStatus::Terminated;

	// Mesh asset registry
	std::vector<MeshAssetData> m_MeshAssets;
	std::vector<uint32_t> m_MeshFreeSlots;
	std::vector<uint32_t> m_MeshGenerations;
	std::unordered_map<std::string, MeshAssetHandle> m_MeshLUT;

	// Material asset registry
	std::vector<MaterialAssetData> m_MaterialAssets;
	std::vector<uint32_t> m_MaterialFreeSlots;
	std::vector<uint32_t> m_MaterialGenerations;
	std::unordered_map<std::string, MaterialAssetHandle> m_MaterialLUT;

	// Texture asset registry
	std::vector<TextureAssetData> m_TextureAssets;
	std::vector<uint32_t> m_TextureFreeSlots;
	std::vector<uint32_t> m_TextureGenerations;
	std::unordered_map<std::string, TextureAssetHandle> m_TextureLUT;
}

using namespace AssetServiceNS;

bool AssetService::Setup(IServiceConfig* systemConfig)
{
	m_ObjectStatus = ObjectStatus::Created;

	return true;
}

bool AssetService::Initialize()
{
	return true;
}

bool AssetService::Update()
{
	return true;
}

bool AssetService::Terminate()
{
	return true;
}

ObjectStatus AssetService::GetStatus()
{
	return ObjectStatus();
}

MeshAssetHandle AssetService::AllocateMeshAsset(const char* name, ObjectLifespan lifespan)
{
	auto l_existing = m_MeshLUT.find(name);
	if (l_existing != m_MeshLUT.end())
	{
		auto& l_asset = m_MeshAssets[l_existing->second.m_Index];
		if (l_asset.m_Residency != AssetResidency::Released)
			return l_existing->second;
	}

	uint32_t l_index;
	if (!m_MeshFreeSlots.empty())
	{
		l_index = m_MeshFreeSlots.back();
		m_MeshFreeSlots.pop_back();
		m_MeshAssets[l_index] = MeshAssetData();
	}
	else
	{
		l_index = static_cast<uint32_t>(m_MeshAssets.size());
		m_MeshAssets.emplace_back();
		m_MeshGenerations.emplace_back(0);
	}

	auto& l_asset = m_MeshAssets[l_index];
	l_asset.m_Lifespan = lifespan;
	l_asset.m_Residency = AssetResidency::Loading;
	l_asset.m_Name = name;

	MeshAssetHandle l_handle;
	l_handle.m_Index = l_index;
	l_handle.m_Generation = m_MeshGenerations[l_index];
	m_MeshLUT[name] = l_handle;

	return l_handle;
}

MeshAssetData* AssetService::GetMeshAsset(MeshAssetHandle handle)
{
	if (!handle.IsValid() || handle.m_Index >= m_MeshAssets.size())
		return nullptr;

	if (handle.m_Generation != m_MeshGenerations[handle.m_Index])
		return nullptr;

	auto& l_asset = m_MeshAssets[handle.m_Index];
	if (l_asset.m_Residency == AssetResidency::Released)
		return nullptr;

	return &l_asset;
}

MeshAssetHandle AssetService::FindMeshAsset(const char* name)
{
	auto l_result = m_MeshLUT.find(name);
	if (l_result != m_MeshLUT.end())
		return l_result->second;
	return MeshAssetHandle{};
}

MaterialAssetHandle AssetService::AllocateMaterialAsset(const char* name, ObjectLifespan lifespan)
{
	auto l_existing = m_MaterialLUT.find(name);
	if (l_existing != m_MaterialLUT.end())
	{
		auto& l_asset = m_MaterialAssets[l_existing->second.m_Index];
		if (l_asset.m_Residency != AssetResidency::Released)
			return l_existing->second;
	}

	uint32_t l_index;
	if (!m_MaterialFreeSlots.empty())
	{
		l_index = m_MaterialFreeSlots.back();
		m_MaterialFreeSlots.pop_back();
		m_MaterialAssets[l_index] = MaterialAssetData();
	}
	else
	{
		l_index = static_cast<uint32_t>(m_MaterialAssets.size());
		m_MaterialAssets.emplace_back();
		m_MaterialGenerations.emplace_back(0);
	}

	auto& l_asset = m_MaterialAssets[l_index];
	l_asset.m_Lifespan = lifespan;
	l_asset.m_Residency = AssetResidency::Loading;
	l_asset.m_Name = name;

	MaterialAssetHandle l_handle;
	l_handle.m_Index = l_index;
	l_handle.m_Generation = m_MaterialGenerations[l_index];
	m_MaterialLUT[name] = l_handle;

	return l_handle;
}

MaterialAssetData* AssetService::GetMaterialAsset(MaterialAssetHandle handle)
{
	if (!handle.IsValid() || handle.m_Index >= m_MaterialAssets.size())
		return nullptr;

	if (handle.m_Generation != m_MaterialGenerations[handle.m_Index])
		return nullptr;

	auto& l_asset = m_MaterialAssets[handle.m_Index];
	if (l_asset.m_Residency == AssetResidency::Released)
		return nullptr;

	return &l_asset;
}

MaterialAssetHandle AssetService::FindMaterialAsset(const char* name)
{
	auto l_result = m_MaterialLUT.find(name);
	if (l_result != m_MaterialLUT.end())
		return l_result->second;
	return MaterialAssetHandle{};
}

TextureAssetHandle AssetService::AllocateTextureAsset(const char* name, ObjectLifespan lifespan)
{
	auto l_existing = m_TextureLUT.find(name);
	if (l_existing != m_TextureLUT.end())
	{
		auto& l_asset = m_TextureAssets[l_existing->second.m_Index];
		if (l_asset.m_Residency != AssetResidency::Released)
			return l_existing->second;
	}

	uint32_t l_index;
	if (!m_TextureFreeSlots.empty())
	{
		l_index = m_TextureFreeSlots.back();
		m_TextureFreeSlots.pop_back();
		m_TextureAssets[l_index] = TextureAssetData();
	}
	else
	{
		l_index = static_cast<uint32_t>(m_TextureAssets.size());
		m_TextureAssets.emplace_back();
		m_TextureGenerations.emplace_back(0);
	}

	auto& l_asset = m_TextureAssets[l_index];
	l_asset.m_Lifespan = lifespan;
	l_asset.m_Residency = AssetResidency::Loading;
	l_asset.m_Name = name;

	TextureAssetHandle l_handle;
	l_handle.m_Index = l_index;
	l_handle.m_Generation = m_TextureGenerations[l_index];
	m_TextureLUT[name] = l_handle;

	return l_handle;
}

TextureAssetData* AssetService::GetTextureAsset(TextureAssetHandle handle)
{
	if (!handle.IsValid() || handle.m_Index >= m_TextureAssets.size())
		return nullptr;

	if (handle.m_Generation != m_TextureGenerations[handle.m_Index])
		return nullptr;

	auto& l_asset = m_TextureAssets[handle.m_Index];
	if (l_asset.m_Residency == AssetResidency::Released)
		return nullptr;

	return &l_asset;
}

TextureAssetHandle AssetService::FindTextureAsset(const char* name)
{
	auto l_result = m_TextureLUT.find(name);
	if (l_result != m_TextureLUT.end())
		return l_result->second;
	return TextureAssetHandle{};
}

void AssetService::ReleaseAssetsByLifespan(ObjectLifespan lifespan)
{
	for (uint32_t i = 0; i < static_cast<uint32_t>(m_MeshAssets.size()); i++)
	{
		auto& l_asset = m_MeshAssets[i];
		if (l_asset.m_Lifespan == lifespan && l_asset.m_Residency != AssetResidency::Released)
		{
			m_MeshLUT.erase(std::string(l_asset.m_Name.c_str()));
			l_asset = MeshAssetData();
			l_asset.m_Residency = AssetResidency::Released;
			m_MeshGenerations[i]++;
			m_MeshFreeSlots.push_back(i);
		}
	}

	for (uint32_t i = 0; i < static_cast<uint32_t>(m_MaterialAssets.size()); i++)
	{
		auto& l_asset = m_MaterialAssets[i];
		if (l_asset.m_Lifespan == lifespan && l_asset.m_Residency != AssetResidency::Released)
		{
			m_MaterialLUT.erase(std::string(l_asset.m_Name.c_str()));
			l_asset = MaterialAssetData();
			l_asset.m_Residency = AssetResidency::Released;
			m_MaterialGenerations[i]++;
			m_MaterialFreeSlots.push_back(i);
		}
	}

	for (uint32_t i = 0; i < static_cast<uint32_t>(m_TextureAssets.size()); i++)
	{
		auto& l_asset = m_TextureAssets[i];
		if (l_asset.m_Lifespan == lifespan && l_asset.m_Residency != AssetResidency::Released)
		{
			m_TextureLUT.erase(std::string(l_asset.m_Name.c_str()));
			l_asset = TextureAssetData();
			l_asset.m_Residency = AssetResidency::Released;
			m_TextureGenerations[i]++;
			m_TextureFreeSlots.push_back(i);
		}
	}
}

bool AssetService::Import(const char* fileName)
{
	auto l_extension = g_Engine->Get<IOService>()->getFileExtension(fileName);
	std::string l_fileName = fileName;

	if (l_extension == ".obj" || l_extension == ".OBJ" || l_extension == ".fbx" || l_extension == ".FBX" || l_extension == ".gltf" || l_extension == ".GLTF" || l_extension == ".md5mesh")
	{
		auto tempTask = g_Engine->Get<TaskScheduler>()->Submit(ITask::Desc("Import Model Task", ITask::Type::Once), [=]()
			{
				AssimpWrapper::Import(l_fileName.c_str());
			});
		tempTask->Activate();
		return true;
	}
	else
	{
		Log(Warning, fileName, " is not supported!");

		return false;
	}
}

bool AssetService::SaveScene(const char* fileName)
{
	return JSONWrapper::SaveScene(fileName);
}

bool AssetService::LoadScene(const char* fileName)
{
	return JSONWrapper::LoadScene(fileName);
}

bool AssetService::Load(const char* fileName, TransformComponent& component)
{
	return JSONWrapper::Load(fileName, component);
}

bool AssetService::Load(const char* fileName, MeshComponent& component, EntityID owner)
{
	return JSONWrapper::Load(fileName, component, owner);
}

bool AssetService::Load(const char* fileName, MaterialComponent& component, EntityID owner)
{
	return JSONWrapper::Load(fileName, component, owner);
}

bool AssetService::Load(const char* fileName, TextureComponent& component, EntityID owner)
{
	return JSONWrapper::Load(fileName, component, owner);
}

// bool AssetService::Load(const char* fileName, SkeletonComponent& component)
// {
//		return JSONWrapper::Load(fileName, component);
// }

// bool AssetService::Load(const char* fileName, AnimationComponent& component)
// {
//		return JSONWrapper::Load(fileName, component);
// }

bool AssetService::Load(const char* fileName, CameraComponent& component)
{
	return JSONWrapper::Load(fileName, component);
}

bool AssetService::Load(const char* fileName, LightComponent& component)
{
	return JSONWrapper::Load(fileName, component);
}

bool AssetService::Save(const char* fileName, const TextureDesc& textureDesc, void* textureData)
{
	return STBWrapper::Save(fileName, textureDesc, textureData);
}

bool AssetService::Save(const MeshComponent& component, std::vector<Vertex>& vertices, std::vector<Index>& indices)
{
	json j;
	JSONWrapper::to_json(j, component);
	
	// Add binary-specific fields
	j["VerticesNumber"] = vertices.size();
	j["IndicesNumber"] = indices.size();

	auto l_workingDir = "../Data/Components/";
	std::filesystem::create_directories(l_workingDir);

	std::string l_baseName;
	auto l_binaryFileName = l_baseName + ".innobin";
	auto l_binaryFilePath = l_workingDir + l_binaryFileName;

	j["File"] = l_binaryFileName;

	std::ofstream l_binaryFile(l_binaryFilePath, std::ios::out | std::ios::trunc | std::ios::binary);
	if (!l_binaryFile.is_open())
	{
		Log(Error, "Failed to open binary mesh file: ", l_binaryFilePath.c_str());
		return false;
	}

	g_Engine->Get<IOService>()->serializeVector(l_binaryFile, vertices);
	g_Engine->Get<IOService>()->serializeVector(l_binaryFile, indices);
	l_binaryFile.close();

	auto filePath = std::string();
	return JSONWrapper::Save(filePath.c_str(), j);
}

bool AssetService::Save(const MaterialComponent& component)
{
	json j;
	JSONWrapper::to_json(j, component);
	auto filePath = GetAssetFilePath("MaterialComponent");
	return JSONWrapper::Save(filePath.c_str(), j);
}

bool AssetService::Save(const TextureComponent& component, void* textureData)
{
	json j;
	JSONWrapper::to_json(j, component);

	auto l_workingDir = "../Data/Components/";
	std::string l_baseName = component.m_InstanceName.c_str();
	auto l_binaryFileName = l_baseName + ".innobin";
	auto l_binaryFilePath = l_workingDir + l_binaryFileName;

	j["File"] = l_binaryFileName;

	bool binaryResult = STBWrapper::Save(l_binaryFilePath.c_str(), component.m_TextureDesc, textureData);
	if (!binaryResult)
	{
		Log(Error, "Failed to save texture binary data: ", l_binaryFilePath.c_str());
		return false;
	}

	auto filePath = GetAssetFilePath(component.m_InstanceName.c_str());
	return JSONWrapper::Save(filePath.c_str(), j);
}

bool AssetService::Save(const CameraComponent& component)
{
	json j;
	JSONWrapper::to_json(j, component);
	auto filePath = std::string();
	return JSONWrapper::Save(filePath.c_str(), j);
}

bool AssetService::Save(const LightComponent& component)
{
	json j;
	JSONWrapper::to_json(j, component);
	auto filePath = std::string();
	return JSONWrapper::Save(filePath.c_str(), j);
}