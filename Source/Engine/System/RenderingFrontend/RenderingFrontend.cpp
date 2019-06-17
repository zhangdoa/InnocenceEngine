#include "RenderingFrontend.h"
#include "../../Component/RenderingFrontendComponent.h"

#include "../ICoreSystem.h"

extern ICoreSystem* g_pCoreSystem;

#include "../RayTracer/RayTracer.h"

INNO_PRIVATE_SCOPE InnoRenderingFrontendNS
{
	ObjectStatus m_objectStatus = ObjectStatus::Terminated;

	IRenderingBackend* m_renderingBackend;

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

	void* m_SkeletonDataComponentPool;
	void* m_AnimationDataComponentPool;

	bool setup(IRenderingBackend* renderingBackend);
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

float InnoRenderingFrontendNS::radicalInverse(unsigned int n, unsigned int base)
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

void InnoRenderingFrontendNS::initializeHaltonSampler()
{
	// in NDC space
	for (unsigned int i = 0; i < 16; i++)
	{
		m_haltonSampler.emplace_back(vec2(radicalInverse(i, 3) * 2.0f - 1.0f, radicalInverse(i, 4) * 2.0f - 1.0f));
	}
}

bool InnoRenderingFrontendNS::setup(IRenderingBackend* renderingBackend)
{
	m_renderingBackend = renderingBackend;

	m_renderingConfig.useMotionBlur = true;
	m_renderingConfig.useTAA = true;
	//m_renderingConfig.useBloom = true;
	m_renderingConfig.drawSky = true;
	//m_renderingConfig.drawTerrain = true;
	//m_renderingConfig.drawDebugObject = true;

	f_sceneLoadingStartCallback = [&]() {
		m_cullingDataPack.clear();

		RenderingFrontendComponent::get().m_CSMGPUDataVector.clear();
		RenderingFrontendComponent::get().m_pointLightGPUDataVector.clear();
		RenderingFrontendComponent::get().m_sphereLightGPUDataVector.clear();

		RenderingFrontendComponent::get().m_opaquePassGPUDatas.clear();
		RenderingFrontendComponent::get().m_opaquePassMeshGPUDatas.clear();
		RenderingFrontendComponent::get().m_opaquePassMaterialGPUDatas.clear();

		RenderingFrontendComponent::get().m_transparentPassGPUDatas.clear();
		RenderingFrontendComponent::get().m_transparentPassMeshGPUDatas.clear();
		RenderingFrontendComponent::get().m_transparentPassMaterialGPUDatas.clear();

		RenderingFrontendComponent::get().m_billboardPassGPUDataQueue.clear();
		RenderingFrontendComponent::get().m_debuggerPassGPUDataQueue.clear();

		RenderingFrontendComponent::get().m_GIPassGPUDatas.clear();
		RenderingFrontendComponent::get().m_GIPassMeshGPUDatas.clear();
		RenderingFrontendComponent::get().m_GIPassMaterialGPUDatas.clear();

		m_isCSMDataPackValid = false;
		m_isSunDataPackValid = false;
		m_isCameraDataPackValid = false;
		m_isMeshDataPackValid = false;
	};

	f_sceneLoadingFinishCallback = [&]() {
		RenderingFrontendComponent::get().m_opaquePassGPUDatas.resize(RenderingFrontendComponent::get().m_maxMeshes);
		RenderingFrontendComponent::get().m_opaquePassMeshGPUDatas.resize(RenderingFrontendComponent::get().m_maxMeshes);
		RenderingFrontendComponent::get().m_opaquePassMaterialGPUDatas.resize(RenderingFrontendComponent::get().m_maxMaterials);

		RenderingFrontendComponent::get().m_transparentPassGPUDatas.resize(RenderingFrontendComponent::get().m_maxMeshes);
		RenderingFrontendComponent::get().m_transparentPassMeshGPUDatas.resize(RenderingFrontendComponent::get().m_maxMeshes);
		RenderingFrontendComponent::get().m_transparentPassMaterialGPUDatas.resize(RenderingFrontendComponent::get().m_maxMaterials);

		RenderingFrontendComponent::get().m_pointLightGPUDataVector.resize(RenderingFrontendComponent::get().m_maxPointLights);
		RenderingFrontendComponent::get().m_sphereLightGPUDataVector.resize(RenderingFrontendComponent::get().m_maxSphereLights);

		RenderingFrontendComponent::get().m_GIPassGPUDatas.resize(RenderingFrontendComponent::get().m_maxMeshes);
		RenderingFrontendComponent::get().m_GIPassMeshGPUDatas.resize(RenderingFrontendComponent::get().m_maxMeshes);
		RenderingFrontendComponent::get().m_GIPassMaterialGPUDatas.resize(RenderingFrontendComponent::get().m_maxMaterials);
	};

	f_bakeGI = []() {
		gatherStaticMeshData();
		m_renderingBackend->bakeGI();
	};
	g_pCoreSystem->getInputSystem()->addButtonStatusCallback(ButtonData{ INNO_KEY_B, ButtonStatus::PRESSED }, &f_bakeGI);

	g_pCoreSystem->getFileSystem()->addSceneLoadingStartCallback(&f_sceneLoadingStartCallback);
	g_pCoreSystem->getFileSystem()->addSceneLoadingFinishCallback(&f_sceneLoadingFinishCallback);

	m_SkeletonDataComponentPool = g_pCoreSystem->getMemorySystem()->allocateMemoryPool(sizeof(SkeletonDataComponent), 2048);
	m_AnimationDataComponentPool = g_pCoreSystem->getMemorySystem()->allocateMemoryPool(sizeof(AnimationDataComponent), 16384);

	InnoRenderingFrontendNS::m_objectStatus = ObjectStatus::Created;
	return true;
}

bool InnoRenderingFrontendNS::initialize()
{
	if (InnoRenderingFrontendNS::m_objectStatus == ObjectStatus::Created)
	{
		initializeHaltonSampler();
		auto l_rayTracer = InnoRayTracer();
		l_rayTracer.Setup();
		l_rayTracer.Initialize();

		InnoRenderingFrontendNS::m_objectStatus = ObjectStatus::Activated;
		g_pCoreSystem->getLogSystem()->printLog(LogType::INNO_DEV_SUCCESS, "RenderingFrontend has been initialized.");
		return true;
	}
	else
	{
		g_pCoreSystem->getLogSystem()->printLog(LogType::INNO_ERROR, "RenderingFrontend: Object is not created!");
		return false;
	}
}

bool InnoRenderingFrontendNS::updateCameraData()
{
	m_isCameraDataPackValid = false;

	auto l_cameraComponents = g_pCoreSystem->getGameSystem()->get<CameraComponent>();
	auto l_mainCamera = l_cameraComponents[0];
	auto l_mainCameraTransformComponent = g_pCoreSystem->getGameSystem()->get<TransformComponent>(l_mainCamera->m_parentEntity);

	auto l_p = l_mainCamera->m_projectionMatrix;

	RenderingFrontendComponent::get().m_cameraGPUData.p_original = l_p;
	RenderingFrontendComponent::get().m_cameraGPUData.p_jittered = l_p;

	if (m_renderingConfig.useTAA)
	{
		//TAA jitter for projection matrix
		auto& l_currentHaltonStep = currentHaltonStep;
		if (l_currentHaltonStep >= 16)
		{
			l_currentHaltonStep = 0;
		}
		RenderingFrontendComponent::get().m_cameraGPUData.p_jittered.m02 = m_haltonSampler[l_currentHaltonStep].x / m_screenResolution.x;
		RenderingFrontendComponent::get().m_cameraGPUData.p_jittered.m12 = m_haltonSampler[l_currentHaltonStep].y / m_screenResolution.y;
		l_currentHaltonStep += 1;
	}

	RenderingFrontendComponent::get().m_cameraGPUData.r =
		InnoMath::getInvertRotationMatrix(
			l_mainCameraTransformComponent->m_globalTransformVector.m_rot
		);

	RenderingFrontendComponent::get().m_cameraGPUData.t =
		InnoMath::getInvertTranslationMatrix(
			l_mainCameraTransformComponent->m_globalTransformVector.m_pos
		);

	RenderingFrontendComponent::get().m_cameraGPUData.r_prev = l_mainCameraTransformComponent->m_globalTransformMatrix_prev.m_rotationMat.inverse();
	RenderingFrontendComponent::get().m_cameraGPUData.t_prev = l_mainCameraTransformComponent->m_globalTransformMatrix_prev.m_translationMat.inverse();

	RenderingFrontendComponent::get().m_cameraGPUData.globalPos = l_mainCameraTransformComponent->m_globalTransformVector.m_pos;

	RenderingFrontendComponent::get().m_cameraGPUData.WHRatio = l_mainCamera->m_WHRatio;

	m_isCameraDataPackValid = true;

	return true;
}

bool InnoRenderingFrontendNS::updateSunData()
{
	m_isSunDataPackValid = false;
	m_isCSMDataPackValid = false;

	auto l_directionalLightComponents = g_pCoreSystem->getGameSystem()->get<DirectionalLightComponent>();
	auto l_directionalLight = l_directionalLightComponents[0];
	auto l_directionalLightTransformComponent = g_pCoreSystem->getGameSystem()->get<TransformComponent>(l_directionalLight->m_parentEntity);

	RenderingFrontendComponent::get().m_sunGPUData.dir = InnoMath::getDirection(direction::BACKWARD, l_directionalLightTransformComponent->m_globalTransformVector.m_rot);
	RenderingFrontendComponent::get().m_sunGPUData.luminance = l_directionalLight->m_color * l_directionalLight->m_luminousFlux;
	RenderingFrontendComponent::get().m_sunGPUData.r = InnoMath::getInvertRotationMatrix(l_directionalLightTransformComponent->m_globalTransformVector.m_rot);

	auto l_CSMSize = l_directionalLight->m_projectionMatrices.size();

	RenderingFrontendComponent::get().m_CSMGPUDataVector.clear();

	for (size_t j = 0; j < l_directionalLight->m_projectionMatrices.size(); j++)
	{
		RenderingFrontendComponent::get().m_CSMGPUDataVector.emplace_back();

		auto l_shadowSplitCorner = vec4(
			l_directionalLight->m_AABBsInWorldSpace[j].m_boundMin.x,
			l_directionalLight->m_AABBsInWorldSpace[j].m_boundMin.z,
			l_directionalLight->m_AABBsInWorldSpace[j].m_boundMax.x,
			l_directionalLight->m_AABBsInWorldSpace[j].m_boundMax.z
		);

		RenderingFrontendComponent::get().m_CSMGPUDataVector[j].p = l_directionalLight->m_projectionMatrices[j];
		RenderingFrontendComponent::get().m_CSMGPUDataVector[j].splitCorners = l_shadowSplitCorner;

		auto l_lightRotMat = l_directionalLightTransformComponent->m_globalTransformMatrix.m_rotationMat.inverse();

		RenderingFrontendComponent::get().m_CSMGPUDataVector[j].v = l_lightRotMat;
	}

	m_isCSMDataPackValid = true;
	m_isSunDataPackValid = true;

	return true;
}

bool InnoRenderingFrontendNS::updateLightData()
{
	auto& l_pointLightComponents = g_pCoreSystem->getGameSystem()->get<PointLightComponent>();
	for (size_t i = 0; i < l_pointLightComponents.size(); i++)
	{
		PointLightGPUData l_PointLightGPUData;
		l_PointLightGPUData.pos = g_pCoreSystem->getGameSystem()->get<TransformComponent>(l_pointLightComponents[i]->m_parentEntity)->m_globalTransformVector.m_pos;
		l_PointLightGPUData.luminance = l_pointLightComponents[i]->m_color * l_pointLightComponents[i]->m_luminousFlux;
		l_PointLightGPUData.luminance.w = l_pointLightComponents[i]->m_attenuationRadius;
		RenderingFrontendComponent::get().m_pointLightGPUDataVector[i] = l_PointLightGPUData;
	}

	auto& l_sphereLightComponents = g_pCoreSystem->getGameSystem()->get<SphereLightComponent>();
	for (size_t i = 0; i < l_sphereLightComponents.size(); i++)
	{
		SphereLightGPUData l_SphereLightGPUData;
		l_SphereLightGPUData.pos = g_pCoreSystem->getGameSystem()->get<TransformComponent>(l_sphereLightComponents[i]->m_parentEntity)->m_globalTransformVector.m_pos;
		l_SphereLightGPUData.luminance = l_sphereLightComponents[i]->m_color * l_sphereLightComponents[i]->m_luminousFlux;
		l_SphereLightGPUData.luminance.w = l_sphereLightComponents[i]->m_sphereRadius;
		RenderingFrontendComponent::get().m_sphereLightGPUDataVector[i] = l_SphereLightGPUData;
	}

	return true;
}

bool InnoRenderingFrontendNS::updateMeshData()
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
						l_materialGPUData.materialType = int(l_cullingData.meshUsageType);
						l_materialGPUData.customMaterial = l_cullingData.material->m_meshCustomMaterial;

						RenderingFrontendComponent::get().m_opaquePassGPUDatas[l_opaquePassIndex] = l_opaquePassGPUData;
						RenderingFrontendComponent::get().m_opaquePassMeshGPUDatas[l_opaquePassIndex] = l_meshGPUData;
						RenderingFrontendComponent::get().m_opaquePassMaterialGPUDatas[l_opaquePassIndex] = l_materialGPUData;
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
						l_materialGPUData.materialType = int(l_cullingData.meshUsageType);
						l_materialGPUData.customMaterial = l_cullingData.material->m_meshCustomMaterial;

						l_sortedTransparentPassGPUDatas.emplace_back(l_transparentPassGPUData);
						RenderingFrontendComponent::get().m_transparentPassMeshGPUDatas[l_transparentPassIndex] = l_meshGPUData;
						RenderingFrontendComponent::get().m_transparentPassMaterialGPUDatas[l_transparentPassIndex] = l_materialGPUData;
						l_transparentPassIndex++;
					}
				}
			}
		}
	}

	RenderingFrontendComponent::get().m_opaquePassDrawcallCount = l_opaquePassIndex;
	RenderingFrontendComponent::get().m_transparentPassDrawcallCount = l_transparentPassIndex;

	// @TODO: use GPU to do OIT
	std::sort(l_sortedTransparentPassGPUDatas.begin(), l_sortedTransparentPassGPUDatas.end(), [&](TransparentPassGPUData a, TransparentPassGPUData b) {
		auto l_t = RenderingFrontendComponent::get().m_cameraGPUData.t;
		auto l_r = RenderingFrontendComponent::get().m_cameraGPUData.r;
		auto m_a_InViewSpace = l_t * l_r * RenderingFrontendComponent::get().m_transparentPassMeshGPUDatas[a.meshGPUDataIndex].m;
		auto m_b_InViewSpace = l_t * l_r * RenderingFrontendComponent::get().m_transparentPassMeshGPUDatas[b.meshGPUDataIndex].m;
		return m_a_InViewSpace.m23 < m_b_InViewSpace.m23;
	});

	RenderingFrontendComponent::get().m_transparentPassGPUDatas = l_sortedTransparentPassGPUDatas;

	m_isMeshDataPackValid = true;

	return true;
}

