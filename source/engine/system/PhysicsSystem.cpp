#include "PhysicsSystem.h"

#include "../component/GameSystemSingletonComponent.h"
#include "../component/RenderingSystemSingletonComponent.h"
#include "../component/WindowSystemSingletonComponent.h"

#include "ICoreSystem.h"

extern ICoreSystem* g_pCoreSystem;

namespace InnoPhysicsSystemNS
{
	void initializeComponents();
	void initializeCameraComponents();
	void generateProjectionMatrix(CameraComponent* cameraComponent);
	void generateRayOfEye(CameraComponent* cameraComponent);
	void generateFrustumVertices(CameraComponent* cameraComponent);
	void initializeVisibleComponents();
	void initializeLightComponents();
	void setupLightComponentRadius(LightComponent* lightComponent);

	std::vector<Vertex> generateNDC();
	void generateAABB(VisibleComponent & visibleComponent);
	void generateAABB(LightComponent & lightComponent);
	void generateAABB(CameraComponent & cameraComponent);
	AABB generateAABB(const std::vector<Vertex>& vertices);
	AABB generateAABB(const vec4& boundMax, const vec4& boundMin);

	void updateCameraComponents();
	void updateLightComponents();
	void updateCulling();

	objectStatus m_objectStatus = objectStatus::SHUTDOWN;

	InnoFuture<void>* m_asyncTask;

	static WindowSystemSingletonComponent* g_WindowSystemSingletonComponent;
	static RenderingSystemSingletonComponent* g_RenderingSystemSingletonComponent;
	static GameSystemSingletonComponent* g_GameSystemSingletonComponent;
}

INNO_SYSTEM_EXPORT bool InnoPhysicsSystem::setup()
{	
	InnoPhysicsSystemNS::g_WindowSystemSingletonComponent = &WindowSystemSingletonComponent::getInstance();
	InnoPhysicsSystemNS::g_RenderingSystemSingletonComponent = &RenderingSystemSingletonComponent::getInstance();
	InnoPhysicsSystemNS::g_GameSystemSingletonComponent = &GameSystemSingletonComponent::getInstance();

	InnoPhysicsSystemNS::m_objectStatus = objectStatus::ALIVE;
	return true;
}

void InnoPhysicsSystemNS::initializeComponents()
{
	initializeCameraComponents();
	initializeVisibleComponents();
	initializeLightComponents();
}

void InnoPhysicsSystemNS::initializeCameraComponents()
{
	for (auto& i : g_GameSystemSingletonComponent->m_CameraComponents)
	{
		generateProjectionMatrix(i);
		generateRayOfEye(i);
		generateFrustumVertices(i);
		generateAABB(*i);
	}
}

void InnoPhysicsSystemNS::generateProjectionMatrix(CameraComponent* cameraComponent)
{
	cameraComponent->m_projectionMatrix.initializeToPerspectiveMatrix((cameraComponent->m_FOVX / 180.0f) * PI<float>, cameraComponent->m_WHRatio, cameraComponent->m_zNear, cameraComponent->m_zFar);
}

void InnoPhysicsSystemNS::generateRayOfEye(CameraComponent * cameraComponent)
{
	cameraComponent->m_rayOfEye.m_origin = g_pCoreSystem->getGameSystem()->get<TransformComponent>(cameraComponent->m_parentEntity)->m_globalTransformVector.m_pos;
	cameraComponent->m_rayOfEye.m_direction = InnoMath::getDirection(
		direction::BACKWARD,
		g_pCoreSystem->getGameSystem()->get<TransformComponent>(cameraComponent->m_parentEntity)->m_localTransformVector.m_rot
		);
}

