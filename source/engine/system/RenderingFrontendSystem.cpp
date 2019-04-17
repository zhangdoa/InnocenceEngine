#include "RenderingFrontendSystem.h"
#include "../component/RenderingFrontendSystemComponent.h"

#include "ICoreSystem.h"

extern ICoreSystem* g_pCoreSystem;

INNO_PRIVATE_SCOPE InnoRenderingFrontendSystemNS
{
	ObjectStatus m_objectStatus = ObjectStatus::SHUTDOWN;

	ThreadSafeUnorderedMap<EntityID, MeshDataComponent*> m_meshMap;
	ThreadSafeUnorderedMap<EntityID, MaterialDataComponent*> m_materialMap;
	ThreadSafeUnorderedMap<EntityID, TextureDataComponent*> m_textureMap;

	void* m_MeshDataComponentPool;
	void* m_MaterialDataComponentPool;
	void* m_TextureDataComponentPool;

	TextureDataComponent* m_iconTemplate_OBJ;
	TextureDataComponent* m_iconTemplate_PNG;
	TextureDataComponent* m_iconTemplate_SHADER;
	TextureDataComponent* m_iconTemplate_UNKNOWN;

	TextureDataComponent* m_iconTemplate_DirectionalLight;
	TextureDataComponent* m_iconTemplate_PointLight;
	TextureDataComponent* m_iconTemplate_SphereLight;

	MeshDataComponent* m_unitLineMDC;
	MeshDataComponent* m_unitQuadMDC;
	MeshDataComponent* m_unitCubeMDC;
	MeshDataComponent* m_unitSphereMDC;
	MeshDataComponent* m_terrainMDC;

	TextureDataComponent* m_basicNormalTDC;
	TextureDataComponent* m_basicAlbedoTDC;
	TextureDataComponent* m_basicMetallicTDC;
	TextureDataComponent* m_basicRoughnessTDC;
	TextureDataComponent* m_basicAOTDC;

	TVec2<unsigned int> m_screenResolution = TVec2<unsigned int>(1280, 720);
	std::string m_windowName;
	bool m_fullScreen = false;

	ThreadSafeQueue<MeshDataComponent*> m_uninitializedMDC;
	ThreadSafeQueue<TextureDataComponent*> m_uninitializedTDC;

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
	std::function<void()> f_captureEnvironment;
	std::function<void()> f_sceneLoadingStartCallback;

	RenderingConfig m_renderingConfig = RenderingConfig();

	bool setup();
	bool initialize();
	bool update();
	bool terminate();

	bool initializeComponentPool();
	bool loadDefaultAssets();

	MeshDataComponent* addMeshDataComponent();
	MaterialDataComponent* addMaterialDataComponent();
	TextureDataComponent* addTextureDataComponent();

	float radicalInverse(unsigned int n, unsigned int base);
	void initializeHaltonSampler();

	bool updateCameraData();
	bool updateSunData();
	bool updateMeshData();
}

bool InnoRenderingFrontendSystemNS::initializeComponentPool()
{
	m_MeshDataComponentPool = g_pCoreSystem->getMemorySystem()->allocateMemoryPool(sizeof(MeshDataComponent), 16384);
	m_MaterialDataComponentPool = g_pCoreSystem->getMemorySystem()->allocateMemoryPool(sizeof(MaterialDataComponent), 32768);
	m_TextureDataComponentPool = g_pCoreSystem->getMemorySystem()->allocateMemoryPool(sizeof(TextureDataComponent), 32768);

	return true;
}

