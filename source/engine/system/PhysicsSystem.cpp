#include "PhysicsSystem.h"

//#include "PhysXWrapper.h"

#include "ICoreSystem.h"

extern ICoreSystem* g_pCoreSystem;

namespace InnoPhysicsSystemNS
{
	bool setup();
	bool update();

	void generateProjectionMatrix(CameraComponent* cameraComponent);
	void generateRayOfEye(CameraComponent* cameraComponent);
	std::vector<Vertex> generateFrustumVertices(CameraComponent* cameraComponent);
	void generateFrustum(CameraComponent* cameraComponent);
	void generatePointLightComponentAttenuationRadius(PointLightComponent* pointLightComponent);
	void generateSphereLightComponentScale(SphereLightComponent* sphereLightComponent);

	void generateAABB(DirectionalLightComponent* directionalLightComponent);
	std::vector<AABB> frustumsVerticesToAABBs(const std::vector<Vertex>& frustumsVertices, const std::vector<float>& splitFactors);

	AABB generateAABB(const std::vector<Vertex>& vertices);
	AABB generateAABB(vec4 boundMax, vec4 boundMin);
	Sphere generateBoundSphere(AABB rhs);

	std::vector<Vertex> generateAABBVertices(vec4 boundMax, vec4 boundMin);
	std::vector<Vertex> generateAABBVertices(AABB rhs);

	MeshDataComponent* generateMeshDataComponent(AABB rhs);
	MeshDataComponent* generateMeshDataComponent(Frustum rhs);

	PhysicsDataComponent* generatePhysicsDataComponent(const ModelMap& modelMap, const EntityID& entityID);

	void updateCameraComponents();
	void updateLightComponents();
	void updateVisibleComponents();
	void updateCulling();
	AABB transformAABBtoWorldSpace(AABB rhs, mat4 globalTm);
	void updateSceneAABB(AABB rhs);

	ObjectStatus m_objectStatus = ObjectStatus::SHUTDOWN;
	EntityID m_entityID;

	std::vector<InnoFuture<void>> m_asyncTask;

	vec4 m_sceneBoundMax = vec4(std::numeric_limits<float>::min(), std::numeric_limits<float>::min(), std::numeric_limits<float>::min(), 1.0f);
	vec4 m_sceneBoundMin = vec4(std::numeric_limits<float>::max(), std::numeric_limits<float>::max(), std::numeric_limits<float>::max(), 1.0f);
	AABB m_SceneAABB;

	InputComponent* m_inputComponent;
	std::function<void()> f_mouseSelect;

	std::atomic<bool> m_isCullingDataPackValid = false;
	std::vector<CullingDataPack> m_cullingDataPack;

	VisibleComponent* m_selectedVisibleComponent;
}

bool InnoPhysicsSystemNS::setup()
{
	m_entityID = g_pCoreSystem->getGameSystem()->createEntity("PhysicsSystemPicker");

	m_inputComponent = g_pCoreSystem->getGameSystem()->spawn<InputComponent>(m_entityID);

	f_mouseSelect = [&]() {
		m_selectedVisibleComponent = nullptr;

		if (g_pCoreSystem->getGameSystem()->get<CameraComponent>().size() > 0)
		{
			auto l_cameraComponents = g_pCoreSystem->getGameSystem()->get<CameraComponent>();
			auto l_mainCamera = l_cameraComponents[0];
			auto l_mainCameraTransformComponent = g_pCoreSystem->getGameSystem()->get<TransformComponent>(l_mainCamera->m_parentEntity);
			Ray l_mouseRay;
			l_mouseRay.m_origin = l_mainCameraTransformComponent->m_globalTransformVector.m_pos;
			l_mouseRay.m_direction = g_pCoreSystem->getInputSystem()->calcMousePositionInWorldSpace();

			for (auto visibleComponent : g_pCoreSystem->getGameSystem()->get<VisibleComponent>())
			{
				if (visibleComponent->m_visiblilityType != VisiblilityType::INNO_INVISIBLE)
				{
					auto l_transformComponent = g_pCoreSystem->getGameSystem()->get<TransformComponent>(visibleComponent->m_parentEntity);
					auto l_globalTm = l_transformComponent->m_globalTransformMatrix.m_transformationMat;

					if (visibleComponent->m_PhysicsDataComponent)
					{
						for (auto& physicsData : visibleComponent->m_PhysicsDataComponent->m_physicsDatas)
						{
							auto l_AABBws = transformAABBtoWorldSpace(physicsData.aabb, l_globalTm);

							if (InnoMath::intersectCheck(l_AABBws, l_mouseRay))
							{
								m_selectedVisibleComponent = visibleComponent;
								break;
							}
						}
					}
				}
			}
		}
	};

	g_pCoreSystem->getGameSystem()->registerButtonStatusCallback(m_inputComponent, ButtonData{ INNO_MOUSE_BUTTON_LEFT, ButtonStatus::PRESSED }, &f_mouseSelect);

	//PhysXWrapper::get().setup();

	m_objectStatus = ObjectStatus::ALIVE;

	return true;
}