void InnoPhysicsSystemNS::generateFrustumVertices(CameraComponent * cameraComponent)
{
	cameraComponent->m_frustumVertices.clear();
	cameraComponent->m_frustumIndices.clear();

	auto l_NDC = generateNDC();
	auto l_pCamera = cameraComponent->m_projectionMatrix;

	auto l_cameraTransformComponent = g_pCoreSystem->getGameSystem()->get<TransformComponent>(cameraComponent->m_parentEntity);
	auto l_rCamera = InnoMath::toRotationMatrix(l_cameraTransformComponent->m_globalTransformVector.m_rot);
	auto l_tCamera = InnoMath::toTranslationMatrix(l_cameraTransformComponent->m_globalTransformVector.m_pos);

	for (auto& l_vertexData : l_NDC)
	{
		vec4 l_mulPos;
		l_mulPos = l_vertexData.m_pos;
		// from projection space to view space
		//Column-Major memory layout
#ifdef USE_COLUMN_MAJOR_MEMORY_LAYOUT
		l_mulPos = InnoMath::mul(l_mulPos, l_pCamera.inverse());
#endif
		//Row-Major memory layout
#ifdef USE_ROW_MAJOR_MEMORY_LAYOUT
		l_mulPos = InnoMath::mul(l_pCamera.inverse(), l_mulPos);
#endif
		// perspective division
		l_mulPos = l_mulPos * (1.0f / l_mulPos.w);
		// from view space to world space
		//Column-Major memory layout
#ifdef USE_COLUMN_MAJOR_MEMORY_LAYOUT
		l_mulPos = InnoMath::mul(l_mulPos, l_rCamera);
		l_mulPos = InnoMath::mul(l_mulPos, l_tCamera);
#endif
		//Row-Major memory layout
#ifdef USE_ROW_MAJOR_MEMORY_LAYOUT
		l_mulPos = InnoMath::mul(l_rCamera, l_mulPos);
		l_mulPos = InnoMath::mul(l_tCamera, l_mulPos);
#endif
		l_vertexData.m_pos = l_mulPos;
	}

	for (auto& l_vertexData : l_NDC)
	{
		l_vertexData.m_normal = vec4(l_vertexData.m_pos.x, l_vertexData.m_pos.y, l_vertexData.m_pos.z, 0.0f).normalize();
	}

	// near clip plane first
	auto l_containerSize = l_NDC.size();
	cameraComponent->m_frustumVertices.reserve(l_containerSize);

	for (size_t i = 0; i < l_containerSize; i++)
	{
		cameraComponent->m_frustumVertices.emplace_back(l_NDC[l_NDC.size() -  1 - i]);
	}

	cameraComponent->m_frustumIndices = { 0, 1, 3, 1, 2, 3,
		4, 5, 0, 5, 1, 0,
		7, 6, 4, 6, 5, 4,
		3, 2, 7, 2, 6 ,7,
		4, 0, 7, 0, 3, 7,
		1, 5, 2, 5, 6, 2 };
}

void InnoPhysicsSystemNS::initializeVisibleComponents()
{
	std::for_each(g_GameSystemSingletonComponent->m_VisibleComponents.begin(), g_GameSystemSingletonComponent->m_VisibleComponents.end(),
		[&](VisibleComponent* i)
	{
		if (i->m_visiblilityType == visiblilityType::EMISSIVE)
		{
			generateAABB(*i);
			g_RenderingSystemSingletonComponent->m_emissiveVisibleComponents.emplace_back(i);
		}
		else if (i->m_visiblilityType == visiblilityType::STATIC_MESH)
		{
			generateAABB(*i);
			g_RenderingSystemSingletonComponent->m_staticMeshVisibleComponents.emplace_back(i);
		}
	}
	);
}

void InnoPhysicsSystemNS::initializeLightComponents()
{
	for (auto& i : g_GameSystemSingletonComponent->m_LightComponents)
	{
		i->m_direction = vec4(0.0f, 0.0f, 1.0f, 0.0f);
		i->m_constantFactor = 1.0f;
		i->m_linearFactor = 0.14f;
		i->m_quadraticFactor = 0.07f;
		setupLightComponentRadius(i);
		i->m_color = vec4(1.0f, 1.0f, 1.0f, 1.0f);

		if (i->m_lightType == lightType::DIRECTIONAL)
		{
			generateAABB(*i);
		}
	}
}

