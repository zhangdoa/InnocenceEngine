#include "PhysicsSystem.h"
#include "../HighLevelSystem/AssetSystem.h"
#include "../HighLevelSystem/GameSystem.h"
#include "../LowLevelSystem/LogSystem.h"
#include "../../component/GameSystemSingletonComponent.h"
#include "../../component/RenderingSystemSingletonComponent.h"
#include "../../component/WindowSystemSingletonComponent.h"
#include "../LowLevelSystem/TaskSystem.h"

namespace InnoPhysicsSystem
{
	void setupComponents();
	void setupCameraComponents();
	void generateProjectionMatrix(CameraComponent* cameraComponent);
	void generateRayOfEye(CameraComponent* cameraComponent);
	void generateFrustumVertices(CameraComponent* cameraComponent);
	void setupVisibleComponents();
	void setupLightComponents();
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

	objectStatus m_PhysicsSystemStatus = objectStatus::SHUTDOWN;

	InnoFuture<void>* m_asyncTask;
}

InnoHighLevelSystem_EXPORT bool InnoPhysicsSystem::setup()
{	
	m_PhysicsSystemStatus = objectStatus::ALIVE;
	return true;
}

void InnoPhysicsSystem::setupComponents()
{
	setupCameraComponents();
	setupVisibleComponents();
	setupLightComponents();
}

void InnoPhysicsSystem::setupCameraComponents()
{
	for (auto& i : GameSystemSingletonComponent::getInstance().m_cameraComponents)
	{
		generateProjectionMatrix(i);
		generateRayOfEye(i);
		generateFrustumVertices(i);
		generateAABB(*i);
	}
}

void InnoPhysicsSystem::generateProjectionMatrix(CameraComponent* cameraComponent)
{
	cameraComponent->m_projectionMatrix.initializeToPerspectiveMatrix((cameraComponent->m_FOVX / 180.0) * PI<double>, cameraComponent->m_WHRatio, cameraComponent->m_zNear, cameraComponent->m_zFar);
}

void InnoPhysicsSystem::generateRayOfEye(CameraComponent * cameraComponent)
{
	cameraComponent->m_rayOfEye.m_origin = InnoGameSystem::getTransformComponent(cameraComponent->m_parentEntity)->m_transformVector.caclGlobalPos();
	cameraComponent->m_rayOfEye.m_direction = InnoGameSystem::getTransformComponent(cameraComponent->m_parentEntity)->m_transformVector.getDirection(direction::BACKWARD);
}

