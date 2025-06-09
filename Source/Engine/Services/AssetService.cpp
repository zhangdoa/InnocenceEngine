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
#include "ComponentManager.h"
#include "RenderingContextService.h"
#include "TemplateAssetService.h"
#include "SceneService.h"
#include "PhysicsSimulationService.h"

#include "../Engine.h"
using namespace Inno;

namespace AssetServiceNS
{
	ObjectStatus m_ObjectStatus = ObjectStatus::Terminated;
}

using namespace AssetServiceNS;

bool AssetService::Setup(ISystemConfig* systemConfig)
{
	g_Engine->Get<ComponentManager>()->RegisterType<ModelComponent>(32768, this);
	g_Engine->Get<ComponentManager>()->RegisterType<DrawCallComponent>(65536, this);

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

bool AssetService::Import(const char* fileName, const char* exportPath)
{
	auto l_extension = g_Engine->Get<IOService>()->getFileExtension(fileName);
	std::string l_fileName = fileName;

	if (l_extension == ".obj" || l_extension == ".OBJ" || l_extension == ".fbx" || l_extension == ".FBX" || l_extension == ".gltf" || l_extension == ".GLTF" || l_extension == ".md5mesh")
	{
		auto tempTask = g_Engine->Get<TaskScheduler>()->Submit(ITask::Desc("Import Model Task", ITask::Type::Once), [=]()
			{
				AssimpWrapper::Import(l_fileName.c_str(), exportPath);
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
	JSONWrapper::Load(fileName, component);

	return component.m_UUID != 0;
}


bool AssetService::Load(const char* fileName, ModelComponent& component)
{
	JSONWrapper::Load(fileName, component);

	return component.m_UUID != 0;
}

bool AssetService::Load(const char* fileName, DrawCallComponent& component)
{
	JSONWrapper::Load(fileName, component);

	return component.m_UUID != 0;
}

bool AssetService::Load(const char* fileName, MeshComponent& component)
{
	JSONWrapper::Load(fileName, component);

	return component.m_UUID != 0;
}

bool AssetService::Load(const char* fileName, MaterialComponent& component)
{
	JSONWrapper::Load(fileName, component);

	return component.m_UUID != 0;
}

bool AssetService::Load(const char* fileName, TextureComponent& component)
{
	JSONWrapper::Load(fileName, component);

	return component.m_UUID != 0;
}

// bool AssetService::Load(const char* fileName, SkeletonComponent& component)
// {
// 	JSONWrapper::Load(fileName, component);

// 	return component.m_UUID != 0;
// }

// bool AssetService::Load(const char* fileName, AnimationComponent& component)
// {
// 	JSONWrapper::Load(fileName, component);

// 	return component.m_UUID != 0;
// }

bool AssetService::Load(const char* fileName, CameraComponent& component)
{
	JSONWrapper::Load(fileName, component);

	return component.m_UUID != 0;
}

bool AssetService::Load(const char* fileName, LightComponent& component)
{
	JSONWrapper::Load(fileName, component);

	return component.m_UUID != 0;
}

bool AssetService::Save(const char* fileName, const TextureDesc& textureDesc, void* textureData)
{
    return STBWrapper::Save(fileName, textureDesc, textureData);
}

bool AssetService::Save(const char* fileName, const TransformComponent& component)
{
	json j;
	JSONWrapper::to_json(j, component);
	return JSONWrapper::Save(fileName, j);
}

bool AssetService::Save(const char* fileName, const ModelComponent& component)
{
	json j;
	JSONWrapper::to_json(j, component);
	return JSONWrapper::Save(fileName, j);
}

bool AssetService::Save(const char* fileName, const DrawCallComponent& component)
{
	json j;
	j["ComponentType"] = component.GetTypeID();
	
	// Add mesh component reference (UUID stored as uint64_t)
	if (component.m_MeshComponent != 0)
	{
		j["MeshComponent"]["Name"] = "mesh_" + std::to_string(component.m_MeshComponent);
	}
	
	// Add material component reference (UUID stored as uint64_t)
	if (component.m_MaterialComponent != 0)
	{
		j["MaterialComponent"]["Name"] = "material_" + std::to_string(component.m_MaterialComponent);
	}
	
	return JSONWrapper::Save(fileName, j);
}

bool AssetService::Save(const char* fileName, const MeshComponent& component, std::vector<Vertex>& vertices, std::vector<Index>& indices)
{
	// Create metadata JSON
	json j;
	j["ComponentType"] = component.GetTypeID();
	j["MeshShape"] = MeshShape::Customized;
	j["VerticesNumber"] = vertices.size();
	j["IndicesNumber"] = indices.size();
	
	// Generate binary file path alongside JSON metadata
	auto l_workingDir = g_Engine->Get<IOService>()->getWorkingDirectory();
	auto l_baseName = g_Engine->Get<IOService>()->getFileName(fileName);
	auto l_binaryFileName = l_baseName + ".InnoMesh";
	auto l_binaryFilePath = l_workingDir + l_binaryFileName;
	
	j["File"] = l_binaryFileName;
	
	// Save binary data (vertices + indices)
	std::ofstream l_binaryFile(l_binaryFilePath, std::ios::out | std::ios::trunc | std::ios::binary);
	if (!l_binaryFile.is_open())
	{
		Log(Error, "Failed to open binary mesh file: ", l_binaryFilePath.c_str());
		return false;
	}
	
	g_Engine->Get<IOService>()->serializeVector(l_binaryFile, vertices);
	g_Engine->Get<IOService>()->serializeVector(l_binaryFile, indices);
	l_binaryFile.close();
	
	// Save JSON metadata
	bool result = JSONWrapper::Save(fileName, j);
	if (result)
	{
		Log(Success, "Saved MeshComponent: ", fileName, " with binary data: ", l_binaryFileName.c_str());
	}
	return result;
}

bool AssetService::Save(const char* fileName, const MaterialComponent& component)
{
	json j;
	// Build material JSON with texture references
	j["ComponentType"] = component.GetTypeID();
	j["ShaderModel"] = component.m_ShaderModel;
	
	// Material attributes
	j["Albedo"]["R"] = component.m_materialAttributes.AlbedoR;
	j["Albedo"]["G"] = component.m_materialAttributes.AlbedoG;
	j["Albedo"]["B"] = component.m_materialAttributes.AlbedoB;
	j["Albedo"]["A"] = component.m_materialAttributes.Alpha;
	j["Metallic"] = component.m_materialAttributes.Metallic;
	j["Roughness"] = component.m_materialAttributes.Roughness;
	j["AO"] = component.m_materialAttributes.AO;
	j["Thickness"] = component.m_materialAttributes.Thickness;
	
	// Texture component references
	if (component.m_TextureComponents.size() > 0)
	{
		for (size_t i = 0; i < component.m_TextureComponents.size(); i++)
		{
			json textureRef;
			// TODO: Get proper texture component name/path
			textureRef["Name"] = "texture_" + std::to_string(component.m_TextureComponents[i]);
			j["TextureComponents"].push_back(textureRef);
		}
	}
	
	return JSONWrapper::Save(fileName, j);
}

bool AssetService::Save(const char* fileName, const TextureComponent& component, void* textureData)
{
	// Create metadata JSON
	json j;
	j["ComponentType"] = component.GetTypeID();
	j["Sampler"] = component.m_TextureDesc.Sampler;
	j["Usage"] = component.m_TextureDesc.Usage;
	j["IsSRGB"] = component.m_TextureDesc.IsSRGB;
	
	// Generate image file path alongside JSON metadata
	auto l_workingDir = g_Engine->Get<IOService>()->getWorkingDirectory();
	auto l_baseName = g_Engine->Get<IOService>()->getFileName(fileName);
	auto l_imageFileName = l_baseName + ".png";  // Default to PNG for simplicity
	auto l_imageFilePath = l_workingDir + l_imageFileName;
	
	j["File"] = l_imageFileName;
	
	// Save binary image data using STBWrapper
	bool binaryResult = STBWrapper::Save(l_imageFilePath.c_str(), component.m_TextureDesc, textureData);
	if (!binaryResult)
	{
		Log(Error, "Failed to save texture binary data: ", l_imageFilePath.c_str());
		return false;
	}
	
	// Save JSON metadata  
	bool result = JSONWrapper::Save(fileName, j);
	if (result)
	{
		Log(Success, "Saved TextureComponent: ", fileName, " with binary data: ", l_imageFileName.c_str());
	}
	return result;
}

bool AssetService::Save(const char* fileName, const CameraComponent& component)
{
	json j;
	JSONWrapper::to_json(j, component);
	return JSONWrapper::Save(fileName, j);
}

bool AssetService::Save(const char* fileName, const LightComponent& component)
{
	json j;
	JSONWrapper::to_json(j, component);
	return JSONWrapper::Save(fileName, j);
}