void InnoPhysicsSystemNS::setupLightComponentRadius(LightComponent * lightComponent)
{
	float l_lightMaxIntensity = std::fmax(std::fmax(lightComponent->m_color.x, lightComponent->m_color.y), lightComponent->m_color.z);
	lightComponent->m_radius = -lightComponent->m_linearFactor + std::sqrt(lightComponent->m_linearFactor * lightComponent->m_linearFactor - 4.0f * lightComponent->m_quadraticFactor * (lightComponent->m_constantFactor - (256.0f / 5.0f) * l_lightMaxIntensity)) / (2.0f * lightComponent->m_quadraticFactor);
}

void InnoPhysicsSystemNS::generateAABB(VisibleComponent & visibleComponent)
{
	float maxX = 0;
	float maxY = 0;
	float maxZ = 0;
	float minX = 0;
	float minY = 0;
	float minZ = 0;

	std::vector<vec4> l_cornerVertices(visibleComponent.m_modelMap.size() * 2);

	for (auto& l_graphicData : visibleComponent.m_modelMap)
	{
		// get corner vertices from sub meshes
		l_cornerVertices.emplace_back(g_pCoreSystem->getAssetSystem()->findMaxVertex(l_graphicData.first));
		l_cornerVertices.emplace_back(g_pCoreSystem->getAssetSystem()->findMinVertex(l_graphicData.first));
	}

	std::for_each(l_cornerVertices.begin(), l_cornerVertices.end(), [&](vec4 val)
	{
		if (val.x >= maxX)
		{
			maxX = val.x;
		};
		if (val.y >= maxY)
		{
			maxY = val.y;
		};
		if (val.z >= maxZ)
		{
			maxZ = val.z;
		};
		if (val.x <= minX)
		{
			minX = val.x;
		};
		if (val.y <= minY)
		{
			minY = val.y;
		};
		if (val.z <= minZ)
		{
			minZ = val.z;
		};
	});

	visibleComponent.m_AABB = generateAABB(vec4(maxX, maxY, maxZ, 1.0f), vec4(minX, minY, minZ, 1.0f));

	auto xx = g_pCoreSystem->getGameSystem()->get<TransformComponent>(visibleComponent.m_parentEntity);

	auto l_worldTm = g_pCoreSystem->getGameSystem()->get<TransformComponent>(visibleComponent.m_parentEntity)->m_globalTransformMatrix.m_transformationMat;

	//Column-Major memory layout
#ifdef USE_COLUMN_MAJOR_MEMORY_LAYOUT
	visibleComponent.m_AABB.m_boundMax = InnoMath::mul(visibleComponent.m_AABB.m_boundMax, l_worldTm);
	visibleComponent.m_AABB.m_boundMin = InnoMath::mul(visibleComponent.m_AABB.m_boundMin,l_worldTm);
	visibleComponent.m_AABB.m_center = InnoMath::mul(visibleComponent.m_AABB.m_center, l_worldTm);
	for (auto& l_vertexData : visibleComponent.m_AABB.m_vertices)
	{
		l_vertexData.m_pos = InnoMath::mul(l_vertexData.m_pos, l_worldTm);
	}
#endif
	//Row-Major memory layout
#ifdef USE_ROW_MAJOR_MEMORY_LAYOUT
	visibleComponent.m_AABB.m_boundMax = InnoMath::mul(l_worldTm, visibleComponent.m_AABB.m_boundMax);
	visibleComponent.m_AABB.m_boundMin = InnoMath::mul(l_worldTm, visibleComponent.m_AABB.m_boundMin);
	visibleComponent.m_AABB.m_center = InnoMath::mul(l_worldTm, visibleComponent.m_AABB.m_center);
	for (auto& l_vertexData : visibleComponent.m_AABB.m_vertices)
	{
		l_vertexData.m_pos = InnoMath::mul(l_worldTm, l_vertexData.m_pos);
	}
#endif
}