INNO_SYSTEM_EXPORT bool InnoPhysicsSystem::setup()
{
	return InnoPhysicsSystemNS::setup();
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
	auto l_cameraTransformComponent = g_pCoreSystem->getGameSystem()->get<TransformComponent>(cameraComponent->m_parentEntity);
	auto l_rCamera = InnoMath::toRotationMatrix(l_cameraTransformComponent->m_globalTransformVector.m_rot);
	auto l_tCamera = InnoMath::toTranslationMatrix(l_cameraTransformComponent->m_globalTransformVector.m_pos);
	auto l_pCamera = cameraComponent->m_projectionMatrix;

	auto l_NDC = InnoMath::generateNDC<float>();

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

	return l_NDC;
}

void InnoPhysicsSystemNS::generateFrustum(CameraComponent * cameraComponent)
{
	auto l_vertices = generateFrustumVertices(cameraComponent);
	cameraComponent->m_frustum = InnoMath::makeFrustum(l_vertices);
}

void InnoPhysicsSystemNS::generatePointLightComponentAttenuationRadius(PointLightComponent* pointLightComponent)
{
	auto l_RGBColor = pointLightComponent->m_color.normalize();
	// "Real-Time Rendering", 4th Edition, p.278
	// https://en.wikipedia.org/wiki/Relative_luminance
	// weight with respect to CIE photometric curve
	auto l_relativeLuminanceRatio = (0.2126f * l_RGBColor.x + 0.7152f * l_RGBColor.y + 0.0722f * l_RGBColor.z);

	// Luminance (nt) is illuminance (lx) per solid angle, while luminous intensity (cd) is luminous flux (lm) per solid angle, thus for one area unit (m^2), the ratio of nt/lx is same as cd/lm
	// For omni isotropic light, after the intergration per solid angle, the luminous flux (lm) is 4 pi times the luminous intensity (cd)
	auto l_weightedLuminousFlux = pointLightComponent->m_luminousFlux * l_relativeLuminanceRatio;

	// 1. get luminous efficacy (lm/w), assume 683 lm/w (100% luminous efficiency) always
	// 2. luminous flux (lm) to radiant flux (w), omitted because linearity assumption in step 1
	// 3. apply inverse square attenuation law with a low threshold of eye sensitivity at 0.03 lx, in ideal situation, lx could convert back to lm with respect to a sphere surface area 4 * PI * r^2
#if defined INNO_PLATFORM_WIN
	pointLightComponent->m_attenuationRadius = std::sqrtf(l_weightedLuminousFlux / (4.0f * PI<float> * 0.03f));
#else
	pointLightComponent->m_attenuationRadius = sqrtf(l_weightedLuminousFlux / (4.0f * PI<float> * 0.03f));
#endif
}

void InnoPhysicsSystemNS::generateSphereLightComponentScale(SphereLightComponent* sphereLightComponent)
{
	g_pCoreSystem->getGameSystem()->get<TransformComponent>(sphereLightComponent->m_parentEntity)->m_localTransformVector.m_scale =
		vec4(sphereLightComponent->m_sphereRadius, sphereLightComponent->m_sphereRadius, sphereLightComponent->m_sphereRadius, 1.0f);
}

