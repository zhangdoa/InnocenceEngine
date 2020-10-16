#include "CameraComponentManager.h"
#include "../Component/CameraComponent.h"
#include "../Template/ObjectPool.h"
#include "../Core/InnoMemory.h"
#include "../Core/InnoRandomizer.h"
#include "../Core/InnoLogger.h"
#include "../Common/CommonMacro.inl"
#include "CommonFunctionDefinitionMacro.inl"
#include "../Common/InnoMathHelper.h"

#include "ITransformComponentManager.h"

#include "../Interface/IModuleManager.h"

extern IModuleManager* g_pModuleManager;

namespace CameraComponentManagerNS
{
	const size_t m_MaxComponentCount = 32;
	size_t m_CurrentComponentIndex = 0;
	TObjectPool<CameraComponent>* m_ComponentPool;
	ThreadSafeVector<CameraComponent*> m_Components;
	ThreadSafeUnorderedMap<InnoEntity*, CameraComponent*> m_ComponentsMap;

	std::function<void()> f_SceneLoadingStartCallback;
	std::function<void()> f_SceneLoadingFinishCallback;

	void generateProjectionMatrix(CameraComponent* cameraComponent);
	void generateFrustum(CameraComponent* cameraComponent);
	void generateRayOfEye(CameraComponent* cameraComponent);
}

void CameraComponentManagerNS::generateProjectionMatrix(CameraComponent* cameraComponent)
{
	auto l_resolution = g_pModuleManager->getRenderingFrontend()->getScreenResolution();
	cameraComponent->m_WHRatio = (float)l_resolution.x / (float)l_resolution.y;
	cameraComponent->m_projectionMatrix = InnoMath::generatePerspectiveMatrix((cameraComponent->m_FOVX / 180.0f) * PI<float>, cameraComponent->m_WHRatio, cameraComponent->m_zNear, cameraComponent->m_zFar);
}

void CameraComponentManagerNS::generateFrustum(CameraComponent* cameraComponent)
{
	auto l_transformComponent = GetComponent(TransformComponent, cameraComponent->m_Owner);

	if (l_transformComponent != nullptr)
	{
		auto l_pCamera = cameraComponent->m_projectionMatrix;
		auto l_rCamera = InnoMath::toRotationMatrix(l_transformComponent->m_globalTransformVector.m_rot);
		auto l_tCamera = InnoMath::toTranslationMatrix(l_transformComponent->m_globalTransformVector.m_pos);

		auto l_vertices = InnoMath::generateFrustumVerticesWS(l_pCamera, l_rCamera, l_tCamera);
		cameraComponent->m_frustum = InnoMath::makeFrustum(&l_vertices[0]);
	}
}

void CameraComponentManagerNS::generateRayOfEye(CameraComponent* cameraComponent)
{
	auto l_transformComponent = GetComponent(TransformComponent, cameraComponent->m_Owner);

	if (l_transformComponent != nullptr)
	{
		cameraComponent->m_rayOfEye.m_origin = l_transformComponent->m_globalTransformVector.m_pos;
		cameraComponent->m_rayOfEye.m_direction = InnoMath::getDirection(Direction::Backward, l_transformComponent->m_globalTransformVector.m_rot);
	}
}

using namespace CameraComponentManagerNS;

bool InnoCameraComponentManager::Setup()
{
	m_ComponentPool = TObjectPool<CameraComponent>::Create(m_MaxComponentCount);
	m_Components.reserve(m_MaxComponentCount);
	m_ComponentsMap.reserve(m_MaxComponentCount);

	f_SceneLoadingStartCallback = [&]() {
		CleanComponentContainers(CameraComponent);
	};

	f_SceneLoadingFinishCallback = [&]() {
	};

	g_pModuleManager->getSceneSystem()->addSceneLoadingStartCallback(&f_SceneLoadingStartCallback);
	g_pModuleManager->getSceneSystem()->addSceneLoadingFinishCallback(&f_SceneLoadingFinishCallback);

	return true;
}

bool InnoCameraComponentManager::Initialize()
{
	return true;
}

bool InnoCameraComponentManager::Simulate()
{
	for (auto i : m_Components)
	{
		i->m_WHRatio = i->m_widthScale / i->m_heightScale;
		generateProjectionMatrix(i);
		generateRayOfEye(i);
		generateFrustum(i);
	}
	return true;
}

bool InnoCameraComponentManager::PostFrame()
{
	return true;
}

bool InnoCameraComponentManager::Terminate()
{
	return true;
}

InnoComponent* InnoCameraComponentManager::Spawn(const InnoEntity* parentEntity, bool serializable, ObjectLifespan objectLifespan)
{
	SpawnComponentImpl(CameraComponent);
}

void InnoCameraComponentManager::Destroy(InnoComponent* component)
{
	DestroyComponentImpl(CameraComponent);
}

InnoComponent* InnoCameraComponentManager::Find(const InnoEntity* parentEntity)
{
	GetComponentImpl(CameraComponent, parentEntity);
}

CameraComponent* InnoCameraComponentManager::Get(std::size_t index)
{
	if (index >= m_Components.size())
	{
		return nullptr;
	}
	return m_Components[index];
}

const std::vector<CameraComponent*>& InnoCameraComponentManager::GetAllComponents()
{
	return m_Components.getRawData();
}