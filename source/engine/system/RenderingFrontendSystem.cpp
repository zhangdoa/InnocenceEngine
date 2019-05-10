#include "RenderingFrontendSystem.h"
#include "../component/RenderingFrontendSystemComponent.h"

#include "ICoreSystem.h"

extern ICoreSystem* g_pCoreSystem;

INNO_PRIVATE_SCOPE InnoRenderingFrontendSystemNS
{
	ObjectStatus m_objectStatus = ObjectStatus::SHUTDOWN;

	IRenderingBackendSystem* m_renderingBackendSystem;

	TVec2<unsigned int> m_screenResolution = TVec2<unsigned int>(1280, 720);
	std::string m_windowName;
	bool m_fullScreen = false;

	ThreadSafeVector<CullingDataPack> m_cullingDataPack;

	std::atomic<bool> m_isCSMDataPackValid = false;

	std::atomic<bool> m_isSunDataPackValid = false;

	std::atomic<bool> m_isCameraDataPackValid = false;

	std::atomic<bool> m_isMeshDataPackValid = false;

	std::vector<Plane> m_debugPlanes;
	std::vector<Sphere> m_debugSpheres;

	std::vector<vec2> m_haltonSampler;
	int currentHaltonStep = 0;

	std::function<void(RenderPassType)> f_reloadShader;
	std::function<void()> f_sceneLoadingStartCallback;
	std::function<void()> f_sceneLoadingFinishCallback;
	std::function<void()> f_bakeGI;

	RenderingConfig m_renderingConfig = RenderingConfig();

	bool setup(IRenderingBackendSystem* renderingBackendSystem);
	bool initialize();
	bool update();
	bool terminate();

	float radicalInverse(unsigned int n, unsigned int base);
	void initializeHaltonSampler();

	bool updateCameraData();
	bool updateSunData();
	bool updateLightData();
	bool updateMeshData();
	bool updateBillboardPassData();
	bool updateDebuggerPassData();

	bool gatherStaticMeshData();
}

float InnoRenderingFrontendSystemNS::radicalInverse(unsigned int n, unsigned int base)
{
	float val = 0.0f;
	float invBase = 1.0f / base, invBi = invBase;
	while (n > 0)
	{
		auto d_i = (n % base);
		val += d_i * invBi;
		n *= (unsigned int)invBase;
		invBi *= invBase;
	}
	return val;
};

void InnoRenderingFrontendSystemNS::initializeHaltonSampler()
{
	// in NDC space
	for (unsigned int i = 0; i < 16; i++)
	{
		m_haltonSampler.emplace_back(vec2(radicalInverse(i, 3) * 2.0f - 1.0f, radicalInverse(i, 4) * 2.0f - 1.0f));
	}
}