bool InnoRenderingFrontendNS::updateBillboardPassData()
{
	RenderingFrontendComponent::get().m_billboardPassGPUDataQueue.clear();

	for (auto i : g_pCoreSystem->getGameSystem()->get<DirectionalLightComponent>())
	{
		BillboardPassGPUData l_billboardPAssGPUData;
		l_billboardPAssGPUData.globalPos = g_pCoreSystem->getGameSystem()->get<TransformComponent>(i->m_parentEntity)->m_globalTransformVector.m_pos;
		l_billboardPAssGPUData.distanceToCamera = (RenderingFrontendComponent::get().m_cameraGPUData.globalPos - l_billboardPAssGPUData.globalPos).length();
		l_billboardPAssGPUData.iconType = WorldEditorIconType::DIRECTIONAL_LIGHT;

		RenderingFrontendComponent::get().m_billboardPassGPUDataQueue.push(l_billboardPAssGPUData);
	}

	for (auto i : g_pCoreSystem->getGameSystem()->get<PointLightComponent>())
	{
		BillboardPassGPUData l_billboardPAssGPUData;
		l_billboardPAssGPUData.globalPos = g_pCoreSystem->getGameSystem()->get<TransformComponent>(i->m_parentEntity)->m_globalTransformVector.m_pos;
		l_billboardPAssGPUData.distanceToCamera = (RenderingFrontendComponent::get().m_cameraGPUData.globalPos - l_billboardPAssGPUData.globalPos).length();
		l_billboardPAssGPUData.iconType = WorldEditorIconType::POINT_LIGHT;

		RenderingFrontendComponent::get().m_billboardPassGPUDataQueue.push(l_billboardPAssGPUData);
	}

	for (auto i : g_pCoreSystem->getGameSystem()->get<SphereLightComponent>())
	{
		BillboardPassGPUData l_billboardPAssGPUData;
		l_billboardPAssGPUData.globalPos = g_pCoreSystem->getGameSystem()->get<TransformComponent>(i->m_parentEntity)->m_globalTransformVector.m_pos;
		l_billboardPAssGPUData.distanceToCamera = (RenderingFrontendComponent::get().m_cameraGPUData.globalPos - l_billboardPAssGPUData.globalPos).length();
		l_billboardPAssGPUData.iconType = WorldEditorIconType::SPHERE_LIGHT;

		RenderingFrontendComponent::get().m_billboardPassGPUDataQueue.push(l_billboardPAssGPUData);
	}

	return true;
}

