#include "AssetService.h"
#include "../Common/ComponentHeaders.h"
#include "../Common/MathHelper.h"
#include "../Common/LogService.h"
#include "../Common/IOService.h"
#include "../Common/TaskScheduler.h"
#include "../Common/ObjectPool.h"
#include "../Common/BCCompression.h"
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
	// deque guarantees reference/pointer stability on push_back, unlike vector.
	// This allows GetMeshAsset to return stable pointers while Allocate appends concurrently.
	std::deque<MeshAssetData> m_MeshAssets;
	std::vector<uint32_t> m_MeshFreeSlots;
	std::vector<uint32_t> m_MeshGenerations;
	std::unordered_map<std::string, MeshAssetHandle> m_MeshLUT;
	std::shared_mutex s_MeshMutex;

	// Material asset registry
	std::deque<MaterialAssetData> m_MaterialAssets;
	std::vector<uint32_t> m_MaterialFreeSlots;
	std::vector<uint32_t> m_MaterialGenerations;
	std::unordered_map<std::string, MaterialAssetHandle> m_MaterialLUT;
	std::shared_mutex s_MaterialMutex;

	// Texture asset registry
	std::deque<TextureAssetData> m_TextureAssets;
	std::vector<uint32_t> m_TextureFreeSlots;
	std::vector<uint32_t> m_TextureGenerations;
	std::unordered_map<std::string, TextureAssetHandle> m_TextureLUT;
	std::shared_mutex s_TextureMutex;
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
	Log(Success, "AssetService: clearing asset registries...");
	// Clear all asset registries before CRT static cleanup.
	// MaterialAssetData contains std::vector<std::string> which must be freed while the
	// heap is still valid — deferring to the static-destructor phase can cause
	// STATUS_HEAP_CORRUPTION on exit if the deques are non-empty.
	std::unique_lock<std::shared_mutex> l_meshLock(s_MeshMutex);
	m_MeshAssets.clear();
	m_MeshFreeSlots.clear();
	m_MeshGenerations.clear();
	m_MeshLUT.clear();
	l_meshLock.unlock();

	std::unique_lock<std::shared_mutex> l_matLock(s_MaterialMutex);
	m_MaterialAssets.clear();
	m_MaterialFreeSlots.clear();
	m_MaterialGenerations.clear();
	m_MaterialLUT.clear();
	l_matLock.unlock();

	std::unique_lock<std::shared_mutex> l_texLock(s_TextureMutex);
	m_TextureAssets.clear();
	m_TextureFreeSlots.clear();
	m_TextureGenerations.clear();
	m_TextureLUT.clear();
	l_texLock.unlock();

	m_ObjectStatus = ObjectStatus::Terminated;
	Log(Success, "AssetService has been terminated.");
	return true;
}

ObjectStatus AssetService::GetStatus()
{
	return ObjectStatus();
}

MeshAssetHandle AssetService::AllocateMeshAsset(const char* name, ObjectLifespan lifespan)
{
	std::unique_lock<std::shared_mutex> l_lock(s_MeshMutex);

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
	std::shared_lock<std::shared_mutex> l_lock(s_MeshMutex);

	if (!handle.IsValid() || handle.m_Index >= m_MeshAssets.size())
		return nullptr;

	if (handle.m_Generation != m_MeshGenerations[handle.m_Index])
		return nullptr;

	auto& l_asset = m_MeshAssets[handle.m_Index];
	if (l_asset.m_Residency == AssetResidency::Released)
		return nullptr;

	return &l_asset;
}

uint32_t AssetService::DebugGetMeshGeneration(uint32_t index)
{
	std::shared_lock<std::shared_mutex> l_lock(s_MeshMutex);
	if (index >= m_MeshGenerations.size())
		return UINT32_MAX;
	return m_MeshGenerations[index];
}

MeshAssetHandle AssetService::FindMeshAsset(const char* name)
{
	std::shared_lock<std::shared_mutex> l_lock(s_MeshMutex);
	auto l_result = m_MeshLUT.find(name);
	if (l_result != m_MeshLUT.end())
		return l_result->second;
	return MeshAssetHandle{};
}