void InnoPhysicsSystem::generateFrustumVertices(CameraComponent * cameraComponent)
{
	cameraComponent->m_frustumVertices.clear();
	cameraComponent->m_frustumIndices.clear();

	auto l_NDC = generateNDC();
	auto l_pCamera = cameraComponent->m_projectionMatrix;

	auto l_rCamera = InnoMath::toRotationMatrix(InnoGameSystem::getTransformComponent(cameraComponent->m_parentEntity)->m_transformVector.caclGlobalRot());
	auto l_tCamera = InnoMath::toTranslationMatrix(InnoGameSystem::getTransformComponent(cameraComponent->m_parentEntity)->m_transformVector.caclGlobalPos());

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
		l_mulPos = l_mulPos * (1.0 / l_mulPos.w);
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
		l_vertexData.m_normal = vec4(l_vertexData.m_pos.x, l_vertexData.m_pos.y, l_vertexData.m_pos.z, 0.0).normalize();
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

void InnoPhysicsSystem::setupVisibleComponents()
{
	for (auto i : GameSystemSingletonComponent::getInstance().m_visibleComponents)
	{
		if (i->m_visiblilityType != visiblilityType::INVISIBLE)
		{
			generateAABB(*i);
			if (i->m_visiblilityType == visiblilityType::EMISSIVE)
			{
				RenderingSystemSingletonComponent::getInstance().m_emissiveVisibleComponents.emplace_back(i);
			}
			else if (i->m_visiblilityType == visiblilityType::STATIC_MESH)
			{
				RenderingSystemSingletonComponent::getInstance().m_staticMeshVisibleComponents.emplace_back(i);
			}

		}
	}
}

void InnoPhysicsSystem::setupLightComponents()
{
	for (auto& i : GameSystemSingletonComponent::getInstance().m_lightComponents)
	{
		i->m_direction = vec4(0.0, 0.0, 1.0, 0.0);
		i->m_constantFactor = 1.0;
		i->m_linearFactor = 0.14;
		i->m_quadraticFactor = 0.07;
		setupLightComponentRadius(i);
		i->m_color = vec4(1.0, 1.0, 1.0, 1.0);

		if (i->m_lightType == lightType::DIRECTIONAL)
		{
			generateAABB(*i);
		}
	}
}

void InnoPhysicsSystem::setupLightComponentRadius(LightComponent * lightComponent)
{
	double l_lightMaxIntensity = std::fmax(std::fmax(lightComponent->m_color.x, lightComponent->m_color.y), lightComponent->m_color.z);
	lightComponent->m_radius = -lightComponent->m_linearFactor + std::sqrt(lightComponent->m_linearFactor * lightComponent->m_linearFactor - 4.0 * lightComponent->m_quadraticFactor * (lightComponent->m_constantFactor - (256.0 / 5.0) * l_lightMaxIntensity)) / (2.0 * lightComponent->m_quadraticFactor);
}

void InnoPhysicsSystem::generateAABB(VisibleComponent & visibleComponent)
{
	double maxX = 0;
	double maxY = 0;
	double maxZ = 0;
	double minX = 0;
	double minY = 0;
	double minZ = 0;

	std::vector<vec4> l_cornerVertices(visibleComponent.m_modelMap.size() * 2);

	for (auto& l_graphicData : visibleComponent.m_modelMap)
	{
		// get corner vertices from sub meshes
		l_cornerVertices.emplace_back(InnoAssetSystem::findMaxVertex(l_graphicData.first));
		l_cornerVertices.emplace_back(InnoAssetSystem::findMinVertex(l_graphicData.first));
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

	visibleComponent.m_AABB = generateAABB(vec4(maxX, maxY, maxZ, 1.0), vec4(minX, minY, minZ, 1.0));

	auto l_worldTm = InnoGameSystem::getTransformComponent(visibleComponent.m_parentEntity)->m_transformVector.caclGlobalTransformationMatrix();

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

void InnoPhysicsSystem::generateAABB(LightComponent & lightComponent)
{
	lightComponent.m_AABBs.clear();
	lightComponent.m_shadowSplitPoints.clear();
	lightComponent.m_projectionMatrices.clear();

	//1.translate the big frustum to light space
	auto l_camera = GameSystemSingletonComponent::getInstance().m_cameraComponents[0];
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
		auto scaleFactor = std::exp(((double)i + 1.0) * 2.0 / E<double>) / std::exp(8.0 / E<double>);
		lightComponent.m_shadowSplitPoints.emplace_back(scaleFactor);
		for (size_t j = 0; j < 4; j++)
		{
			auto l_splitedPlaneCornerPos = l_frustumVertices[j].m_pos + (l_frustumVertices[j + 4].m_pos - l_frustumVertices[j].m_pos) * scaleFactor;
			l_frustumsCornerPos.emplace_back(l_splitedPlaneCornerPos);
		}
	}

	//2.3 transform to light space
	auto l_lightRotMat = InnoGameSystem::getTransformComponent(lightComponent.m_parentEntity)->m_transformVector.caclGlobalTransformationMatrix().inverse();
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

void InnoPhysicsSystem::generateAABB(CameraComponent & cameraComponent)
{
	auto l_frustumCorners = cameraComponent.m_frustumVertices;
	cameraComponent.m_AABB = generateAABB(l_frustumCorners);
}

AABB InnoPhysicsSystem::generateAABB(const std::vector<Vertex>& vertices)
{
	double maxX = vertices[0].m_pos.x;
	double maxY = vertices[0].m_pos.y;
	double maxZ = vertices[0].m_pos.z;
	double minX = vertices[0].m_pos.x;
	double minY = vertices[0].m_pos.y;
	double minZ = vertices[0].m_pos.z;

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

	return generateAABB(vec4(maxX, maxY, maxZ, 1.0), vec4(minX, minY, minZ, 1.0));
}

AABB InnoPhysicsSystem::generateAABB(const vec4 & boundMax, const vec4 & boundMin)
{
	AABB l_AABB;

	l_AABB.m_boundMin = boundMin;
	l_AABB.m_boundMax = boundMax;

	l_AABB.m_center = (l_AABB.m_boundMax + l_AABB.m_boundMin) * 0.5;
	l_AABB.m_sphereRadius = std::max<double>(std::max<double>((l_AABB.m_boundMax.x - l_AABB.m_boundMin.x) / 2.0, (l_AABB.m_boundMax.y - l_AABB.m_boundMin.y) / 2.0), (l_AABB.m_boundMax.z - l_AABB.m_boundMin.z) / 2.0);

	Vertex l_VertexData_1;
	l_VertexData_1.m_pos = (vec4(boundMax.x, boundMax.y, boundMax.z, 1.0));
	l_VertexData_1.m_texCoord = vec2(1.0, 1.0);

	Vertex l_VertexData_2;
	l_VertexData_2.m_pos = (vec4(boundMax.x, boundMin.y, boundMax.z, 1.0));
	l_VertexData_2.m_texCoord = vec2(1.0, 0.0);

	Vertex l_VertexData_3;
	l_VertexData_3.m_pos = (vec4(boundMin.x, boundMin.y, boundMax.z, 1.0));
	l_VertexData_3.m_texCoord = vec2(0.0, 0.0);

	Vertex l_VertexData_4;
	l_VertexData_4.m_pos = (vec4(boundMin.x, boundMax.y, boundMax.z, 1.0));
	l_VertexData_4.m_texCoord = vec2(0.0, 1.0);

	Vertex l_VertexData_5;
	l_VertexData_5.m_pos = (vec4(boundMax.x, boundMax.y, boundMin.z, 1.0));
	l_VertexData_5.m_texCoord = vec2(1.0, 1.0);

	Vertex l_VertexData_6;
	l_VertexData_6.m_pos = (vec4(boundMax.x, boundMin.y, boundMin.z, 1.0));
	l_VertexData_6.m_texCoord = vec2(1.0, 0.0);

	Vertex l_VertexData_7;
	l_VertexData_7.m_pos = (vec4(boundMin.x, boundMin.y, boundMin.z, 1.0));
	l_VertexData_7.m_texCoord = vec2(0.0, 0.0);

	Vertex l_VertexData_8;
	l_VertexData_8.m_pos = (vec4(boundMin.x, boundMax.y, boundMin.z, 1.0));
	l_VertexData_8.m_texCoord = vec2(0.0, 1.0);


	l_AABB.m_vertices = { l_VertexData_1, l_VertexData_2, l_VertexData_3, l_VertexData_4, l_VertexData_5, l_VertexData_6, l_VertexData_7, l_VertexData_8 };

	for (auto& l_vertexData : l_AABB.m_vertices)
	{
		l_vertexData.m_normal = vec4(l_vertexData.m_pos.x, l_vertexData.m_pos.y, l_vertexData.m_pos.z, 0.0).normalize();
	}

	l_AABB.m_indices = { 0, 1, 3, 1, 2, 3,
		4, 5, 0, 5, 1, 0,
		7, 6, 4, 6, 5, 4,
		3, 2, 7, 2, 6 ,7,
		4, 0, 7, 0, 3, 7,
		1, 5, 2, 5, 6, 2 };

	return l_AABB;
}

InnoHighLevelSystem_EXPORT bool InnoPhysicsSystem::initialize()
{
	setupComponents();
	m_PhysicsSystemStatus = objectStatus::ALIVE;
	InnoLogSystem::printLog("PhysicsSystem has been initialized.");
	return true;
}

void InnoPhysicsSystem::updateCameraComponents()
{
	if (GameSystemSingletonComponent::getInstance().m_cameraComponents.size() > 0)
	{
		for (auto& i : GameSystemSingletonComponent::getInstance().m_cameraComponents)
		{
			generateRayOfEye(i);
			generateFrustumVertices(i);
			generateAABB(*i);
		}
	}
}

void InnoPhysicsSystem::updateLightComponents()
{
	if (GameSystemSingletonComponent::getInstance().m_lightComponents.size() > 0)
	{
		// generate AABB for CSM
		for (auto& i : GameSystemSingletonComponent::getInstance().m_lightComponents)
		{
			setupLightComponentRadius(i);
			if (i->m_lightType == lightType::DIRECTIONAL)
			{
				generateAABB(*i);
			}
		}
	}
}

void InnoPhysicsSystem::updateCulling()
{
	RenderingSystemSingletonComponent::getInstance().m_selectedVisibleComponents.clear();
	RenderingSystemSingletonComponent::getInstance().m_inFrustumVisibleComponents.clear();

	if (GameSystemSingletonComponent::getInstance().m_cameraComponents.size() > 0)
	{
		WindowSystemSingletonComponent::getInstance().m_mouseRay.m_origin = InnoGameSystem::getTransformComponent(GameSystemSingletonComponent::getInstance().m_cameraComponents[0]->m_parentEntity)->m_transformVector.caclGlobalPos();
		WindowSystemSingletonComponent::getInstance().m_mouseRay.m_direction = WindowSystemSingletonComponent::getInstance().m_mousePositionInWorldSpace;

		auto l_cameraAABB = GameSystemSingletonComponent::getInstance().m_cameraComponents[0]->m_AABB;

		auto l_ray = GameSystemSingletonComponent::getInstance().m_cameraComponents[0]->m_rayOfEye;

		for (auto& j : GameSystemSingletonComponent::getInstance().m_visibleComponents)
		{
			if (j->m_visiblilityType == visiblilityType::STATIC_MESH || j->m_visiblilityType == visiblilityType::EMISSIVE)
			{
				if (InnoMath::intersectCheck(j->m_AABB, WindowSystemSingletonComponent::getInstance().m_mouseRay))
				{
					RenderingSystemSingletonComponent::getInstance().m_selectedVisibleComponents.emplace_back(j);
				}
				if (InnoMath::intersectCheck(l_cameraAABB, j->m_AABB))
				{
					RenderingSystemSingletonComponent::getInstance().m_inFrustumVisibleComponents.emplace_back(j);
				}
			}
		}
	}
}


InnoHighLevelSystem_EXPORT bool InnoPhysicsSystem::update()
{
	m_asyncTask = &InnoTaskSystem::submit([]()
	{
		updateCameraComponents();
		updateLightComponents();
		updateCulling();
	});
	return true;
}

InnoHighLevelSystem_EXPORT bool InnoPhysicsSystem::terminate()
{
	m_PhysicsSystemStatus = objectStatus::SHUTDOWN;
	InnoLogSystem::printLog("PhysicsSystem has been terminated.");
	return true;
}

InnoHighLevelSystem_EXPORT objectStatus InnoPhysicsSystem::getStatus()
{
	return m_PhysicsSystemStatus;
}

std::vector<Vertex> InnoPhysicsSystem::generateNDC()
{
	Vertex l_VertexData_1;
	l_VertexData_1.m_pos = vec4(1.0, 1.0, 1.0, 1.0);
	l_VertexData_1.m_texCoord = vec2(1.0, 1.0);

	Vertex l_VertexData_2;
	l_VertexData_2.m_pos = vec4(1.0, -1.0, 1.0, 1.0);
	l_VertexData_2.m_texCoord = vec2(1.0, 0.0);

	Vertex l_VertexData_3;
	l_VertexData_3.m_pos = vec4(-1.0, -1.0, 1.0, 1.0);
	l_VertexData_3.m_texCoord = vec2(0.0, 0.0);

	Vertex l_VertexData_4;
	l_VertexData_4.m_pos = vec4(-1.0, 1.0, 1.0, 1.0);
	l_VertexData_4.m_texCoord = vec2(0.0, 1.0);

	Vertex l_VertexData_5;
	l_VertexData_5.m_pos = vec4(1.0, 1.0, -1.0, 1.0);
	l_VertexData_5.m_texCoord = vec2(1.0, 1.0);

	Vertex l_VertexData_6;
	l_VertexData_6.m_pos = vec4(1.0, -1.0, -1.0, 1.0);
	l_VertexData_6.m_texCoord = vec2(1.0, 0.0);

	Vertex l_VertexData_7;
	l_VertexData_7.m_pos = vec4(-1.0, -1.0, -1.0, 1.0);
	l_VertexData_7.m_texCoord = vec2(0.0, 0.0);

	Vertex l_VertexData_8;
	l_VertexData_8.m_pos = vec4(-1.0, 1.0, -1.0, 1.0);
	l_VertexData_8.m_texCoord = vec2(0.0, 1.0);

	std::vector<Vertex> l_vertices = { l_VertexData_1, l_VertexData_2, l_VertexData_3, l_VertexData_4, l_VertexData_5, l_VertexData_6, l_VertexData_7, l_VertexData_8 };

	for (auto& l_vertexData : l_vertices)
	{
		l_vertexData.m_normal = vec4(l_vertexData.m_pos.x, l_vertexData.m_pos.y, l_vertexData.m_pos.z, 0.0).normalize();
	}

	return l_vertices;
}
