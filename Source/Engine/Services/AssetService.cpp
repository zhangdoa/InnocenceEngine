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

bool AssetService::Load(const char* fileName, ModelComponent& component)
{
	return JSONWrapper::Load(fileName, component);
}

bool AssetService::Load(const char* fileName, DrawCallComponent& component)
{
	return JSONWrapper::Load(fileName, component);
}

bool AssetService::Load(const char* fileName, MeshComponent& component)
{
	return JSONWrapper::Load(fileName, component);
}

bool AssetService::Load(const char* fileName, MaterialComponent& component)
{
	return JSONWrapper::Load(fileName, component);
}

bool AssetService::Load(const char* fileName, TextureComponent& component)
{
	return JSONWrapper::Load(fileName, component);
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

bool AssetService::Save(const ModelComponent& component)
{
	json j;
	JSONWrapper::to_json(j, component);
	auto filePath = GetAssetFilePath(component.m_InstanceName.c_str());
	return JSONWrapper::Save(filePath.c_str(), j);
}

bool AssetService::Save(const DrawCallComponent& component)
{
	json j;
	JSONWrapper::to_json(j, component);
	auto filePath = GetAssetFilePath(component.m_InstanceName.c_str());
	return JSONWrapper::Save(filePath.c_str(), j);
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

	std::string l_baseName = component.m_InstanceName.c_str();
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
	auto filePath = GetAssetFilePath(component.m_InstanceName.c_str());
	return JSONWrapper::Save(filePath.c_str(), j);
}

bool AssetService::Save(const LightComponent& component)
{
	json j;
	JSONWrapper::to_json(j, component);
	auto filePath = GetAssetFilePath(component.m_InstanceName.c_str());
	return JSONWrapper::Save(filePath.c_str(), j);
}