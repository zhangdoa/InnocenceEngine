#include "PhysicsSystem.h"
#include "../Common/CommonMacro.inl"
#include "../ComponentManager/ITransformComponentManager.h"
#include "../ComponentManager/IVisibleComponentManager.h"

#include "../Common/InnoMathHelper.h"

#if defined INNO_PLATFORM_WIN
#include "PhysXWrapper.h"
#endif

#include "../ModuleManager/IModuleManager.h"

extern IModuleManager* g_pModuleManager;

namespace InnoPhysicsSystemNS
{
	bool setup();
	bool update();

	void generateProjectionMatrix(CameraComponent* cameraComponent);
	void generateRayOfEye(CameraComponent* cameraComponent);
	std::vector<Vertex> worldToViewSpace(const std::vector<Vertex>& rhs, mat4 t, mat4 r);
	std::vector<Vertex> viewToWorldSpace(const std::vector<Vertex>& rhs, mat4 t, mat4 r);
	std::vector<Vertex> generateFrustumVerticesVS(CameraComponent* cameraComponent);
	std::vector<Vertex> generateFrustumVerticesWS(CameraComponent* cameraComponent);

	void generateFrustum(CameraComponent* cameraComponent);
	void generatePointLightComponentAttenuationRadius(PointLightComponent* pointLightComponent);
	void generateSphereLightComponentScale(SphereLightComponent* sphereLightComponent);

	void generateAABB(DirectionalLightComponent* directionalLightComponent);
	std::vector<AABB> splitVerticesToAABBs(const std::vector<Vertex>& frustumsVertices, const std::vector<float>& splitFactors);

	std::vector<Vertex> generateAABBVertices(vec4 boundMax, vec4 boundMin);
	std::vector<Vertex> generateAABBVertices(const AABB& rhs);

	bool generatePhysicsDataComponent(MeshDataComponent* MDC);
	bool generatePhysicsDataComponent(VisibleComponent* VC);

	void updateCameraComponents();
	void updateLightComponents();
	void updateCulling();
	void updateVisibleSceneBoundary(const AABB& rhs);
	void updateTotalSceneBoundary(const AABB& rhs);

	ObjectStatus m_objectStatus = ObjectStatus::Terminated;

	vec4 m_visibleSceneBoundMax;
	vec4 m_visibleSceneBoundMin;
	AABB m_visibleSceneAABB;

	vec4 m_totalSceneBoundMax;
	vec4 m_totalSceneBoundMin;
	AABB m_totalSceneAABB;

	void* m_PhysicsDataComponentPool;

	std::atomic<bool> m_isCullingDataPackValid = false;
	ThreadSafeVector<CullingDataPack> m_cullingDataPack;

	std::function<void()> f_sceneLoadingStartCallback;
}

bool InnoPhysicsSystemNS::setup()
{
	m_PhysicsDataComponentPool = g_pModuleManager->getMemorySystem()->allocateMemoryPool(sizeof(PhysicsDataComponent), 16384);

#if defined INNO_PLATFORM_WIN
	PhysXWrapper::get().setup();
#endif

	f_sceneLoadingStartCallback = [&]() {
		m_cullingDataPack.clear();
		m_isCullingDataPackValid = false;
	};

	g_pModuleManager->getFileSystem()->addSceneLoadingStartCallback(&f_sceneLoadingStartCallback);

	m_objectStatus = ObjectStatus::Created;
	return true;
}

bool InnoPhysicsSystem::setup()
{
	return InnoPhysicsSystemNS::setup();
}

void InnoPhysicsSystemNS::generateProjectionMatrix(CameraComponent * cameraComponent)
{
	auto l_resolution = g_pModuleManager->getRenderingFrontend()->getScreenResolution();
	cameraComponent->m_WHRatio = (float)l_resolution.x / (float)l_resolution.y;
	cameraComponent->m_projectionMatrix = InnoMath::generatePerspectiveMatrix((cameraComponent->m_FOVX / 180.0f) * PI<float>, cameraComponent->m_WHRatio, cameraComponent->m_zNear, cameraComponent->m_zFar);
}

