#include "CameraSystem.h"
#include "../Component/CameraComponent.h"
#include "../Core/InnoRandomizer.h"
#include "../Core/InnoLogger.h"
#include "../Common/InnoMathHelper.h"

#include "../Interface/IEngine.h"

using namespace Inno;
extern IEngine* g_Engine;

namespace CameraSystemNS
{
	const size_t m_MaxComponentCount = 32;

	void GenerateProjectionMatrix(CameraComponent* cameraComponent);
	void SplitVertices(const std::vector<Vertex>& frustumsVertices, const std::vector<float>& splitFactors, std::vector<Vertex> &splitVertices);
	void GenerateFrustum(CameraComponent* cameraComponent);
	void GenerateRayOfEye(CameraComponent* cameraComponent);

	CameraComponent* m_MainCamera;
	CameraComponent* m_ActiveCamera;
	
	std::vector<float> m_CSMSplitFactors = { 0.05f, 0.15f, 0.35f, 1.0f };
}

void CameraSystemNS::GenerateProjectionMatrix(CameraComponent* cameraComponent)
{
	auto l_resolution = g_Engine->getRenderingFrontend()->getScreenResolution();
	cameraComponent->m_WHRatio = (float)l_resolution.x / (float)l_resolution.y;
	cameraComponent->m_projectionMatrix = InnoMath::generatePerspectiveMatrix((cameraComponent->m_FOVX / 180.0f) * PI<float>, cameraComponent->m_WHRatio, cameraComponent->m_zNear, cameraComponent->m_zFar);
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
			auto l_splitPlaneCornerPos = frustumsVertices[j].m_pos + l_direction * splitFactors[i];
			l_frustumsCornerPos.emplace_back(l_splitPlaneCornerPos);
		}
	}

	//https://docs.microsoft.com/windows/desktop/DxTechArts/common-techniques-to-improve-shadow-depth-maps
	//3. assemble split frustum corners
	auto l_renderingConfig = g_Engine->getRenderingFrontend()->getRenderingConfig();

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
	auto l_transformComponent = g_Engine->getComponentManager()->Find<TransformComponent>(cameraComponent->m_Owner);
	if (l_transformComponent != nullptr)
	{
		auto l_rCamera = InnoMath::toRotationMatrix(l_transformComponent->m_globalTransformVector.m_rot);
		auto l_tCamera = InnoMath::toTranslationMatrix(l_transformComponent->m_globalTransformVector.m_pos);
		auto l_frustumVerticesVS = InnoMath::generateFrustumVerticesVS(cameraComponent->m_projectionMatrix);

		// calculate split vertices in world space and assemble AABBs
		auto l_frustumVerticesWS = InnoMath::viewToWorldSpace(l_frustumVerticesVS, l_tCamera, l_rCamera);
		cameraComponent->m_splitFrustumVerticesWS.resize(32);
		SplitVertices(l_frustumVerticesWS, m_CSMSplitFactors, cameraComponent->m_splitFrustumVerticesWS);

		auto l_pCamera = cameraComponent->m_projectionMatrix;
		auto l_vertices = InnoMath::generateFrustumVerticesWS(l_pCamera, l_rCamera, l_tCamera);
		cameraComponent->m_frustum = InnoMath::makeFrustum(&l_vertices[0]);
	}
}

void CameraSystemNS::GenerateRayOfEye(CameraComponent* cameraComponent)
{
	auto l_transformComponent = g_Engine->getComponentManager()->Find<TransformComponent>(cameraComponent->m_Owner);

	if (l_transformComponent != nullptr)
	{
		cameraComponent->m_rayOfEye.m_origin = l_transformComponent->m_globalTransformVector.m_pos;
		cameraComponent->m_rayOfEye.m_direction = InnoMath::getDirection(Direction::Backward, l_transformComponent->m_globalTransformVector.m_rot);
	}
}

using namespace CameraSystemNS;

bool InnoCameraSystem::Setup(ISystemConfig* systemConfig)
{
	g_Engine->getComponentManager()->RegisterType<CameraComponent>(m_MaxComponentCount, this);

	return true;
}

bool InnoCameraSystem::Initialize()
{
	return true;
}

bool InnoCameraSystem::Update()
{
	auto l_components = g_Engine->getComponentManager()->GetAll<CameraComponent>();

	for (auto i : l_components)
	{
		i->m_WHRatio = i->m_widthScale / i->m_heightScale;
		GenerateProjectionMatrix(i);
		GenerateRayOfEye(i);
		GenerateFrustum(i);
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

void InnoCameraSystem::SetMainCamera(CameraComponent* cameraComponent)
{
	CameraSystemNS::m_MainCamera = cameraComponent;
}

CameraComponent* InnoCameraSystem::GetMainCamera()
{
	return CameraSystemNS::m_MainCamera;
}

void InnoCameraSystem::SetActiveCamera(CameraComponent* cameraComponent)
{
	CameraSystemNS::m_ActiveCamera = cameraComponent;
}

CameraComponent* InnoCameraSystem::GetActiveCamera()
{
	return CameraSystemNS::m_ActiveCamera;
}