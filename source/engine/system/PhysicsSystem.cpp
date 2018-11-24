#include "PhysicsSystem.h"

#include "../component/GameSystemSingletonComponent.h"
#include "../component/WindowSystemSingletonComponent.h"
#include "../component/AssetSystemSingletonComponent.h"
#include "../component/PhysicsSystemSingletonComponent.h"

#include "ICoreSystem.h"

extern ICoreSystem* g_pCoreSystem;

namespace InnoPhysicsSystemNS
{
	void generateProjectionMatrix(CameraComponent* cameraComponent);
	void generateRayOfEye(CameraComponent* cameraComponent);
	std::vector<Vertex> generateFrustumVertices(CameraComponent* cameraComponent);
	void generateLightComponentRadius(LightComponent* lightComponent);

	std::vector<Vertex> generateNDC();
	PhysicsDataComponent* generatePhysicsDataComponent(const modelMap& modelMap);
	MeshDataComponent* generateMeshDataComponent(AABB rhs);

	void generateAABB(LightComponent* lightComponent);
	AABB generateAABB(const std::vector<Vertex>& vertices);
	AABB generateAABB(vec4 boundMax, vec4 boundMin);
	std::vector<Vertex> generateAABBVertices(vec4 boundMax, vec4 boundMin);
	std::vector<Vertex> generateAABBVertices(AABB rhs);

	void updateCameraComponents();
	void updateLightComponents();
	void updateVisibleComponents();
	void updateCulling();
	AABB transformAABBtoWorldSpace(AABB rhs, mat4 globalTm);
	void updateSceneAABB(AABB rhs);

	objectStatus m_objectStatus = objectStatus::SHUTDOWN;

	InnoFuture<void>* m_asyncTask;

	static WindowSystemSingletonComponent* g_WindowSystemSingletonComponent;
	static GameSystemSingletonComponent* g_GameSystemSingletonComponent;
	static AssetSystemSingletonComponent* g_AssetSystemSingletonComponent;
	static PhysicsSystemSingletonComponent* g_PhysicsSystemSingletonComponent;
	vec4 m_sceneBoundMax = vec4(std::numeric_limits<float>::min(), std::numeric_limits<float>::min(), std::numeric_limits<float>::min(), 1.0f);
	vec4 m_sceneBoundMin = vec4(std::numeric_limits<float>::max(), std::numeric_limits<float>::max(), std::numeric_limits<float>::max(), 1.0f);

	std::vector<CameraComponent*> m_initializedCameraComponents;
	std::vector<LightComponent*> m_initializedLightComponents;
	std::vector<VisibleComponent*> m_initializedVisibleComponents;
}

INNO_SYSTEM_EXPORT bool InnoPhysicsSystem::setup()
{	
	InnoPhysicsSystemNS::g_WindowSystemSingletonComponent = &WindowSystemSingletonComponent::getInstance();
	InnoPhysicsSystemNS::g_GameSystemSingletonComponent = &GameSystemSingletonComponent::getInstance();
	InnoPhysicsSystemNS::g_AssetSystemSingletonComponent = &AssetSystemSingletonComponent::getInstance();
	InnoPhysicsSystemNS::g_PhysicsSystemSingletonComponent = &PhysicsSystemSingletonComponent::getInstance();

	InnoPhysicsSystemNS::m_objectStatus = objectStatus::ALIVE;
	return true;
}

void InnoPhysicsSystemNS::generateProjectionMatrix(CameraComponent * cameraComponent)
{
	cameraComponent->m_projectionMatrix = InnoMath::generatePerspectiveMatrix((cameraComponent->m_FOVX / 180.0f) * PI<float>, cameraComponent->m_WHRatio, cameraComponent->m_zNear, cameraComponent->m_zFar);
}

void InnoPhysicsSystemNS::generateRayOfEye(CameraComponent * cameraComponent)
{
	cameraComponent->m_rayOfEye.m_origin = g_pCoreSystem->getGameSystem()->get<TransformComponent>(cameraComponent->m_parentEntity)->m_globalTransformVector.m_pos;
	cameraComponent->m_rayOfEye.m_direction = InnoMath::getDirection(
		direction::BACKWARD,
		g_pCoreSystem->getGameSystem()->get<TransformComponent>(cameraComponent->m_parentEntity)->m_localTransformVector.m_rot
		);
}

