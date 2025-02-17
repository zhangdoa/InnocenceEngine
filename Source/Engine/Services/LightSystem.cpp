#include "LightSystem.h"
#include "../Component/LightComponent.h"
#include "../Common/Randomizer.h"
#include "../Common/LogService.h"
#include "../Common/MathHelper.h"
#include "ComponentManager.h"
#include "CameraSystem.h"
#include "PhysicsSimulationService.h"
#include "RenderingConfigurationService.h"
#include "RenderingContextService.h"
#include "../Engine.h"

using namespace Inno;

namespace Inno
{
	struct LightSystemImpl
	{
		const size_t m_MaxComponentCount = 8192;

		AABB SnapAABBToShadowMap(const AABB& rhs, float shadowMapResolution);
		void AlignMatrixToTexels(Mat4& matrix, float shadowMapResolution);
		void UpdateCSMData(LightComponent* rhs);
		void UpdateColorTemperature(LightComponent* rhs);
		void UpdateAttenuationRadius(LightComponent* rhs);

		ObjectStatus m_ObjectStatus = ObjectStatus::Invalid;
	};
}

AABB LightSystemImpl::SnapAABBToShadowMap(const AABB &rhs, float shadowMapResolution)
{
    Vec4 vWorldUnitsPerTexel = rhs.m_extend / shadowMapResolution;
    Vec4 vTexelPerWorldUnit = vWorldUnitsPerTexel.reciprocal();

    // Snap center to texel grid with a bit extra rounding offset
	Vec4 snappedCenter = rhs.m_center.scale(vTexelPerWorldUnit) + 0.5f;
	snappedCenter = Vec4(floor(snappedCenter.x), floor(snappedCenter.y), floor(snappedCenter.z), 1.0f);
	snappedCenter = snappedCenter.scale(vWorldUnitsPerTexel);

    AABB snappedAABB;
    snappedAABB.m_center = snappedCenter;
    snappedAABB.m_extend = rhs.m_extend; // Keep same size
    snappedAABB.m_boundMin = snappedAABB.m_center - snappedAABB.m_extend * 0.5f;
    snappedAABB.m_boundMax = snappedAABB.m_center + snappedAABB.m_extend * 0.5f;
    
    return snappedAABB;
}

void LightSystemImpl::AlignMatrixToTexels(Mat4& matrix, float shadowMapResolution)
{
    matrix.m30 = floor(matrix.m30 * shadowMapResolution) / shadowMapResolution;
    matrix.m31 = floor(matrix.m31 * shadowMapResolution) / shadowMapResolution;
}

void LightSystemImpl::UpdateCSMData(LightComponent* rhs)
{	
	rhs->m_ViewMatrices.clear();
	rhs->m_ProjectionMatrices.clear();
	rhs->m_LitRegion_WorldSpace.clear();
	rhs->m_LitRegion_LightSpace.clear();

	auto l_cameraComponent = static_cast<ICameraSystem*>(g_Engine->Get<ComponentManager>()->GetComponentSystem<CameraComponent>())->GetMainCamera();
	if (l_cameraComponent == nullptr)
		return;

	auto l_transformComponent = g_Engine->Get<ComponentManager>()->Find<TransformComponent>(rhs->m_Owner);
	auto l_rotationMatrix = l_transformComponent->m_globalTransformMatrix.m_rotationMat;
	auto l_rotationMatrix_inverse = l_rotationMatrix.inverse();

	auto& l_splitFrustumVerticesWS = l_cameraComponent->m_splitFrustumVerticesWS;

	// calculate AABBs in light space and generate the matrices
	for (size_t i = 0; i < 4; i++)
	{
		AABB l_AABB_worldSpace = Math::GenerateAABB(&l_splitFrustumVerticesWS[i * 8], 8);
		rhs->m_LitRegion_WorldSpace.emplace_back(l_AABB_worldSpace);

		// Rotating AABB to light space to be an OBB then generate a new AABB there
		// is the same as directly extending the AABB to cover its bounding sphere and then rotate only the center.
		AABB l_AABB_lightSpace = Math::ExtendAABBToBoundingSphere(l_AABB_worldSpace);
		l_AABB_lightSpace = Math::RotateAABBToNewSpace(l_AABB_lightSpace, l_rotationMatrix_inverse);

		auto l_shadowMapResolution = static_cast<float>(g_Engine->Get<RenderingConfigurationService>()->GetRenderingConfig().shadowMapResolution);
		l_AABB_lightSpace = SnapAABBToShadowMap(l_AABB_lightSpace, l_shadowMapResolution);
		rhs->m_LitRegion_LightSpace.emplace_back(l_AABB_lightSpace);

		AlignMatrixToTexels(l_rotationMatrix_inverse, l_shadowMapResolution);
		rhs->m_ViewMatrices.emplace_back(l_rotationMatrix_inverse);
		
		// This should be infinite by concept.
		// const float l_zCompensation = 65536.0f;
		// l_AABB_lightSpace.m_boundMin.z -= l_zCompensation;
		// l_AABB_lightSpace.m_boundMax.z += l_zCompensation;
		auto l_projectionMatrix = Math::GenerateOrthographicMatrix(
			l_AABB_lightSpace.m_boundMin.x, l_AABB_lightSpace.m_boundMax.x
		, l_AABB_lightSpace.m_boundMin.y, l_AABB_lightSpace.m_boundMax.y
		, l_AABB_lightSpace.m_boundMax.z, l_AABB_lightSpace.m_boundMin.z);
		rhs->m_ProjectionMatrices.emplace_back(l_projectionMatrix);
	}
}