bool InnoRenderingFrontendSystemNS::loadDefaultAssets()
{
	m_basicNormalTDC = g_pCoreSystem->getAssetSystem()->loadTexture("res//textures//basic_normal.png", TextureSamplerType::SAMPLER_2D, TextureUsageType::NORMAL);
	m_basicAlbedoTDC = g_pCoreSystem->getAssetSystem()->loadTexture("res//textures//basic_albedo.png", TextureSamplerType::SAMPLER_2D, TextureUsageType::ALBEDO);
	m_basicMetallicTDC = g_pCoreSystem->getAssetSystem()->loadTexture("res//textures//basic_metallic.png", TextureSamplerType::SAMPLER_2D, TextureUsageType::METALLIC);
	m_basicRoughnessTDC = g_pCoreSystem->getAssetSystem()->loadTexture("res//textures//basic_roughness.png", TextureSamplerType::SAMPLER_2D, TextureUsageType::ROUGHNESS);
	m_basicAOTDC = g_pCoreSystem->getAssetSystem()->loadTexture("res//textures//basic_ao.png", TextureSamplerType::SAMPLER_2D, TextureUsageType::AMBIENT_OCCLUSION);

	m_iconTemplate_OBJ = g_pCoreSystem->getAssetSystem()->loadTexture("res//textures//InnoFileTypeIcons_OBJ.png", TextureSamplerType::SAMPLER_2D, TextureUsageType::NORMAL);
	m_iconTemplate_PNG = g_pCoreSystem->getAssetSystem()->loadTexture("res//textures//InnoFileTypeIcons_PNG.png", TextureSamplerType::SAMPLER_2D, TextureUsageType::NORMAL);
	m_iconTemplate_SHADER = g_pCoreSystem->getAssetSystem()->loadTexture("res//textures//InnoFileTypeIcons_SHADER.png", TextureSamplerType::SAMPLER_2D, TextureUsageType::NORMAL);
	m_iconTemplate_UNKNOWN = g_pCoreSystem->getAssetSystem()->loadTexture("res//textures//InnoFileTypeIcons_UNKNOWN.png", TextureSamplerType::SAMPLER_2D, TextureUsageType::NORMAL);

	m_iconTemplate_DirectionalLight = g_pCoreSystem->getAssetSystem()->loadTexture("res//textures//InnoWorldEditorIcons_DirectionalLight.png", TextureSamplerType::SAMPLER_2D, TextureUsageType::NORMAL);
	m_iconTemplate_PointLight = g_pCoreSystem->getAssetSystem()->loadTexture("res//textures//InnoWorldEditorIcons_PointLight.png", TextureSamplerType::SAMPLER_2D, TextureUsageType::NORMAL);
	m_iconTemplate_SphereLight = g_pCoreSystem->getAssetSystem()->loadTexture("res//textures//InnoWorldEditorIcons_SphereLight.png", TextureSamplerType::SAMPLER_2D, TextureUsageType::NORMAL);

	m_unitLineMDC = addMeshDataComponent();
	g_pCoreSystem->getAssetSystem()->addUnitLine(*m_unitLineMDC);
	m_unitLineMDC->m_meshPrimitiveTopology = MeshPrimitiveTopology::TRIANGLE_STRIP;
	m_unitLineMDC->m_meshShapeType = MeshShapeType::LINE;
	m_unitLineMDC->m_objectStatus = ObjectStatus::STANDBY;
	g_pCoreSystem->getPhysicsSystem()->generatePhysicsDataComponent(m_unitLineMDC);

	m_unitQuadMDC = addMeshDataComponent();
	g_pCoreSystem->getAssetSystem()->addUnitQuad(*m_unitQuadMDC);
	m_unitQuadMDC->m_meshPrimitiveTopology = MeshPrimitiveTopology::TRIANGLE_STRIP;
	m_unitQuadMDC->m_meshShapeType = MeshShapeType::QUAD;
	m_unitQuadMDC->m_objectStatus = ObjectStatus::STANDBY;
	g_pCoreSystem->getPhysicsSystem()->generatePhysicsDataComponent(m_unitQuadMDC);

	m_unitCubeMDC = addMeshDataComponent();
	g_pCoreSystem->getAssetSystem()->addUnitCube(*m_unitCubeMDC);
	m_unitCubeMDC->m_meshPrimitiveTopology = MeshPrimitiveTopology::TRIANGLE;
	m_unitCubeMDC->m_meshShapeType = MeshShapeType::CUBE;
	m_unitCubeMDC->m_objectStatus = ObjectStatus::STANDBY;
	g_pCoreSystem->getPhysicsSystem()->generatePhysicsDataComponent(m_unitCubeMDC);

	m_unitSphereMDC = addMeshDataComponent();
	g_pCoreSystem->getAssetSystem()->addUnitSphere(*m_unitSphereMDC);
	m_unitSphereMDC->m_meshPrimitiveTopology = MeshPrimitiveTopology::TRIANGLE_STRIP;
	m_unitSphereMDC->m_meshShapeType = MeshShapeType::SPHERE;
	m_unitSphereMDC->m_objectStatus = ObjectStatus::STANDBY;
	g_pCoreSystem->getPhysicsSystem()->generatePhysicsDataComponent(m_unitSphereMDC);

	m_terrainMDC = addMeshDataComponent();
	g_pCoreSystem->getAssetSystem()->addTerrain(*m_terrainMDC);
	m_terrainMDC->m_meshPrimitiveTopology = MeshPrimitiveTopology::TRIANGLE;
	m_terrainMDC->m_objectStatus = ObjectStatus::STANDBY;
	g_pCoreSystem->getPhysicsSystem()->generatePhysicsDataComponent(m_terrainMDC);

	return true;
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

bool InnoRenderingFrontendSystemNS::setup()
{
	m_renderingConfig.useMotionBlur = true;
	m_renderingConfig.useTAA = true;
	//m_renderingConfig.useBloom = true;
	m_renderingConfig.drawSky = true;
	//m_renderingConfig.drawTerrain = true;

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

	g_pCoreSystem->getFileSystem()->addSceneLoadingStartCallback(&f_sceneLoadingStartCallback);

	InnoRenderingFrontendSystemNS::initializeComponentPool();

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

bool InnoRenderingFrontendSystemNS::updateMeshData()
{
	m_isMeshDataPackValid = false;

	RenderingFrontendSystemComponent::get().m_opaquePassGPUDataQueue.clear();
	RenderingFrontendSystemComponent::get().m_transparentPassGPUDataQueue.clear();

	for (auto& i : m_cullingDataPack)
	{
		if (i.visibleComponent != nullptr && i.MDC != nullptr)
		{
			if (i.MDC->m_objectStatus == ObjectStatus::ALIVE)
			{
				auto l_modelPair = i.visibleComponent->m_modelMap.find(i.MDC);

				if (l_modelPair != i.visibleComponent->m_modelMap.end())
				{
					GeometryPassGPUData l_geometryPassGPUData;

					l_geometryPassGPUData.MDC = i.MDC;

					l_geometryPassGPUData.meshGPUData.m = i.m;
					l_geometryPassGPUData.meshGPUData.m_prev = i.m_prev;
					l_geometryPassGPUData.meshGPUData.normalMat = i.normalMat;
					l_geometryPassGPUData.meshGPUData.UUID = i.visibleComponent->m_UUID;

					l_geometryPassGPUData.normalTDC = l_modelPair->second->m_texturePack.m_normalTDC.second;
					l_geometryPassGPUData.albedoTDC = l_modelPair->second->m_texturePack.m_albedoTDC.second;
					l_geometryPassGPUData.metallicTDC = l_modelPair->second->m_texturePack.m_metallicTDC.second;
					l_geometryPassGPUData.roughnessTDC = l_modelPair->second->m_texturePack.m_roughnessTDC.second;
					l_geometryPassGPUData.AOTDC = l_modelPair->second->m_texturePack.m_aoTDC.second;

					l_geometryPassGPUData.materialGPUData.useNormalTexture = !(l_geometryPassGPUData.normalTDC == nullptr);
					l_geometryPassGPUData.materialGPUData.useAlbedoTexture = !(l_geometryPassGPUData.albedoTDC == nullptr);
					l_geometryPassGPUData.materialGPUData.useMetallicTexture = !(l_geometryPassGPUData.metallicTDC == nullptr);
					l_geometryPassGPUData.materialGPUData.useRoughnessTexture = !(l_geometryPassGPUData.roughnessTDC == nullptr);
					l_geometryPassGPUData.materialGPUData.useAOTexture = !(l_geometryPassGPUData.AOTDC == nullptr);

					l_geometryPassGPUData.materialGPUData.customMaterial = l_modelPair->second->m_meshCustomMaterial;

					if (i.visibleComponent->m_visiblilityType == VisiblilityType::INNO_OPAQUE)
					{
						RenderingFrontendSystemComponent::get().m_opaquePassGPUDataQueue.push(l_geometryPassGPUData);
					}

					else if (i.visibleComponent->m_visiblilityType == VisiblilityType::INNO_TRANSPARENT)
					{
						RenderingFrontendSystemComponent::get().m_transparentPassGPUDataQueue.push(l_geometryPassGPUData);
					}
				}
			}
		}
	}

	m_isMeshDataPackValid = true;

	return true;
}

bool InnoRenderingFrontendSystemNS::update()
{
	updateCameraData();

	updateSunData();

	// copy culling data pack for local scope
	auto& l_cullingDataPack = g_pCoreSystem->getPhysicsSystem()->getCullingDataPack();
	if (l_cullingDataPack.has_value() && l_cullingDataPack.value().size() > 0)
	{
		m_cullingDataPack.setRawData(std::move(l_cullingDataPack.value()));
	}

	updateMeshData();

	return true;
}

bool InnoRenderingFrontendSystemNS::terminate()
{
	m_objectStatus = ObjectStatus::SHUTDOWN;
	g_pCoreSystem->getLogSystem()->printLog(LogType::INNO_DEV_SUCCESS, "RenderingFrontendSystem has been terminated.");
	return true;
}

INNO_SYSTEM_EXPORT bool InnoRenderingFrontendSystem::setup()
{
	return InnoRenderingFrontendSystemNS::setup();
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

INNO_SYSTEM_EXPORT void InnoRenderingFrontendSystem::loadDefaultAssets()
{
	InnoRenderingFrontendSystemNS::loadDefaultAssets();
}

INNO_SYSTEM_EXPORT MeshDataComponent * InnoRenderingFrontendSystem::addMeshDataComponent()
{
	return InnoRenderingFrontendSystemNS::addMeshDataComponent();
}

INNO_SYSTEM_EXPORT MaterialDataComponent * InnoRenderingFrontendSystem::addMaterialDataComponent()
{
	return InnoRenderingFrontendSystemNS::addMaterialDataComponent();
}

INNO_SYSTEM_EXPORT TextureDataComponent * InnoRenderingFrontendSystem::addTextureDataComponent()
{
	return InnoRenderingFrontendSystemNS::addTextureDataComponent();
}

MeshDataComponent* InnoRenderingFrontendSystemNS::addMeshDataComponent()
{
	auto l_rawPtr = g_pCoreSystem->getMemorySystem()->spawnObject(m_MeshDataComponentPool, sizeof(MeshDataComponent));
	auto l_MDC = new(l_rawPtr)MeshDataComponent();
	auto l_parentEntity = InnoMath::createEntityID();
	l_MDC->m_parentEntity = l_parentEntity;
	auto l_meshMap = &m_meshMap;
	l_meshMap->emplace(std::pair<EntityID, MeshDataComponent*>(l_parentEntity, l_MDC));
	return l_MDC;
}

MaterialDataComponent* InnoRenderingFrontendSystemNS::addMaterialDataComponent()
{
	auto l_rawPtr = g_pCoreSystem->getMemorySystem()->spawnObject(m_MaterialDataComponentPool, sizeof(MaterialDataComponent));
	auto l_MDC = new(l_rawPtr)MaterialDataComponent();
	auto l_parentEntity = InnoMath::createEntityID();
	l_MDC->m_parentEntity = l_parentEntity;
	auto l_materialMap = &m_materialMap;
	l_materialMap->emplace(std::pair<EntityID, MaterialDataComponent*>(l_parentEntity, l_MDC));
	return l_MDC;
}

TextureDataComponent* InnoRenderingFrontendSystemNS::addTextureDataComponent()
{
	auto l_rawPtr = g_pCoreSystem->getMemorySystem()->spawnObject(m_TextureDataComponentPool, sizeof(TextureDataComponent));
	auto l_TDC = new(l_rawPtr)TextureDataComponent();
	auto l_parentEntity = InnoMath::createEntityID();
	l_TDC->m_parentEntity = l_parentEntity;
	auto l_textureMap = &m_textureMap;
	l_textureMap->emplace(std::pair<EntityID, TextureDataComponent*>(l_parentEntity, l_TDC));
	return l_TDC;
}

MeshDataComponent* InnoRenderingFrontendSystem::getMeshDataComponent(EntityID EntityID)
{
	auto result = InnoRenderingFrontendSystemNS::m_meshMap.find(EntityID);
	if (result != InnoRenderingFrontendSystemNS::m_meshMap.end())
	{
		return result->second;
	}
	else
	{
		g_pCoreSystem->getLogSystem()->printLog(LogType::INNO_ERROR, "RenderingFrontendSystem: can't find MeshDataComponent by EntityID: " + EntityID + " !");
		return nullptr;
	}
}

TextureDataComponent * InnoRenderingFrontendSystem::getTextureDataComponent(EntityID EntityID)
{
	auto result = InnoRenderingFrontendSystemNS::m_textureMap.find(EntityID);
	if (result != InnoRenderingFrontendSystemNS::m_textureMap.end())
	{
		return result->second;
	}
	else
	{
		g_pCoreSystem->getLogSystem()->printLog(LogType::INNO_ERROR, "RenderingFrontendSystem: can't find TextureDataComponent by EntityID: " + EntityID + " !");
		return nullptr;
	}
}

MeshDataComponent * InnoRenderingFrontendSystem::getMeshDataComponent(MeshShapeType meshShapeType)
{
	switch (meshShapeType)
	{
	case MeshShapeType::LINE:
		return InnoRenderingFrontendSystemNS::m_unitLineMDC; break;
	case MeshShapeType::QUAD:
		return InnoRenderingFrontendSystemNS::m_unitQuadMDC; break;
	case MeshShapeType::CUBE:
		return InnoRenderingFrontendSystemNS::m_unitCubeMDC; break;
	case MeshShapeType::SPHERE:
		return InnoRenderingFrontendSystemNS::m_unitSphereMDC; break;
	case MeshShapeType::TERRAIN:
		return InnoRenderingFrontendSystemNS::m_terrainMDC; break;
	case MeshShapeType::CUSTOM:
		g_pCoreSystem->getLogSystem()->printLog(LogType::INNO_ERROR, "RenderingFrontendSystem: wrong MeshShapeType passed to InnoRenderingFrontendSystem::getMeshDataComponent() !");
		return nullptr; break;
	default:
		return nullptr; break;
	}
}

TextureDataComponent * InnoRenderingFrontendSystem::getTextureDataComponent(TextureUsageType textureUsageType)
{
	switch (textureUsageType)
	{
	case TextureUsageType::INVISIBLE:
		return nullptr; break;
	case TextureUsageType::NORMAL:
		return InnoRenderingFrontendSystemNS::m_basicNormalTDC; break;
	case TextureUsageType::ALBEDO:
		return InnoRenderingFrontendSystemNS::m_basicAlbedoTDC; break;
	case TextureUsageType::METALLIC:
		return InnoRenderingFrontendSystemNS::m_basicMetallicTDC; break;
	case TextureUsageType::ROUGHNESS:
		return InnoRenderingFrontendSystemNS::m_basicRoughnessTDC; break;
	case TextureUsageType::AMBIENT_OCCLUSION:
		return InnoRenderingFrontendSystemNS::m_basicAOTDC; break;
	case TextureUsageType::RENDER_TARGET:
		return nullptr; break;
	default:
		return nullptr; break;
	}
}

INNO_SYSTEM_EXPORT TextureDataComponent* InnoRenderingFrontendSystem::getTextureDataComponent(FileExplorerIconType iconType)
{
	switch (iconType)
	{
	case FileExplorerIconType::OBJ:
		return InnoRenderingFrontendSystemNS::m_iconTemplate_OBJ; break;
	case FileExplorerIconType::PNG:
		return InnoRenderingFrontendSystemNS::m_iconTemplate_PNG; break;
	case FileExplorerIconType::SHADER:
		return InnoRenderingFrontendSystemNS::m_iconTemplate_SHADER; break;
	case FileExplorerIconType::UNKNOWN:
		return InnoRenderingFrontendSystemNS::m_iconTemplate_UNKNOWN; break;
	default:
		return nullptr; break;
	}
}

INNO_SYSTEM_EXPORT TextureDataComponent * InnoRenderingFrontendSystem::getTextureDataComponent(WorldEditorIconType iconType)
{
	switch (iconType)
	{
	case WorldEditorIconType::DIRECTIONAL_LIGHT:
		return InnoRenderingFrontendSystemNS::m_iconTemplate_DirectionalLight; break;
	case WorldEditorIconType::POINT_LIGHT:
		return InnoRenderingFrontendSystemNS::m_iconTemplate_PointLight; break;
	case WorldEditorIconType::SPHERE_LIGHT:
		return InnoRenderingFrontendSystemNS::m_iconTemplate_SphereLight; break;
	default:
		return nullptr; break;
	}
}

bool InnoRenderingFrontendSystem::removeMeshDataComponent(EntityID EntityID)
{
	auto l_meshMap = &InnoRenderingFrontendSystemNS::m_meshMap;
	auto l_mesh = l_meshMap->find(EntityID);
	if (l_mesh != l_meshMap->end())
	{
		g_pCoreSystem->getMemorySystem()->destroyObject(InnoRenderingFrontendSystemNS::m_MeshDataComponentPool, sizeof(MeshDataComponent), l_mesh->second);
		l_meshMap->erase(EntityID);
		return true;
	}
	else
	{
		g_pCoreSystem->getLogSystem()->printLog(LogType::INNO_ERROR, "RenderingFrontendSystem: can't remove MeshDataComponent by EntityID: " + EntityID + " !");
		return false;
	}
}

bool InnoRenderingFrontendSystem::removeTextureDataComponent(EntityID EntityID)
{
	auto l_textureMap = &InnoRenderingFrontendSystemNS::m_textureMap;
	auto l_texture = l_textureMap->find(EntityID);
	if (l_texture != l_textureMap->end())
	{
		for (auto& i : l_texture->second->m_textureData)
		{
			// @TODO
		}

		g_pCoreSystem->getMemorySystem()->destroyObject(InnoRenderingFrontendSystemNS::m_TextureDataComponentPool, sizeof(TextureDataComponent), l_texture->second);
		l_textureMap->erase(EntityID);
		return true;
	}
	else
	{
		g_pCoreSystem->getLogSystem()->printLog(LogType::INNO_ERROR, "RenderingFrontendSystem: can't remove TextureDataComponent by EntityID: " + EntityID + " !");
		return false;
	}
}

INNO_SYSTEM_EXPORT void InnoRenderingFrontendSystem::registerUninitializedMeshDataComponent(MeshDataComponent * rhs)
{
	InnoRenderingFrontendSystemNS::m_uninitializedMDC.push(rhs);
}

INNO_SYSTEM_EXPORT void InnoRenderingFrontendSystem::registerUninitializedTextureDataComponent(TextureDataComponent * rhs)
{
	InnoRenderingFrontendSystemNS::m_uninitializedTDC.push(rhs);
}

bool InnoRenderingFrontendSystem::releaseRawDataForMeshDataComponent(EntityID EntityID)
{
	auto l_meshMap = &InnoRenderingFrontendSystemNS::m_meshMap;
	auto l_mesh = l_meshMap->find(EntityID);
	if (l_mesh != l_meshMap->end())
	{
		// @TODO:
		l_mesh->second->m_vertices.clear();
		l_mesh->second->m_vertices.shrink_to_fit();
		l_mesh->second->m_indices.clear();
		l_mesh->second->m_indices.shrink_to_fit();
		return true;
	}
	else
	{
		g_pCoreSystem->getLogSystem()->printLog(LogType::INNO_ERROR, "InnoRenderingFrontendSystem: can't release raw data for MeshDataComponent by EntityID: " + EntityID + " !");
		return false;
	}
}

bool InnoRenderingFrontendSystem::releaseRawDataForTextureDataComponent(EntityID EntityID)
{
	auto l_textureMap = &InnoRenderingFrontendSystemNS::m_textureMap;
	auto l_texture = l_textureMap->find(EntityID);
	if (l_texture != l_textureMap->end())
	{
		for (auto& i : l_texture->second->m_textureData)
		{
			// @TODO:
		}
		return true;
	}
	else
	{
		g_pCoreSystem->getLogSystem()->printLog(LogType::INNO_ERROR, "InnoRenderingFrontendSystem: can't release raw data for TextureDataComponent by EntityID : " + EntityID + " !");
		return false;
	}
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