bool InnoRenderingFrontendSystemNS::setup(IRenderingBackendSystem* renderingBackendSystem)
{
	m_renderingBackendSystem = renderingBackendSystem;

	m_renderingConfig.useMotionBlur = true;
	m_renderingConfig.useTAA = true;
	//m_renderingConfig.useBloom = true;
	m_renderingConfig.drawSky = true;
	//m_renderingConfig.drawTerrain = true;
	//m_renderingConfig.drawDebugObject = true;

	f_sceneLoadingStartCallback = [&]() {
		m_cullingDataPack.clear();

		RenderingFrontendSystemComponent::get().m_CSMGPUDataVector.clear();
		RenderingFrontendSystemComponent::get().m_pointLightGPUDataVector.clear();
		RenderingFrontendSystemComponent::get().m_sphereLightGPUDataVector.clear();

		RenderingFrontendSystemComponent::get().m_opaquePassGPUDataQueue.clear();
		RenderingFrontendSystemComponent::get().m_transparentPassGPUDataQueue.clear();
		RenderingFrontendSystemComponent::get().m_billboardPassGPUDataQueue.clear();
		RenderingFrontendSystemComponent::get().m_debuggerPassGPUDataQueue.clear();

		m_isCSMDataPackValid = false;
		m_isSunDataPackValid = false;
		m_isCameraDataPackValid = false;
		m_isMeshDataPackValid = false;
	};

	f_sceneLoadingFinishCallback = [&]() {
		// point light
		RenderingFrontendSystemComponent::get().m_pointLightGPUDataVector.reserve(RenderingFrontendSystemComponent::get().m_maxPointLights);

		for (size_t i = 0; i < RenderingFrontendSystemComponent::get().m_maxPointLights; i++)
		{
			RenderingFrontendSystemComponent::get().m_pointLightGPUDataVector.emplace_back();
		}

		// sphere light
		RenderingFrontendSystemComponent::get().m_sphereLightGPUDataVector.reserve(RenderingFrontendSystemComponent::get().m_maxSphereLights);

		for (size_t i = 0; i < RenderingFrontendSystemComponent::get().m_maxSphereLights; i++)
		{
			RenderingFrontendSystemComponent::get().m_sphereLightGPUDataVector.emplace_back();
		}
	};

	f_bakeGI = []() {
		gatherStaticMeshData();
		m_renderingBackendSystem->bakeGI();
	};
	g_pCoreSystem->getInputSystem()->addButtonStatusCallback(ButtonData{ INNO_KEY_B, ButtonStatus::PRESSED }, &f_bakeGI);

	g_pCoreSystem->getFileSystem()->addSceneLoadingStartCallback(&f_sceneLoadingStartCallback);
	g_pCoreSystem->getFileSystem()->addSceneLoadingFinishCallback(&f_sceneLoadingFinishCallback);

	return true;
}

bool InnoRenderingFrontendSystemNS::initialize()
{
	initializeHaltonSampler();
	m_objectStatus = ObjectStatus::ALIVE;

	g_pCoreSystem->getLogSystem()->printLog(LogType::INNO_DEV_SUCCESS, "RenderingFrontendSystem has been initialized.");

	return true;
}

bool InnoRenderingFrontendSystemNS::updateCameraData()
{
	m_isCameraDataPackValid = false;

	auto l_cameraComponents = g_pCoreSystem->getGameSystem()->get<CameraComponent>();
	auto l_mainCamera = l_cameraComponents[0];
	auto l_mainCameraTransformComponent = g_pCoreSystem->getGameSystem()->get<TransformComponent>(l_mainCamera->m_parentEntity);

	auto l_p = l_mainCamera->m_projectionMatrix;

	RenderingFrontendSystemComponent::get().m_cameraGPUData.p_original = l_p;
	RenderingFrontendSystemComponent::get().m_cameraGPUData.p_jittered = l_p;

	if (m_renderingConfig.useTAA)
	{
		//TAA jitter for projection matrix
		auto& l_currentHaltonStep = currentHaltonStep;
		if (l_currentHaltonStep >= 16)
		{
			l_currentHaltonStep = 0;
		}
		RenderingFrontendSystemComponent::get().m_cameraGPUData.p_jittered.m02 = m_haltonSampler[l_currentHaltonStep].x / m_screenResolution.x;
		RenderingFrontendSystemComponent::get().m_cameraGPUData.p_jittered.m12 = m_haltonSampler[l_currentHaltonStep].y / m_screenResolution.y;
		l_currentHaltonStep += 1;
	}

	RenderingFrontendSystemComponent::get().m_cameraGPUData.r =
		InnoMath::getInvertRotationMatrix(
			l_mainCameraTransformComponent->m_globalTransformVector.m_rot
		);

	RenderingFrontendSystemComponent::get().m_cameraGPUData.t =
		InnoMath::getInvertTranslationMatrix(
			l_mainCameraTransformComponent->m_globalTransformVector.m_pos
		);

	RenderingFrontendSystemComponent::get().m_cameraGPUData.r_prev = l_mainCameraTransformComponent->m_globalTransformMatrix_prev.m_rotationMat.inverse();
	RenderingFrontendSystemComponent::get().m_cameraGPUData.t_prev = l_mainCameraTransformComponent->m_globalTransformMatrix_prev.m_translationMat.inverse();

	RenderingFrontendSystemComponent::get().m_cameraGPUData.globalPos = l_mainCameraTransformComponent->m_globalTransformVector.m_pos;

	RenderingFrontendSystemComponent::get().m_cameraGPUData.WHRatio = l_mainCamera->m_WHRatio;

	m_isCameraDataPackValid = true;

	return true;
}