void InnoPhysicsSystemNS::generateRayOfEye(CameraComponent * cameraComponent)
{
	auto l_transformComponent = GetComponent(TransformComponent, cameraComponent->m_parentEntity);
	cameraComponent->m_rayOfEye.m_origin = l_transformComponent->m_globalTransformVector.m_pos;
	cameraComponent->m_rayOfEye.m_direction = InnoMath::getDirection(direction::BACKWARD, l_transformComponent->m_localTransformVector.m_rot);
}

std::vector<Vertex> InnoPhysicsSystemNS::worldToViewSpace(const std::vector<Vertex>& rhs, mat4 t, mat4 r)
{
	auto l_result = rhs;

	for (auto& l_vertexData : l_result)
	{
		auto l_mulPos = InnoMath::worldToViewSpace(l_vertexData.m_pos, t, r);
		l_vertexData.m_pos = l_mulPos;
	}

	for (auto& l_vertexData : l_result)
	{
		l_vertexData.m_normal = vec4(l_vertexData.m_pos.x, l_vertexData.m_pos.y, l_vertexData.m_pos.z, 0.0f).normalize();
	}

	return l_result;
}

std::vector<Vertex> InnoPhysicsSystemNS::viewToWorldSpace(const std::vector<Vertex>& rhs, mat4 t, mat4 r)
{
	auto l_result = rhs;

	for (auto& l_vertexData : l_result)
	{
		auto l_mulPos = InnoMath::viewToWorldSpace(l_vertexData.m_pos, t, r);
		l_vertexData.m_pos = l_mulPos;
	}

	for (auto& l_vertexData : l_result)
	{
		l_vertexData.m_normal = vec4(l_vertexData.m_pos.x, l_vertexData.m_pos.y, l_vertexData.m_pos.z, 0.0f).normalize();
	}

	return l_result;
}

std::vector<Vertex> InnoPhysicsSystemNS::generateFrustumVerticesVS(CameraComponent * cameraComponent)
{
	auto l_cameraTransformComponent = GetComponent(TransformComponent, cameraComponent->m_parentEntity);
	auto l_pCamera = cameraComponent->m_projectionMatrix;

	std::vector<Vertex> rhs(8);

	InnoMath::generateNDC<float>(&rhs[0]);

	for (auto& i : rhs)
	{
		i.m_pos = InnoMath::clipToViewSpace(i.m_pos, l_pCamera);
	}

	// near clip plane first
	// @TODO: reverse only along Z axis, not simple mirrored version
	std::reverse(rhs.begin(), rhs.end());

	std::vector<Vertex> l_vertices(8);

	for (unsigned int i = 0; i < 8; i++)
	{
		l_vertices[i] = rhs[i];
	}

	return l_vertices;
}

std::vector<Vertex> InnoPhysicsSystemNS::generateFrustumVerticesWS(CameraComponent * cameraComponent)
{
	auto l_cameraTransformComponent = GetComponent(TransformComponent, cameraComponent->m_parentEntity);
	auto l_rCamera = InnoMath::toRotationMatrix(l_cameraTransformComponent->m_globalTransformVector.m_rot);
	auto l_tCamera = InnoMath::toTranslationMatrix(l_cameraTransformComponent->m_globalTransformVector.m_pos);

	auto rhs = generateFrustumVerticesVS(cameraComponent);

	for (auto& i : rhs)
	{
		i.m_pos = InnoMath::viewToWorldSpace(i.m_pos, l_tCamera, l_rCamera);
	}

	for (auto& i : rhs)
	{
		i.m_normal = vec4(i.m_pos.x, i.m_pos.y, i.m_pos.z, 0.0f).normalize();
	}

	return rhs;
}

