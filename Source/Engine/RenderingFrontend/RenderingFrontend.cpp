#include "RenderingFrontend.h"
#include "../Common/CommonMacro.inl"
#include "../ComponentManager/ITransformComponentManager.h"
#include "../ComponentManager/IVisibleComponentManager.h"
#include "../ComponentManager/IDirectionalLightComponentManager.h"
#include "../ComponentManager/IPointLightComponentManager.h"
#include "../ComponentManager/ISphereLightComponentManager.h"
#include "../ComponentManager/ICameraComponentManager.h"

#include "../ModuleManager/IModuleManager.h"

extern IModuleManager* g_pModuleManager;

#include "../RayTracer/RayTracer.h"

template <typename T>
class DoubleBuffer
{
public:
	DoubleBuffer() = default;
	~DoubleBuffer() = default;

	T GetValue()
	{
		std::lock_guard<std::shared_mutex> lock{ Mutex };
		if (IsAReady)
		{
			return A;
		}
		else
		{
			return B;
		}
	}

	void SetValue(const T& value)
	{
		std::unique_lock<std::shared_mutex>Lock{ Mutex };
		if (IsAReady)
		{
			B = value;
			IsAReady = false;
		}
		else
		{
			A = value;
			IsAReady = true;
		}
	}

private:
	std::atomic<bool> IsAReady = true;
	std::shared_mutex Mutex;
	T A;
	T B;
};