bool InnoRenderingFrontendSystemNS::updateSunData()
{
	m_isSunDataPackValid = false;
	m_isCSMDataPackValid = false;

	auto l_directionalLightComponents = g_pCoreSystem->getGameSystem()->get<DirectionalLightComponent>();
	auto l_directionalLight = l_directionalLightComponents[0];
	auto l_directionalLightTransformComponent = g_pCoreSystem->getGameSystem()->get<TransformComponent>(l_directionalLight->m_parentEntity);

	RenderingFrontendSystemComponent::get().m_sunGPUData.dir = InnoMath::getDirection(direction::BACKWARD, l_directionalLightTransformComponent->m_globalTransformVector.m_rot);
	RenderingFrontendSystemComponent::get().m_sunGPUData.luminance = l_directionalLight->m_color * l_directionalLight->m_luminousFlux;
	RenderingFrontendSystemComponent::get().m_sunGPUData.r = InnoMath::getInvertRotationMatrix(l_directionalLightTransformComponent->m_globalTransformVector.m_rot);

	auto l_CSMSize = l_directionalLight->m_projectionMatrices.size();

	RenderingFrontendSystemComponent::get().m_CSMGPUDataVector.clear();

	for (size_t j = 0; j < l_directionalLight->m_projectionMatrices.size(); j++)
	{
		RenderingFrontendSystemComponent::get().m_CSMGPUDataVector.emplace_back();

		auto l_shadowSplitCorner = vec4(
			l_directionalLight->m_AABBsInWorldSpace[j].m_boundMin.x,
			l_directionalLight->m_AABBsInWorldSpace[j].m_boundMin.z,
			l_directionalLight->m_AABBsInWorldSpace[j].m_boundMax.x,
			l_directionalLight->m_AABBsInWorldSpace[j].m_boundMax.z
		);

		RenderingFrontendSystemComponent::get().m_CSMGPUDataVector[j].p = l_directionalLight->m_projectionMatrices[j];
		RenderingFrontendSystemComponent::get().m_CSMGPUDataVector[j].splitCorners = l_shadowSplitCorner;

		auto l_lightRotMat = l_directionalLightTransformComponent->m_globalTransformMatrix.m_rotationMat.inverse();

		RenderingFrontendSystemComponent::get().m_CSMGPUDataVector[j].v = l_lightRotMat;
	}

	m_isCSMDataPackValid = true;
	m_isSunDataPackValid = true;

	return true;
}

bool InnoRenderingFrontendSystemNS::updateLightData()
{
	auto& l_pointLightComponents = g_pCoreSystem->getGameSystem()->get<PointLightComponent>();
	for (size_t i = 0; i < l_pointLightComponents.size(); i++)
	{
		PointLightGPUData l_PointLightGPUData;
		l_PointLightGPUData.pos = g_pCoreSystem->getGameSystem()->get<TransformComponent>(l_pointLightComponents[i]->m_parentEntity)->m_globalTransformVector.m_pos;
		l_PointLightGPUData.luminance = l_pointLightComponents[i]->m_color * l_pointLightComponents[i]->m_luminousFlux;
		l_PointLightGPUData.luminance.w = l_pointLightComponents[i]->m_attenuationRadius;
		RenderingFrontendSystemComponent::get().m_pointLightGPUDataVector[i] = l_PointLightGPUData;
	}

	auto& l_sphereLightComponents = g_pCoreSystem->getGameSystem()->get<SphereLightComponent>();
	for (size_t i = 0; i < l_sphereLightComponents.size(); i++)
	{
		SphereLightGPUData l_SphereLightGPUData;
		l_SphereLightGPUData.pos = g_pCoreSystem->getGameSystem()->get<TransformComponent>(l_sphereLightComponents[i]->m_parentEntity)->m_globalTransformVector.m_pos;
		l_SphereLightGPUData.luminance = l_sphereLightComponents[i]->m_color * l_sphereLightComponents[i]->m_luminousFlux;
		l_SphereLightGPUData.luminance.w = l_sphereLightComponents[i]->m_sphereRadius;
		RenderingFrontendSystemComponent::get().m_sphereLightGPUDataVector[i] = l_SphereLightGPUData;
	}

	return true;
}

