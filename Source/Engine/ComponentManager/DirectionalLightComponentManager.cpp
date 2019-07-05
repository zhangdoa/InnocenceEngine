#include "DirectionalLightComponentManager.h"
#include "../Component/DirectionalLightComponent.h"
#include "../Core/InnoMemory.h"
#include "../Core/InnoLogger.h"
#include "../Common/CommonMacro.inl"
#include "CommonFunctionDefinitionMacro.inl"
#include "../Common/InnoMathHelper.h"

#include "ITransformComponentManager.h"
#include "ICameraComponentManager.h"

#include "../ModuleManager/IModuleManager.h"

extern IModuleManager* g_pModuleManager;

namespace DirectionalLightComponentManagerNS
{
	const size_t m_MaxComponentCount = 16;
	size_t m_CurrentComponentIndex = 0;
	IObjectPool* m_ComponentPool;
	ThreadSafeVector<DirectionalLightComponent*> m_Components;
	ThreadSafeUnorderedMap<InnoEntity*, DirectionalLightComponent*> m_ComponentsMap;

	std::function<void()> f_SceneLoadingStartCallback;
	std::function<void()> f_SceneLoadingFinishCallback;

	std::vector<AABB> m_SplitAABB;
	std::vector<mat4> m_projectionMatrices;

	std::vector<AABB> splitVerticesToAABBs(const std::vector<Vertex>& frustumsVertices, const std::vector<float>& splitFactors);
	void UpdateCSMData(DirectionalLightComponent* rhs);
}

std::vector<AABB> DirectionalLightComponentManagerNS::splitVerticesToAABBs(const std::vector<Vertex>& frustumsVertices, const std::vector<float>& splitFactors)
{
	std::vector<vec4> l_frustumsCornerPos;
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
	std::vector<Vertex> l_frustumsCornerVertices(32);

	static bool l_fitToScene = true;
	if (l_fitToScene)
	{
		for (size_t i = 0; i < 4; i++)
		{
			l_frustumsCornerVertices[i * 8].m_pos = l_frustumsCornerPos[0];
			l_frustumsCornerVertices[i * 8 + 1].m_pos = l_frustumsCornerPos[1];
			l_frustumsCornerVertices[i * 8 + 2].m_pos = l_frustumsCornerPos[2];
			l_frustumsCornerVertices[i * 8 + 3].m_pos = l_frustumsCornerPos[3];

			for (size_t j = 4; j < 8; j++)
			{
				l_frustumsCornerVertices[i * 8 + j].m_pos = l_frustumsCornerPos[i * 4 + j];
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
				l_frustumsCornerVertices[i * 8 + j].m_pos = l_frustumsCornerPos[i * 4 + j];
			}
		}
	}

	//4. assemble split frustums
	std::vector<std::vector<Vertex>> l_splitFrustums;
	l_splitFrustums.reserve(4);

	for (size_t i = 0; i < 4; i++)
	{
		auto l_splitFrustum = std::vector<Vertex>(l_frustumsCornerVertices.begin() + i * 8, l_frustumsCornerVertices.begin() + 8 + i * 8);
		l_splitFrustums.emplace_back(l_splitFrustum);
	}

	//5. generate AABBs for the split frustums
	std::vector<AABB> l_AABBs;
	l_AABBs.reserve(4);

	for (size_t i = 0; i < 4; i++)
	{
		l_AABBs.emplace_back(InnoMath::generateAABB(&l_splitFrustums[i][0], l_splitFrustums[i].size()));
	}

	return l_AABBs;
}