INNO_PRIVATE_SCOPE InnoRenderingFrontendNS
{
	ObjectStatus m_objectStatus = ObjectStatus::Terminated;

	IRenderingBackend* m_renderingBackend;
	IRayTracer* m_rayTracer;

	TVec2<unsigned int> m_screenResolution = TVec2<unsigned int>(1280, 720);
	std::string m_windowName;
	bool m_fullScreen = false;

	RenderingCapability m_renderingCapability;

	CameraGPUData m_cameraGPUData;
	SunGPUData m_sunGPUData;
	std::vector<CSMGPUData> m_CSMGPUData;
	std::vector<PointLightGPUData> m_pointLightGPUData;
	std::vector<SphereLightGPUData> m_sphereLightGPUData;
	SkyGPUData m_skyGPUData;

	unsigned int m_opaquePassDrawCallCount = 0;
	std::vector<OpaquePassGPUData> m_opaquePassGPUData;
	std::vector<MeshGPUData> m_opaquePassMeshGPUData;
	std::vector<MaterialGPUData> m_opaquePassMaterialGPUData;

	unsigned int m_transparentPassDrawCallCount = 0;
	std::vector<TransparentPassGPUData> m_transparentPassGPUData;
	std::vector<MeshGPUData> m_transparentPassMeshGPUData;
	std::vector<MaterialGPUData> m_transparentPassMaterialGPUData;

	unsigned int m_billboardPassDrawCallCount = 0;
	std::vector<BillboardPassGPUData> m_billboardPassGPUData;

	unsigned int m_debuggerPassDrawCallCount = 0;
	std::vector<DebuggerPassGPUData> m_debuggerPassGPUData;

	unsigned int m_GIPassDrawCallCount = 0;
	std::vector<OpaquePassGPUData> m_GIPassGPUData;
	std::vector<MeshGPUData> m_GIPassMeshGPUData;
	std::vector<MaterialGPUData> m_GIPassMaterialGPUData;

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
	m_rayTracer = new InnoRayTracer();

	m_renderingConfig.useMotionBlur = true;
	m_renderingConfig.useTAA = true;
	//m_renderingConfig.useBloom = true;
	m_renderingConfig.drawSky = true;
	//m_renderingConfig.drawTerrain = true;
	//m_renderingConfig.drawDebugObject = true;

	m_renderingCapability.maxCSMSplits = 4;
	m_renderingCapability.maxPointLights = 1024;
	m_renderingCapability.maxSphereLights = 128;
	m_renderingCapability.maxMeshes = 16384;
	m_renderingCapability.maxMaterials = 32768;
	m_renderingCapability.maxTextures = 32768;

	f_sceneLoadingStartCallback = [&]() {
		m_cullingDataPack.clear();

		m_CSMGPUData.clear();
		m_pointLightGPUData.clear();
		m_sphereLightGPUData.clear();

		m_opaquePassGPUData.clear();
		m_opaquePassMeshGPUData.clear();
		m_opaquePassMaterialGPUData.clear();
		m_opaquePassDrawCallCount = 0;

		m_transparentPassGPUData.clear();
		m_transparentPassMeshGPUData.clear();
		m_transparentPassMaterialGPUData.clear();
		m_transparentPassDrawCallCount = 0;

		m_billboardPassGPUData.clear();
		m_debuggerPassGPUData.clear();

		m_GIPassGPUData.clear();
		m_GIPassMeshGPUData.clear();
		m_GIPassMaterialGPUData.clear();
		m_GIPassDrawCallCount = 0;

		m_isCSMDataPackValid = false;
		m_isSunDataPackValid = false;
		m_isCameraDataPackValid = false;
		m_isMeshDataPackValid = false;
	};

	f_sceneLoadingFinishCallback = [&]() {
		m_opaquePassGPUData.resize(m_renderingCapability.maxMeshes);
		m_opaquePassMeshGPUData.resize(m_renderingCapability.maxMeshes);
		m_opaquePassMaterialGPUData.resize(m_renderingCapability.maxMaterials);

		m_transparentPassGPUData.resize(m_renderingCapability.maxMeshes);
		m_transparentPassMeshGPUData.resize(m_renderingCapability.maxMeshes);
		m_transparentPassMaterialGPUData.resize(m_renderingCapability.maxMaterials);

		m_pointLightGPUData.resize(m_renderingCapability.maxPointLights);
		m_sphereLightGPUData.resize(m_renderingCapability.maxSphereLights);

		m_GIPassGPUData.resize(m_renderingCapability.maxMeshes);
		m_GIPassMeshGPUData.resize(m_renderingCapability.maxMeshes);
		m_GIPassMaterialGPUData.resize(m_renderingCapability.maxMaterials);
	};

	f_bakeGI = []() {
		gatherStaticMeshData();
		m_renderingBackend->bakeGI();
	};
	g_pModuleManager->getEventSystem()->addButtonStatusCallback(ButtonData{ INNO_KEY_B, ButtonStatus::PRESSED }, &f_bakeGI);

	g_pModuleManager->getFileSystem()->addSceneLoadingStartCallback(&f_sceneLoadingStartCallback);
	g_pModuleManager->getFileSystem()->addSceneLoadingFinishCallback(&f_sceneLoadingFinishCallback);

	m_SkeletonDataComponentPool = g_pModuleManager->getMemorySystem()->allocateMemoryPool(sizeof(SkeletonDataComponent), 2048);
	m_AnimationDataComponentPool = g_pModuleManager->getMemorySystem()->allocateMemoryPool(sizeof(AnimationDataComponent), 16384);

	m_rayTracer->Setup();

	InnoRenderingFrontendNS::m_objectStatus = ObjectStatus::Created;
	return true;
}

bool InnoRenderingFrontendNS::initialize()
{
	if (InnoRenderingFrontendNS::m_objectStatus == ObjectStatus::Created)
	{
		initializeHaltonSampler();
		m_rayTracer->Initialize();

		InnoRenderingFrontendNS::m_objectStatus = ObjectStatus::Activated;
		g_pModuleManager->getLogSystem()->printLog(LogType::INNO_DEV_SUCCESS, "RenderingFrontend has been initialized.");
		return true;
	}
	else
	{
		g_pModuleManager->getLogSystem()->printLog(LogType::INNO_ERROR, "RenderingFrontend: Object is not created!");
		return false;
	}
}