bool InnoRenderingFrontendSystemNS::updateMeshData()
{
	m_isMeshDataPackValid = false;

	RenderingFrontendSystemComponent::get().m_opaquePassGPUDataQueue.clear();
	RenderingFrontendSystemComponent::get().m_transparentPassGPUDataQueue.clear();

	std::vector<GeometryPassGPUData> l_sortedTransparentPassGPUDataVector;

	for (auto& i : m_cullingDataPack)
	{
		if (i.mesh != nullptr)
		{
			if (i.mesh->m_objectStatus == ObjectStatus::ALIVE)
			{
				if (i.material != nullptr)
				{
					GeometryPassGPUData l_geometryPassGPUData;

					l_geometryPassGPUData.MDC = i.mesh;

					l_geometryPassGPUData.meshGPUData.m = i.m;
					l_geometryPassGPUData.meshGPUData.m_prev = i.m_prev;
					l_geometryPassGPUData.meshGPUData.normalMat = i.normalMat;
					l_geometryPassGPUData.meshGPUData.UUID = (float)i.UUID;

					l_geometryPassGPUData.normalTDC = i.material->m_texturePack.m_normalTDC.second;
					l_geometryPassGPUData.albedoTDC = i.material->m_texturePack.m_albedoTDC.second;
					l_geometryPassGPUData.metallicTDC = i.material->m_texturePack.m_metallicTDC.second;
					l_geometryPassGPUData.roughnessTDC = i.material->m_texturePack.m_roughnessTDC.second;
					l_geometryPassGPUData.AOTDC = i.material->m_texturePack.m_aoTDC.second;

					l_geometryPassGPUData.materialGPUData.useNormalTexture = !(l_geometryPassGPUData.normalTDC == nullptr);
					l_geometryPassGPUData.materialGPUData.useAlbedoTexture = !(l_geometryPassGPUData.albedoTDC == nullptr);
					l_geometryPassGPUData.materialGPUData.useMetallicTexture = !(l_geometryPassGPUData.metallicTDC == nullptr);
					l_geometryPassGPUData.materialGPUData.useRoughnessTexture = !(l_geometryPassGPUData.roughnessTDC == nullptr);
					l_geometryPassGPUData.materialGPUData.useAOTexture = !(l_geometryPassGPUData.AOTDC == nullptr);

					l_geometryPassGPUData.materialGPUData.customMaterial = i.material->m_meshCustomMaterial;

					if (i.visiblilityType == VisiblilityType::INNO_OPAQUE)
					{
						RenderingFrontendSystemComponent::get().m_opaquePassGPUDataQueue.push(l_geometryPassGPUData);
					}

					else if (i.visiblilityType == VisiblilityType::INNO_TRANSPARENT)
					{
						l_sortedTransparentPassGPUDataVector.emplace_back(l_geometryPassGPUData);
					}
				}
			}
		}
	}

	// @TODO: use GPU to do OIT
	std::sort(l_sortedTransparentPassGPUDataVector.begin(), l_sortedTransparentPassGPUDataVector.end(), [](GeometryPassGPUData a, GeometryPassGPUData b) {
		auto l_t = RenderingFrontendSystemComponent::get().m_cameraGPUData.t;
		auto l_r = RenderingFrontendSystemComponent::get().m_cameraGPUData.r;
		auto m_a_InViewSpace = l_t * l_r * a.meshGPUData.m;
		auto m_b_InViewSpace = l_t * l_r * b.meshGPUData.m;
		return m_a_InViewSpace.m23 < m_b_InViewSpace.m23;
	});

	for (auto i : l_sortedTransparentPassGPUDataVector)
	{
		RenderingFrontendSystemComponent::get().m_transparentPassGPUDataQueue.push(i);
	}

	m_isMeshDataPackValid = true;

	return true;
}

