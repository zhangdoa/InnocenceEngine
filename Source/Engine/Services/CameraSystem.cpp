#include "CameraSystem.h"
#include "../Component/CameraComponent.h"
#include "../Common/LogService.h"
#include "../Common/Randomizer.h"
#include "../Common/MathHelper.h"
#include "RenderingConfigurationService.h"
#include "RenderingContextService.h"
#include "ComponentManager.h"

#include "../Engine.h"

using namespace Inno;
;

namespace CameraSystemNS
{
	const size_t m_MaxComponentCount = 32;

	void GenerateCSMSplitFactors(float lambda = 0.75f);
	void GenerateProjectionMatrix(CameraComponent* cameraComponent);
	void SplitVertices(const std::vector<Vertex>& frustumsVertices, const std::vector<float>& splitFactors, std::vector<Vertex> &splitVertices);
	void GenerateFrustum(CameraComponent* cameraComponent);
	void GenerateRayOfEye(CameraComponent* cameraComponent);

	CameraComponent* m_MainCamera;
	CameraComponent* m_ActiveCamera;
	
	const uint32_t m_MaxCSMCount = 4;
	std::vector<float> m_CSMSplitFactors;
}

void CameraSystemNS::GenerateCSMSplitFactors(float lambda)
{
	m_CSMSplitFactors.clear();
    m_CSMSplitFactors.reserve(m_MaxCSMCount);

	auto near = m_MainCamera->m_zNear;
	auto far = m_MainCamera->m_zFar;
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
	cameraComponent->m_projectionMatrix = Math::GeneratePerspectiveMatrix((cameraComponent->m_FOVX / 180.0f) * PI<float>, cameraComponent->m_WHRatio, cameraComponent->m_zNear, cameraComponent->m_zFar);
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

void CameraSystemNS::GenerateFrustum(CameraComponent* cameraComponent)
{
	// get frustum vertices in view space
	auto l_pCamera = cameraComponent->m_projectionMatrix;
	auto l_frustumVerticesVS = Math::GenerateFrustumInViewSpace(l_pCamera);

	auto l_rCamera = Math::toRotationMatrix(cameraComponent->m_Transform.m_rot);
	auto l_tCamera = Math::toTranslationMatrix(Vec4(cameraComponent->m_Transform.m_pos, 1.0f));
	auto l_frustumVerticesWS = Math::ViewToWorldSpace(l_frustumVerticesVS, l_tCamera, l_rCamera);
	cameraComponent->m_frustum = Math::ToFrustum(&l_frustumVerticesWS[0]);

	cameraComponent->m_splitFrustumVerticesWS.resize(m_CSMSplitFactors.size() * 8);
	SplitVertices(l_frustumVerticesWS, m_CSMSplitFactors, cameraComponent->m_splitFrustumVerticesWS);
}

void CameraSystemNS::GenerateRayOfEye(CameraComponent* cameraComponent)
{
	cameraComponent->m_rayOfEye.m_origin = cameraComponent->m_Transform.m_pos;
	cameraComponent->m_rayOfEye.m_direction = Math::getDirection(Direction::Backward, cameraComponent->m_Transform.m_rot);
}

using namespace CameraSystemNS;

bool CameraSystem::Setup(ISystemConfig* systemConfig)
{
	g_Engine->Get<ComponentManager>()->RegisterType<CameraComponent>(m_MaxComponentCount, this);

	return true;
}

bool CameraSystem::Initialize()
{
	return true;
}

bool CameraSystem::Update()
{
	if (!m_MainCamera)
		return true;

	GenerateCSMSplitFactors();

	auto l_components = g_Engine->Get<ComponentManager>()->GetAll<CameraComponent>();

	for (auto i : l_components)
	{
		i->m_WHRatio = i->m_widthScale / i->m_heightScale;
		GenerateProjectionMatrix(i);
		GenerateRayOfEye(i);
		GenerateFrustum(i);
	}
	return true;
}

bool CameraSystem::Terminate()
{
	return true;
}

ObjectStatus CameraSystem::GetStatus()
{
	return ObjectStatus();
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