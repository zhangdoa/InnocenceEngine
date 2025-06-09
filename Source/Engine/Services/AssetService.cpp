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