bool InnoRenderingFrontendSystemNS::updateBillboardPassData()
{
	RenderingFrontendSystemComponent::get().m_billboardPassGPUDataQueue.clear();

	for (auto i : g_pCoreSystem->getGameSystem()->get<DirectionalLightComponent>())
	{
		BillboardPassGPUData l_billboardPAssGPUData;
		l_billboardPAssGPUData.globalPos = g_pCoreSystem->getGameSystem()->get<TransformComponent>(i->m_parentEntity)->m_globalTransformVector.m_pos;
		l_billboardPAssGPUData.distanceToCamera = (RenderingFrontendSystemComponent::get().m_cameraGPUData.globalPos - l_billboardPAssGPUData.globalPos).length();
		l_billboardPAssGPUData.iconType = WorldEditorIconType::DIRECTIONAL_LIGHT;

		RenderingFrontendSystemComponent::get().m_billboardPassGPUDataQueue.push(l_billboardPAssGPUData);
	}

	for (auto i : g_pCoreSystem->getGameSystem()->get<PointLightComponent>())
	{
		BillboardPassGPUData l_billboardPAssGPUData;
		l_billboardPAssGPUData.globalPos = g_pCoreSystem->getGameSystem()->get<TransformComponent>(i->m_parentEntity)->m_globalTransformVector.m_pos;
		l_billboardPAssGPUData.distanceToCamera = (RenderingFrontendSystemComponent::get().m_cameraGPUData.globalPos - l_billboardPAssGPUData.globalPos).length();
		l_billboardPAssGPUData.iconType = WorldEditorIconType::POINT_LIGHT;

		RenderingFrontendSystemComponent::get().m_billboardPassGPUDataQueue.push(l_billboardPAssGPUData);
	}

	for (auto i : g_pCoreSystem->getGameSystem()->get<SphereLightComponent>())
	{
		BillboardPassGPUData l_billboardPAssGPUData;
		l_billboardPAssGPUData.globalPos = g_pCoreSystem->getGameSystem()->get<TransformComponent>(i->m_parentEntity)->m_globalTransformVector.m_pos;
		l_billboardPAssGPUData.distanceToCamera = (RenderingFrontendSystemComponent::get().m_cameraGPUData.globalPos - l_billboardPAssGPUData.globalPos).length();
		l_billboardPAssGPUData.iconType = WorldEditorIconType::SPHERE_LIGHT;

		RenderingFrontendSystemComponent::get().m_billboardPassGPUDataQueue.push(l_billboardPAssGPUData);
	}

	return true;
}

bool InnoRenderingFrontendSystemNS::updateDebuggerPassData()
{
	return true;
}