void InnoPhysicsSystemNS::generateAABB(DirectionalLightComponent* directionalLightComponent)
{
	directionalLightComponent->m_AABBsInWorldSpace.clear();
	directionalLightComponent->m_projectionMatrices.clear();

	//1. get frustum vertices and the maxium draw distance
	auto l_cameraComponents = g_pCoreSystem->getGameSystem()->get<CameraComponent>();
	auto l_camera = l_cameraComponents[0];
	auto l_frustumVertices = generateFrustumVertices(l_camera);
	auto l_distance = l_camera->m_zFar - l_camera->m_zNear;
	std::vector<float> l_CSMSplitFactors = { 20.48f / l_distance, 128.0f / l_distance, 1024.0f / l_distance, 1.0f };

	//2.calculate AABBs in world space
	auto l_AABBsWS = frustumsVerticesToAABBs(l_frustumVertices, l_CSMSplitFactors);

	//3. save the AABB for bound area detection
	directionalLightComponent->m_AABBsInWorldSpace = std::move(l_AABBsWS);

	//4. transform frustum vertices to light space
	auto l_lightRotMat = g_pCoreSystem->getGameSystem()->get<TransformComponent>(directionalLightComponent->m_parentEntity)->m_globalTransformMatrix.m_rotationMat.inverse();
	for (size_t i = 0; i < l_frustumVertices.size(); i++)
	{
		//Column-Major memory layout
#ifdef USE_COLUMN_MAJOR_MEMORY_LAYOUT
		l_frustumVertices[i].m_pos = InnoMath::mul(l_frustumVertices[i].m_pos, l_lightRotMat);
#endif
		//Row-Major memory layout
#ifdef USE_ROW_MAJOR_MEMORY_LAYOUT
		l_frustumVertices[i].m_pos = InnoMath::mul(l_lightRotMat, l_frustumVertices[i].m_pos);
#endif
	}

	//5.calculate AABBs in light space
	auto l_AABBsLS = frustumsVerticesToAABBs(l_frustumVertices, l_CSMSplitFactors);

	//6. extend AABB to include the sphere
	for (size_t i = 0; i < 4; i++)
	{
		auto sphereRadius = (l_AABBsLS[i].m_boundMax - l_AABBsLS[i].m_center).length();
		auto l_boundMax = l_AABBsLS[i].m_center + sphereRadius;
		l_boundMax.w = 1.0f;
		auto l_boundMin = l_AABBsLS[i].m_center - sphereRadius;
		l_boundMin.w = 1.0f;
		l_AABBsLS[i] = generateAABB(l_boundMax, l_boundMin);
	}

	//7. generate projection matrices
	directionalLightComponent->m_projectionMatrices.reserve(4);

	for (size_t i = 0; i < 4; i++)
	{
		vec4 l_maxExtents = l_AABBsLS[i].m_boundMax;
		vec4 l_minExtents = l_AABBsLS[i].m_boundMin;

		mat4 p = InnoMath::generateOrthographicMatrix(l_minExtents.x, l_maxExtents.x, l_minExtents.y, l_maxExtents.y, l_minExtents.z, l_maxExtents.z);
		directionalLightComponent->m_projectionMatrices.emplace_back(p);
	}
}

std::vector<AABB> InnoPhysicsSystemNS::frustumsVerticesToAABBs(const std::vector<Vertex>& frustumsVertices, const std::vector<float>& splitFactors)
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
			auto l_splitedPlaneCornerPos = frustumsVertices[j].m_pos + l_direction * splitFactors[i];
			l_frustumsCornerPos.emplace_back(l_splitedPlaneCornerPos);
		}
	}

	//3. assemble splited frustum corners
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

	//4. assemble splitted frustums
	std::vector<std::vector<Vertex>> l_splitedFrustums;
	l_splitedFrustums.reserve(4);

	for (size_t i = 0; i < 4; i++)
	{
		l_splitedFrustums.emplace_back(std::vector<Vertex>(l_frustumsCornerVertices.begin() + i * 8, l_frustumsCornerVertices.begin() + 8 + i * 8));
	}

	//5. generate AABBs for the splited frustums
	std::vector<AABB> l_AABBs;
	l_AABBs.reserve(4);

	for (size_t i = 0; i < 4; i++)
	{
		l_AABBs.emplace_back(generateAABB(l_splitedFrustums[i]));
	}

	return l_AABBs;
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

	l_AABB.m_center = (boundMax + boundMin) * 0.5f;
	l_AABB.m_extend = boundMax - boundMin;

	return l_AABB;
}

Sphere InnoPhysicsSystemNS::generateBoundSphere(AABB rhs)
{
	Sphere l_result;
	l_result.m_center = rhs.m_center;
	l_result.m_radius = (rhs.m_boundMax - rhs.m_center).length();
	return l_result;
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

	return l_vertices;
}