void InnoPhysicsSystemNS::generateFrustum(CameraComponent * cameraComponent)
{
	auto l_vertices = generateFrustumVerticesWS(cameraComponent);
	cameraComponent->m_frustum = InnoMath::makeFrustum(&l_vertices[0]);
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
	GetComponent(TransformComponent, sphereLightComponent->m_parentEntity)->m_localTransformVector.m_scale =
		vec4(sphereLightComponent->m_sphereRadius, sphereLightComponent->m_sphereRadius, sphereLightComponent->m_sphereRadius, 1.0f);
}

void InnoPhysicsSystemNS::generateAABB(DirectionalLightComponent* directionalLightComponent)
{
	directionalLightComponent->m_AABBsInWorldSpace.clear();
	directionalLightComponent->m_projectionMatrices.clear();

	//1. get frustum vertices in view space
	auto l_cameraComponents = g_pModuleManager->getGameSystem()->get<CameraComponent>();
	auto l_cameraComponent = l_cameraComponents[0];
	auto l_cameraTransformComponent = GetComponent(TransformComponent, l_cameraComponent->m_parentEntity);
	auto l_rCamera = InnoMath::toRotationMatrix(l_cameraTransformComponent->m_globalTransformVector.m_rot);
	auto l_tCamera = InnoMath::toTranslationMatrix(l_cameraTransformComponent->m_globalTransformVector.m_pos);
	auto l_frustumVerticesVS = generateFrustumVerticesVS(l_cameraComponent);

	// extend scene AABB to include the bound sphere, for to eliminate rotation conflict
	auto l_sphereRadius = (m_totalSceneAABB.m_boundMax - m_totalSceneAABB.m_center).length();
	auto l_boundMax = m_totalSceneAABB.m_center + l_sphereRadius;
	l_boundMax.w = 1.0f;
	auto l_boundMin = m_totalSceneAABB.m_center - l_sphereRadius;
	l_boundMin.w = 1.0f;

	// transform scene AABB vertices to view space
	auto l_sceneAABBVerticesWS = generateAABBVertices(l_boundMax, l_boundMin);
	auto l_sceneAABBVerticesVS = worldToViewSpace(l_sceneAABBVerticesWS, l_tCamera, l_rCamera);
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
		static bool l_adjustSidePlane = false;
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

	auto l_frustumVerticesWS = viewToWorldSpace(l_frustumVerticesVS, l_tCamera, l_rCamera);

	std::vector<float> l_CSMSplitFactors = { 0.05f, 0.25f, 0.55f, 1.0f };

	//2. calculate AABBs in world space
	auto l_frustumsAABBsWS = splitVerticesToAABBs(l_frustumVerticesWS, l_CSMSplitFactors);

	//3. save the AABB for bound area detection
	directionalLightComponent->m_AABBsInWorldSpace.setRawData(std::move(l_frustumsAABBsWS));

	//4. transform frustum vertices to light space
	auto l_lightRotMat = GetComponent(TransformComponent, directionalLightComponent->m_parentEntity)->m_globalTransformMatrix.m_rotationMat.inverse();
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
	directionalLightComponent->m_projectionMatrices.reserve(4);

	for (size_t i = 0; i < 4; i++)
	{
		vec4 l_maxExtents = l_AABBsLS[i].m_boundMax;
		vec4 l_minExtents = l_AABBsLS[i].m_boundMin;

		mat4 p = InnoMath::generateOrthographicMatrix(l_minExtents.x, l_maxExtents.x, l_minExtents.y, l_maxExtents.y, l_minExtents.z, l_maxExtents.z);
		directionalLightComponent->m_projectionMatrices.emplace_back(p);
	}
}

std::vector<AABB> InnoPhysicsSystemNS::splitVerticesToAABBs(const std::vector<Vertex>& frustumsVertices, const std::vector<float>& splitFactors)
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