std::vector<Vertex> InnoPhysicsSystemNS::generateFrustumVertices(CameraComponent * cameraComponent)
{
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
	std::reverse(l_NDC.begin(), l_NDC.end());

	return std::move(l_NDC);
}

void InnoPhysicsSystemNS::generateLightComponentRadius(LightComponent * lightComponent)
{
	auto l_RGBColor = lightComponent->m_color.normalize();
	// "Real-Time Rendering", 4th Edition, p.278
	// https://en.wikipedia.org/wiki/Relative_luminance
	// weight with respect to CIE photometric curve
	auto l_reletiveLuminanceRatio = (0.2126f * l_RGBColor.x + 0.7152f * l_RGBColor.y + 0.0722f * l_RGBColor.z);

	// Luminance (nt) is illuminance (lx) per solid angle, while luminous intensity (cd) is luminous flux (lm) per solid angle, thus for one area unit (m^2), the ratio of nt/lx is same as cd/lm
	// For omni isotropic light, after the intergration per solid angle, the luminous flux (lm) is 4 pi times the luminous intensity (cd)
	auto l_weightedLuminousFlux = lightComponent->m_luminousFlux * l_reletiveLuminanceRatio;

	// 1. get luminous efficacy (lm/w), assume 683 lm/w (100% luminous efficiency) always
	// 2. luminous flux (lm) to radiant flux (w), omitted because linearity assumption in step 1
	// 3. apply inverse square attenuation law with a low threshold of eye sensitivity at 0.03 lx, in ideal situation, lx could convert back to lm with respect to a sphere surface area 4 * PI * r^2
	lightComponent->m_radius = std::sqrtf(l_weightedLuminousFlux / (4.0f * PI<float> * 0.03f));
}

PhysicsDataComponent* InnoPhysicsSystemNS::generatePhysicsDataComponent(const modelMap& modelMap)
{
	auto l_PDC = g_pCoreSystem->getMemorySystem()->spawn<PhysicsDataComponent>();

	for (auto& l_MDC : modelMap)
	{
		physicsData l_physicsData;

		auto l_AABB = generateAABB(l_MDC.first->m_vertices);
		auto l_MDCforAABB = generateMeshDataComponent(l_AABB);

		l_physicsData.MDC = l_MDC.first;
		l_physicsData.wireframeMDC = l_MDCforAABB;
		l_physicsData.AABB = l_AABB;

		l_PDC->m_physicsDatas.emplace_back(l_physicsData);
	}

	return l_PDC;
}

