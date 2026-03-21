#include "LightSystem.h"
#include "../Component/LightComponent.h"
#include "../Component/TransformComponent.h"
#include "../Common/Randomizer.h"
#include "../Common/LogService.h"
#include "../Common/MathHelper.h"
#include "EntityRegistry.h"
#include "ComponentManager.h" // TODO Phase2-migrate: bridge for not-yet-migrated consumers (JSONWrapper, PerFrameDataService, BillboardDrawCallService, Baker — Tasks 10-12)
#include "CameraSystem.h"
#include "PhysicsSimulationService.h"
#include "RenderingConfigurationService.h"
#include "../Engine.h"

using namespace Inno;

namespace Inno
{
	struct LightSystemImpl
	{
		const size_t m_MaxComponentCount = 8192;

		AABB SnapAABBToShadowMap(const AABB& rhs, float shadowMapResolution);
		void AlignMatrixToTexels(Mat4& matrix, float shadowMapResolution);
		void UpdateCSMData(EntityID EntityID, const LightComponent& Light);
		void UpdateColorTemperature(LightComponent& Light);
		void UpdateAttenuationRadius(LightComponent& Light);

		ObjectStatus m_ObjectStatus = ObjectStatus::Invalid;

		std::unordered_map<EntityID, std::vector<AABB>> m_LitRegionWorldSpace;
		std::unordered_map<EntityID, std::vector<AABB>> m_LitRegionLightSpace;
		std::unordered_map<EntityID, std::vector<Mat4>> m_ViewMatrices;
		std::unordered_map<EntityID, std::vector<Mat4>> m_ProjectionMatrices;
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

void LightSystemImpl::UpdateCSMData(EntityID EntityID, const LightComponent& Light)
{
	auto& l_ViewMatrices = m_ViewMatrices[EntityID];
	auto& l_ProjectionMatrices = m_ProjectionMatrices[EntityID];
	auto& l_LitRegionWorldSpace = m_LitRegionWorldSpace[EntityID];
	auto& l_LitRegionLightSpace = m_LitRegionLightSpace[EntityID];

	l_ViewMatrices.clear();
	l_ProjectionMatrices.clear();
	l_LitRegionWorldSpace.clear();
	l_LitRegionLightSpace.clear();

	auto l_cameraComponent = static_cast<ICameraSystem*>(g_Engine->Get<CameraSystem>())->GetMainCamera();
	if (l_cameraComponent == nullptr)
		return;

	auto* l_Transform = g_Engine->Get<EntityRegistry>()->Get<TransformComponent>(EntityID);
	if (!l_Transform)
	{
		Log(Warning, "LightSystem: light entity has no TransformComponent, skipping CSM update.");
		return;
	}
	auto l_rotationMatrix = Math::toRotationMatrix(l_Transform->m_LocalRot);
	auto l_rotationMatrix_inverse = l_rotationMatrix.inverse();

	auto& l_splitFrustumVerticesWS = l_cameraComponent->m_SplitFrustumVerticesWS;

	for (size_t i = 0; i < 4; i++)
	{
		AABB l_AABB_worldSpace = Math::GenerateAABB(&l_splitFrustumVerticesWS[i * 8], 8);
		l_LitRegionWorldSpace.emplace_back(l_AABB_worldSpace);

		AABB l_AABB_lightSpace = Math::ExtendAABBToBoundingSphere(l_AABB_worldSpace);
		l_AABB_lightSpace = Math::RotateAABBToNewSpace(l_AABB_lightSpace, l_rotationMatrix_inverse);

		auto l_shadowMapResolution = static_cast<float>(g_Engine->Get<RenderingConfigurationService>()->GetRenderingConfig().shadowMapResolution);
		l_AABB_lightSpace = SnapAABBToShadowMap(l_AABB_lightSpace, l_shadowMapResolution);
		l_LitRegionLightSpace.emplace_back(l_AABB_lightSpace);

		AlignMatrixToTexels(l_rotationMatrix_inverse, l_shadowMapResolution);
		l_ViewMatrices.emplace_back(l_rotationMatrix_inverse);

		auto l_projectionMatrix = Math::GenerateOrthographicMatrix(
			l_AABB_lightSpace.m_boundMin.x, l_AABB_lightSpace.m_boundMax.x
		, l_AABB_lightSpace.m_boundMin.y, l_AABB_lightSpace.m_boundMax.y
		, l_AABB_lightSpace.m_boundMax.z, l_AABB_lightSpace.m_boundMin.z);
		l_ProjectionMatrices.emplace_back(l_projectionMatrix);
	}
}

void LightSystemImpl::UpdateColorTemperature(LightComponent& Light)
{
	if (!Light.m_UseColorTemperature)
		return;

	Light.m_RGBColor = Math::ColorTemperatureToRGB(Light.m_ColorTemperature);
}

void LightSystemImpl::UpdateAttenuationRadius(LightComponent& Light)
{
	auto l_RGBColor = Light.m_RGBColor.normalize();
	auto l_relativeLuminanceRatio = (0.2126f * l_RGBColor.x + 0.7152f * l_RGBColor.y + 0.0722f * l_RGBColor.z);
	auto l_weightedLuminousFlux = Light.m_LuminousFlux * l_relativeLuminanceRatio;
	Light.m_Shape.x = std::sqrtf(l_weightedLuminousFlux / (4.0f * PI<float> * 0.03f));
}

bool LightSystem::Setup(ISystemConfig* systemConfig)
{
	m_Impl = new LightSystemImpl();
	g_Engine->Get<ComponentManager>()->RegisterType<LightComponent>(m_Impl->m_MaxComponentCount, this); // TODO Phase2-migrate: bridge — remove when all consumers use EntityRegistry
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
	auto& l_Storage = g_Engine->Get<EntityRegistry>()->Storage<LightComponent>();
	auto& l_Lights = l_Storage.All();
	const auto& l_Owners = l_Storage.AllOwners();

	for (size_t i = 0; i < l_Lights.size(); i++)
	{
		LightComponent& l_Light = l_Lights[i];
		EntityID l_EntityID = l_Owners[i];

		m_Impl->UpdateColorTemperature(l_Light);
		switch (l_Light.m_LightType)
		{
		case LightType::Directional:
			m_Impl->UpdateCSMData(l_EntityID, l_Light);
			break;
		case LightType::Point:
			m_Impl->UpdateAttenuationRadius(l_Light);
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
	m_Impl->m_LitRegionWorldSpace.clear();
	m_Impl->m_LitRegionLightSpace.clear();
	m_Impl->m_ViewMatrices.clear();
	m_Impl->m_ProjectionMatrices.clear();

	delete m_Impl;
	return true;
}

ObjectStatus LightSystem::GetStatus()
{
	return m_Impl->m_ObjectStatus;
}

const std::unordered_map<EntityID, std::vector<Math::AABB>>& LightSystem::GetLitRegionWorldSpace() const
{
	return m_Impl->m_LitRegionWorldSpace;
}

const std::unordered_map<EntityID, std::vector<Math::AABB>>& LightSystem::GetLitRegionLightSpace() const
{
	return m_Impl->m_LitRegionLightSpace;
}

const std::unordered_map<EntityID, std::vector<Math::Mat4>>& LightSystem::GetViewMatrices() const
{
	return m_Impl->m_ViewMatrices;
}

const std::unordered_map<EntityID, std::vector<Math::Mat4>>& LightSystem::GetProjectionMatrices() const
{
	return m_Impl->m_ProjectionMatrices;
}