bool InnoRenderingFrontendNS::updateDebuggerPassData()
{
	return true;
}

bool InnoRenderingFrontendNS::gatherStaticMeshData()
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

					RenderingFrontendComponent::get().m_GIPassGPUDatas[l_index] = l_GIPassGPUData;
					RenderingFrontendComponent::get().m_GIPassMeshGPUDatas[l_index] = l_meshGPUData;
					RenderingFrontendComponent::get().m_GIPassMaterialGPUDatas[l_index] = l_materialGPUData;
					l_index++;
				}
			}
		}
	}

	RenderingFrontendComponent::get().m_GIPassDrawcallCount = l_index;

	return true;
}

bool InnoRenderingFrontendNS::update()
{
	if (InnoRenderingFrontendNS::m_objectStatus == ObjectStatus::Activated)
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

		RenderingFrontendComponent::get().m_skyGPUData.p_inv = RenderingFrontendComponent::get().m_cameraGPUData.p_original.inverse();
		RenderingFrontendComponent::get().m_skyGPUData.r_inv = RenderingFrontendComponent::get().m_cameraGPUData.r.inverse();
		RenderingFrontendComponent::get().m_skyGPUData.viewportSize.x = (float)m_screenResolution.x;
		RenderingFrontendComponent::get().m_skyGPUData.viewportSize.y = (float)m_screenResolution.y;

		return true;
	}
	else
	{
		InnoRenderingFrontendNS::m_objectStatus = ObjectStatus::Suspended;
		return false;
	}
}