void InnoPhysicsSystemNS::generateAABB(LightComponent * lightComponent)
{
	lightComponent->m_AABBs.clear();
	lightComponent->m_projectionMatrices.clear();

	//1. get frustum vertices
	auto l_camera = g_GameSystemSingletonComponent->m_CameraComponents[0];
	auto l_frustumVertices = generateFrustumVertices(l_camera);

	//2.calculate splited planes' corners
	std::vector<vec4> l_frustumsCornerPos;
	l_frustumsCornerPos.reserve(20);

	//2.1 first 4 corner
	for (size_t i = 0; i < 4; i++)
	{
		l_frustumsCornerPos.emplace_back(l_frustumVertices[i].m_pos);
	}

	auto l_splitFactor = 1.3f;
	//2.2 other 16 corner based on e^(2 * (i + 1)) / e^8
	for (size_t i = 0; i < 4; i++)
	{
		auto scaleFactor = std::exp(((float)i + 1.0f) * l_splitFactor) / std::exp(4.0f * l_splitFactor);
		for (size_t j = 0; j < 4; j++)
		{
			auto l_splitedPlaneCornerPos = l_frustumVertices[j].m_pos + (l_frustumVertices[j + 4].m_pos - l_frustumVertices[j].m_pos) * scaleFactor;
			l_frustumsCornerPos.emplace_back(l_splitedPlaneCornerPos);
		}
	}

	//2.3 transform to light space
	auto l_lightRotMat = g_pCoreSystem->getGameSystem()->get<TransformComponent>(lightComponent->m_parentEntity)->m_globalTransformMatrix.m_rotationMat.inverse();
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

	//2.4 assemble splited frustum corners
	std::vector<Vertex> l_frustumsCornerVertices;
	l_frustumsCornerVertices.reserve(32);
	for (size_t i = 0; i < 4; i++)
	{
		for (size_t j = 0; j < 8; j++)
		{
			l_frustumsCornerVertices.emplace_back();
			l_frustumsCornerVertices[i * 8 + j].m_pos = l_frustumsCornerPos[i * 4 + j];
		}
	}

	//2.5 assemble splitted frustums
	std::vector<std::vector<Vertex>> l_splitedFrustums;
	l_splitedFrustums.reserve(4);

	for (size_t i = 0; i < 4; i++)
	{
		l_splitedFrustums.emplace_back(std::vector<Vertex>(l_frustumsCornerVertices.begin() + i * 8, l_frustumsCornerVertices.begin() + 8 + i * 8));
	}

	//3 generate AABBs for the splited frustums
	std::vector<AABB> l_AABBs;
	l_AABBs.reserve(4);
	for (size_t i = 0; i < 4; i++)
	{
		l_AABBs.emplace_back(generateAABB(l_splitedFrustums[i]));
	}

	//3.1 extend AABB to include the sphere
	for (size_t i = 0; i < 4; i++)
	{
		auto l_maxExtendFactor = (l_AABBs[i].m_boundMax - l_AABBs[i].m_center) * l_AABBs[i].m_sphereRadius;
		auto l_minExtendFactor = (l_AABBs[i].m_boundMin - l_AABBs[i].m_center) * l_AABBs[i].m_sphereRadius;
		l_AABBs[i] = generateAABB(l_AABBs[i].m_boundMax + l_maxExtendFactor, l_AABBs[i].m_boundMin + l_minExtendFactor);
	}

	lightComponent->m_AABBs = std::move(l_AABBs);

	//4. generate projection matrices
	lightComponent->m_projectionMatrices.reserve(4);

	for (size_t i = 0; i < 4; i++)
	{
		vec4 l_maxExtents = lightComponent->m_AABBs[i].m_boundMax;
		vec4 l_minExtents = lightComponent->m_AABBs[i].m_boundMin;

		mat4 p = InnoMath::generateToOrthographicMatrix(l_minExtents.x, l_maxExtents.x, l_minExtents.y, l_maxExtents.y, l_minExtents.z, l_maxExtents.z);
		lightComponent->m_projectionMatrices.emplace_back(p);
	}
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

AABB InnoPhysicsSystemNS::generateAABB(vec4 boundMax, vec4 boundMin)
{
	AABB l_AABB;

	l_AABB.m_boundMin = boundMin;
	l_AABB.m_boundMax = boundMax;

	l_AABB.m_center = (l_AABB.m_boundMax + l_AABB.m_boundMin) * 0.5f;
	l_AABB.m_sphereRadius = std::max<float>(std::max<float>((l_AABB.m_boundMax.x - l_AABB.m_boundMin.x) / 2.0f, (l_AABB.m_boundMax.y - l_AABB.m_boundMin.y) / 2.0f), (l_AABB.m_boundMax.z - l_AABB.m_boundMin.z) / 2.0f);

	return l_AABB;
}

std::vector<Vertex> InnoPhysicsSystemNS::generateAABBVertices(vec4 boundMax, vec4 boundMin)
{
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

	std::vector<Vertex> l_vertices;

	l_vertices.reserve(8);

	l_vertices = { l_VertexData_1, l_VertexData_2, l_VertexData_3, l_VertexData_4, l_VertexData_5, l_VertexData_6, l_VertexData_7, l_VertexData_8 };

	for (auto& l_vertexData : l_vertices)
	{
		l_vertexData.m_normal = vec4(l_vertexData.m_pos.x, l_vertexData.m_pos.y, l_vertexData.m_pos.z, 0.0f).normalize();
	}

	return std::move(l_vertices);
}

std::vector<Vertex> InnoPhysicsSystemNS::generateAABBVertices(AABB rhs)
{
	auto boundMax = rhs.m_boundMax;
	auto boundMin = rhs.m_boundMin;

	return std::move(generateAABBVertices(boundMax, boundMin));
}

MeshDataComponent* InnoPhysicsSystemNS::generateMeshDataComponent(AABB rhs)
{
	auto l_MDC = g_pCoreSystem->getMemorySystem()->spawn<MeshDataComponent>();
	l_MDC->m_parentEntity = InnoMath::createEntityID();

	l_MDC->m_vertices = generateAABBVertices(rhs);
	
	l_MDC->m_indices.reserve(36);

	l_MDC->m_indices = { 0, 1, 3, 1, 2, 3,
		4, 5, 0, 5, 1, 0,
		7, 6, 4, 6, 5, 4,
		3, 2, 7, 2, 6 ,7,
		4, 0, 7, 0, 3, 7,
		1, 5, 2, 5, 6, 2 };

	l_MDC->m_indicesSize = l_MDC->m_indices.size();

	l_MDC->m_objectStatus = objectStatus::STANDBY;
	g_AssetSystemSingletonComponent->m_uninitializedMeshComponents.push(l_MDC);

	return l_MDC;
}

AABB InnoPhysicsSystemNS::transformAABBtoWorldSpace(AABB rhs, mat4 globalTm)
{
	AABB l_AABB;

	//Column-Major memory layout
#ifdef USE_COLUMN_MAJOR_MEMORY_LAYOUT
	l_AABB.m_boundMax = InnoMath::mul(rhs.m_boundMax, globalTm);
	l_AABB.m_boundMin = InnoMath::mul(rhs.m_boundMin, globalTm);
	l_AABB.m_center = InnoMath::mul(rhs.m_center, globalTm);
#endif
	//Row-Major memory layout
#ifdef USE_ROW_MAJOR_MEMORY_LAYOUT
	l_AABB.m_boundMax = InnoMath::mul(globalTm, rhs.m_boundMax);
	l_AABB.m_boundMin = InnoMath::mul(globalTm, rhs.m_boundMin);
	l_AABB.m_center = InnoMath::mul(globalTm, rhs.m_center);
#endif
	l_AABB.m_sphereRadius = std::max<float>(std::max<float>((l_AABB.m_boundMax.x - l_AABB.m_boundMin.x) / 2.0f, (l_AABB.m_boundMax.y - l_AABB.m_boundMin.y) / 2.0f), (l_AABB.m_boundMax.z - l_AABB.m_boundMin.z) / 2.0f);

	return l_AABB;
}

INNO_SYSTEM_EXPORT bool InnoPhysicsSystem::initialize()
{
	InnoPhysicsSystemNS::m_objectStatus = objectStatus::ALIVE;
	g_pCoreSystem->getLogSystem()->printLog(logType::INNO_DEV_SUCCESS, "PhysicsSystem has been initialized.");
	return true;
}

void InnoPhysicsSystemNS::updateCameraComponents()
{
	for (auto& i : g_GameSystemSingletonComponent->m_CameraComponents)
	{
		generateProjectionMatrix(i);
		generateRayOfEye(i);
	}
}

void InnoPhysicsSystemNS::updateLightComponents()
{
	for (auto& i : g_GameSystemSingletonComponent->m_LightComponents)
	{
		if (i->m_lightType == lightType::POINT)
		{
			generateLightComponentRadius(i);
		}
		if (i->m_lightType == lightType::DIRECTIONAL)
		{
			generateAABB(i);
		}
	}
}

void InnoPhysicsSystemNS::updateVisibleComponents()
{
	if (InnoPhysicsSystemNS::g_PhysicsSystemSingletonComponent->m_uninitializedVisibleComponents.size() > 0)
	{
		VisibleComponent* l_visibleComponent;
		if (InnoPhysicsSystemNS::g_PhysicsSystemSingletonComponent->m_uninitializedVisibleComponents.tryPop(l_visibleComponent))
		{
			auto l_physicsComponent = generatePhysicsDataComponent(l_visibleComponent->m_modelMap);
			l_visibleComponent->m_PhysicsDataComponent = l_physicsComponent;
			InnoPhysicsSystemNS::m_initializedVisibleComponents.emplace_back(l_visibleComponent);
		}
	}
}

void InnoPhysicsSystemNS::updateSceneAABB(AABB rhs)
{
	auto boundMax = rhs.m_boundMax;
	auto boundMin = rhs.m_boundMin;

	if (boundMax.x > InnoPhysicsSystemNS::m_sceneBoundMax.x 
		&&boundMax.y > InnoPhysicsSystemNS::m_sceneBoundMax.y
		&&boundMax.z > InnoPhysicsSystemNS::m_sceneBoundMax.z)
	{
		InnoPhysicsSystemNS::m_sceneBoundMax = boundMax;
	}
	if (boundMax.x < InnoPhysicsSystemNS::m_sceneBoundMax.x
		&&boundMax.y < InnoPhysicsSystemNS::m_sceneBoundMax.y
		&&boundMax.z < InnoPhysicsSystemNS::m_sceneBoundMax.z)
	{
		InnoPhysicsSystemNS::m_sceneBoundMin = boundMin;
	}
}

void InnoPhysicsSystemNS::updateCulling()
{
	g_PhysicsSystemSingletonComponent->m_cullingDataPack.clear();
	g_PhysicsSystemSingletonComponent->m_AABBWireframeDataPack.clear();
	
	if (g_GameSystemSingletonComponent->m_CameraComponents.size() > 0)
	{
		Ray l_mouseRay;
		l_mouseRay.m_origin = g_pCoreSystem->getGameSystem()->get<TransformComponent>(g_GameSystemSingletonComponent->m_CameraComponents[0]->m_parentEntity)->m_globalTransformVector.m_pos;
		l_mouseRay.m_direction = g_WindowSystemSingletonComponent->m_mousePositionInWorldSpace;

		//auto l_cameraAABB = g_GameSystemSingletonComponent->m_CameraComponents[0]->m_AABB;
		auto l_eyeRay = g_GameSystemSingletonComponent->m_CameraComponents[0]->m_rayOfEye;
		
		for (auto visibleComponent : m_initializedVisibleComponents)
		{
			if (visibleComponent->m_visiblilityType != visiblilityType::INVISIBLE)
			{
				auto l_transformComponent = g_pCoreSystem->getGameSystem()->get<TransformComponent>(visibleComponent->m_parentEntity);
				auto l_globalTm = l_transformComponent->m_globalTransformMatrix.m_transformationMat;

				if (visibleComponent->m_PhysicsDataComponent)
				{
					for (auto& physicsData : visibleComponent->m_PhysicsDataComponent->m_physicsDatas)
					{
						auto l_AABBws = transformAABBtoWorldSpace(physicsData.AABB, l_globalTm);

						if (InnoMath::intersectCheck(l_AABBws, l_mouseRay))
						{
							AABBWireframeDataPack l_AABBWireframeDataPack;
							l_AABBWireframeDataPack.m = l_globalTm;
							l_AABBWireframeDataPack.MDC = physicsData.wireframeMDC;
							g_PhysicsSystemSingletonComponent->m_AABBWireframeDataPack.emplace_back(l_AABBWireframeDataPack);
						}
						//if (InnoMath::intersectCheck(l_AABBws, l_cameraAABB))
						//{
							cullingDataPack l_cullingDataPack;
							l_cullingDataPack.m = l_globalTm;
							l_cullingDataPack.m_prev = l_transformComponent->m_globalTransformMatrix_prev.m_transformationMat;
							l_cullingDataPack.normalMat = l_transformComponent->m_globalTransformMatrix.m_rotationMat;
							l_cullingDataPack.visibleComponentEntityID = visibleComponent->m_parentEntity;
							l_cullingDataPack.MDCEntityID = physicsData.MDC->m_parentEntity;
							l_cullingDataPack.visiblilityType = visibleComponent->m_visiblilityType;
							g_PhysicsSystemSingletonComponent->m_cullingDataPack.emplace_back(l_cullingDataPack);
						//}

						updateSceneAABB(l_AABBws);
					}
				}
			}
		}
	}
}

INNO_SYSTEM_EXPORT bool InnoPhysicsSystem::update()
{
	InnoPhysicsSystemNS::m_asyncTask = &g_pCoreSystem->getTaskSystem()->submit([]()
	{
		InnoPhysicsSystemNS::updateCameraComponents();
		InnoPhysicsSystemNS::updateLightComponents();
		InnoPhysicsSystemNS::updateVisibleComponents();
		InnoPhysicsSystemNS::updateCulling();
	});
	return true;
}

INNO_SYSTEM_EXPORT bool InnoPhysicsSystem::terminate()
{
	InnoPhysicsSystemNS::m_objectStatus = objectStatus::SHUTDOWN;
	g_pCoreSystem->getLogSystem()->printLog(logType::INNO_DEV_SUCCESS, "PhysicsSystem has been terminated.");
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
