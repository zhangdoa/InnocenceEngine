#include "LightComponentManager.h"
#include "../Component/LightComponent.h"
#include "../Core/InnoMemory.h"
#include "../Core/InnoLogger.h"
#include "../Common/CommonMacro.inl"
#include "CommonFunctionDefinitionMacro.inl"
#include "../Common/InnoMathHelper.h"

#include "ITransformComponentManager.h"
#include "ICameraComponentManager.h"

#include "../ModuleManager/IModuleManager.h"

extern IModuleManager* g_pModuleManager;

namespace LightComponentManagerNS
{
	void splitVerticesToAABBs(const std::vector<Vertex>& frustumsVertices, const std::vector<float>& splitFactors, std::vector<AABB>& splitAABB);
	void UpdateCSMData(LightComponent* rhs);
	void UpdateColorTemperature(LightComponent* rhs);
	void UpdateAttenuationRadius(LightComponent* rhs);

	const size_t m_MaxComponentCount = 8192;
	size_t m_CurrentComponentIndex = 0;
	IObjectPool* m_ComponentPool;
	ThreadSafeVector<LightComponent*> m_Components;
	ThreadSafeUnorderedMap<InnoEntity*, LightComponent*> m_ComponentsMap;

	LightComponent* m_Sun = 0;
	std::function<void()> f_SceneLoadingStartCallback;
	std::function<void()> f_SceneLoadingFinishCallback;

	std::vector<float> m_CSMSplitFactors = { 0.05f, 0.25f, 0.55f, 1.0f };
	std::vector<Vec4> m_frustumsCornerPos;
	std::vector<Vertex> m_frustumsCornerVertices;
	std::vector<AABB> m_SplitAABBWS;
	std::vector<AABB> m_SplitAABBLS;
	std::vector<Mat4> m_projectionMatrices;
}

void LightComponentManagerNS::splitVerticesToAABBs(const std::vector<Vertex>& frustumsVertices, const std::vector<float>& splitFactors, std::vector<AABB>& splitAABB)
{
	m_frustumsCornerPos.clear();
	splitAABB.clear();

	//1. first 4 corner
	for (size_t i = 0; i < 4; i++)
	{
		m_frustumsCornerPos.emplace_back(frustumsVertices[i].m_pos);
	}

	//2. other 16 corner based on the split factors
	for (size_t i = 0; i < 4; i++)
	{
		for (size_t j = 0; j < 4; j++)
		{
			auto l_direction = (frustumsVertices[j + 4].m_pos - frustumsVertices[j].m_pos);
			auto l_splitPlaneCornerPos = frustumsVertices[j].m_pos + l_direction * splitFactors[i];
			m_frustumsCornerPos.emplace_back(l_splitPlaneCornerPos);
		}
	}
	//https://docs.microsoft.com/windows/desktop/DxTechArts/common-techniques-to-improve-shadow-depth-maps
	//3. assemble split frustum corners
	auto l_renderingConfig = g_pModuleManager->getRenderingFrontend()->getRenderingConfig();

	if (l_renderingConfig.CSMFitToScene)
	{
		for (size_t i = 0; i < 4; i++)
		{
			m_frustumsCornerVertices[i * 8].m_pos = m_frustumsCornerPos[0];
			m_frustumsCornerVertices[i * 8 + 1].m_pos = m_frustumsCornerPos[1];
			m_frustumsCornerVertices[i * 8 + 2].m_pos = m_frustumsCornerPos[2];
			m_frustumsCornerVertices[i * 8 + 3].m_pos = m_frustumsCornerPos[3];

			for (size_t j = 4; j < 8; j++)
			{
				m_frustumsCornerVertices[i * 8 + j].m_pos = m_frustumsCornerPos[i * 4 + j];
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
				m_frustumsCornerVertices[i * 8 + j].m_pos = m_frustumsCornerPos[i * 4 + j];
			}
		}
	}

	//4. generate AABBs for the split frustums
	for (size_t i = 0; i < 4; i++)
	{
		splitAABB.emplace_back(InnoMath::generateAABB(&m_frustumsCornerVertices[i * 8], 8));
	}
}

