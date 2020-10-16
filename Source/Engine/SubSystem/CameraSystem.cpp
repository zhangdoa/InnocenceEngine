#include "CameraSystem.h"
#include "../Component/CameraComponent.h"
#include "../Core/InnoRandomizer.h"
#include "../Core/InnoLogger.h"
#include "../Common/InnoMathHelper.h"

#include "../Interface/IModuleManager.h"

extern IModuleManager* g_pModuleManager;

namespace CameraSystemNS
{
	const size_t m_MaxComponentCount = 32;

	void generateProjectionMatrix(CameraComponent* cameraComponent);
	void generateFrustum(CameraComponent* cameraComponent);
	void generateRayOfEye(CameraComponent* cameraComponent);
}

void CameraSystemNS::generateProjectionMatrix(CameraComponent* cameraComponent)
{
	auto l_resolution = g_pModuleManager->getRenderingFrontend()->getScreenResolution();
	cameraComponent->m_WHRatio = (float)l_resolution.x / (float)l_resolution.y;
	cameraComponent->m_projectionMatrix = InnoMath::generatePerspectiveMatrix((cameraComponent->m_FOVX / 180.0f) * PI<float>, cameraComponent->m_WHRatio, cameraComponent->m_zNear, cameraComponent->m_zFar);
}

void CameraSystemNS::generateFrustum(CameraComponent* cameraComponent)
{
	auto l_transformComponent = g_pModuleManager->getComponentManager()->Find<TransformComponent>(cameraComponent->m_Owner);

	if (l_transformComponent != nullptr)
	{
		auto l_pCamera = cameraComponent->m_projectionMatrix;
		auto l_rCamera = InnoMath::toRotationMatrix(l_transformComponent->m_globalTransformVector.m_rot);
		auto l_tCamera = InnoMath::toTranslationMatrix(l_transformComponent->m_globalTransformVector.m_pos);

		auto l_vertices = InnoMath::generateFrustumVerticesWS(l_pCamera, l_rCamera, l_tCamera);
		cameraComponent->m_frustum = InnoMath::makeFrustum(&l_vertices[0]);
	}
}

void CameraSystemNS::generateRayOfEye(CameraComponent* cameraComponent)
{
	auto l_transformComponent = g_pModuleManager->getComponentManager()->Find<TransformComponent>(cameraComponent->m_Owner);

	if (l_transformComponent != nullptr)
	{
		cameraComponent->m_rayOfEye.m_origin = l_transformComponent->m_globalTransformVector.m_pos;
		cameraComponent->m_rayOfEye.m_direction = InnoMath::getDirection(Direction::Backward, l_transformComponent->m_globalTransformVector.m_rot);
	}
}

using namespace CameraSystemNS;

bool InnoCameraSystem::Setup(ISystemConfig* systemConfig)
{
	g_pModuleManager->getComponentManager()->RegisterType<CameraComponent>(m_MaxComponentCount);

	return true;
}

bool InnoCameraSystem::Initialize()
{
	return true;
}

bool InnoCameraSystem::Update()
{
	auto l_components = g_pModuleManager->getComponentManager()->GetAll<CameraComponent>();

	for (auto i : l_components)
	{
		i->m_WHRatio = i->m_widthScale / i->m_heightScale;
		generateProjectionMatrix(i);
		generateRayOfEye(i);
		generateFrustum(i);
	}
	return true;
}

bool InnoCameraSystem::OnFrameEnd()
{
	return true;
}

bool InnoCameraSystem::Terminate()
{
	return true;
}

ObjectStatus InnoCameraSystem::GetStatus()
{
	return ObjectStatus();
}