bool InnoRenderingFrontendNS::updateCameraData()
{
	m_isCameraDataPackValid = false;

	auto l_cameraComponents = GetComponentManager(CameraComponent)->GetAllComponents();
	auto l_mainCamera = l_cameraComponents[0];
	auto l_mainCameraTransformComponent = GetComponent(TransformComponent, l_mainCamera->m_parentEntity);

	auto l_p = l_mainCamera->m_projectionMatrix;

	m_cameraGPUData.p_original = l_p;
	m_cameraGPUData.p_jittered = l_p;

	if (m_renderingConfig.useTAA)
	{
		//TAA jitter for projection matrix
		auto& l_currentHaltonStep = currentHaltonStep;
		if (l_currentHaltonStep >= 16)
		{
			l_currentHaltonStep = 0;
		}
		m_cameraGPUData.p_jittered.m02 = m_haltonSampler[l_currentHaltonStep].x / m_screenResolution.x;
		m_cameraGPUData.p_jittered.m12 = m_haltonSampler[l_currentHaltonStep].y / m_screenResolution.y;
		l_currentHaltonStep += 1;
	}

	m_cameraGPUData.r =
		InnoMath::getInvertRotationMatrix(
			l_mainCameraTransformComponent->m_globalTransformVector.m_rot
		);

	m_cameraGPUData.t =
		InnoMath::getInvertTranslationMatrix(
			l_mainCameraTransformComponent->m_globalTransformVector.m_pos
		);

	m_cameraGPUData.r_prev = l_mainCameraTransformComponent->m_globalTransformMatrix_prev.m_rotationMat.inverse();
	m_cameraGPUData.t_prev = l_mainCameraTransformComponent->m_globalTransformMatrix_prev.m_translationMat.inverse();

	m_cameraGPUData.globalPos = l_mainCameraTransformComponent->m_globalTransformVector.m_pos;

	m_cameraGPUData.WHRatio = l_mainCamera->m_WHRatio;
	m_cameraGPUData.zNear = l_mainCamera->m_zNear;
	m_cameraGPUData.zFar = l_mainCamera->m_zFar;

	m_isCameraDataPackValid = true;

	return true;
}

bool InnoRenderingFrontendNS::updateSunData()
{
	m_isSunDataPackValid = false;
	m_isCSMDataPackValid = false;

	auto l_directionalLightComponents = GetComponentManager(DirectionalLightComponent)->GetAllComponents();
	auto l_directionalLight = l_directionalLightComponents[0];
	auto l_directionalLightTransformComponent = GetComponent(TransformComponent, l_directionalLight->m_parentEntity);

	m_sunGPUData.dir = InnoMath::getDirection(direction::BACKWARD, l_directionalLightTransformComponent->m_globalTransformVector.m_rot);
	m_sunGPUData.luminance = l_directionalLight->m_color * l_directionalLight->m_luminousFlux;
	m_sunGPUData.r = InnoMath::getInvertRotationMatrix(l_directionalLightTransformComponent->m_globalTransformVector.m_rot);

	auto l_CSMSize = l_directionalLight->m_projectionMatrices.size();

	m_CSMGPUData.clear();

	for (size_t j = 0; j < l_directionalLight->m_projectionMatrices.size(); j++)
	{
		m_CSMGPUData.emplace_back();

		auto l_shadowSplitCorner = vec4(
			l_directionalLight->m_AABBsInWorldSpace[j].m_boundMin.x,
			l_directionalLight->m_AABBsInWorldSpace[j].m_boundMin.z,
			l_directionalLight->m_AABBsInWorldSpace[j].m_boundMax.x,
			l_directionalLight->m_AABBsInWorldSpace[j].m_boundMax.z
		);

		m_CSMGPUData[j].p = l_directionalLight->m_projectionMatrices[j];
		m_CSMGPUData[j].splitCorners = l_shadowSplitCorner;

		auto l_lightRotMat = l_directionalLightTransformComponent->m_globalTransformMatrix.m_rotationMat.inverse();

		m_CSMGPUData[j].v = l_lightRotMat;
	}

	m_isCSMDataPackValid = true;
	m_isSunDataPackValid = true;

	return true;
}

