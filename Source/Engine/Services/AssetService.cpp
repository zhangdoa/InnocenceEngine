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

#define recordLoaded( funcName, type, value ) \
bool AssetService::RecordLoaded##funcName(const char * fileName, type value) \
{ \
	m_loaded##funcName.emplace(fileName, value); \
\
	return true; \
}

#define findLoaded( funcName, type, value ) \
bool AssetService::FindLoaded##funcName(const char * fileName, type value) \
{ \
	auto l_loaded##funcName = m_loaded##funcName.find(fileName); \
	if (l_loaded##funcName != m_loaded##funcName.end()) \
	{ \
		value = l_loaded##funcName->second; \
	\
		return true; \
	} \
	else \
	{ \
	Log(Verbose, "", fileName, " is not loaded yet."); \
	\
	return false; \
	} \
}

namespace AssetServiceNS
{
	std::function<void(ModelComponent*)> f_LoadModelTask;

	TObjectPool<RenderableSet>* m_renderableSetPool;
	TObjectPool<Model>* m_modelPool;
	std::vector<RenderableSet*> m_renderableSetList;

	std::unordered_map<std::string, RenderableSet*> m_loadedRenderableSet;
	std::unordered_map<std::string, Model*> m_loadedModel;
	std::unordered_map<std::string, TextureComponent*> m_loadedTexture;
	std::unordered_map<std::string, AnimationComponent*> m_loadedAnimation;
	std::unordered_map<std::string, SkeletonComponent*> m_loadedSkeleton;

	std::atomic<uint64_t> m_currentRenderableSetIndex = 0;
	std::shared_mutex m_mutexRenderableSet;
	std::shared_mutex m_mutexModel;

	ObjectStatus m_ObjectStatus = ObjectStatus::Terminated;
}

using namespace AssetServiceNS;

bool AssetService::Setup(ISystemConfig* systemConfig)
{
	g_Engine->Get<ComponentManager>()->RegisterType<ModelComponent>(32768, this);

	f_LoadModelTask = [=](ModelComponent* i)
	{
		if (!i->m_modelFileName.empty())
		{
			i->m_Model = LoadModel(i->m_modelFileName.c_str());
		}
		else if (i->m_MeshShape != MeshShape::Customized)
		{
			i->m_Model = AddModel(i->m_MeshShape, ShaderModel::Opaque);
			auto l_pair = GetRenderableSet(i->m_Model->renderableSets.m_startOffset);
			g_Engine->getRenderingServer()->Initialize(l_pair->material);
		}
		else
		{
			Log(Warning, "Custom shape mesh specified without a model preset file.");
			return;
		}

		g_Engine->Get<PhysicsSimulationService>()->CreateCollisionComponents(i);
		i->m_ObjectStatus = ObjectStatus::Activated;
	};

	m_renderableSetPool = TObjectPool<RenderableSet>::Create(65536);
	m_renderableSetList.reserve(65536);
	m_modelPool = TObjectPool<Model>::Create(4096);

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

bool AssetService::ConvertModel(const char* fileName, const char* exportPath)
{
	auto l_extension = g_Engine->Get<IOService>()->getFileExtension(fileName);
	std::string l_fileName = fileName;

	if (l_extension == ".obj" || l_extension == ".OBJ" || l_extension == ".fbx" || l_extension == ".FBX" || l_extension == ".gltf" || l_extension == ".GLTF" || l_extension == ".md5mesh")
	{
		auto tempTask = g_Engine->Get<TaskScheduler>()->Submit(ITask::Desc("Convert Model Task", ITask::Type::Once), [=]()
			{
				AssimpWrapper::ConvertModel(l_fileName.c_str(), exportPath);
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

Model* AssetService::LoadModel(const char* fileName)
{
	auto l_extension = g_Engine->Get<IOService>()->getFileExtension(fileName);
	if (l_extension == ".InnoModel")
	{
		Model* l_loadedModel;

		if (FindLoadedModel(fileName, l_loadedModel))
		{
			return l_loadedModel;
		}
		else
		{
			auto l_result = JSONWrapper::LoadModel(fileName);
			RecordLoadedModel(fileName, l_result);

			return l_result;
		}
	}
	else
	{
		Log(Warning, fileName, " is not supported!");
		return nullptr;
	}
}

TextureComponent* AssetService::LoadTexture(const char* fileName)
{
	TextureComponent* l_TextureComp;

	if (FindLoadedTexture(fileName, l_TextureComp))
	{
		return l_TextureComp;
	}
	else
	{
		l_TextureComp = STBWrapper::LoadTexture(fileName);
		if (l_TextureComp)
		{
			RecordLoadedTexture(fileName, l_TextureComp);
		}
		else
		{
			auto l_defaultMaterial = g_Engine->Get<TemplateAssetService>()->GetDefaultMaterialComponent();
			l_TextureComp = l_defaultMaterial->m_TextureSlots[0].m_Texture;
		}
		return l_TextureComp;
	}
}

bool AssetService::SaveTexture(const char* fileName, const TextureDesc& textureDesc, void* textureData)
{
    return STBWrapper::SaveTexture(fileName, textureDesc, textureData);
}

bool AssetService::SaveTexture(const char* fileName, TextureComponent* TextureComp)
{
	return STBWrapper::SaveTexture(fileName, TextureComp->m_TextureDesc, TextureComp->m_InitialData);
}

bool AssetService::LoadAssetsForComponents(bool AsyncLoad)
{
	auto l_modelComponents = g_Engine->Get<ComponentManager>()->GetAll<ModelComponent>();
	for (auto i : l_modelComponents)
	{
		auto l_loadModelTask = g_Engine->Get<TaskScheduler>()->Submit(ITask::Desc("Load Model Task", ITask::Type::Once, 4), f_LoadModelTask, i);
		l_loadModelTask->Activate();
	}

	return true;
}

recordLoaded(RenderableSet, RenderableSet*, pair)
findLoaded(RenderableSet, RenderableSet*&, pair)

recordLoaded(Model, Model*, model)
findLoaded(Model, Model*&, model)

recordLoaded(Texture, TextureComponent*, texture)
findLoaded(Texture, TextureComponent*&, texture)

recordLoaded(Skeleton, SkeletonComponent*, skeleton)
findLoaded(Skeleton, SkeletonComponent*&, skeleton)

recordLoaded(Animation, AnimationComponent*, animation)
findLoaded(Animation, AnimationComponent*&, animation)

ArrayRangeInfo AssetService::AddRenderableSets(uint64_t count)
{
	std::unique_lock<std::shared_mutex> lock{ m_mutexRenderableSet };

	ArrayRangeInfo l_result;
	l_result.m_startOffset = m_currentRenderableSetIndex;
	l_result.m_count = count;

	m_currentRenderableSetIndex += count;

	for (size_t i = 0; i < count; i++)
	{
		m_renderableSetList.emplace_back(m_renderableSetPool->Spawn());
	}

	Log(Verbose, "Added ", count, " renderable sets.");
	return l_result;
}

RenderableSet* AssetService::GetRenderableSet(uint64_t index)
{
	std::unique_lock<std::shared_mutex> lock{ m_mutexRenderableSet };

	return m_renderableSetList[index];
}

Model* AssetService::AddModel()
{
	std::unique_lock<std::shared_mutex> lock{ m_mutexModel };

	return m_modelPool->Spawn();
}

Model* AssetService::AddModel(MeshShape shape, ShaderModel shaderModel)
{
	auto l_mesh = g_Engine->Get<TemplateAssetService>()->GetMeshComponent(shape);
	auto l_material = g_Engine->getRenderingServer()->AddMaterialComponent();
	l_material->m_ShaderModel = shaderModel;
	l_material->m_ObjectStatus = ObjectStatus::Created;

	auto l_result = AddModel();
	l_result->renderableSets = AddRenderableSets(1);

	auto l_pair = GetRenderableSet(l_result->renderableSets.m_startOffset);
	l_pair->mesh = l_mesh;
	l_pair->material = l_material;

	return l_result;
}
