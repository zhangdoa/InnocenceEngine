#include "CameraSystem.h"
#include "../Component/CameraComponent.h"
#include "../Component/TransformComponent.h"
#include "../Common/LogService.h"
#include "../Common/Randomizer.h"
#include "../Common/MathHelper.h"
#include "RenderingConfigurationService.h"
#include "EntityRegistry.h"
#include "ComponentManager.h" // TODO Phase2-migrate: bridge for not-yet-migrated consumers (JSONWrapper, PerFrameDataService, Baker, RayTracer — Tasks 10-12)

#include "../Engine.h"

using namespace Inno;

namespace CameraSystemNS
{
	const size_t m_MaxComponentCount = 32;

	void GenerateCSMSplitFactors(float lambda = 0.75f);
	void GenerateProjectionMatrix(CameraComponent* cameraComponent);
	void SplitVertices(const std::vector<Vertex>& frustumsVertices, const std::vector<float>& splitFactors, std::vector<Vertex> &splitVertices);
	void GenerateFrustum(CameraComponent* cameraComponent, EntityID EntityID);
	void GenerateRayOfEye(CameraComponent* cameraComponent, EntityID EntityID);

	CameraComponent* m_MainCamera;
	CameraComponent* m_ActiveCamera;

	ObjectStatus m_ObjectStatus = ObjectStatus::Invalid;

	const uint32_t m_MaxCSMCount = 4;
	std::vector<float> m_CSMSplitFactors;
}

void CameraSystemNS::GenerateCSMSplitFactors(float lambda)
{
	m_CSMSplitFactors.clear();
    m_CSMSplitFactors.reserve(m_MaxCSMCount);

	auto near = m_MainCamera->m_ZNear;
	auto far = m_MainCamera->m_ZFar;
    for (int i = 1; i <= m_MaxCSMCount; i++)
    {
        float logSplit = near * std::pow((far / near), (float)i / (float)m_MaxCSMCount);
        float uniformSplit = near + (far - near) * ((float)i / (float)m_MaxCSMCount);
        float split = logSplit * lambda + uniformSplit * (1.0f - lambda);
        m_CSMSplitFactors.emplace_back(split);
    }
}

void CameraSystemNS::GenerateProjectionMatrix(CameraComponent* cameraComponent)
{
	auto l_resolution = g_Engine->Get<RenderingConfigurationService>()->GetScreenResolution();
	cameraComponent->m_WHRatio = (float)l_resolution.x / (float)l_resolution.y;
	cameraComponent->m_ProjectionMatrix = Math::GeneratePerspectiveMatrix((cameraComponent->m_FOVX / 180.0f) * PI<float>, cameraComponent->m_WHRatio, cameraComponent->m_ZNear, cameraComponent->m_ZFar);
}

void CameraSystemNS::SplitVertices(const std::vector<Vertex> &frustumsVertices, const std::vector<float> &splitFactors, std::vector<Vertex> &splitVertices)
{
	std::vector<Vec3> l_frustumsCornerPos;
	l_frustumsCornerPos.reserve(20);

	//1. first 4 corner
	for (size_t i = 0; i < 4; i++)
	{
		l_frustumsCornerPos.emplace_back(frustumsVertices[i].m_pos);
	}

	//2. other 16 corner based on the split factors
	for (size_t i = 0; i < 4; i++)
	{
		for (size_t j = 0; j < 4; j++)
		{
			auto l_direction = (frustumsVertices[j + 4].m_pos - frustumsVertices[j].m_pos);
			l_direction = l_direction.normalize();
			auto l_splitPlaneCornerPos = frustumsVertices[j].m_pos + l_direction * splitFactors[i];
			l_frustumsCornerPos.emplace_back(l_splitPlaneCornerPos);
		}
	}

	//https://docs.microsoft.com/windows/desktop/DxTechArts/common-techniques-to-improve-shadow-depth-maps
	//3. assemble split frustum corners
	auto l_renderingConfig = g_Engine->Get<RenderingConfigurationService>()->GetRenderingConfig();

	if (l_renderingConfig.CSMFitToScene)
	{
		for (size_t i = 0; i < 4; i++)
		{
			splitVertices[i * 8].m_pos = l_frustumsCornerPos[0];
			splitVertices[i * 8 + 1].m_pos = l_frustumsCornerPos[1];
			splitVertices[i * 8 + 2].m_pos = l_frustumsCornerPos[2];
			splitVertices[i * 8 + 3].m_pos = l_frustumsCornerPos[3];

			for (size_t j = 4; j < 8; j++)
			{
				splitVertices[i * 8 + j].m_pos = l_frustumsCornerPos[i * 4 + j];
			}
		}
	}
	// fit to cascade
	else
	{
		for (size_t i = 0; i < 4; i++)
		{
			for (size_t j = 0; j < 8; j++)
			{
				splitVertices[i * 8 + j].m_pos = l_frustumsCornerPos[i * 4 + j];
			}
		}
	}
}