bool InnoRenderingFrontendNS::updateLightData()
{
	auto& l_pointLightComponents = GetComponentManager(PointLightComponent)->GetAllComponents();
	for (size_t i = 0; i < l_pointLightComponents.size(); i++)
	{
		PointLightGPUData l_PointLightGPUData;
		l_PointLightGPUData.pos = GetComponent(TransformComponent, l_pointLightComponents[i]->m_parentEntity)->m_globalTransformVector.m_pos;
		l_PointLightGPUData.luminance = l_pointLightComponents[i]->m_color * l_pointLightComponents[i]->m_luminousFlux;
		l_PointLightGPUData.luminance.w = l_pointLightComponents[i]->m_attenuationRadius;
		m_pointLightGPUData[i] = l_PointLightGPUData;
	}

	auto& l_sphereLightComponents = GetComponentManager(SphereLightComponent)->GetAllComponents();
	for (size_t i = 0; i < l_sphereLightComponents.size(); i++)
	{
		SphereLightGPUData l_SphereLightGPUData;
		l_SphereLightGPUData.pos = GetComponent(TransformComponent, l_sphereLightComponents[i]->m_parentEntity)->m_globalTransformVector.m_pos;
		l_SphereLightGPUData.luminance = l_sphereLightComponents[i]->m_color * l_sphereLightComponents[i]->m_luminousFlux;
		l_SphereLightGPUData.luminance.w = l_sphereLightComponents[i]->m_sphereRadius;
		m_sphereLightGPUData[i] = l_SphereLightGPUData;
	}

	return true;
}

bool InnoRenderingFrontendNS::updateMeshData()
{
	m_isMeshDataPackValid = false;

	unsigned int l_opaquePassIndex = 0;
	unsigned int l_transparentPassIndex = 0;

	std::vector<TransparentPassGPUData> l_sortedTransparentPassGPUData;

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

						m_opaquePassGPUData[l_opaquePassIndex] = l_opaquePassGPUData;
						m_opaquePassMeshGPUData[l_opaquePassIndex] = l_meshGPUData;
						m_opaquePassMaterialGPUData[l_opaquePassIndex] = l_materialGPUData;
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

						l_sortedTransparentPassGPUData.emplace_back(l_transparentPassGPUData);
						m_transparentPassMeshGPUData[l_transparentPassIndex] = l_meshGPUData;
						m_transparentPassMaterialGPUData[l_transparentPassIndex] = l_materialGPUData;
						l_transparentPassIndex++;
					}
				}
			}
		}
	}

	m_opaquePassDrawCallCount = l_opaquePassIndex;
	m_transparentPassDrawCallCount = l_transparentPassIndex;

	// @TODO: use GPU to do OIT
	std::sort(l_sortedTransparentPassGPUData.begin(), l_sortedTransparentPassGPUData.end(), [&](TransparentPassGPUData a, TransparentPassGPUData b) {
		auto l_t = m_cameraGPUData.t;
		auto l_r = m_cameraGPUData.r;
		auto m_a_InViewSpace = l_t * l_r * m_transparentPassMeshGPUData[a.meshGPUDataIndex].m;
		auto m_b_InViewSpace = l_t * l_r * m_transparentPassMeshGPUData[b.meshGPUDataIndex].m;
		return m_a_InViewSpace.m23 < m_b_InViewSpace.m23;
	});

	m_transparentPassGPUData = l_sortedTransparentPassGPUData;

	m_isMeshDataPackValid = true;

	return true;
}

bool InnoRenderingFrontendNS::updateBillboardPassData()
{
	unsigned int l_index = 0;

	m_billboardPassGPUData.clear();

	for (auto i : GetComponentManager(DirectionalLightComponent)->GetAllComponents())
	{
		BillboardPassGPUData l_billboardPAssGPUData;
		l_billboardPAssGPUData.globalPos = GetComponent(TransformComponent, i->m_parentEntity)->m_globalTransformVector.m_pos;
		l_billboardPAssGPUData.distanceToCamera = (m_cameraGPUData.globalPos - l_billboardPAssGPUData.globalPos).length();
		l_billboardPAssGPUData.iconType = WorldEditorIconType::DIRECTIONAL_LIGHT;

		m_billboardPassGPUData.emplace_back(l_billboardPAssGPUData);
		l_index++;
	}

	for (auto i : GetComponentManager(PointLightComponent)->GetAllComponents())
	{
		BillboardPassGPUData l_billboardPAssGPUData;
		l_billboardPAssGPUData.globalPos = GetComponent(TransformComponent, i->m_parentEntity)->m_globalTransformVector.m_pos;
		l_billboardPAssGPUData.distanceToCamera = (m_cameraGPUData.globalPos - l_billboardPAssGPUData.globalPos).length();
		l_billboardPAssGPUData.iconType = WorldEditorIconType::POINT_LIGHT;

		m_billboardPassGPUData.emplace_back(l_billboardPAssGPUData);
		l_index++;
	}

	for (auto i : GetComponentManager(SphereLightComponent)->GetAllComponents())
	{
		BillboardPassGPUData l_billboardPAssGPUData;
		l_billboardPAssGPUData.globalPos = GetComponent(TransformComponent, i->m_parentEntity)->m_globalTransformVector.m_pos;
		l_billboardPAssGPUData.distanceToCamera = (m_cameraGPUData.globalPos - l_billboardPAssGPUData.globalPos).length();
		l_billboardPAssGPUData.iconType = WorldEditorIconType::SPHERE_LIGHT;

		m_billboardPassGPUData.emplace_back(l_billboardPAssGPUData);
		l_index++;
	}

	m_billboardPassDrawCallCount = l_index;

	return true;
}

