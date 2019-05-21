#include "RenderingFrontendSystem.h"
#include "../component/RenderingFrontendSystemComponent.h"

#include "ICoreSystem.h"

extern ICoreSystem* g_pCoreSystem;

INNO_PRIVATE_SCOPE InnoRenderingFrontendSystemNS
{
	ObjectStatus m_objectStatus = ObjectStatus::Terminated;

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

		RenderingFrontendSystemComponent::get().m_opaquePassGPUDatas.clear();
		RenderingFrontendSystemComponent::get().m_opaquePassMeshGPUDatas.clear();
		RenderingFrontendSystemComponent::get().m_opaquePassMaterialGPUDatas.clear();

		RenderingFrontendSystemComponent::get().m_transparentPassGPUDatas.clear();
		RenderingFrontendSystemComponent::get().m_transparentPassMeshGPUDatas.clear();
		RenderingFrontendSystemComponent::get().m_transparentPassMaterialGPUDatas.clear();

		RenderingFrontendSystemComponent::get().m_billboardPassGPUDataQueue.clear();
		RenderingFrontendSystemComponent::get().m_debuggerPassGPUDataQueue.clear();

		RenderingFrontendSystemComponent::get().m_GIPassGPUDatas.clear();
		RenderingFrontendSystemComponent::get().m_GIPassMeshGPUDatas.clear();
		RenderingFrontendSystemComponent::get().m_GIPassMaterialGPUDatas.clear();

		m_isCSMDataPackValid = false;
		m_isSunDataPackValid = false;
		m_isCameraDataPackValid = false;
		m_isMeshDataPackValid = false;
	};

	f_sceneLoadingFinishCallback = [&]() {
		RenderingFrontendSystemComponent::get().m_opaquePassGPUDatas.resize(RenderingFrontendSystemComponent::get().m_maxMeshes);
		RenderingFrontendSystemComponent::get().m_opaquePassMeshGPUDatas.resize(RenderingFrontendSystemComponent::get().m_maxMeshes);
		RenderingFrontendSystemComponent::get().m_opaquePassMaterialGPUDatas.resize(RenderingFrontendSystemComponent::get().m_maxMaterials);

		RenderingFrontendSystemComponent::get().m_transparentPassGPUDatas.resize(RenderingFrontendSystemComponent::get().m_maxMeshes);
		RenderingFrontendSystemComponent::get().m_transparentPassMeshGPUDatas.resize(RenderingFrontendSystemComponent::get().m_maxMeshes);
		RenderingFrontendSystemComponent::get().m_transparentPassMaterialGPUDatas.resize(RenderingFrontendSystemComponent::get().m_maxMaterials);

		RenderingFrontendSystemComponent::get().m_pointLightGPUDataVector.resize(RenderingFrontendSystemComponent::get().m_maxPointLights);
		RenderingFrontendSystemComponent::get().m_sphereLightGPUDataVector.resize(RenderingFrontendSystemComponent::get().m_maxSphereLights);

		RenderingFrontendSystemComponent::get().m_GIPassGPUDatas.resize(RenderingFrontendSystemComponent::get().m_maxMeshes);
		RenderingFrontendSystemComponent::get().m_GIPassMeshGPUDatas.resize(RenderingFrontendSystemComponent::get().m_maxMeshes);
		RenderingFrontendSystemComponent::get().m_GIPassMaterialGPUDatas.resize(RenderingFrontendSystemComponent::get().m_maxMaterials);
	};

	f_bakeGI = []() {
		gatherStaticMeshData();
		m_renderingBackendSystem->bakeGI();
	};
	g_pCoreSystem->getInputSystem()->addButtonStatusCallback(ButtonData{ INNO_KEY_B, ButtonStatus::PRESSED }, &f_bakeGI);

	g_pCoreSystem->getFileSystem()->addSceneLoadingStartCallback(&f_sceneLoadingStartCallback);
	g_pCoreSystem->getFileSystem()->addSceneLoadingFinishCallback(&f_sceneLoadingFinishCallback);

	InnoRenderingFrontendSystemNS::m_objectStatus = ObjectStatus::Created;
	return true;
}