void InnoPhysicsSystemNS::generateAABB(LightComponent & lightComponent)
{
	lightComponent.m_AABBs.clear();
	lightComponent.m_shadowSplitPoints.clear();
	lightComponent.m_projectionMatrices.clear();

	//1.translate the big frustum to light space
	auto l_camera = g_GameSystemSingletonComponent->m_CameraComponents[0];
	auto l_frustumVertices = l_camera->m_frustumVertices;

	//2.calculate splited planes' corners
	std::vector<vec4> l_frustumsCornerPos;
	l_frustumsCornerPos.reserve(20);

	//2.1 first 4 corner
	for (size_t i = 0; i < 4; i++)
	{
		l_frustumsCornerPos.emplace_back(l_frustumVertices[i].m_pos);
	}

	//2.2 other 16 corner based on e^2i / e^8
	lightComponent.m_shadowSplitPoints.reserve(4);

	for (size_t i = 0; i < 4; i++)
	{
		auto scaleFactor = std::exp(((float)i + 1.0f) * 2.0f / E<float>) / std::exp(8.0f / E<float>);
		lightComponent.m_shadowSplitPoints.emplace_back(scaleFactor);
		for (size_t j = 0; j < 4; j++)
		{
			auto l_splitedPlaneCornerPos = l_frustumVertices[j].m_pos + (l_frustumVertices[j + 4].m_pos - l_frustumVertices[j].m_pos) * scaleFactor;
			l_frustumsCornerPos.emplace_back(l_splitedPlaneCornerPos);
		}
	}

	//2.3 transform to light space
	auto l_lightRotMat = g_pCoreSystem->getGameSystem()->get<TransformComponent>(lightComponent.m_parentEntity)->m_globalTransformMatrix.m_transformationMat.inverse();
	for (size_t i = 0; i < l_frustumsCornerPos.size(); i++)
	{
		//Column-Major memory layout
#ifdef USE_COLUMN_MAJOR_MEMORY_LAYOUT
		l_frustumsCornerPos[i] = InnoMath::mul(l_frustumsCornerPos[i], l_lightRotMat);
#endif
		//Row-Major memory layout
#ifdef USE_ROW_MAJOR_MEMORY_LAYOUT
		l_frustumsCornerPos[i] = InnoMath::mul(l_lightRotMat, l_frustumsCornerPos[i]);
#endif	
	}

	//3.assemble splited frustums
	std::vector<Vertex> l_frustumsCornerVertices;
	auto l_NDC = generateNDC();

	for (size_t i = 0; i < 4; i++)
	{
		l_frustumsCornerVertices.insert(l_frustumsCornerVertices.end(), l_NDC.begin(), l_NDC.end());
	}
	for (size_t i = 0; i < 4; i++)
	{
		for (size_t j = 0; j < 8; j++)
		{
			l_frustumsCornerVertices[i * 8 + j].m_pos = l_frustumsCornerPos[i * 4 + j];
		}
	}

	std::vector<std::vector<Vertex>> l_splitedFrustums;
	l_splitedFrustums.reserve(4);

	for (size_t i = 0; i < 4; i++)
	{
		l_splitedFrustums.emplace_back(std::vector<Vertex>(l_frustumsCornerVertices.begin() + i * 8, l_frustumsCornerVertices.begin() + 8 + i * 8));
	}

	//4.generate AABBs for the splited frustums
	lightComponent.m_AABBs.reserve(4);

	for (size_t i = 0; i < 4; i++)
	{
		lightComponent.m_AABBs.emplace_back(generateAABB(l_splitedFrustums[i]));
	}

	lightComponent.m_projectionMatrices.reserve(4);

	for (size_t i = 0; i < 4; i++)
	{
		vec4 l_maxExtents = lightComponent.m_AABBs[i].m_boundMax;
		vec4 l_minExtents = lightComponent.m_AABBs[i].m_boundMin;
		vec4 l_center = lightComponent.m_AABBs[i].m_center;

		mat4 p;
		p.initializeToOrthographicMatrix(l_minExtents.x, l_maxExtents.x, l_minExtents.y, l_maxExtents.y, l_minExtents.z, l_maxExtents.z);
		lightComponent.m_projectionMatrices.emplace_back(p);
	}
}