bool InnoRenderingFrontendSystemNS::gatherStaticMeshData()
{
	RenderingFrontendSystemComponent::get().m_GIPassGPUDataQueue.clear();

	for (auto visibleComponent : g_pCoreSystem->getGameSystem()->get<VisibleComponent>())
	{
		if (visibleComponent->m_visiblilityType != VisiblilityType::INNO_INVISIBLE
			&& visibleComponent->m_objectStatus == ObjectStatus::ALIVE
			&& visibleComponent->m_meshUsageType == MeshUsageType::STATIC
			)
		{
			auto l_transformComponent = g_pCoreSystem->getGameSystem()->get<TransformComponent>(visibleComponent->m_parentEntity);
			auto l_globalTm = l_transformComponent->m_globalTransformMatrix.m_transformationMat;
			if (visibleComponent->m_PDC)
			{
				for (auto& l_modelPair : visibleComponent->m_modelMap)
				{
					GeometryPassGPUData l_geometryPassGPUData;

					l_geometryPassGPUData.MDC = l_modelPair.first;

					l_geometryPassGPUData.meshGPUData.m = l_transformComponent->m_globalTransformMatrix.m_transformationMat;
					l_geometryPassGPUData.meshGPUData.m_prev = l_transformComponent->m_globalTransformMatrix_prev.m_transformationMat;
					l_geometryPassGPUData.meshGPUData.normalMat = l_transformComponent->m_globalTransformMatrix.m_rotationMat;
					l_geometryPassGPUData.meshGPUData.UUID = (float)visibleComponent->m_UUID;

					l_geometryPassGPUData.normalTDC = l_modelPair.second->m_texturePack.m_normalTDC.second;
					l_geometryPassGPUData.albedoTDC = l_modelPair.second->m_texturePack.m_albedoTDC.second;
					l_geometryPassGPUData.metallicTDC = l_modelPair.second->m_texturePack.m_metallicTDC.second;
					l_geometryPassGPUData.roughnessTDC = l_modelPair.second->m_texturePack.m_roughnessTDC.second;
					l_geometryPassGPUData.AOTDC = l_modelPair.second->m_texturePack.m_aoTDC.second;

					l_geometryPassGPUData.materialGPUData.useNormalTexture = !(l_geometryPassGPUData.normalTDC == nullptr);
					l_geometryPassGPUData.materialGPUData.useAlbedoTexture = !(l_geometryPassGPUData.albedoTDC == nullptr);
					l_geometryPassGPUData.materialGPUData.useMetallicTexture = !(l_geometryPassGPUData.metallicTDC == nullptr);
					l_geometryPassGPUData.materialGPUData.useRoughnessTexture = !(l_geometryPassGPUData.roughnessTDC == nullptr);
					l_geometryPassGPUData.materialGPUData.useAOTexture = !(l_geometryPassGPUData.AOTDC == nullptr);

					l_geometryPassGPUData.materialGPUData.customMaterial = l_modelPair.second->m_meshCustomMaterial;

					RenderingFrontendSystemComponent::get().m_GIPassGPUDataQueue.push(l_geometryPassGPUData);
				}
			}
		}
	}

	return true;
}

bool InnoRenderingFrontendSystemNS::update()
{
	updateCameraData();

	updateSunData();

	updateLightData();

	// copy culling data pack for local scope
	auto l_cullingDataPack = g_pCoreSystem->getPhysicsSystem()->getCullingDataPack();
	if (l_cullingDataPack.has_value() && l_cullingDataPack.value().size() > 0)
	{
		m_cullingDataPack.setRawData(std::move(l_cullingDataPack.value()));
	}

	updateMeshData();

	updateBillboardPassData();

	updateDebuggerPassData();

	RenderingFrontendSystemComponent::get().m_skyGPUData.p_inv = RenderingFrontendSystemComponent::get().m_cameraGPUData.p_original.inverse();
	RenderingFrontendSystemComponent::get().m_skyGPUData.r_inv = RenderingFrontendSystemComponent::get().m_cameraGPUData.r.inverse();
	RenderingFrontendSystemComponent::get().m_skyGPUData.viewportSize.x = (float)m_screenResolution.x;
	RenderingFrontendSystemComponent::get().m_skyGPUData.viewportSize.y = (float)m_screenResolution.y;

	return true;
}

bool InnoRenderingFrontendSystemNS::terminate()
{
	m_objectStatus = ObjectStatus::SHUTDOWN;
	g_pCoreSystem->getLogSystem()->printLog(LogType::INNO_DEV_SUCCESS, "RenderingFrontendSystem has been terminated.");
	return true;
}

INNO_SYSTEM_EXPORT bool InnoRenderingFrontendSystem::setup(IRenderingBackendSystem* renderingBackendSystem)
{
	return InnoRenderingFrontendSystemNS::setup(renderingBackendSystem);
}

INNO_SYSTEM_EXPORT bool InnoRenderingFrontendSystem::initialize()
{
	return InnoRenderingFrontendSystemNS::initialize();
}

INNO_SYSTEM_EXPORT bool InnoRenderingFrontendSystem::update()
{
	return InnoRenderingFrontendSystemNS::update();
}

INNO_SYSTEM_EXPORT bool InnoRenderingFrontendSystem::terminate()
{
	return InnoRenderingFrontendSystemNS::terminate();
}