std::vector<Vertex> InnoPhysicsSystemNS::generateAABBVertices(AABB rhs)
{
	auto boundMax = rhs.m_boundMax;
	auto boundMin = rhs.m_boundMin;

	return generateAABBVertices(boundMax, boundMin);
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

	l_MDC->m_objectStatus = ObjectStatus::STANDBY;

	g_pCoreSystem->getVisionSystem()->getRenderingFrontend()->registerUninitializedMeshDataComponent(l_MDC);

	return l_MDC;
}

MeshDataComponent* InnoPhysicsSystemNS::generateMeshDataComponent(Frustum rhs)
{
	return nullptr;
}

PhysicsDataComponent* InnoPhysicsSystemNS::generatePhysicsDataComponent(const ModelMap& modelMap, const EntityID& entityID)
{
	auto l_PDC = g_pCoreSystem->getMemorySystem()->spawn<PhysicsDataComponent>();
	l_PDC->m_parentEntity = entityID;

	for (auto& l_MDC : modelMap)
	{
		PhysicsData l_physicsData;

		auto l_AABB = generateAABB(l_MDC.first->m_vertices);
		auto l_MDCforAABB = generateMeshDataComponent(l_AABB);
		auto l_sphere = generateBoundSphere(l_AABB);

		l_physicsData.MDC = l_MDC.first;
		l_physicsData.wireframeMDC = l_MDCforAABB;
		l_physicsData.aabb = l_AABB;
		l_physicsData.sphere = l_sphere;

		l_PDC->m_physicsDatas.emplace_back(l_physicsData);
	}

	g_pCoreSystem->getLogSystem()->printLog(LogType::INNO_DEV_VERBOSE, "PhysicsSystem: PhysicsDataComponent has been generated for " + entityID + ".");

	return l_PDC;
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

	l_AABB.m_extend = l_AABB.m_boundMax - l_AABB.m_boundMin;

	return l_AABB;
}

INNO_SYSTEM_EXPORT bool InnoPhysicsSystem::initialize()
{
	InnoPhysicsSystemNS::m_objectStatus = ObjectStatus::ALIVE;
	g_pCoreSystem->getLogSystem()->printLog(LogType::INNO_DEV_SUCCESS, "PhysicsSystem has been initialized.");
	return true;
}

void InnoPhysicsSystemNS::updateCameraComponents()
{
	for (auto& i : g_pCoreSystem->getGameSystem()->get<CameraComponent>())
	{
		generateProjectionMatrix(i);
		generateRayOfEye(i);
		generateFrustum(i);
	}
}

void InnoPhysicsSystemNS::updateLightComponents()
{
	for (auto& i : g_pCoreSystem->getGameSystem()->get<DirectionalLightComponent>())
	{
		generateAABB(i);
	}
	for (auto& i : g_pCoreSystem->getGameSystem()->get<PointLightComponent>())
	{
		generatePointLightComponentAttenuationRadius(i);
	}
	for (auto& i : g_pCoreSystem->getGameSystem()->get<SphereLightComponent>())
	{
		generateSphereLightComponentScale(i);
	}
}

void InnoPhysicsSystemNS::updateVisibleComponents()
{
}

template<class T>
bool intersectCheck(const TFrustum<T> & lhs, const TAABB<T> & rhs)
{
	auto l_isCenterInside = isPointInFrustum(rhs.m_center, lhs);
	if (l_isCenterInside)
	{
		return true;
	}

	auto l_isMaxInside = isPointInFrustum(rhs.m_boundMax, lhs);
	if (l_isMaxInside)
	{
		return true;
	}

	auto l_isMinInside = isPointInFrustum(rhs.m_boundMin, lhs);
	if (l_isMinInside)
	{
		return true;
	}

	return false;
}