void InnoPhysicsSystemNS::generateAABB(CameraComponent & cameraComponent)
{
	auto l_frustumCorners = cameraComponent.m_frustumVertices;
	cameraComponent.m_AABB = generateAABB(l_frustumCorners);
}

AABB InnoPhysicsSystemNS::generateAABB(const std::vector<Vertex>& vertices)
{
	float maxX = vertices[0].m_pos.x;
	float maxY = vertices[0].m_pos.y;
	float maxZ = vertices[0].m_pos.z;
	float minX = vertices[0].m_pos.x;
	float minY = vertices[0].m_pos.y;
	float minZ = vertices[0].m_pos.z;

	for (auto& l_vertexData : vertices)
	{
		if (l_vertexData.m_pos.x >= maxX)
		{
			maxX = l_vertexData.m_pos.x;
		}
		if (l_vertexData.m_pos.y >= maxY)
		{
			maxY = l_vertexData.m_pos.y;
		}
		if (l_vertexData.m_pos.z >= maxZ)
		{
			maxZ = l_vertexData.m_pos.z;
		}
		if (l_vertexData.m_pos.x <= minX)
		{
			minX = l_vertexData.m_pos.x;
		}
		if (l_vertexData.m_pos.y <= minY)
		{
			minY = l_vertexData.m_pos.y;
		}
		if (l_vertexData.m_pos.z <= minZ)
		{
			minZ = l_vertexData.m_pos.z;
		}
	}

	return generateAABB(vec4(maxX, maxY, maxZ, 1.0f), vec4(minX, minY, minZ, 1.0f));
}

AABB InnoPhysicsSystemNS::generateAABB(const vec4 & boundMax, const vec4 & boundMin)
{
	AABB l_AABB;

	l_AABB.m_boundMin = boundMin;
	l_AABB.m_boundMax = boundMax;

	l_AABB.m_center = (l_AABB.m_boundMax + l_AABB.m_boundMin) * 0.5f;
	l_AABB.m_sphereRadius = std::max<float>(std::max<float>((l_AABB.m_boundMax.x - l_AABB.m_boundMin.x) / 2.0f, (l_AABB.m_boundMax.y - l_AABB.m_boundMin.y) / 2.0f), (l_AABB.m_boundMax.z - l_AABB.m_boundMin.z) / 2.0f);

	Vertex l_VertexData_1;
	l_VertexData_1.m_pos = (vec4(boundMax.x, boundMax.y, boundMax.z, 1.0f));
	l_VertexData_1.m_texCoord = vec2(1.0f, 1.0f);

	Vertex l_VertexData_2;
	l_VertexData_2.m_pos = (vec4(boundMax.x, boundMin.y, boundMax.z, 1.0f));
	l_VertexData_2.m_texCoord = vec2(1.0f, 0.0f);

	Vertex l_VertexData_3;
	l_VertexData_3.m_pos = (vec4(boundMin.x, boundMin.y, boundMax.z, 1.0f));
	l_VertexData_3.m_texCoord = vec2(0.0f, 0.0f);

	Vertex l_VertexData_4;
	l_VertexData_4.m_pos = (vec4(boundMin.x, boundMax.y, boundMax.z, 1.0f));
	l_VertexData_4.m_texCoord = vec2(0.0f, 1.0f);

	Vertex l_VertexData_5;
	l_VertexData_5.m_pos = (vec4(boundMax.x, boundMax.y, boundMin.z, 1.0f));
	l_VertexData_5.m_texCoord = vec2(1.0f, 1.0f);

	Vertex l_VertexData_6;
	l_VertexData_6.m_pos = (vec4(boundMax.x, boundMin.y, boundMin.z, 1.0f));
	l_VertexData_6.m_texCoord = vec2(1.0f, 0.0f);

	Vertex l_VertexData_7;
	l_VertexData_7.m_pos = (vec4(boundMin.x, boundMin.y, boundMin.z, 1.0f));
	l_VertexData_7.m_texCoord = vec2(0.0f, 0.0f);

	Vertex l_VertexData_8;
	l_VertexData_8.m_pos = (vec4(boundMin.x, boundMax.y, boundMin.z, 1.0f));
	l_VertexData_8.m_texCoord = vec2(0.0f, 1.0f);


	l_AABB.m_vertices = { l_VertexData_1, l_VertexData_2, l_VertexData_3, l_VertexData_4, l_VertexData_5, l_VertexData_6, l_VertexData_7, l_VertexData_8 };

	for (auto& l_vertexData : l_AABB.m_vertices)
	{
		l_vertexData.m_normal = vec4(l_vertexData.m_pos.x, l_vertexData.m_pos.y, l_vertexData.m_pos.z, 0.0f).normalize();
	}

	l_AABB.m_indices = { 0, 1, 3, 1, 2, 3,
		4, 5, 0, 5, 1, 0,
		7, 6, 4, 6, 5, 4,
		3, 2, 7, 2, 6 ,7,
		4, 0, 7, 0, 3, 7,
		1, 5, 2, 5, 6, 2 };

	return l_AABB;
}