bool InnoRenderingFrontendNS::updateDebuggerPassData()
{
	unsigned int l_index = 0;
	// @TODO: Implementation
	m_debuggerPassDrawCallCount = l_index;

	return true;
}

bool InnoRenderingFrontendNS::gatherStaticMeshData()
{
	unsigned int l_index = 0;

	auto l_visibleComponents = GetComponentManager(VisibleComponent)->GetAllComponents();
	for (auto visibleComponent : l_visibleComponents)
	{
		if (visibleComponent->m_visiblilityType == VisiblilityType::INNO_OPAQUE
			&& visibleComponent->m_objectStatus == ObjectStatus::Activated
			&& visibleComponent->m_meshUsageType == MeshUsageType::STATIC
			)
		{
			auto l_transformComponent = GetComponent(TransformComponent, visibleComponent->m_parentEntity);
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

					m_GIPassGPUData[l_index] = l_GIPassGPUData;
					m_GIPassMeshGPUData[l_index] = l_meshGPUData;
					m_GIPassMaterialGPUData[l_index] = l_materialGPUData;
					l_index++;
				}
			}
		}
	}

	m_GIPassDrawCallCount = l_index;

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
		auto l_cullingDataPack = g_pModuleManager->getPhysicsSystem()->getCullingDataPack();
		if (l_cullingDataPack.has_value() && l_cullingDataPack.value().size() > 0)
		{
			m_cullingDataPack.setRawData(std::move(l_cullingDataPack.value()));
		}

		updateMeshData();

		updateBillboardPassData();

		updateDebuggerPassData();

		m_skyGPUData.p_inv = m_cameraGPUData.p_original.inverse();
		m_skyGPUData.r_inv = m_cameraGPUData.r.inverse();
		m_skyGPUData.viewportSize.x = (float)m_screenResolution.x;
		m_skyGPUData.viewportSize.y = (float)m_screenResolution.y;

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
	m_rayTracer->Terminate();

	m_objectStatus = ObjectStatus::Terminated;
	g_pModuleManager->getLogSystem()->printLog(LogType::INNO_DEV_SUCCESS, "RenderingFrontend has been terminated.");
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

SkeletonDataComponent * InnoRenderingFrontend::addSkeletonDataComponent()
{
	static std::atomic<unsigned int> skeletonCount = 0;
	auto l_rawPtr = g_pModuleManager->getMemorySystem()->spawnObject(InnoRenderingFrontendNS::m_SkeletonDataComponentPool, sizeof(SkeletonDataComponent));
	auto l_SDC = new(l_rawPtr)SkeletonDataComponent();
	auto l_parentEntity = g_pModuleManager->getEntityManager()->Spawn(ObjectSource::Runtime, ObjectUsage::Engine, ("Skeleton_" + std::to_string(skeletonCount) + "/").c_str());
	l_SDC->m_parentEntity = l_parentEntity;
	skeletonCount++;
	return l_SDC;
}

