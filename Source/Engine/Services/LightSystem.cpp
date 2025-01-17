#include "LightSystem.h"
#include "../Component/LightComponent.h"
#include "../Common/Randomizer.h"
#include "../Common/LogService.h"
#include "../Common/MathHelper.h"
#include "ComponentManager.h"
#include "CameraSystem.h"
#include "PhysicsSystem.h"
#include "RenderingConfigurationService.h"
#include "RenderingContextService.h"
#include "../Engine.h"

using namespace Inno;

namespace LightSystemNS
{
	const size_t m_MaxComponentCount = 8192;

	AABB SnapAABBToShadowMap(const AABB& rhs, float shadowMapResolution);
	void UpdateSingleSMData(LightComponent* rhs);
	void UpdateCSMData(LightComponent* rhs);
	void UpdateColorTemperature(LightComponent* rhs);
	void UpdateAttenuationRadius(LightComponent* rhs);
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

	return Math::generateAABB(l_boundMax, l_boundMin);
}

void LightSystemNS::UpdateSingleSMData(LightComponent* rhs)
{
	rhs->m_ViewMatrices.clear();
	rhs->m_ProjectionMatrices.clear();
	rhs->m_SplitAABBWS.clear();

	auto l_cameraComponent = static_cast<CameraSystem*>(g_Engine->Get<ComponentManager>()->GetComponentSystem<CameraComponent>())->GetMainCamera();
	if (l_cameraComponent == nullptr)
	{
		return;
	}

	auto l_totalSceneAABB = g_Engine->Get<PhysicsSystem>()->getVisibleSceneAABB();
	if(l_totalSceneAABB.m_extend.x == 0.0f || l_totalSceneAABB.m_extend.y == 0.0f || l_totalSceneAABB.m_extend.z == 0.0f)
		return;
	
	auto& l_splitFrustumVerticesWS = l_cameraComponent->m_splitFrustumVerticesWS;
	auto l_frustumAABB = Math::generateAABB(&l_splitFrustumVerticesWS[0], 8);

	auto l_min = Math::elementWiseMin(l_frustumAABB.m_boundMin, l_totalSceneAABB.m_boundMin);
	auto l_max = Math::elementWiseMax(l_frustumAABB.m_boundMax, l_totalSceneAABB.m_boundMax);

	auto l_AABB = Math::generateAABB(l_max, l_min);
	l_AABB = Math::extendAABBToBoundingSphere(l_AABB);
	l_AABB = SnapAABBToShadowMap(l_AABB, static_cast<float>(g_Engine->Get<RenderingConfigurationService>()->GetRenderingConfig().shadowMapResolution));
	rhs->m_SplitAABBWS.emplace_back(l_AABB);

	auto l_transformComponent = g_Engine->Get<ComponentManager>()->Find<TransformComponent>(rhs->m_Owner);
	auto l_r = l_transformComponent->m_globalTransformMatrix.m_rotationMat;
	rhs->m_ViewMatrices.emplace_back(l_r.inverse());

	Mat4 l_p = Math::generateOrthographicMatrix(l_AABB.m_boundMin.x, l_AABB.m_boundMax.x, l_AABB.m_boundMin.y, l_AABB.m_boundMax.y, l_AABB.m_boundMin.z, l_AABB.m_boundMax.z);
	rhs->m_ProjectionMatrices.emplace_back(l_p);
}

void LightSystemNS::UpdateCSMData(LightComponent* rhs)
{	
	rhs->m_ViewMatrices.clear();
	rhs->m_ProjectionMatrices.clear();
	rhs->m_SplitAABBWS.clear();
	rhs->m_SplitAABBLS.clear();

	auto l_cameraComponent = static_cast<ICameraSystem*>(g_Engine->Get<ComponentManager>()->GetComponentSystem<CameraComponent>())->GetMainCamera();
	if (l_cameraComponent == nullptr)
	{
		return;
	}

	auto l_totalSceneAABBWS = g_Engine->Get<PhysicsSystem>()->getVisibleSceneAABB();
	if(l_totalSceneAABBWS.m_extend.x == 0.0f || l_totalSceneAABBWS.m_extend.y == 0.0f || l_totalSceneAABBWS.m_extend.z == 0.0f)
		return;

	auto l_transformComponent = g_Engine->Get<ComponentManager>()->Find<TransformComponent>(rhs->m_Owner);
	auto l_r = l_transformComponent->m_globalTransformMatrix.m_rotationMat;
	auto l_rInv = l_r.inverse();
	auto l_totalSceneAABBLS = Math::extendAABBToBoundingSphere(l_totalSceneAABBWS);
	l_totalSceneAABBLS = Math::rotateAABBToNewSpace(l_totalSceneAABBLS, l_rInv);

	auto& l_splitFrustumVerticesWS = l_cameraComponent->m_splitFrustumVerticesWS;

	// calculate AABBs in light space and generate the matrices
	for (size_t i = 0; i < 4; i++)
	{
		AABB l_aabbWS = Math::generateAABB(&l_splitFrustumVerticesWS[i * 8], 8);
		rhs->m_SplitAABBWS.emplace_back(l_aabbWS);

		AABB l_aabbLS = Math::extendAABBToBoundingSphere(l_aabbWS);
		l_aabbLS = Math::rotateAABBToNewSpace(l_aabbLS, l_rInv);

		l_aabbLS.m_boundMin.z = l_totalSceneAABBLS.m_boundMin.z;
		l_aabbLS.m_boundMax.z = l_totalSceneAABBLS.m_boundMax.z;
		l_aabbLS.m_extend = l_aabbLS.m_boundMax - l_aabbLS.m_boundMin;
		l_aabbLS.m_center = l_aabbLS.m_boundMax - l_aabbLS.m_extend * 0.5f;

		l_aabbLS = SnapAABBToShadowMap(l_aabbLS, static_cast<float>(g_Engine->Get<RenderingConfigurationService>()->GetRenderingConfig().shadowMapResolution));
		rhs->m_SplitAABBLS.emplace_back(l_aabbLS);

		auto l_v = l_rInv;
		rhs->m_ViewMatrices.emplace_back(l_v);

		Mat4 p = Math::generateOrthographicMatrix(l_aabbLS.m_boundMin.x, l_aabbLS.m_boundMax.x, l_aabbLS.m_boundMin.y, l_aabbLS.m_boundMax.y, l_aabbLS.m_boundMin.z, l_aabbLS.m_boundMax.z);
		rhs->m_ProjectionMatrices.emplace_back(p);
	}
}

void LightSystemNS::UpdateColorTemperature(LightComponent* rhs)
{
	if (rhs->m_UseColorTemperature)
	{
		rhs->m_RGBColor = Math::colorTemperatureToRGB(rhs->m_ColorTemperature);
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

bool LightSystem::Setup(ISystemConfig* systemConfig)
{
	g_Engine->Get<ComponentManager>()->RegisterType<LightComponent>(m_MaxComponentCount, this);

	return true;
}

bool LightSystem::Initialize()
{
	return true;
}

bool LightSystem::Update()
{
	auto l_renderingConfig = g_Engine->Get<RenderingConfigurationService>()->GetRenderingConfig();
	auto l_components = g_Engine->Get<ComponentManager>()->GetAll<LightComponent>();
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

bool LightSystem::OnFrameEnd()
{
	return true;
}

bool LightSystem::Terminate()
{
	return true;
}

ObjectStatus LightSystem::GetStatus()
{
	return ObjectStatus();
}