INNO_SYSTEM_EXPORT bool InnoPhysicsSystem::initialize()
{
	InnoPhysicsSystemNS::initializeComponents();
	InnoPhysicsSystemNS::m_objectStatus = objectStatus::ALIVE;
	g_pCoreSystem->getLogSystem()->printLog("PhysicsSystem has been initialized.");
	return true;
}

void InnoPhysicsSystemNS::updateCameraComponents()
{
	if (g_GameSystemSingletonComponent->m_CameraComponents.size() > 0)
	{
		std::for_each(g_GameSystemSingletonComponent->m_CameraComponents.begin(), g_GameSystemSingletonComponent->m_CameraComponents.end(),
			[&](CameraComponent* i)
		{
			generateRayOfEye(i);
			generateFrustumVertices(i);
			generateAABB(*i);
		}
		);
	}
}

void InnoPhysicsSystemNS::updateLightComponents()
{
	if (g_GameSystemSingletonComponent->m_LightComponents.size() > 0)
	{
		// generate AABB for CSM
		std::for_each(g_GameSystemSingletonComponent->m_LightComponents.begin(), g_GameSystemSingletonComponent->m_LightComponents.end(),
			[&](LightComponent* i)
		{
			setupLightComponentRadius(i);
			if (i->m_lightType == lightType::DIRECTIONAL)
			{
				generateAABB(*i);
			}
		}
		);
	}
}

void InnoPhysicsSystemNS::updateCulling()
{
	g_RenderingSystemSingletonComponent->m_selectedVisibleComponents.clear();
	g_RenderingSystemSingletonComponent->m_inFrustumVisibleComponents.clear();

	if (g_GameSystemSingletonComponent->m_CameraComponents.size() > 0)
	{
		g_WindowSystemSingletonComponent->m_mouseRay.m_origin = g_pCoreSystem->getGameSystem()->get<TransformComponent>(g_GameSystemSingletonComponent->m_CameraComponents[0]->m_parentEntity)->m_globalTransformVector.m_pos;
		g_WindowSystemSingletonComponent->m_mouseRay.m_direction = g_WindowSystemSingletonComponent->m_mousePositionInWorldSpace;

		auto l_cameraAABB = g_GameSystemSingletonComponent->m_CameraComponents[0]->m_AABB;

		auto l_ray = g_GameSystemSingletonComponent->m_CameraComponents[0]->m_rayOfEye;

		std::for_each(g_GameSystemSingletonComponent->m_VisibleComponents.begin(), g_GameSystemSingletonComponent->m_VisibleComponents.end(),
			[&](VisibleComponent* j) 
		{
			if (j->m_visiblilityType == visiblilityType::STATIC_MESH || j->m_visiblilityType == visiblilityType::EMISSIVE)
			{
				if (InnoMath::intersectCheck(j->m_AABB, g_WindowSystemSingletonComponent->m_mouseRay))
				{
					g_RenderingSystemSingletonComponent->m_selectedVisibleComponents.emplace_back(j);
				}
				if (InnoMath::intersectCheck(l_cameraAABB, j->m_AABB))
				{
					g_RenderingSystemSingletonComponent->m_inFrustumVisibleComponents.emplace_back(j);
				}
			}
		}
		);
	}
}