bool InnoRenderingFrontendNS::terminate()
{
	m_objectStatus = ObjectStatus::Terminated;
	g_pCoreSystem->getLogSystem()->printLog(LogType::INNO_DEV_SUCCESS, "RenderingFrontend has been terminated.");
	return true;
}

bool InnoRenderingFrontend::setup(IRenderingBackend* renderingBackend)
{
	return InnoRenderingFrontendNS::setup(renderingBackend);
}

bool InnoRenderingFrontend::initialize()
{
	return InnoRenderingFrontendNS::initialize();
}

bool InnoRenderingFrontend::update()
{
	return InnoRenderingFrontendNS::update();
}

bool InnoRenderingFrontend::terminate()
{
	return InnoRenderingFrontendNS::terminate();
}

ObjectStatus InnoRenderingFrontend::getStatus()
{
	return InnoRenderingFrontendNS::m_objectStatus;
}

MeshDataComponent * InnoRenderingFrontend::addMeshDataComponent()
{
	return InnoRenderingFrontendNS::m_renderingBackend->addMeshDataComponent();
}

MaterialDataComponent * InnoRenderingFrontend::addMaterialDataComponent()
{
	return InnoRenderingFrontendNS::m_renderingBackend->addMaterialDataComponent();
}

TextureDataComponent * InnoRenderingFrontend::addTextureDataComponent()
{
	return InnoRenderingFrontendNS::m_renderingBackend->addTextureDataComponent();
}