INNO_SYSTEM_EXPORT ObjectStatus InnoRenderingFrontendSystem::getStatus()
{
	return InnoRenderingFrontendSystemNS::m_objectStatus;
}

INNO_SYSTEM_EXPORT MeshDataComponent * InnoRenderingFrontendSystem::addMeshDataComponent()
{
	return InnoRenderingFrontendSystemNS::m_renderingBackendSystem->addMeshDataComponent();
}

INNO_SYSTEM_EXPORT MaterialDataComponent * InnoRenderingFrontendSystem::addMaterialDataComponent()
{
	return InnoRenderingFrontendSystemNS::m_renderingBackendSystem->addMaterialDataComponent();
}

INNO_SYSTEM_EXPORT TextureDataComponent * InnoRenderingFrontendSystem::addTextureDataComponent()
{
	return InnoRenderingFrontendSystemNS::m_renderingBackendSystem->addTextureDataComponent();
}

INNO_SYSTEM_EXPORT MeshDataComponent * InnoRenderingFrontendSystem::getMeshDataComponent(EntityID meshID)
{
	return InnoRenderingFrontendSystemNS::m_renderingBackendSystem->getMeshDataComponent(meshID);
}

INNO_SYSTEM_EXPORT TextureDataComponent * InnoRenderingFrontendSystem::getTextureDataComponent(EntityID textureID)
{
	return InnoRenderingFrontendSystemNS::m_renderingBackendSystem->getTextureDataComponent(textureID);
}

INNO_SYSTEM_EXPORT MeshDataComponent * InnoRenderingFrontendSystem::getMeshDataComponent(MeshShapeType meshShapeType)
{
	return InnoRenderingFrontendSystemNS::m_renderingBackendSystem->getMeshDataComponent(meshShapeType);
}

INNO_SYSTEM_EXPORT TextureDataComponent * InnoRenderingFrontendSystem::getTextureDataComponent(TextureUsageType textureUsageType)
{
	return InnoRenderingFrontendSystemNS::m_renderingBackendSystem->getTextureDataComponent(textureUsageType);
}

INNO_SYSTEM_EXPORT TextureDataComponent * InnoRenderingFrontendSystem::getTextureDataComponent(FileExplorerIconType iconType)
{
	return InnoRenderingFrontendSystemNS::m_renderingBackendSystem->getTextureDataComponent(iconType);
}

INNO_SYSTEM_EXPORT TextureDataComponent * InnoRenderingFrontendSystem::getTextureDataComponent(WorldEditorIconType iconType)
{
	return InnoRenderingFrontendSystemNS::m_renderingBackendSystem->getTextureDataComponent(iconType);
}

INNO_SYSTEM_EXPORT bool InnoRenderingFrontendSystem::removeMeshDataComponent(EntityID entityID)
{
	return InnoRenderingFrontendSystemNS::m_renderingBackendSystem->removeMeshDataComponent(entityID);
}

INNO_SYSTEM_EXPORT bool InnoRenderingFrontendSystem::removeTextureDataComponent(EntityID entityID)
{
	return InnoRenderingFrontendSystemNS::m_renderingBackendSystem->removeTextureDataComponent(entityID);
}

INNO_SYSTEM_EXPORT TVec2<unsigned int> InnoRenderingFrontendSystem::getScreenResolution()
{
	return InnoRenderingFrontendSystemNS::m_screenResolution;
}

INNO_SYSTEM_EXPORT bool InnoRenderingFrontendSystem::setScreenResolution(TVec2<unsigned int> screenResolution)
{
	InnoRenderingFrontendSystemNS::m_screenResolution = screenResolution;
	return true;
}

INNO_SYSTEM_EXPORT RenderingConfig InnoRenderingFrontendSystem::getRenderingConfig()
{
	return InnoRenderingFrontendSystemNS::m_renderingConfig;
}

INNO_SYSTEM_EXPORT bool InnoRenderingFrontendSystem::setRenderingConfig(RenderingConfig renderingConfig)
{
	InnoRenderingFrontendSystemNS::m_renderingConfig = renderingConfig;
	return true;
}