void LightComponentManagerNS::UpdateCSMData(LightComponent* rhs)
{
	m_projectionMatrices.clear();

	//1. get frustum vertices in view space
	auto l_cameraComponent = GetComponentManager(CameraComponent)->GetMainCamera();
	if (l_cameraComponent == nullptr)
	{
		return;
	}
	auto l_cameraTransformComponent = GetComponent(TransformComponent, l_cameraComponent->m_ParentEntity);
	if (l_cameraTransformComponent == nullptr)
	{
		return;
	}
	auto l_rCamera = InnoMath::toRotationMatrix(l_cameraTransformComponent->m_globalTransformVector.m_rot);
	auto l_tCamera = InnoMath::toTranslationMatrix(l_cameraTransformComponent->m_globalTransformVector.m_pos);
	auto l_frustumVerticesVS = InnoMath::generateFrustumVerticesVS(l_cameraComponent->m_projectionMatrix);

	// extend scene AABB to include the bound sphere, for to eliminate rotation conflict
	auto l_totalSceneAABB = g_pModuleManager->getPhysicsSystem()->getVisibleSceneAABB();
	auto l_sphereRadius = (l_totalSceneAABB.m_boundMax - l_totalSceneAABB.m_center).length();
	auto l_boundMax = l_totalSceneAABB.m_center + l_sphereRadius;
	l_boundMax.w = 1.0f;
	auto l_boundMin = l_totalSceneAABB.m_center - l_sphereRadius;
	l_boundMin.w = 1.0f;

	// transform scene AABB vertices to view space
	auto l_sceneAABBVerticesWS = InnoMath::generateAABBVertices(l_boundMax, l_boundMin);
	auto l_sceneAABBVerticesVS = InnoMath::worldToViewSpace(l_sceneAABBVerticesWS, l_tCamera, l_rCamera);
	auto l_sceneAABBVS = InnoMath::generateAABB(&l_sceneAABBVerticesVS[0], l_sceneAABBVerticesVS.size());

	auto l_renderingConfig = g_pModuleManager->getRenderingFrontend()->getRenderingConfig();

	if (l_renderingConfig.CSMAdjustDrawDistance)
	{
		// compare draw distance and z component of the farest scene AABB vertex in view space
		auto l_distance_original = std::abs(l_frustumVerticesVS[4].m_pos.z - l_frustumVerticesVS[0].m_pos.z);
		auto l_distance_adjusted = l_frustumVerticesVS[0].m_pos.z - l_sceneAABBVS.m_boundMin.z;

		// scene is inside the view frustum
		if (l_distance_adjusted > 0)
		{
			// adjust draw distance and frustum vertices
			if (l_distance_adjusted < l_distance_original)
			{
				// move the far plane closer to the new far point
				for (size_t i = 4; i < l_frustumVerticesVS.size(); i++)
				{
					l_frustumVerticesVS[i].m_pos.x = l_frustumVerticesVS[i].m_pos.x * l_distance_adjusted / l_distance_original;
					l_frustumVerticesVS[i].m_pos.y = l_frustumVerticesVS[i].m_pos.y * l_distance_adjusted / l_distance_original;
					l_frustumVerticesVS[i].m_pos.z = l_sceneAABBVS.m_boundMin.z;
				}
			}
		}
	}

	// @TODO: eliminate false positive off the side plane
	if (l_renderingConfig.CSMAdjustSidePlane)
	{
		// Adjust x and y to include the scene
		// +x axis
		if (l_sceneAABBVS.m_boundMax.x > l_frustumVerticesVS[2].m_pos.x)
		{
			l_frustumVerticesVS[2].m_pos.x = l_sceneAABBVS.m_boundMax.x;
			l_frustumVerticesVS[3].m_pos.x = l_sceneAABBVS.m_boundMax.x;
			l_frustumVerticesVS[6].m_pos.x = l_sceneAABBVS.m_boundMax.x;
			l_frustumVerticesVS[7].m_pos.x = l_sceneAABBVS.m_boundMax.x;
		}
		// -x axis
		if (l_sceneAABBVS.m_boundMin.x < l_frustumVerticesVS[0].m_pos.x)
		{
			l_frustumVerticesVS[0].m_pos.x = l_sceneAABBVS.m_boundMin.x;
			l_frustumVerticesVS[1].m_pos.x = l_sceneAABBVS.m_boundMin.x;
			l_frustumVerticesVS[4].m_pos.x = l_sceneAABBVS.m_boundMin.x;
			l_frustumVerticesVS[5].m_pos.x = l_sceneAABBVS.m_boundMin.x;
		}
		// +y axis
		if (l_sceneAABBVS.m_boundMax.y > l_frustumVerticesVS[0].m_pos.y)
		{
			l_frustumVerticesVS[0].m_pos.y = l_sceneAABBVS.m_boundMax.y;
			l_frustumVerticesVS[3].m_pos.y = l_sceneAABBVS.m_boundMax.y;
			l_frustumVerticesVS[4].m_pos.y = l_sceneAABBVS.m_boundMax.y;
			l_frustumVerticesVS[7].m_pos.y = l_sceneAABBVS.m_boundMax.y;
		}
		// -y axis
		if (l_sceneAABBVS.m_boundMin.y < l_frustumVerticesVS[1].m_pos.y)
		{
			l_frustumVerticesVS[1].m_pos.y = l_sceneAABBVS.m_boundMin.y;
			l_frustumVerticesVS[2].m_pos.y = l_sceneAABBVS.m_boundMin.y;
			l_frustumVerticesVS[5].m_pos.y = l_sceneAABBVS.m_boundMin.y;
			l_frustumVerticesVS[6].m_pos.y = l_sceneAABBVS.m_boundMin.y;
		}
	}

	auto l_frustumVerticesWS = InnoMath::viewToWorldSpace(l_frustumVerticesVS, l_tCamera, l_rCamera);

	//2. calculate AABBs in world space
	splitVerticesToAABBs(l_frustumVerticesWS, m_CSMSplitFactors, m_SplitAABBWS);

	//4. transform frustum vertices to light space
	auto l_lightRotMat = GetComponent(TransformComponent, rhs->m_ParentEntity)->m_globalTransformMatrix.m_rotationMat.inverse();
	auto l_frustumVerticesLS = l_frustumVerticesWS;

	for (size_t i = 0; i < l_frustumVerticesLS.size(); i++)
	{
#ifdef USE_COLUMN_MAJOR_MEMORY_LAYOUT
		l_frustumVerticesLS[i].m_pos = InnoMath::mul(l_frustumVerticesLS[i].m_pos, l_lightRotMat);
#endif
#ifdef USE_ROW_MAJOR_MEMORY_LAYOUT
		l_frustumVerticesLS[i].m_pos = InnoMath::mul(l_lightRotMat, l_frustumVerticesLS[i].m_pos);
#endif
	}

	//5.calculate AABBs in light space
	splitVerticesToAABBs(l_frustumVerticesLS, m_CSMSplitFactors, m_SplitAABBLS);

	//6. extend AABB to include the bound sphere, for to eliminate rotation conflict
	for (size_t i = 0; i < 4; i++)
	{
		auto sphereRadius = (m_SplitAABBLS[i].m_boundMax - m_SplitAABBLS[i].m_center).length();
		auto l_boundMax = m_SplitAABBLS[i].m_center + sphereRadius;
		l_boundMax.w = 1.0f;
		auto l_boundMin = m_SplitAABBLS[i].m_center - sphereRadius;
		l_boundMin.w = 1.0f;
		m_SplitAABBLS[i] = InnoMath::generateAABB(l_boundMax, l_boundMin);
	}

	//7. generate projection matrices
	for (size_t i = 0; i < 4; i++)
	{
		Vec4 l_maxExtents = m_SplitAABBLS[i].m_boundMax;
		Vec4 l_minExtents = m_SplitAABBLS[i].m_boundMin;

		Mat4 p = InnoMath::generateOrthographicMatrix(l_minExtents.x, l_maxExtents.x, l_minExtents.y, l_maxExtents.y, l_minExtents.z, l_maxExtents.z);
		m_projectionMatrices.emplace_back(p);
	}
}