MeshDataComponent * InnoRenderingFrontend::getMeshDataComponent(MeshShapeType meshShapeType)
{
	return InnoRenderingFrontendNS::m_renderingBackend->getMeshDataComponent(meshShapeType);
}

TextureDataComponent * InnoRenderingFrontend::getTextureDataComponent(TextureUsageType textureUsageType)
{
	return InnoRenderingFrontendNS::m_renderingBackend->getTextureDataComponent(textureUsageType);
}

TextureDataComponent * InnoRenderingFrontend::getTextureDataComponent(FileExplorerIconType iconType)
{
	return InnoRenderingFrontendNS::m_renderingBackend->getTextureDataComponent(iconType);
}

TextureDataComponent * InnoRenderingFrontend::getTextureDataComponent(WorldEditorIconType iconType)
{
	return InnoRenderingFrontendNS::m_renderingBackend->getTextureDataComponent(iconType);
}

TVec2<unsigned int> InnoRenderingFrontend::getScreenResolution()
{
	return InnoRenderingFrontendNS::m_screenResolution;
}

bool InnoRenderingFrontend::setScreenResolution(TVec2<unsigned int> screenResolution)
{
	InnoRenderingFrontendNS::m_screenResolution = screenResolution;
	return true;
}

RenderingConfig InnoRenderingFrontend::getRenderingConfig()
{
	return InnoRenderingFrontendNS::m_renderingConfig;
}