std::vector<Vertex> InnoPhysicsSystemNS::generateAABBVertices(vec4 boundMax, vec4 boundMin)
{
	std::vector<Vertex> l_vertices(8);

	l_vertices[0].m_pos = (vec4(boundMax.x, boundMax.y, boundMax.z, 1.0f));
	l_vertices[0].m_texCoord = vec2(1.0f, 1.0f);

	l_vertices[1].m_pos = (vec4(boundMax.x, boundMin.y, boundMax.z, 1.0f));
	l_vertices[1].m_texCoord = vec2(1.0f, 0.0f);

	l_vertices[2].m_pos = (vec4(boundMin.x, boundMin.y, boundMax.z, 1.0f));
	l_vertices[2].m_texCoord = vec2(0.0f, 0.0f);

	l_vertices[3].m_pos = (vec4(boundMin.x, boundMax.y, boundMax.z, 1.0f));
	l_vertices[3].m_texCoord = vec2(0.0f, 1.0f);

	l_vertices[4].m_pos = (vec4(boundMax.x, boundMax.y, boundMin.z, 1.0f));
	l_vertices[4].m_texCoord = vec2(1.0f, 1.0f);

	l_vertices[5].m_pos = (vec4(boundMax.x, boundMin.y, boundMin.z, 1.0f));
	l_vertices[5].m_texCoord = vec2(1.0f, 0.0f);

	l_vertices[6].m_pos = (vec4(boundMin.x, boundMin.y, boundMin.z, 1.0f));
	l_vertices[6].m_texCoord = vec2(0.0f, 0.0f);

	l_vertices[7].m_pos = (vec4(boundMin.x, boundMax.y, boundMin.z, 1.0f));
	l_vertices[7].m_texCoord = vec2(0.0f, 1.0f);

	for (auto& l_vertexData : l_vertices)
	{
		l_vertexData.m_normal = vec4(l_vertexData.m_pos.x, l_vertexData.m_pos.y, l_vertexData.m_pos.z, 0.0f).normalize();
	}

	return l_vertices;
}

std::vector<Vertex> InnoPhysicsSystemNS::generateAABBVertices(const AABB& rhs)
{
	auto boundMax = rhs.m_boundMax;
	auto boundMin = rhs.m_boundMin;

	return generateAABBVertices(boundMax, boundMin);
}

bool InnoPhysicsSystemNS::generatePhysicsDataComponent(MeshDataComponent* MDC)
{
	auto l_rawPtr = g_pModuleManager->getMemorySystem()->spawnObject(m_PhysicsDataComponentPool, sizeof(PhysicsDataComponent));
	auto l_PDC = new(l_rawPtr)PhysicsDataComponent();

	l_PDC->m_parentEntity = MDC->m_parentEntity;

	auto l_boundMax = InnoMath::minVec4<float>;
	l_boundMax.w = 1.0f;

	auto l_boundMin = InnoMath::maxVec4<float>;
	l_boundMin.w = 1.0f;

	auto l_AABB = InnoMath::generateAABB(&MDC->m_vertices[0], MDC->m_vertices.size());
	auto l_sphere = InnoMath::generateBoundSphere(l_AABB);

	if (InnoMath::isAGreaterThanBVec3(l_AABB.m_boundMax, l_boundMax))
	{
		l_boundMax = l_AABB.m_boundMax;
	}
	if (InnoMath::isALessThanBVec3(l_AABB.m_boundMin, l_boundMin))
	{
		l_boundMin = l_AABB.m_boundMin;
	}
	l_PDC->m_AABB = l_AABB;
	l_PDC->m_sphere = l_sphere;

	g_pModuleManager->getLogSystem()->printLog(LogType::INNO_DEV_VERBOSE, "PhysicsSystem: PhysicsDataComponent has been generated for MeshDataComponent:" + std::string(MDC->m_parentEntity->m_entityName.c_str()) + ".");

	MDC->m_PDC = l_PDC;

	return true;
}