void LightComponentManagerNS::UpdateColorTemperature(LightComponent * rhs)
{
	if (rhs->m_UseColorTemperature)
	{
		rhs->m_RGBColor = InnoMath::colorTemperatureToRGB(rhs->m_ColorTemperature);
	}
}

void LightComponentManagerNS::UpdateAttenuationRadius(LightComponent* rhs)
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

using namespace LightComponentManagerNS;

bool InnoLightComponentManager::Setup()
{
	m_ComponentPool = InnoMemory::CreateObjectPool<LightComponent>(m_MaxComponentCount);
	m_Components.reserve(m_MaxComponentCount);
	m_ComponentsMap.reserve(m_MaxComponentCount);
	m_frustumsCornerPos.reserve(20);
	m_frustumsCornerVertices.resize(32);
	m_SplitAABBWS.reserve(4);
	m_SplitAABBLS.reserve(4);
	m_projectionMatrices.reserve(4);

	f_SceneLoadingStartCallback = [&]() {
		CleanComponentContainers(LightComponent);
	};

	f_SceneLoadingFinishCallback = [&]() {
	};

	g_pModuleManager->getFileSystem()->addSceneLoadingStartCallback(&f_SceneLoadingStartCallback);
	g_pModuleManager->getFileSystem()->addSceneLoadingFinishCallback(&f_SceneLoadingFinishCallback);

	return true;
}

bool InnoLightComponentManager::Initialize()
{
	return true;
}

bool InnoLightComponentManager::Simulate()
{
	for (auto i : m_Components)
	{
		UpdateColorTemperature(i);
		switch (i->m_LightType)
		{
		case LightType::Directional:
			// @TODO: Better to limit the directional light count
			m_Sun = i;
			UpdateCSMData(i);
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

bool InnoLightComponentManager::Terminate()
{
	return true;
}

InnoComponent * InnoLightComponentManager::Spawn(const InnoEntity* parentEntity, ObjectSource objectSource, ObjectOwnership objectUsage)
{
	SpawnComponentImpl(LightComponent);
}

void InnoLightComponentManager::Destroy(InnoComponent * component)
{
	DestroyComponentImpl(LightComponent);
}

InnoComponent* InnoLightComponentManager::Find(const InnoEntity * parentEntity)
{
	GetComponentImpl(LightComponent, parentEntity);
}

const std::vector<LightComponent*>& InnoLightComponentManager::GetAllComponents()
{
	return m_Components.getRawData();
}

const LightComponent * InnoLightComponentManager::GetSun()
{
	return m_Sun;
}

const std::vector<AABB>& InnoLightComponentManager::GetSunSplitAABB()
{
	return m_SplitAABBWS;
}

const std::vector<Mat4>& InnoLightComponentManager::GetSunProjectionMatrices()
{
	return m_projectionMatrices;
}