AssetService::MaterialAssetAllocation AssetService::AllocateMaterialAsset(const char* name, ObjectLifespan lifespan)
{
	std::unique_lock<std::shared_mutex> l_lock(s_MaterialMutex);

	auto l_existing = m_MaterialLUT.find(name);
	if (l_existing != m_MaterialLUT.end())
	{
		auto& l_asset = m_MaterialAssets[l_existing->second.m_Index];
		if (l_asset.m_Residency != AssetResidency::Released)
			return { l_existing->second, false };
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

	return { l_handle, true };
}

MaterialAssetData* AssetService::GetMaterialAsset(MaterialAssetHandle handle)
{
	std::shared_lock<std::shared_mutex> l_lock(s_MaterialMutex);

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
	std::shared_lock<std::shared_mutex> l_lock(s_MaterialMutex);
	auto l_result = m_MaterialLUT.find(name);
	if (l_result != m_MaterialLUT.end())
		return l_result->second;
	return MaterialAssetHandle{};
}

TextureAssetHandle AssetService::AllocateTextureAsset(const char* name, ObjectLifespan lifespan)
{
	std::unique_lock<std::shared_mutex> l_lock(s_TextureMutex);

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
	std::shared_lock<std::shared_mutex> l_lock(s_TextureMutex);

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
	std::shared_lock<std::shared_mutex> l_lock(s_TextureMutex);
	auto l_result = m_TextureLUT.find(name);
	if (l_result != m_TextureLUT.end())
		return l_result->second;
	return TextureAssetHandle{};
}

void AssetService::ReleaseAssetsByLifespan(ObjectLifespan lifespan)
{
	uint32_t l_meshReleased = 0, l_matReleased = 0, l_texReleased = 0;
	{
		std::unique_lock<std::shared_mutex> l_lock(s_MeshMutex);
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
				++l_meshReleased;
			}
		}
	}

	{
		std::unique_lock<std::shared_mutex> l_lock(s_MaterialMutex);
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
				++l_matReleased;
			}
		}
	}

	{
		std::unique_lock<std::shared_mutex> l_lock(s_TextureMutex);
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
				++l_texReleased;
			}
		}
	}

	Log(Verbose, "AssetService::ReleaseAssetsByLifespan(", lifespan,
		") — released meshes=", l_meshReleased, " materials=", l_matReleased, " textures=", l_texReleased);
}

std::string AssetService::GetAssetFilePath(const char* componentName)
{
	auto l_dataDir = g_Engine->Get<IOService>()->getDataDirectory();
	auto l_name = std::string(componentName) + ".json";

	// Search order: project components, then generated (imported) components
	auto l_projectPath = std::string(INNO_PROJECT_NAME) + "/Components/" + l_name;
	if (std::filesystem::exists(l_dataDir + l_projectPath))
		return l_projectPath;

	auto l_generatedPath = "Generated/Components/" + l_name;
	if (std::filesystem::exists(l_dataDir + l_generatedPath))
		return l_generatedPath;

	// Default to generated (where imports write to)
	return l_generatedPath;
}

std::string AssetService::GetBinaryFilePath(const char* binaryFileName)
{
	auto l_dataDir = g_Engine->Get<IOService>()->getDataDirectory();

	// Search order: project components, then generated (imported) components
	auto l_projectPath = l_dataDir + INNO_PROJECT_NAME + std::string("/Components/") + binaryFileName;
	if (std::filesystem::exists(l_projectPath))
		return l_projectPath;

	auto l_generatedPath = l_dataDir + "Generated/Components/" + binaryFileName;
	if (std::filesystem::exists(l_generatedPath))
		return l_generatedPath;

	// Default to generated (where imports write to)
	return l_generatedPath;
}

std::string AssetService::GetComponentDirectory()
{
	return g_Engine->Get<IOService>()->getComponentDirectory();
}