bool InnoRenderingFrontend::setRenderingConfig(RenderingConfig renderingConfig)
{
	InnoRenderingFrontendNS::m_renderingConfig = renderingConfig;
	return true;
}

SkeletonDataComponent * InnoRenderingFrontend::addSkeletonDataComponent()
{
	static std::atomic<unsigned int> skeletonCount = 0;
	auto l_rawPtr = g_pCoreSystem->getMemorySystem()->spawnObject(InnoRenderingFrontendNS::m_SkeletonDataComponentPool, sizeof(SkeletonDataComponent));
	auto l_SDC = new(l_rawPtr)SkeletonDataComponent();
	auto l_parentEntity = g_pCoreSystem->getGameSystem()->createEntity(EntityName(("Skeleton_" + std::to_string(skeletonCount) + "/").c_str()), ObjectSource::Runtime, ObjectUsage::Engine);
	l_SDC->m_parentEntity = l_parentEntity;
	skeletonCount++;
	return l_SDC;
}

AnimationDataComponent * InnoRenderingFrontend::addAnimationDataComponent()
{
	static std::atomic<unsigned int> animationCount = 0;
	auto l_rawPtr = g_pCoreSystem->getMemorySystem()->spawnObject(InnoRenderingFrontendNS::m_AnimationDataComponentPool, sizeof(AnimationDataComponent));
	auto l_ADC = new(l_rawPtr)AnimationDataComponent();
	auto l_parentEntity = g_pCoreSystem->getGameSystem()->createEntity(EntityName(("Animation_" + std::to_string(animationCount) + "/").c_str()), ObjectSource::Runtime, ObjectUsage::Engine);
	l_ADC->m_parentEntity = l_parentEntity;
	animationCount++;
	return l_ADC;
}