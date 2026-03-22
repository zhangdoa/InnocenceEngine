#include "CameraService.h"
#include "../Component/CameraComponent.h"
#include "../Component/TransformComponent.h"
#include "../Common/LogService.h"
#include "../Common/Randomizer.h"
#include "../Common/MathHelper.h"
#include "RenderingConfigurationService.h"
#include "EntityRegistry.h"

#include "../Engine.h"

using namespace Inno;

namespace CameraServiceNS
{
	const size_t m_MaxComponentCount = 32;

	void GenerateProjectionMatrix(CameraComponent* cameraComponent);
	void GenerateFrustum(CameraComponent* cameraComponent, EntityID EntityID);
	void GenerateRayOfEye(CameraComponent* cameraComponent, EntityID EntityID);

	CameraComponent* m_MainCamera;
	CameraComponent* m_ActiveCamera;

	ObjectStatus m_ObjectStatus = ObjectStatus::Invalid;
}


void CameraServiceNS::GenerateProjectionMatrix(CameraComponent* cameraComponent)
{
	auto l_resolution = g_Engine->Get<RenderingConfigurationService>()->GetScreenResolution();
	cameraComponent->m_WHRatio = (float)l_resolution.x / (float)l_resolution.y;
	cameraComponent->m_ProjectionMatrix = Math::GeneratePerspectiveMatrix((cameraComponent->m_FOVX / 180.0f) * PI<float>, cameraComponent->m_WHRatio, cameraComponent->m_ZNear, cameraComponent->m_ZFar);
}


void CameraServiceNS::GenerateFrustum(CameraComponent* cameraComponent, EntityID EntityID)
{
	auto l_pCamera = cameraComponent->m_ProjectionMatrix;
	auto l_frustumVerticesVS = Math::GenerateFrustumInViewSpace(l_pCamera);

	auto* l_Transform = g_Engine->Get<EntityRegistry>()->Get<TransformComponent>(EntityID);
	auto l_rCamera = l_Transform ? Math::toRotationMatrix(l_Transform->m_LocalRot) : Mat4();
	auto l_tCamera = l_Transform ? Math::toTranslationMatrix(Vec4(l_Transform->m_LocalPos, 1.0f)) : Mat4();
	auto l_frustumVerticesWS = Math::ViewToWorldSpace(l_frustumVerticesVS, l_tCamera, l_rCamera);
	cameraComponent->m_Frustum = Math::ToFrustum(&l_frustumVerticesWS[0]);

	std::copy(l_frustumVerticesWS.begin(), l_frustumVerticesWS.end(), cameraComponent->m_FrustumVerticesWS.begin());
}

void CameraServiceNS::GenerateRayOfEye(CameraComponent* cameraComponent, EntityID EntityID)
{
	auto* l_Transform = g_Engine->Get<EntityRegistry>()->Get<TransformComponent>(EntityID);
	if (l_Transform)
	{
		cameraComponent->m_RayOfEye.m_origin = l_Transform->m_LocalPos;
		cameraComponent->m_RayOfEye.m_direction = Math::getDirection(Direction::Backward, l_Transform->m_LocalRot);
	}
}

using namespace CameraServiceNS;

bool CameraService::Setup(IServiceConfig* systemConfig)
{
	return true;
}

bool CameraService::Initialize()
{
	m_ObjectStatus = ObjectStatus::Activated;
	return true;
}

bool CameraService::Update()
{
	if (!m_MainCamera)
		return true;

	auto& l_Storage = g_Engine->Get<EntityRegistry>()->Storage<CameraComponent>();
	auto& l_Cameras = l_Storage.All();
	const auto& l_Owners = l_Storage.AllOwners();

	for (size_t i = 0; i < l_Cameras.size(); i++)
	{
		CameraComponent& l_Camera = l_Cameras[i];
		EntityID l_EntityID = l_Owners[i];

		l_Camera.m_WHRatio = l_Camera.m_WidthScale / l_Camera.m_HeightScale;
		GenerateProjectionMatrix(&l_Camera);
		GenerateRayOfEye(&l_Camera, l_EntityID);
		GenerateFrustum(&l_Camera, l_EntityID);
	}
	return true;
}

bool CameraService::Terminate()
{
	m_ObjectStatus = ObjectStatus::Terminated;
	return true;
}

ObjectStatus CameraService::GetStatus()
{
	return m_ObjectStatus;
}

void CameraService::SetMainCamera(CameraComponent* cameraComponent)
{
	CameraServiceNS::m_MainCamera = cameraComponent;
}

CameraComponent* CameraService::GetMainCamera()
{
	return CameraServiceNS::m_MainCamera;
}

void CameraService::SetActiveCamera(CameraComponent* cameraComponent)
{
	CameraServiceNS::m_ActiveCamera = cameraComponent;
}

CameraComponent* CameraService::GetActiveCamera()
{
	return CameraServiceNS::m_ActiveCamera;
}