AnimationDataComponent * InnoRenderingFrontend::addAnimationDataComponent()
{
	static std::atomic<unsigned int> animationCount = 0;
	auto l_rawPtr = g_pModuleManager->getMemorySystem()->spawnObject(InnoRenderingFrontendNS::m_AnimationDataComponentPool, sizeof(AnimationDataComponent));
	auto l_ADC = new(l_rawPtr)AnimationDataComponent();
	auto l_parentEntity = g_pModuleManager->getEntityManager()->Spawn(ObjectSource::Runtime, ObjectUsage::Engine, ("Animation_" + std::to_string(animationCount) + "/").c_str());
	l_ADC->m_parentEntity = l_parentEntity;
	animationCount++;
	return l_ADC;
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

RenderingCapability InnoRenderingFrontend::getRenderingCapability()
{
	return InnoRenderingFrontendNS::m_renderingCapability;
}

CameraGPUData InnoRenderingFrontend::getCameraGPUData()
{
	return InnoRenderingFrontendNS::m_cameraGPUData;
}

SunGPUData InnoRenderingFrontend::getSunGPUData()
{
	return InnoRenderingFrontendNS::m_sunGPUData;
}

const std::vector<CSMGPUData>& InnoRenderingFrontend::getCSMGPUData()
{
	return InnoRenderingFrontendNS::m_CSMGPUData;
}

const std::vector<PointLightGPUData>& InnoRenderingFrontend::getPointLightGPUData()
{
	return InnoRenderingFrontendNS::m_pointLightGPUData;
}

const std::vector<SphereLightGPUData>& InnoRenderingFrontend::getSphereLightGPUData()
{
	return InnoRenderingFrontendNS::m_sphereLightGPUData;
}

SkyGPUData InnoRenderingFrontend::getSkyGPUData()
{
	return InnoRenderingFrontendNS::m_skyGPUData;
}

unsigned int InnoRenderingFrontend::getOpaquePassDrawCallCount()
{
	return InnoRenderingFrontendNS::m_opaquePassDrawCallCount;
}

const std::vector<OpaquePassGPUData>& InnoRenderingFrontend::getOpaquePassGPUData()
{
	return InnoRenderingFrontendNS::m_opaquePassGPUData;
}

const std::vector<MeshGPUData>& InnoRenderingFrontend::getOpaquePassMeshGPUData()
{
	return InnoRenderingFrontendNS::m_opaquePassMeshGPUData;
}

const std::vector<MaterialGPUData>& InnoRenderingFrontend::getOpaquePassMaterialGPUData()
{
	return InnoRenderingFrontendNS::m_opaquePassMaterialGPUData;
}

unsigned int InnoRenderingFrontend::getTransparentPassDrawCallCount()
{
	return InnoRenderingFrontendNS::m_transparentPassDrawCallCount;
}

const std::vector<TransparentPassGPUData>& InnoRenderingFrontend::getTransparentPassGPUData()
{
	return InnoRenderingFrontendNS::m_transparentPassGPUData;
}

const std::vector<MeshGPUData>& InnoRenderingFrontend::getTransparentPassMeshGPUData()
{
	return InnoRenderingFrontendNS::m_transparentPassMeshGPUData;
}

const std::vector<MaterialGPUData>& InnoRenderingFrontend::getTransparentPassMaterialGPUData()
{
	return InnoRenderingFrontendNS::m_transparentPassMaterialGPUData;
}

unsigned int InnoRenderingFrontend::getBillboardPassDrawCallCount()
{
	return InnoRenderingFrontendNS::m_billboardPassDrawCallCount;
}

const std::vector<BillboardPassGPUData>& InnoRenderingFrontend::getBillboardPassGPUData()
{
	return InnoRenderingFrontendNS::m_billboardPassGPUData;
}

unsigned int InnoRenderingFrontend::getDebuggerPassDrawCallCount()
{
	return InnoRenderingFrontendNS::m_debuggerPassDrawCallCount;
}

const std::vector<DebuggerPassGPUData>& InnoRenderingFrontend::getDebuggerPassGPUData()
{
	return InnoRenderingFrontendNS::m_debuggerPassGPUData;
}

unsigned int InnoRenderingFrontend::getGIPassDrawCallCount()
{
	return InnoRenderingFrontendNS::m_GIPassDrawCallCount;
}

const std::vector<OpaquePassGPUData>& InnoRenderingFrontend::getGIPassGPUData()
{
	return InnoRenderingFrontendNS::m_GIPassGPUData;
}

const std::vector<MeshGPUData>& InnoRenderingFrontend::getGIPassMeshGPUData()
{
	return InnoRenderingFrontendNS::m_GIPassMeshGPUData;
}

const std::vector<MaterialGPUData>& InnoRenderingFrontend::getGIPassMaterialGPUData()
{
	return InnoRenderingFrontendNS::m_GIPassMaterialGPUData;
}