void LightSystemImpl::UpdateColorTemperature(LightComponent* rhs)
{
	if (!rhs->m_UseColorTemperature)
		return;

	rhs->m_RGBColor = Math::ColorTemperatureToRGB(rhs->m_ColorTemperature);
}

void LightSystemImpl::UpdateAttenuationRadius(LightComponent* rhs)
{
	auto l_RGBColor = rhs->m_RGBColor.normalize();
	// "Real-Time Rendering", 4th Edition, p.278
	// https://en.wikipedia.org/wiki/Relative_luminance
	// weight with respect to CIE photometric curve
	auto l_relativeLuminanceRatio = (0.2126f * l_RGBColor.x + 0.7152f * l_RGBColor.y + 0.0722f * l_RGBColor.z);

	// Luminance (nt) is illuminance (lx) per solid angle, while luminous intensity (cd) is luminous flux (lm) per solid angle, thus for one area unit (m^2), the ratio of nt/lx is same as cd/lm
	// For omni-isotropic light, after the integration per solid angle, the luminous flux (lm) is 4 pi times the luminous intensity (cd)
	auto l_weightedLuminousFlux = rhs->m_LuminousFlux * l_relativeLuminanceRatio;

	// 1. get luminous efficacy (lm/w), assume 683 lm/w (100% luminous efficiency) always
	// 2. luminous flux (lm) to radiant flux (w), omitted because linearity assumption in step 1
	// 3. apply inverse square attenuation law with a low threshold of eye sensitivity at 0.03 lx, in ideal situation, lx could convert back to lm with respect to a sphere surface area 4 * PI * r^2
	rhs->m_Shape.x = std::sqrtf(l_weightedLuminousFlux / (4.0f * PI<float> * 0.03f));
}

bool LightSystem::Setup(ISystemConfig* systemConfig)
{
	m_Impl = new LightSystemImpl();
	g_Engine->Get<ComponentManager>()->RegisterType<LightComponent>(m_Impl->m_MaxComponentCount, this);
	m_Impl->m_ObjectStatus = ObjectStatus::Created;

	return true;
}

bool LightSystem::Initialize()
{
	m_Impl->m_ObjectStatus = ObjectStatus::Activated;
	return true;
}

bool LightSystem::Update()
{
	auto l_renderingConfig = g_Engine->Get<RenderingConfigurationService>()->GetRenderingConfig();
	auto l_components = g_Engine->Get<ComponentManager>()->GetAll<LightComponent>();
	for (auto i : l_components)
	{
		m_Impl->UpdateColorTemperature(i);
		switch (i->m_LightType)
		{
		case LightType::Directional:
			// @TODO: Better to limit the directional light count
			m_Impl->UpdateCSMData(i);
			break;
		case LightType::Point:
			m_Impl->UpdateAttenuationRadius(i);
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

bool LightSystem::Terminate()
{
	delete m_Impl;
	return true;
}

ObjectStatus LightSystem::GetStatus()
{
	return m_Impl->m_ObjectStatus;
}