bool InnoRenderingFrontendSystemNS::initialize()
{
	if (InnoRenderingFrontendSystemNS::m_objectStatus == ObjectStatus::Created)
	{
		initializeHaltonSampler();

		InnoRenderingFrontendSystemNS::m_objectStatus = ObjectStatus::Activated;
		g_pCoreSystem->getLogSystem()->printLog(LogType::INNO_DEV_SUCCESS, "RenderingFrontendSystem has been initialized.");
		return true;
	}
	else
	{
		g_pCoreSystem->getLogSystem()->printLog(LogType::INNO_ERROR, "RenderingFrontendSystem: Object is not created!");
		return false;
	}
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

	unsigned int l_opaquePassIndex = 0;
	unsigned int l_transparentPassIndex = 0;

	std::vector<TransparentPassGPUData> l_sortedTransparentPassGPUDatas;

	for (size_t i = 0; i < m_cullingDataPack.size(); i++)
	{
		auto l_cullingData = m_cullingDataPack[i];
		if (l_cullingData.mesh != nullptr)
		{
			if (l_cullingData.mesh->m_objectStatus == ObjectStatus::Activated)
			{
				if (l_cullingData.material != nullptr)
				{
					MeshGPUData l_meshGPUData;
					l_meshGPUData.m = l_cullingData.m;
					l_meshGPUData.m_prev = l_cullingData.m_prev;
					l_meshGPUData.normalMat = l_cullingData.normalMat;
					l_meshGPUData.UUID = (float)l_cullingData.UUID;

					if (l_cullingData.visiblilityType == VisiblilityType::INNO_OPAQUE)
					{
						OpaquePassGPUData l_opaquePassGPUData;

						l_opaquePassGPUData.MDC = l_cullingData.mesh;
						l_opaquePassGPUData.normalTDC = l_cullingData.material->m_texturePack.m_normalTDC.second;
						l_opaquePassGPUData.albedoTDC = l_cullingData.material->m_texturePack.m_albedoTDC.second;
						l_opaquePassGPUData.metallicTDC = l_cullingData.material->m_texturePack.m_metallicTDC.second;
						l_opaquePassGPUData.roughnessTDC = l_cullingData.material->m_texturePack.m_roughnessTDC.second;
						l_opaquePassGPUData.AOTDC = l_cullingData.material->m_texturePack.m_aoTDC.second;

						MaterialGPUData l_materialGPUData;

						l_materialGPUData.useNormalTexture = !(l_opaquePassGPUData.normalTDC == nullptr);
						l_materialGPUData.useAlbedoTexture = !(l_opaquePassGPUData.albedoTDC == nullptr);
						l_materialGPUData.useMetallicTexture = !(l_opaquePassGPUData.metallicTDC == nullptr);
						l_materialGPUData.useRoughnessTexture = !(l_opaquePassGPUData.roughnessTDC == nullptr);
						l_materialGPUData.useAOTexture = !(l_opaquePassGPUData.AOTDC == nullptr);
						l_materialGPUData.customMaterial = l_cullingData.material->m_meshCustomMaterial;

						RenderingFrontendSystemComponent::get().m_opaquePassGPUDatas[l_opaquePassIndex] = l_opaquePassGPUData;
						RenderingFrontendSystemComponent::get().m_opaquePassMeshGPUDatas[l_opaquePassIndex] = l_meshGPUData;
						RenderingFrontendSystemComponent::get().m_opaquePassMaterialGPUDatas[l_opaquePassIndex] = l_materialGPUData;
						l_opaquePassIndex++;
					}
					else if (l_cullingData.visiblilityType == VisiblilityType::INNO_TRANSPARENT)
					{
						TransparentPassGPUData l_transparentPassGPUData;

						l_transparentPassGPUData.MDC = l_cullingData.mesh;
						l_transparentPassGPUData.meshGPUDataIndex = l_transparentPassIndex;
						l_transparentPassGPUData.materialGPUDataIndex = l_transparentPassIndex;

						MaterialGPUData l_materialGPUData;

						l_materialGPUData.useNormalTexture = false;
						l_materialGPUData.useAlbedoTexture = false;
						l_materialGPUData.useMetallicTexture = false;
						l_materialGPUData.useRoughnessTexture = false;
						l_materialGPUData.useAOTexture = false;
						l_materialGPUData.customMaterial = l_cullingData.material->m_meshCustomMaterial;

						l_sortedTransparentPassGPUDatas.emplace_back(l_transparentPassGPUData);
						RenderingFrontendSystemComponent::get().m_transparentPassMeshGPUDatas[l_transparentPassIndex] = l_meshGPUData;
						RenderingFrontendSystemComponent::get().m_transparentPassMaterialGPUDatas[l_transparentPassIndex] = l_materialGPUData;
						l_transparentPassIndex++;
					}
				}
			}
		}
	}

	RenderingFrontendSystemComponent::get().m_opaquePassDrawcallCount = l_opaquePassIndex;
	RenderingFrontendSystemComponent::get().m_transparentPassDrawcallCount = l_transparentPassIndex;

	// @TODO: use GPU to do OIT
	std::sort(l_sortedTransparentPassGPUDatas.begin(), l_sortedTransparentPassGPUDatas.end(), [&](TransparentPassGPUData a, TransparentPassGPUData b) {
		auto l_t = RenderingFrontendSystemComponent::get().m_cameraGPUData.t;
		auto l_r = RenderingFrontendSystemComponent::get().m_cameraGPUData.r;
		auto m_a_InViewSpace = l_t * l_r * RenderingFrontendSystemComponent::get().m_transparentPassMeshGPUDatas[a.meshGPUDataIndex].m;
		auto m_b_InViewSpace = l_t * l_r * RenderingFrontendSystemComponent::get().m_transparentPassMeshGPUDatas[b.meshGPUDataIndex].m;
		return m_a_InViewSpace.m23 < m_b_InViewSpace.m23;
	});

	RenderingFrontendSystemComponent::get().m_transparentPassGPUDatas = l_sortedTransparentPassGPUDatas;

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
	unsigned int l_index = 0;

	for (auto visibleComponent : g_pCoreSystem->getGameSystem()->get<VisibleComponent>())
	{
		if (visibleComponent->m_visiblilityType == VisiblilityType::INNO_OPAQUE
			&& visibleComponent->m_objectStatus == ObjectStatus::Activated
			&& visibleComponent->m_meshUsageType == MeshUsageType::STATIC
			)
		{
			auto l_transformComponent = g_pCoreSystem->getGameSystem()->get<TransformComponent>(visibleComponent->m_parentEntity);
			auto l_globalTm = l_transformComponent->m_globalTransformMatrix.m_transformationMat;
			if (visibleComponent->m_PDC)
			{
				for (auto& l_modelPair : visibleComponent->m_modelMap)
				{
					OpaquePassGPUData l_GIPassGPUData;

					l_GIPassGPUData.MDC = l_modelPair.first;
					l_GIPassGPUData.normalTDC = l_modelPair.second->m_texturePack.m_normalTDC.second;
					l_GIPassGPUData.albedoTDC = l_modelPair.second->m_texturePack.m_albedoTDC.second;
					l_GIPassGPUData.metallicTDC = l_modelPair.second->m_texturePack.m_metallicTDC.second;
					l_GIPassGPUData.roughnessTDC = l_modelPair.second->m_texturePack.m_roughnessTDC.second;
					l_GIPassGPUData.AOTDC = l_modelPair.second->m_texturePack.m_aoTDC.second;

					MeshGPUData l_meshGPUData;

					l_meshGPUData.m = l_transformComponent->m_globalTransformMatrix.m_transformationMat;
					l_meshGPUData.m_prev = l_transformComponent->m_globalTransformMatrix_prev.m_transformationMat;
					l_meshGPUData.normalMat = l_transformComponent->m_globalTransformMatrix.m_rotationMat;
					l_meshGPUData.UUID = (float)visibleComponent->m_UUID;

					MaterialGPUData l_materialGPUData;

					l_materialGPUData.useNormalTexture = !(l_GIPassGPUData.normalTDC == nullptr);
					l_materialGPUData.useAlbedoTexture = !(l_GIPassGPUData.albedoTDC == nullptr);
					l_materialGPUData.useMetallicTexture = !(l_GIPassGPUData.metallicTDC == nullptr);
					l_materialGPUData.useRoughnessTexture = !(l_GIPassGPUData.roughnessTDC == nullptr);
					l_materialGPUData.useAOTexture = !(l_GIPassGPUData.AOTDC == nullptr);

					l_materialGPUData.customMaterial = l_modelPair.second->m_meshCustomMaterial;

					RenderingFrontendSystemComponent::get().m_GIPassGPUDatas[l_index] = l_GIPassGPUData;
					RenderingFrontendSystemComponent::get().m_GIPassMeshGPUDatas[l_index] = l_meshGPUData;
					RenderingFrontendSystemComponent::get().m_GIPassMaterialGPUDatas[l_index] = l_materialGPUData;
					l_index++;
				}
			}
		}
	}

	RenderingFrontendSystemComponent::get().m_GIPassDrawcallCount = l_index;

	return true;
}

bool InnoRenderingFrontendSystemNS::update()
{
	if (InnoRenderingFrontendSystemNS::m_objectStatus == ObjectStatus::Activated)
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
	else
	{
		InnoRenderingFrontendSystemNS::m_objectStatus = ObjectStatus::Suspended;
		return false;
	}
}

bool InnoRenderingFrontendSystemNS::terminate()
{
	m_objectStatus = ObjectStatus::Terminated;
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