void CameraSystemNS::GenerateFrustum(CameraComponent* cameraComponent, EntityID EntityID)
{
	auto l_pCamera = cameraComponent->m_ProjectionMatrix;
	auto l_frustumVerticesVS = Math::GenerateFrustumInViewSpace(l_pCamera);

	auto* l_Transform = g_Engine->Get<EntityRegistry>()->Get<TransformComponent>(EntityID);
	auto l_rCamera = l_Transform ? Math::toRotationMatrix(l_Transform->m_LocalRot) : Mat4();
	auto l_tCamera = l_Transform ? Math::toTranslationMatrix(Vec4(l_Transform->m_LocalPos, 1.0f)) : Mat4();
	auto l_frustumVerticesWS = Math::ViewToWorldSpace(l_frustumVerticesVS, l_tCamera, l_rCamera);
	cameraComponent->m_Frustum = Math::ToFrustum(&l_frustumVerticesWS[0]);

	cameraComponent->m_SplitFrustumVerticesWS.resize(m_CSMSplitFactors.size() * 8);
	SplitVertices(l_frustumVerticesWS, m_CSMSplitFactors, cameraComponent->m_SplitFrustumVerticesWS);
}

void CameraSystemNS::GenerateRayOfEye(CameraComponent* cameraComponent, EntityID EntityID)
{
	auto* l_Transform = g_Engine->Get<EntityRegistry>()->Get<TransformComponent>(EntityID);
	if (l_Transform)
	{
		cameraComponent->m_RayOfEye.m_origin = l_Transform->m_LocalPos;
		cameraComponent->m_RayOfEye.m_direction = Math::getDirection(Direction::Backward, l_Transform->m_LocalRot);
	}
}

using namespace CameraSystemNS;

bool CameraSystem::Setup(ISystemConfig* systemConfig)
{
	g_Engine->Get<ComponentManager>()->RegisterType<CameraComponent>(m_MaxComponentCount, this); // TODO Phase2-migrate: bridge — remove when all consumers use EntityRegistry
	return true;
}

bool CameraSystem::Initialize()
{
	m_ObjectStatus = ObjectStatus::Activated;
	return true;
}

bool CameraSystem::Update()
{
	if (!m_MainCamera)
		return true;

	GenerateCSMSplitFactors();

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

bool CameraSystem::Terminate()
{
	m_ObjectStatus = ObjectStatus::Terminated;
	return true;
}

ObjectStatus CameraSystem::GetStatus()
{
	return m_ObjectStatus;
}

void CameraSystem::SetMainCamera(CameraComponent* cameraComponent)
{
	CameraSystemNS::m_MainCamera = cameraComponent;
}

CameraComponent* CameraSystem::GetMainCamera()
{
	return CameraSystemNS::m_MainCamera;
}

void CameraSystem::SetActiveCamera(CameraComponent* cameraComponent)
{
	CameraSystemNS::m_ActiveCamera = cameraComponent;
}

CameraComponent* CameraSystem::GetActiveCamera()
{
	return CameraSystemNS::m_ActiveCamera;
}