bool InnoPhysicsSystemNS::generatePhysicsDataComponent(VisibleComponent* VC)
{
	auto l_rawPtr = g_pModuleManager->getMemorySystem()->spawnObject(m_PhysicsDataComponentPool, sizeof(PhysicsDataComponent));
	auto l_PDC = new(l_rawPtr)PhysicsDataComponent();

	l_PDC->m_parentEntity = VC->m_parentEntity;

	auto l_boundMax = InnoMath::minVec4<float>;
	l_boundMax.w = 1.0f;

	auto l_boundMin = InnoMath::maxVec4<float>;
	l_boundMin.w = 1.0f;

	AABB l_AABB;
	Sphere l_sphere;

	for (auto& l_MDC : VC->m_modelMap)
	{
		auto l_AABB = l_MDC.first->m_PDC->m_AABB;
		auto l_sphere = l_MDC.first->m_PDC->m_sphere;

		if (InnoMath::isAGreaterThanBVec3(l_AABB.m_boundMax, l_boundMax))
		{
			l_boundMax = l_AABB.m_boundMax;
		}
		if (InnoMath::isALessThanBVec3(l_AABB.m_boundMin, l_boundMin))
		{
			l_boundMin = l_AABB.m_boundMin;
		}
	}

	l_PDC->m_AABB = InnoMath::generateAABB(l_boundMax, l_boundMin);
	l_PDC->m_sphere = InnoMath::generateBoundSphere(l_PDC->m_AABB);

#if defined INNO_PLATFORM_WIN
	if (VC->m_simulatePhysics)
	{
		auto l_transformComponent = GetComponent(TransformComponent, VC->m_parentEntity);
		switch (VC->m_meshShapeType)
		{
		case MeshShapeType::CUBE:
			PhysXWrapper::get().createPxBox(l_transformComponent, l_transformComponent->m_localTransformVector.m_pos, l_transformComponent->m_localTransformVector.m_rot, l_transformComponent->m_localTransformVector.m_scale);
			break;
		case MeshShapeType::SPHERE:
			PhysXWrapper::get().createPxSphere(l_transformComponent, l_transformComponent->m_localTransformVector.m_pos, l_transformComponent->m_localTransformVector.m_scale.x);
			break;
		case MeshShapeType::CUSTOM:
			PhysXWrapper::get().createPxBox(l_transformComponent, l_transformComponent->m_localTransformVector.m_pos, l_transformComponent->m_localTransformVector.m_rot, l_boundMax - l_boundMin);
			break;
		default:
			break;
		}
	}
#endif

	VC->m_PDC = l_PDC;
	g_pModuleManager->getLogSystem()->printLog(LogType::INNO_DEV_VERBOSE, "PhysicsSystem: PhysicsDataComponent has been generated for VisibleComponent:" + std::string(VC->m_parentEntity->m_entityName.c_str()) + ".");

	return true;
}

bool InnoPhysicsSystem::initialize()
{
	if (InnoPhysicsSystemNS::m_objectStatus == ObjectStatus::Created)
	{
		InnoPhysicsSystemNS::m_objectStatus = ObjectStatus::Activated;
		g_pModuleManager->getLogSystem()->printLog(LogType::INNO_DEV_SUCCESS, "PhysicsSystem has been initialized.");
		return true;
	}
	else
	{
		g_pModuleManager->getLogSystem()->printLog(LogType::INNO_ERROR, "PhysicsSystem: Object is not created!");
		return false;
	}
	return true;
}

void InnoPhysicsSystemNS::updateCameraComponents()
{
	for (auto& i : g_pModuleManager->getGameSystem()->get<CameraComponent>())
	{
		i->m_WHRatio = i->m_widthScale / i->m_heightScale;
		generateProjectionMatrix(i);
		generateRayOfEye(i);
		generateFrustum(i);
	}
}