void InnoPhysicsSystemNS::updateCulling()
{
	m_cullingDataPack.clear();

	if (g_pCoreSystem->getGameSystem()->get<CameraComponent>().size() > 0)
	{
		auto l_cameraComponents = g_pCoreSystem->getGameSystem()->get<CameraComponent>();
		auto l_mainCamera = l_cameraComponents[0];
		auto l_mainCameraTransformComponent = g_pCoreSystem->getGameSystem()->get<TransformComponent>(l_mainCamera->m_parentEntity);

		auto l_cameraFrustum = l_mainCamera->m_frustum;
		auto l_eyeRay = l_mainCamera->m_rayOfEye;

		for (auto visibleComponent : g_pCoreSystem->getGameSystem()->get<VisibleComponent>())
		{
			if (visibleComponent->m_visiblilityType != VisiblilityType::INNO_INVISIBLE && visibleComponent->m_objectStatus == ObjectStatus::ALIVE)
			{
				auto l_transformComponent = g_pCoreSystem->getGameSystem()->get<TransformComponent>(visibleComponent->m_parentEntity);
				auto l_globalTm = l_transformComponent->m_globalTransformMatrix.m_transformationMat;

				if (visibleComponent->m_PhysicsDataComponent)
				{
					for (auto& physicsData : visibleComponent->m_PhysicsDataComponent->m_physicsDatas)
					{
						auto l_OBBws = transformAABBtoWorldSpace(physicsData.aabb, l_globalTm);
						updateSceneAABB(l_OBBws);

						auto l_boundingSphere = Sphere();
						l_boundingSphere.m_center = l_OBBws.m_center;
						l_boundingSphere.m_radius = l_OBBws.m_extend.length();

						if (InnoMath::intersectCheck(l_cameraFrustum, l_boundingSphere))
						{
							CullingDataPack l_cullingDataPack;

							l_cullingDataPack.m = l_globalTm;
							l_cullingDataPack.m_prev = l_transformComponent->m_globalTransformMatrix_prev.m_transformationMat;
							l_cullingDataPack.normalMat = l_transformComponent->m_globalTransformMatrix.m_rotationMat;
							l_cullingDataPack.visibleComponent = visibleComponent;
							l_cullingDataPack.MDC = physicsData.MDC;

							m_cullingDataPack.emplace_back(l_cullingDataPack);
						}
					}
				}
			}
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
	if (boundMin.x < InnoPhysicsSystemNS::m_sceneBoundMin.x
		&&boundMin.y < InnoPhysicsSystemNS::m_sceneBoundMin.y
		&&boundMin.z < InnoPhysicsSystemNS::m_sceneBoundMin.z)
	{
		InnoPhysicsSystemNS::m_sceneBoundMin = boundMin;
	}

	m_SceneAABB = generateAABB(InnoPhysicsSystemNS::m_sceneBoundMax, InnoPhysicsSystemNS::m_sceneBoundMin);
}

bool InnoPhysicsSystemNS::update()
{
	if (g_pCoreSystem->getFileSystem()->isLoadingScene())
	{
		m_cullingDataPack.clear();
		m_isCullingDataPackValid = false;
		return true;
	}

	updateCameraComponents();
	updateLightComponents();
	updateVisibleComponents();

	m_isCullingDataPackValid = false;
	updateCulling();
	m_isCullingDataPackValid = true;

	auto preparePhysicsDataTask = g_pCoreSystem->getTaskSystem()->submit([]()
	{
	});

	m_asyncTask.emplace_back(std::move(preparePhysicsDataTask));

	g_pCoreSystem->getTaskSystem()->shrinkFutureContainer(m_asyncTask);

	return true;
}

INNO_SYSTEM_EXPORT bool InnoPhysicsSystem::update()
{
	return InnoPhysicsSystemNS::update();
}

INNO_SYSTEM_EXPORT bool InnoPhysicsSystem::terminate()
{
	InnoPhysicsSystemNS::m_objectStatus = ObjectStatus::SHUTDOWN;
	g_pCoreSystem->getLogSystem()->printLog(LogType::INNO_DEV_SUCCESS, "PhysicsSystem has been terminated.");
	return true;
}

INNO_SYSTEM_EXPORT ObjectStatus InnoPhysicsSystem::getStatus()
{
	return InnoPhysicsSystemNS::m_objectStatus;
}

INNO_SYSTEM_EXPORT PhysicsDataComponent* InnoPhysicsSystem::generatePhysicsDataComponent(const ModelMap& modelMap, const EntityID& entityID)
{
	auto l_physicsComponent = InnoPhysicsSystemNS::generatePhysicsDataComponent(modelMap, entityID);
	return l_physicsComponent;
}

INNO_SYSTEM_EXPORT std::optional<std::vector<CullingDataPack>> InnoPhysicsSystem::getCullingDataPack()
{
	if (InnoPhysicsSystemNS::m_isCullingDataPackValid)
	{
		return InnoPhysicsSystemNS::m_cullingDataPack;
	}

	return std::nullopt;
}

INNO_SYSTEM_EXPORT AABB InnoPhysicsSystem::getSceneAABB()
{
	return InnoPhysicsSystemNS::m_SceneAABB;
}