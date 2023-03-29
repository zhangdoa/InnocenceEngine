#include "LightSystem.h"
#include "../Component/LightComponent.h"
#include "../Core/InnoRandomizer.h"
#include "../Core/InnoLogger.h"
#include "../Common/InnoMathHelper.h"

#include "../Interface/IEngine.h"

using namespace Inno;
extern IEngine* g_Engine;

namespace LightSystemNS
{
	const size_t m_MaxComponentCount = 8192;

	AABB ExtendAABBToBoundingSphere(const AABB& rhs);
	AABB SnapAABBToShadowMap(const AABB& rhs, float shadowMapResolution);
	void SplitVertices(const std::vector<Vertex>& frustumsVertices, const std::vector<float>& splitFactors, std::vector<Vertex> &splitVertices);
	void UpdateSingleSMData(LightComponent* rhs);
	void UpdateCSMData(LightComponent* rhs);
	void UpdateColorTemperature(LightComponent* rhs);
	void UpdateAttenuationRadius(LightComponent* rhs);

	std::vector<float> m_CSMSplitFactors = { 0.05f, 0.15f, 0.35f, 1.0f };
}

AABB LightSystemNS::ExtendAABBToBoundingSphere(const AABB &rhs)
{
		auto l_sphereRadius = (rhs.m_boundMax - rhs.m_center).length();
		auto l_boundMax = rhs.m_center + l_sphereRadius;
		auto l_boundMin = rhs.m_center - l_sphereRadius;

		l_boundMax.w = 1.0f;
		l_boundMin.w = 1.0f;
		return InnoMath::generateAABB(l_boundMax, l_boundMin);
}

AABB LightSystemNS::SnapAABBToShadowMap(const AABB &rhs, float shadowMapResolution)
{	
	auto l_boundMax = rhs.m_boundMax;
	auto l_boundMin = rhs.m_boundMin;

	auto vWorldUnitsPerTexel = rhs.m_extend / shadowMapResolution;
	auto vTexelPerWorldUnit = vWorldUnitsPerTexel.reciprocal();
	l_boundMax.scale(vTexelPerWorldUnit);
	l_boundMax = Vec4(floor(l_boundMax.x), floor(l_boundMax.y), floor(l_boundMax.z), floor(l_boundMax.w));
	l_boundMax.scale(vWorldUnitsPerTexel);
	l_boundMin.scale(vTexelPerWorldUnit);
	l_boundMin = Vec4(floor(l_boundMin.x), floor(l_boundMin.y), floor(l_boundMin.z), floor(l_boundMin.w));
	l_boundMin.scale(vWorldUnitsPerTexel);
	l_boundMax.w = 1.0f;
	l_boundMin.w = 1.0f;

	return InnoMath::generateAABB(l_boundMax, l_boundMin);
}