void InnoPhysicsSystemNS::updateLightComponents()
{
	for (auto& i : g_pModuleManager->getGameSystem()->get<DirectionalLightComponent>())
	{
		generateAABB(i);
	}
	for (auto& i : g_pModuleManager->getGameSystem()->get<PointLightComponent>())
	{
		generatePointLightComponentAttenuationRadius(i);
	}
	for (auto& i : g_pModuleManager->getGameSystem()->get<SphereLightComponent>())
	{
		generateSphereLightComponentScale(i);
	}
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
	m_isCullingDataPackValid = false;

	m_cullingDataPack.clear();

	m_visibleSceneBoundMax = InnoMath::minVec4<float>;
	m_visibleSceneBoundMax.w = 1.0f;

	m_visibleSceneBoundMin = InnoMath::maxVec4<float>;
	m_visibleSceneBoundMin.w = 1.0f;

	m_totalSceneBoundMax = InnoMath::minVec4<float>;
	m_totalSceneBoundMax.w = 1.0f;

	m_totalSceneBoundMin = InnoMath::maxVec4<float>;
	m_totalSceneBoundMin.w = 1.0f;

	std::vector<CullingDataPack> l_cullingDataPacks;

	if (g_pModuleManager->getGameSystem()->get<CameraComponent>().size() > 0)
	{
		auto l_cameraComponents = g_pModuleManager->getGameSystem()->get<CameraComponent>();
		auto l_mainCamera = l_cameraComponents[0];
		auto l_mainCameraTransformComponent = GetComponent(TransformComponent, l_mainCamera->m_parentEntity);

		auto l_cameraFrustum = l_mainCamera->m_frustum;
		auto l_eyeRay = l_mainCamera->m_rayOfEye;

		auto l_visibleComponents = GetComponentManager(VisibleComponent)->GetAllComponents();
		for (auto visibleComponent : l_visibleComponents)
		{
			if (visibleComponent->m_visiblilityType != VisiblilityType::INNO_INVISIBLE && visibleComponent->m_objectStatus == ObjectStatus::Activated)
			{
				auto l_transformComponent = GetComponent(TransformComponent, visibleComponent->m_parentEntity);
				auto l_globalTm = l_transformComponent->m_globalTransformMatrix.m_transformationMat;

				if (visibleComponent->m_PDC)
				{
					for (auto& l_modelPair : visibleComponent->m_modelMap)
					{
						auto l_PDC = l_modelPair.first->m_PDC;
						auto l_OBBws = InnoMath::transformAABBSpace(l_PDC->m_AABB, l_globalTm);

						auto l_boundingSphere = Sphere();
						l_boundingSphere.m_center = l_OBBws.m_center;
						l_boundingSphere.m_radius = l_OBBws.m_extend.length();

						if (InnoMath::intersectCheck(l_cameraFrustum, l_boundingSphere))
						{
							updateVisibleSceneBoundary(l_OBBws);

							thread_local CullingDataPack l_cullingDataPack;

							l_cullingDataPack.m = l_globalTm;
							l_cullingDataPack.m_prev = l_transformComponent->m_globalTransformMatrix_prev.m_transformationMat;
							l_cullingDataPack.normalMat = l_transformComponent->m_globalTransformMatrix.m_rotationMat;
							l_cullingDataPack.mesh = l_modelPair.first;
							l_cullingDataPack.material = l_modelPair.second;
							l_cullingDataPack.visiblilityType = visibleComponent->m_visiblilityType;
							l_cullingDataPack.meshUsageType = visibleComponent->m_meshUsageType;
							l_cullingDataPack.UUID = visibleComponent->m_UUID;

							l_cullingDataPacks.emplace_back(l_cullingDataPack);
						}

						updateTotalSceneBoundary(l_OBBws);
					}
				}
			}
		}
	}

	m_visibleSceneAABB = InnoMath::generateAABB(InnoPhysicsSystemNS::m_visibleSceneBoundMax, InnoPhysicsSystemNS::m_visibleSceneBoundMin);
	m_totalSceneAABB = InnoMath::generateAABB(InnoPhysicsSystemNS::m_totalSceneBoundMax, InnoPhysicsSystemNS::m_totalSceneBoundMin);

	m_cullingDataPack.setRawData(std::move(l_cullingDataPacks));

	m_isCullingDataPackValid = true;
}