void DirectionalLightComponentManagerNS::UpdateCSMData(DirectionalLightComponent* rhs)
{
	m_SplitAABB.clear();
	m_projectionMatrices.clear();

	//1. get frustum vertices in view space
	auto l_cameraComponents = GetComponentManager(CameraComponent)->GetAllComponents();
	auto l_cameraComponent = l_cameraComponents[0];
	auto l_cameraTransformComponent = GetComponent(TransformComponent, l_cameraComponent->m_parentEntity);
	auto l_rCamera = InnoMath::toRotationMatrix(l_cameraTransformComponent->m_globalTransformVector.m_rot);
	auto l_tCamera = InnoMath::toTranslationMatrix(l_cameraTransformComponent->m_globalTransformVector.m_pos);
	auto l_frustumVerticesVS = InnoMath::generateFrustumVerticesVS(l_cameraComponent->m_projectionMatrix);

	static bool l_adjustDrawDistance = false;
	if (l_adjustDrawDistance)
	{
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

			// @TODO: eliminate false positive off the side plane
			static bool l_adjustSidePlane = true;
			if (l_adjustSidePlane)
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
		}
	}

	auto l_frustumVerticesWS = InnoMath::viewToWorldSpace(l_frustumVerticesVS, l_tCamera, l_rCamera);

	std::vector<float> l_CSMSplitFactors = { 0.05f, 0.25f, 0.55f, 1.0f };

	//2. calculate AABBs in world space
	auto l_frustumsAABBsWS = splitVerticesToAABBs(l_frustumVerticesWS, l_CSMSplitFactors);

	//3. save the AABB for bound area detection
	m_SplitAABB = std::move(l_frustumsAABBsWS);

	//4. transform frustum vertices to light space
	auto l_lightRotMat = GetComponent(TransformComponent, rhs->m_parentEntity)->m_globalTransformMatrix.m_rotationMat.inverse();
	auto l_frustumVerticesLS = l_frustumVerticesWS;

	for (size_t i = 0; i < l_frustumVerticesLS.size(); i++)
	{
		//Column-Major memory layout
#ifdef USE_COLUMN_MAJOR_MEMORY_LAYOUT
		l_frustumVerticesLS[i].m_pos = InnoMath::mul(l_frustumVerticesLS[i].m_pos, l_lightRotMat);
#endif
		//Row-Major memory layout
#ifdef USE_ROW_MAJOR_MEMORY_LAYOUT
		l_frustumVerticesLS[i].m_pos = InnoMath::mul(l_lightRotMat, l_frustumVerticesLS[i].m_pos);
#endif
	}

	//5.calculate AABBs in light space
	auto l_AABBsLS = splitVerticesToAABBs(l_frustumVerticesLS, l_CSMSplitFactors);

	//6. extend AABB to include the bound sphere, for to eliminate rotation conflict
	for (size_t i = 0; i < 4; i++)
	{
		auto sphereRadius = (l_AABBsLS[i].m_boundMax - l_AABBsLS[i].m_center).length();
		auto l_boundMax = l_AABBsLS[i].m_center + sphereRadius;
		l_boundMax.w = 1.0f;
		auto l_boundMin = l_AABBsLS[i].m_center - sphereRadius;
		l_boundMin.w = 1.0f;
		l_AABBsLS[i] = InnoMath::generateAABB(l_boundMax, l_boundMin);
	}

	//7. generate projection matrices
	m_projectionMatrices.reserve(4);

	for (size_t i = 0; i < 4; i++)
	{
		vec4 l_maxExtents = l_AABBsLS[i].m_boundMax;
		vec4 l_minExtents = l_AABBsLS[i].m_boundMin;

		mat4 p = InnoMath::generateOrthographicMatrix(l_minExtents.x, l_maxExtents.x, l_minExtents.y, l_maxExtents.y, l_minExtents.z, l_maxExtents.z);
		m_projectionMatrices.emplace_back(p);
	}
}

using namespace DirectionalLightComponentManagerNS;

bool InnoDirectionalLightComponentManager::Setup()
{
	m_ComponentPool = InnoMemory::CreateObjectPool(sizeof(DirectionalLightComponent), m_MaxComponentCount);
	m_Components.reserve(m_MaxComponentCount);

	f_SceneLoadingStartCallback = [&]() {
		CleanComponentContainers(DirectionalLightComponent);
	};

	f_SceneLoadingFinishCallback = [&]() {
	};

	g_pModuleManager->getFileSystem()->addSceneLoadingStartCallback(&f_SceneLoadingStartCallback);
	g_pModuleManager->getFileSystem()->addSceneLoadingFinishCallback(&f_SceneLoadingFinishCallback);

	return true;
}

bool InnoDirectionalLightComponentManager::Initialize()
{
	return true;
}

bool InnoDirectionalLightComponentManager::Simulate()
{
	for (auto i : m_Components)
	{
		UpdateCSMData(i);
	}
	return true;
}

bool InnoDirectionalLightComponentManager::Terminate()
{
	return true;
}

InnoComponent * InnoDirectionalLightComponentManager::Spawn(const InnoEntity* parentEntity, ObjectSource objectSource, ObjectUsage objectUsage)
{
	SpawnComponentImpl(DirectionalLightComponent);
}

void InnoDirectionalLightComponentManager::Destory(InnoComponent * component)
{
	DestroyComponentImpl(DirectionalLightComponent);
}

InnoComponent* InnoDirectionalLightComponentManager::Find(const InnoEntity * parentEntity)
{
	GetComponentImpl(DirectionalLightComponent, parentEntity);
}

const std::vector<DirectionalLightComponent*>& InnoDirectionalLightComponentManager::GetAllComponents()
{
	return m_Components.getRawData();
}

const std::vector<AABB>& InnoDirectionalLightComponentManager::GetSplitAABB()
{
	return m_SplitAABB;
}

const std::vector<mat4>& InnoDirectionalLightComponentManager::GetProjectionMatrices()
{
	return m_projectionMatrices;
}