INNO_SYSTEM_EXPORT bool InnoPhysicsSystem::update()
{
	InnoPhysicsSystemNS::m_asyncTask = &g_pCoreSystem->getTaskSystem()->submit([]()
	{
		InnoPhysicsSystemNS::updateCameraComponents();
		InnoPhysicsSystemNS::updateLightComponents();
		InnoPhysicsSystemNS::updateCulling();
	});
	return true;
}

INNO_SYSTEM_EXPORT bool InnoPhysicsSystem::terminate()
{
	InnoPhysicsSystemNS::m_objectStatus = objectStatus::SHUTDOWN;
	g_pCoreSystem->getLogSystem()->printLog("PhysicsSystem has been terminated.");
	return true;
}

INNO_SYSTEM_EXPORT objectStatus InnoPhysicsSystem::getStatus()
{
	return InnoPhysicsSystemNS::m_objectStatus;
}

std::vector<Vertex> InnoPhysicsSystemNS::generateNDC()
{
	Vertex l_VertexData_1;
	l_VertexData_1.m_pos = vec4(1.0f, 1.0f, 1.0f, 1.0f);
	l_VertexData_1.m_texCoord = vec2(1.0f, 1.0f);

	Vertex l_VertexData_2;
	l_VertexData_2.m_pos = vec4(1.0f, -1.0f, 1.0f, 1.0f);
	l_VertexData_2.m_texCoord = vec2(1.0f, 0.0f);

	Vertex l_VertexData_3;
	l_VertexData_3.m_pos = vec4(-1.0f, -1.0f, 1.0f, 1.0f);
	l_VertexData_3.m_texCoord = vec2(0.0f, 0.0f);

	Vertex l_VertexData_4;
	l_VertexData_4.m_pos = vec4(-1.0f, 1.0f, 1.0f, 1.0f);
	l_VertexData_4.m_texCoord = vec2(0.0f, 1.0f);

	Vertex l_VertexData_5;
	l_VertexData_5.m_pos = vec4(1.0f, 1.0f, -1.0f, 1.0f);
	l_VertexData_5.m_texCoord = vec2(1.0f, 1.0f);

	Vertex l_VertexData_6;
	l_VertexData_6.m_pos = vec4(1.0f, -1.0f, -1.0f, 1.0f);
	l_VertexData_6.m_texCoord = vec2(1.0f, 0.0f);

	Vertex l_VertexData_7;
	l_VertexData_7.m_pos = vec4(-1.0f, -1.0f, -1.0f, 1.0f);
	l_VertexData_7.m_texCoord = vec2(0.0f, 0.0f);

	Vertex l_VertexData_8;
	l_VertexData_8.m_pos = vec4(-1.0f, 1.0f, -1.0f, 1.0f);
	l_VertexData_8.m_texCoord = vec2(0.0f, 1.0f);

	std::vector<Vertex> l_vertices = { l_VertexData_1, l_VertexData_2, l_VertexData_3, l_VertexData_4, l_VertexData_5, l_VertexData_6, l_VertexData_7, l_VertexData_8 };

	for (auto& l_vertexData : l_vertices)
	{
		l_vertexData.m_normal = vec4(l_vertexData.m_pos.x, l_vertexData.m_pos.y, l_vertexData.m_pos.z, 0.0f).normalize();
	}

	return l_vertices;
}