void InnoPhysicsSystemNS::updateVisibleSceneBoundary(const AABB& rhs)
{
	auto boundMax = rhs.m_boundMax;
	auto boundMin = rhs.m_boundMin;

	if (boundMax.x > m_visibleSceneBoundMax.x)
	{
		m_visibleSceneBoundMax.x = boundMax.x;
	}
	if (boundMax.y > m_visibleSceneBoundMax.y)
	{
		m_visibleSceneBoundMax.y = boundMax.y;
	}
	if (boundMax.z > m_visibleSceneBoundMax.z)
	{
		m_visibleSceneBoundMax.z = boundMax.z;
	}
	if (boundMin.x < m_visibleSceneBoundMin.x)
	{
		m_visibleSceneBoundMin.x = boundMin.x;
	}
	if (boundMin.y < m_visibleSceneBoundMin.y)
	{
		m_visibleSceneBoundMin.y = boundMin.y;
	}
	if (boundMin.z < m_visibleSceneBoundMin.z)
	{
		m_visibleSceneBoundMin.z = boundMin.z;
	}
}

void InnoPhysicsSystemNS::updateTotalSceneBoundary(const AABB& rhs)
{
	auto boundMax = rhs.m_boundMax;
	auto boundMin = rhs.m_boundMin;

	if (boundMax.x > m_totalSceneBoundMax.x)
	{
		m_totalSceneBoundMax.x = boundMax.x;
	}
	if (boundMax.y > m_totalSceneBoundMax.y)
	{
		m_totalSceneBoundMax.y = boundMax.y;
	}
	if (boundMax.z > m_totalSceneBoundMax.z)
	{
		m_totalSceneBoundMax.z = boundMax.z;
	}
	if (boundMin.x < m_totalSceneBoundMin.x)
	{
		m_totalSceneBoundMin.x = boundMin.x;
	}
	if (boundMin.y < m_totalSceneBoundMin.y)
	{
		m_totalSceneBoundMin.y = boundMin.y;
	}
	if (boundMin.z < m_totalSceneBoundMin.z)
	{
		m_totalSceneBoundMin.z = boundMin.z;
	}
}

bool InnoPhysicsSystemNS::update()
{
	if (g_pModuleManager->getFileSystem()->isLoadingScene())
	{
		return true;
	}

#if defined INNO_PLATFORM_WIN
	PhysXWrapper::get().update();
#endif

	g_pModuleManager->getTaskSystem()->submit("CameraComponentsUpdateTask", [&]()
	{
		updateCameraComponents();
	});
	//g_pModuleManager->getTaskSystem()->submit("LightComponentsUpdateTask", [&]()
	//{
	updateLightComponents();
	//});
	g_pModuleManager->getTaskSystem()->submit("CullingTask", [&]()
	{
		updateCulling();
	});

	return true;
}

bool InnoPhysicsSystem::update()
{
	return InnoPhysicsSystemNS::update();
}

bool InnoPhysicsSystem::terminate()
{
	InnoPhysicsSystemNS::m_objectStatus = ObjectStatus::Terminated;
	g_pModuleManager->getLogSystem()->printLog(LogType::INNO_DEV_SUCCESS, "PhysicsSystem has been terminated.");
	return true;
}

ObjectStatus InnoPhysicsSystem::getStatus()
{
	return InnoPhysicsSystemNS::m_objectStatus;
}

bool InnoPhysicsSystem::generatePhysicsDataComponent(MeshDataComponent* MDC)
{
	return InnoPhysicsSystemNS::generatePhysicsDataComponent(MDC);
}

std::optional<std::vector<CullingDataPack>> InnoPhysicsSystem::getCullingDataPack()
{
	if (InnoPhysicsSystemNS::m_isCullingDataPackValid)
	{
		return InnoPhysicsSystemNS::m_cullingDataPack.getRawData();
	}

	return std::nullopt;
}

AABB InnoPhysicsSystem::getSceneAABB()
{
	return InnoPhysicsSystemNS::m_visibleSceneAABB;
}

bool InnoPhysicsSystem::generatePhysicsDataComponent(VisibleComponent * VC)
{
	return InnoPhysicsSystemNS::generatePhysicsDataComponent(VC);
}