void LightSystemNS::SplitVertices(const std::vector<Vertex> &frustumsVertices, const std::vector<float> &splitFactors, std::vector<Vertex> &splitVertices)
{
	std::vector<Vec4> l_frustumsCornerPos;
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

void LightSystemNS::UpdateSingleSMData(LightComponent* rhs)
{
	rhs->m_ViewMatrices.clear();
	rhs->m_ProjectionMatrices.clear();
	rhs->m_SplitAABBWS.clear();

	auto l_totalSceneAABB = g_Engine->getPhysicsSystem()->getVisibleSceneAABB();
	if(l_totalSceneAABB.m_extend.x == 0.0f || l_totalSceneAABB.m_extend.y == 0.0f || l_totalSceneAABB.m_extend.z == 0.0f)
		return;
		
	l_totalSceneAABB = ExtendAABBToBoundingSphere(l_totalSceneAABB);
	l_totalSceneAABB = SnapAABBToShadowMap(l_totalSceneAABB, 1024.0f);
	rhs->m_SplitAABBWS.emplace_back(l_totalSceneAABB);

	auto l_transformComponent = g_Engine->getComponentManager()->Find<TransformComponent>(rhs->m_Owner);
	auto l_r = l_transformComponent->m_globalTransformMatrix.m_rotationMat;
	rhs->m_ViewMatrices.emplace_back(l_r.inverse());

	Mat4 l_p = InnoMath::generateOrthographicMatrix(l_totalSceneAABB.m_boundMin.x, l_totalSceneAABB.m_boundMax.x, l_totalSceneAABB.m_boundMin.y, l_totalSceneAABB.m_boundMax.y, l_totalSceneAABB.m_boundMin.z, l_totalSceneAABB.m_boundMax.z);
	rhs->m_ProjectionMatrices.emplace_back(l_p);
}

void LightSystemNS::UpdateCSMData(LightComponent* rhs)
{	
	rhs->m_ViewMatrices.clear();
	rhs->m_ProjectionMatrices.clear();
	rhs->m_SplitAABBWS.clear();

	// get frustum vertices in view space
	auto l_cameraComponent = static_cast<ICameraSystem*>(g_Engine->getComponentManager()->GetComponentSystem<CameraComponent>())->GetMainCamera();
	if (l_cameraComponent == nullptr)
	{
		return;
	}
	auto l_cameraTransformComponent = g_Engine->getComponentManager()->Find<TransformComponent>(l_cameraComponent->m_Owner);
	if (l_cameraTransformComponent == nullptr)
	{
		return;
	}

	auto l_transformComponent = g_Engine->getComponentManager()->Find<TransformComponent>(rhs->m_Owner);
	auto l_r = l_transformComponent->m_globalTransformMatrix.m_rotationMat;

	auto l_rCamera = InnoMath::toRotationMatrix(l_cameraTransformComponent->m_globalTransformVector.m_rot);
	auto l_tCamera = InnoMath::toTranslationMatrix(l_cameraTransformComponent->m_globalTransformVector.m_pos);
	auto l_frustumVerticesVS = InnoMath::generateFrustumVerticesVS(l_cameraComponent->m_projectionMatrix);

	// calculate split vertices in world space and assemble AABBs
	auto l_frustumVerticesWS = InnoMath::viewToWorldSpace(l_frustumVerticesVS, l_tCamera, l_rCamera);
	std::vector<Vertex> l_splitFrustumVerticesWS;
	l_splitFrustumVerticesWS.resize(32);
	SplitVertices(l_frustumVerticesWS, m_CSMSplitFactors, l_splitFrustumVerticesWS);

	// calculate AABBs in light space and generate the matrices
	for (size_t i = 0; i < 4; i++)
	{
		AABB l_aabbWS = InnoMath::generateAABB(&l_splitFrustumVerticesWS[i * 8], 8);	
		AABB l_aabbLS = ExtendAABBToBoundingSphere(l_aabbWS);
#ifdef USE_COLUMN_MAJOR_MEMORY_LAYOUT
		l_aabbLS.m_center = InnoMath::mul(l_aabbLS.m_center, l_r.inverse());
#endif
#ifdef USE_ROW_MAJOR_MEMORY_LAYOUT
		l_aabbLS.m_center = InnoMath::mul(l_r.inverse(), l_aabbLS.m_center);
#endif
		l_aabbLS.m_boundMin = l_aabbLS.m_center - l_aabbLS.m_extend * 0.5f;
		l_aabbLS.m_boundMax = l_aabbLS.m_center + l_aabbLS.m_extend * 0.5f;

		rhs->m_ViewMatrices.emplace_back(l_r.inverse());
		rhs->m_SplitAABBWS.emplace_back(l_aabbWS);
		l_aabbLS = SnapAABBToShadowMap(l_aabbLS, 1024.0f);
		Mat4 p = InnoMath::generateOrthographicMatrix(l_aabbLS.m_boundMin.x, l_aabbLS.m_boundMax.x, l_aabbLS.m_boundMin.y, l_aabbLS.m_boundMax.y, l_aabbLS.m_boundMin.z, l_aabbLS.m_boundMax.z);
		rhs->m_ProjectionMatrices.emplace_back(p);
	}
}

void LightSystemNS::UpdateColorTemperature(LightComponent* rhs)
{
	if (rhs->m_UseColorTemperature)
	{
		rhs->m_RGBColor = InnoMath::colorTemperatureToRGB(rhs->m_ColorTemperature);
	}
}

void LightSystemNS::UpdateAttenuationRadius(LightComponent* rhs)
{
	auto l_RGBColor = rhs->m_RGBColor.normalize();
	// "Real-Time Rendering", 4th Edition, p.278
	// https://en.wikipedia.org/wiki/Relative_luminance
	// weight with respect to CIE photometric curve
	auto l_relativeLuminanceRatio = (0.2126f * l_RGBColor.x + 0.7152f * l_RGBColor.y + 0.0722f * l_RGBColor.z);

	// Luminance (nt) is illuminance (lx) per solid angle, while luminous intensity (cd) is luminous flux (lm) per solid angle, thus for one area unit (m^2), the ratio of nt/lx is same as cd/lm
	// For omni isotropic light, after the intergration per solid angle, the luminous flux (lm) is 4 pi times the luminous intensity (cd)
	auto l_weightedLuminousFlux = rhs->m_LuminousFlux * l_relativeLuminanceRatio;

	// 1. get luminous efficacy (lm/w), assume 683 lm/w (100% luminous efficiency) always
	// 2. luminous flux (lm) to radiant flux (w), omitted because linearity assumption in step 1
	// 3. apply inverse square attenuation law with a low threshold of eye sensitivity at 0.03 lx, in ideal situation, lx could convert back to lm with respect to a sphere surface area 4 * PI * r^2
#if defined INNO_PLATFORM_WIN
	rhs->m_Shape.x = std::sqrtf(l_weightedLuminousFlux / (4.0f * PI<float> * 0.03f));
#else
	rhs->m_Shape.x = sqrtf(l_weightedLuminousFlux / (4.0f * PI<float> * 0.03f));
#endif
}

using namespace LightSystemNS;

bool InnoLightSystem::Setup(ISystemConfig* systemConfig)
{
	g_Engine->getComponentManager()->RegisterType<LightComponent>(m_MaxComponentCount, this);

	return true;
}

bool InnoLightSystem::Initialize()
{
	return true;
}

bool InnoLightSystem::Update()
{
	auto l_renderingConfig = g_Engine->getRenderingFrontend()->getRenderingConfig();
	auto l_components = g_Engine->getComponentManager()->GetAll<LightComponent>();
	for (auto i : l_components)
	{
		UpdateColorTemperature(i);
		switch (i->m_LightType)
		{
		case LightType::Directional:
			// @TODO: Better to limit the directional light count
			if (l_renderingConfig.useCSM)
			{
				UpdateCSMData(i);
			}
			else
			{
				UpdateSingleSMData(i);
			}
			break;
		case LightType::Point:
			UpdateAttenuationRadius(i);
			break;
		case LightType::Spot:
			break;
		case LightType::Sphere:
			break;
		case LightType::Disk:
			break;
		case LightType::Tube:
			break;
		case LightType::Rectangle:
			break;
		default:
			break;
		}
	}
	return true;
}

bool InnoLightSystem::OnFrameEnd()
{
	return true;
}

bool InnoLightSystem::Terminate()
{
	return true;
}

ObjectStatus InnoLightSystem::GetStatus()
{
	return ObjectStatus();
}