bool AssetService::Import(const char* fileName)
{
	auto* l_io = g_Engine->Get<IOService>();
	auto l_extension = l_io->getFileExtension(fileName);
	std::string l_fileName = fileName;

	if (l_extension == ".obj" || l_extension == ".OBJ" || l_extension == ".fbx" || l_extension == ".FBX" || l_extension == ".gltf" || l_extension == ".GLTF" || l_extension == ".ply" || l_extension == ".PLY" || l_extension == ".md5mesh")
	{
		// Each import task runs concurrently — asset registry access is protected per-type by
		// shared_mutex inside Allocate*/Get*/Find* functions.
		auto tempTask = g_Engine->Get<TaskScheduler>()->Submit(ITask::Desc("Import Model Task", ITask::Type::Once), [=]()
			{
				AssimpWrapper::Import(l_fileName.c_str());
			});
		tempTask->Activate();
		return true;
	}
	else if (l_extension == ".png" || l_extension == ".PNG" || l_extension == ".jpg" || l_extension == ".JPG" || l_extension == ".jpeg" || l_extension == ".JPEG" || l_extension == ".tga" || l_extension == ".TGA")
	{
		// Auto-detect material slot + sRGB from filename suffix using the
		// AmbientCG / common-PBR convention. Unknown suffixes fall through
		// as albedo (slot 1, sRGB) — the safest default for an unlabelled
		// colour image.
		std::string l_baseName = l_io->getFileName(fileName);
		auto l_dot = l_baseName.find_last_of('.');
		if (l_dot != std::string::npos)
			l_baseName.erase(l_dot);
		std::string l_lowered;
		l_lowered.reserve(l_baseName.size());
		for (char c : l_baseName)
			l_lowered.push_back(static_cast<char>(std::tolower(static_cast<unsigned char>(c))));

		uint32_t l_slot   = 1;
		bool     l_isSRGB = true;
		if (l_lowered.find("normal") != std::string::npos)         { l_slot = 0; l_isSRGB = false; }
		else if (l_lowered.find("metalness") != std::string::npos) { l_slot = 2; l_isSRGB = false; }
		else if (l_lowered.find("metallic")  != std::string::npos) { l_slot = 2; l_isSRGB = false; }
		else if (l_lowered.find("roughness") != std::string::npos) { l_slot = 3; l_isSRGB = false; }
		else if (l_lowered.find("ambientocclusion") != std::string::npos
		      || l_lowered.find("_ao") != std::string::npos)        { l_slot = 4; l_isSRGB = false; }
		else if (l_lowered.find("color")     != std::string::npos
		      || l_lowered.find("albedo")    != std::string::npos
		      || l_lowered.find("basecolor") != std::string::npos)  { l_slot = 1; l_isSRGB = true;  }

		std::string l_instanceName = l_baseName + ".TextureComponent";
		auto tempTask = g_Engine->Get<TaskScheduler>()->Submit(ITask::Desc("Import Texture Task", ITask::Type::Once),
			[fileNameCopy = l_fileName, instanceName = std::move(l_instanceName), l_slot, l_isSRGB]()
			{
				ImportTexture(fileNameCopy.c_str(), TextureSampler::Sampler2D, TextureUsage::Sample, l_isSRGB, l_slot, instanceName.c_str());
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

bool AssetService::ImportSync(const char* fileName)
{
	auto l_extension = g_Engine->Get<IOService>()->getFileExtension(fileName);

	if (l_extension == ".obj" || l_extension == ".OBJ" || l_extension == ".fbx" || l_extension == ".FBX" || l_extension == ".gltf" || l_extension == ".GLTF" || l_extension == ".ply" || l_extension == ".PLY" || l_extension == ".md5mesh")
	{
		// Offline/synchronous import: run directly on the calling thread with no scheduler involvement.
		// This avoids task queue races during engine teardown and is the correct semantic for tools/tests.
		return AssimpWrapper::Import(fileName);
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

std::string AssetService::ImportTexture(const char*    absolutePath,
                                        TextureSampler sampler,
                                        TextureUsage   usage,
                                        bool           isSRGB,
                                        uint32_t       slotIndex,
                                        const char*    instanceName)
{
	if (!absolutePath || !instanceName || !*instanceName)
	{
		Log(Error, "AssetService::ImportTexture: absolutePath and instanceName are required.");
		return {};
	}

	if (!g_Engine->Get<IOService>()->isFileExist(absolutePath))
	{
		Log(Warning, "AssetService::ImportTexture: file not found: ", absolutePath);
		return {};
	}

	TextureComponent l_Texture = {};
	l_Texture.m_InstanceName        = instanceName;
	l_Texture.m_TextureDesc.Sampler = sampler;
	l_Texture.m_TextureDesc.Usage   = usage;
	l_Texture.m_TextureDesc.IsSRGB  = isSRGB;

	void* l_RawData = STBWrapper::Load(absolutePath, l_Texture);
	if (!l_RawData)
	{
		Log(Error, "AssetService::ImportTexture: STB decode failed: ", absolutePath);
		return {};
	}

	TextureDesc l_CompressedDesc = {};
	void* l_TextureData = BCCompression::CompressRGBAToBC(l_Texture.m_TextureDesc, l_RawData, slotIndex, l_CompressedDesc);
	if (!l_TextureData)
	{
		Log(Error, "AssetService::ImportTexture: BC compress failed: ", absolutePath);
		return {};
	}
	l_Texture.m_TextureDesc = l_CompressedDesc;

	if (!Save(l_Texture, l_TextureData))
	{
		Log(Error, "AssetService::ImportTexture: Save failed: ", absolutePath);
		return {};
	}

	Log(Success, "AssetService::ImportTexture: saved ", instanceName, " from ", absolutePath);
	return std::string(instanceName);
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

	auto l_componentDir = GetComponentDirectory();
	std::filesystem::create_directories(l_componentDir);

	std::string l_baseName = component.m_InstanceName.c_str();
	auto l_binaryFileName = l_baseName + ".innobin";
	auto l_binaryFilePath = l_componentDir + l_binaryFileName;

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

	auto filePath = GetAssetFilePath(component.m_InstanceName.c_str());
	return JSONWrapper::Save(filePath.c_str(), j);
}

bool AssetService::Save(const MaterialComponent& component)
{
	json j;
	JSONWrapper::to_json(j, component);
	auto filePath = GetAssetFilePath(component.m_InstanceName.c_str());
	return JSONWrapper::Save(filePath.c_str(), j);
}

bool AssetService::Save(const TextureComponent& component, void* textureData)
{
	json j;
	JSONWrapper::to_json(j, component);

	auto l_componentDir = GetComponentDirectory();
	std::filesystem::create_directories(l_componentDir);

	std::string l_baseName = component.m_InstanceName.c_str();
	auto l_binaryFileName = l_baseName + ".innobin";
	auto l_binaryFilePath = l_componentDir + l_binaryFileName;

	j["File"] = l_binaryFileName;

	bool binaryResult;
	if (component.m_TextureDesc.PixelDataType == TexturePixelDataType::Compressed)
	{
		uint32_t blockBytes;
		switch (component.m_TextureDesc.PixelDataFormat)
		{
		case TexturePixelDataFormat::BC1: blockBytes = 8;  break;
		case TexturePixelDataFormat::BC4: blockBytes = 8;  break;
		case TexturePixelDataFormat::BC3: blockBytes = 16; break;
		case TexturePixelDataFormat::BC5: blockBytes = 16; break;
		default:                          blockBytes = 0;  break;
		}
		if (blockBytes == 0)
		{
			Log(Error, "AssetService::Save: unknown BC format for texture: ", l_binaryFilePath.c_str());
			return false;
		}
		uint32_t blocksX = (component.m_TextureDesc.Width  + 3) / 4;
		uint32_t blocksY = (component.m_TextureDesc.Height + 3) / 4;
		size_t dataSize  = static_cast<size_t>(blocksX) * blocksY * blockBytes;
		auto* l_rawData = static_cast<const char*>(textureData);
		std::ofstream l_bcFile(l_binaryFilePath, std::ios::out | std::ios::trunc | std::ios::binary);
		if (!l_bcFile.is_open())
		{
			Log(Error, "AssetService::Save: cannot open BC binary file: ", l_binaryFilePath.c_str());
			binaryResult = false;
		}
		else
		{
			l_bcFile.write(l_rawData, static_cast<std::streamsize>(dataSize));
			binaryResult = l_bcFile.good();
			l_bcFile.close();
		}
	}
	else
	{
		binaryResult = STBWrapper::Save(l_binaryFilePath.c_str(), component.m_TextureDesc, textureData);
	}
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