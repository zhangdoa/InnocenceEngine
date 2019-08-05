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

namespace InnoRenderingFrontendNS
{
	ObjectStatus m_objectStatus = ObjectStatus::Terminated;

	IRenderingServer* m_renderingServer;
	IRayTracer* m_rayTracer;

	TVec2<unsigned int> m_screenResolution = TVec2<unsigned int>(1280, 720);
	std::string m_windowName;
	bool m_fullScreen = false;

	RenderingCapability m_renderingCapability;

	RenderPassDesc m_DefaultRenderPassDesc;

	DoubleBuffer<CameraGPUData> m_cameraGPUData;
	DoubleBuffer<SunGPUData> m_sunGPUData;
	std::vector<CSMGPUData> m_CSMGPUData;
	std::vector<PointLightGPUData> m_pointLightGPUData;
	std::vector<SphereLightGPUData> m_sphereLightGPUData;
	DoubleBuffer<SkyGPUData> m_skyGPUData;

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

	ThreadSafeQueue<MeshDataComponent*> m_uninitializedMeshes;
	ThreadSafeQueue<MaterialDataComponent*> m_uninitializedMaterials;

	TextureDataComponent* m_iconTemplate_OBJ;
	TextureDataComponent* m_iconTemplate_PNG;
	TextureDataComponent* m_iconTemplate_SHADER;
	TextureDataComponent* m_iconTemplate_UNKNOWN;

	TextureDataComponent* m_iconTemplate_DirectionalLight;
	TextureDataComponent* m_iconTemplate_PointLight;
	TextureDataComponent* m_iconTemplate_SphereLight;

	MeshDataComponent* m_unitLineMesh;
	MeshDataComponent* m_unitQuadMesh;
	MeshDataComponent* m_unitCubeMesh;
	MeshDataComponent* m_unitSphereMesh;
	MeshDataComponent* m_terrainMesh;

	TextureDataComponent* m_basicNormalTexture;
	TextureDataComponent* m_basicAlbedoTexture;
	TextureDataComponent* m_basicMetallicTexture;
	TextureDataComponent* m_basicRoughnessTexture;
	TextureDataComponent* m_basicAOTexture;

	MaterialDataComponent* m_basicMaterial;

	bool setup(IRenderingServer* renderingServer);
	bool loadDefaultAssets();
	bool initialize();
	bool update();
	bool terminate();

	float radicalInverse(unsigned int n, unsigned int base);
	void initializeHaltonSampler();

	bool updateCameraData();
	bool updateSunData();
	bool updateSkyData();
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

bool InnoRenderingFrontendNS::setup(IRenderingServer* renderingServer)
{
	m_renderingServer = renderingServer;
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

	m_DefaultRenderPassDesc.m_UseMultiFrames = false;
	m_DefaultRenderPassDesc.m_RenderTargetCount = 1;
	m_DefaultRenderPassDesc.m_RenderTargetDesc.SamplerType = TextureSamplerType::Sampler2D;
	m_DefaultRenderPassDesc.m_RenderTargetDesc.UsageType = TextureUsageType::ColorAttachment;
	m_DefaultRenderPassDesc.m_RenderTargetDesc.PixelDataFormat = TexturePixelDataFormat::RGBA;
	m_DefaultRenderPassDesc.m_RenderTargetDesc.MinFilterMethod = TextureFilterMethod::Nearest;
	m_DefaultRenderPassDesc.m_RenderTargetDesc.MagFilterMethod = TextureFilterMethod::Nearest;
	m_DefaultRenderPassDesc.m_RenderTargetDesc.WrapMethod = TextureWrapMethod::Edge;
	m_DefaultRenderPassDesc.m_RenderTargetDesc.Width = m_screenResolution.x;
	m_DefaultRenderPassDesc.m_RenderTargetDesc.Height = m_screenResolution.y;
	m_DefaultRenderPassDesc.m_RenderTargetDesc.PixelDataType = TexturePixelDataType::FLOAT16;

	m_DefaultRenderPassDesc.m_GraphicsPipelineDesc.m_ViewportDesc.m_Width = (float)m_screenResolution.x;
	m_DefaultRenderPassDesc.m_GraphicsPipelineDesc.m_ViewportDesc.m_Height = (float)m_screenResolution.y;

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
		m_renderingServer->BakeGIData();
	};
	g_pModuleManager->getEventSystem()->addButtonStatusCallback(ButtonState{ INNO_KEY_B, true }, ButtonEvent{ EventLifeTime::OneShot, &f_bakeGI });

	g_pModuleManager->getFileSystem()->addSceneLoadingStartCallback(&f_sceneLoadingStartCallback);
	g_pModuleManager->getFileSystem()->addSceneLoadingFinishCallback(&f_sceneLoadingFinishCallback);

	m_SkeletonDataComponentPool = g_pModuleManager->getMemorySystem()->allocateMemoryPool(sizeof(SkeletonDataComponent), 2048);
	m_AnimationDataComponentPool = g_pModuleManager->getMemorySystem()->allocateMemoryPool(sizeof(AnimationDataComponent), 16384);

	m_rayTracer->Setup();

	InnoRenderingFrontendNS::m_objectStatus = ObjectStatus::Created;
	return true;
}

bool InnoRenderingFrontendNS::loadDefaultAssets()
{
	m_basicNormalTexture = g_pModuleManager->getAssetSystem()->loadTexture("Res//Textures//basic_normal.png", TextureSamplerType::Sampler2D, TextureUsageType::Normal);
	m_basicAlbedoTexture = g_pModuleManager->getAssetSystem()->loadTexture("Res//Textures//basic_albedo.png", TextureSamplerType::Sampler2D, TextureUsageType::Albedo);
	m_basicMetallicTexture = g_pModuleManager->getAssetSystem()->loadTexture("Res//Textures//basic_metallic.png", TextureSamplerType::Sampler2D, TextureUsageType::Metallic);
	m_basicRoughnessTexture = g_pModuleManager->getAssetSystem()->loadTexture("Res//Textures//basic_roughness.png", TextureSamplerType::Sampler2D, TextureUsageType::Roughness);
	m_basicAOTexture = g_pModuleManager->getAssetSystem()->loadTexture("Res//Textures//basic_ao.png", TextureSamplerType::Sampler2D, TextureUsageType::AmbientOcclusion);

	m_basicMaterial = m_renderingServer->AddMaterialDataComponent("BasicMaterial/");
	m_basicMaterial->m_normalTexture = m_basicNormalTexture;
	m_basicMaterial->m_albedoTexture = m_basicAlbedoTexture;
	m_basicMaterial->m_metallicTexture = m_basicMetallicTexture;
	m_basicMaterial->m_roughnessTexture = m_basicRoughnessTexture;
	m_basicMaterial->m_aoTexture = m_basicAOTexture;

	m_iconTemplate_OBJ = g_pModuleManager->getAssetSystem()->loadTexture("Res//Textures//InnoFileTypeIcons_OBJ.png", TextureSamplerType::Sampler2D, TextureUsageType::Normal);
	m_iconTemplate_PNG = g_pModuleManager->getAssetSystem()->loadTexture("Res//Textures//InnoFileTypeIcons_PNG.png", TextureSamplerType::Sampler2D, TextureUsageType::Normal);
	m_iconTemplate_SHADER = g_pModuleManager->getAssetSystem()->loadTexture("Res//Textures//InnoFileTypeIcons_SHADER.png", TextureSamplerType::Sampler2D, TextureUsageType::Normal);
	m_iconTemplate_UNKNOWN = g_pModuleManager->getAssetSystem()->loadTexture("Res//Textures//InnoFileTypeIcons_UNKNOWN.png", TextureSamplerType::Sampler2D, TextureUsageType::Normal);

	m_iconTemplate_DirectionalLight = g_pModuleManager->getAssetSystem()->loadTexture("Res//Textures//InnoWorldEditorIcons_DirectionalLight.png", TextureSamplerType::Sampler2D, TextureUsageType::Normal);
	m_iconTemplate_PointLight = g_pModuleManager->getAssetSystem()->loadTexture("Res//Textures//InnoWorldEditorIcons_PointLight.png", TextureSamplerType::Sampler2D, TextureUsageType::Normal);
	m_iconTemplate_SphereLight = g_pModuleManager->getAssetSystem()->loadTexture("Res//Textures//InnoWorldEditorIcons_SphereLight.png", TextureSamplerType::Sampler2D, TextureUsageType::Normal);

	m_unitLineMesh = m_renderingServer->AddMeshDataComponent("UnitLineMesh/");
	g_pModuleManager->getAssetSystem()->addUnitLine(*m_unitLineMesh);
	m_unitLineMesh->m_meshPrimitiveTopology = MeshPrimitiveTopology::TriangleStrip;
	m_unitLineMesh->m_meshShapeType = MeshShapeType::Line;
	m_unitLineMesh->m_objectStatus = ObjectStatus::Created;
	g_pModuleManager->getPhysicsSystem()->generatePhysicsDataComponent(m_unitLineMesh);

	m_unitQuadMesh = m_renderingServer->AddMeshDataComponent("UnitQuadMesh/");
	g_pModuleManager->getAssetSystem()->addUnitQuad(*m_unitQuadMesh);
	m_unitQuadMesh->m_meshPrimitiveTopology = MeshPrimitiveTopology::Triangle;
	m_unitQuadMesh->m_meshShapeType = MeshShapeType::Quad;
	m_unitQuadMesh->m_objectStatus = ObjectStatus::Created;
	g_pModuleManager->getPhysicsSystem()->generatePhysicsDataComponent(m_unitQuadMesh);

	m_unitCubeMesh = m_renderingServer->AddMeshDataComponent("UnitCubeMesh/");
	g_pModuleManager->getAssetSystem()->addUnitCube(*m_unitCubeMesh);
	m_unitCubeMesh->m_meshPrimitiveTopology = MeshPrimitiveTopology::Triangle;
	m_unitCubeMesh->m_meshShapeType = MeshShapeType::Cube;
	m_unitCubeMesh->m_objectStatus = ObjectStatus::Created;
	g_pModuleManager->getPhysicsSystem()->generatePhysicsDataComponent(m_unitCubeMesh);

	m_unitSphereMesh = m_renderingServer->AddMeshDataComponent("UnitSphereMesh/");
	g_pModuleManager->getAssetSystem()->addUnitSphere(*m_unitSphereMesh);
	m_unitSphereMesh->m_meshPrimitiveTopology = MeshPrimitiveTopology::Triangle;
	m_unitSphereMesh->m_meshShapeType = MeshShapeType::Sphere;
	m_unitSphereMesh->m_objectStatus = ObjectStatus::Created;
	g_pModuleManager->getPhysicsSystem()->generatePhysicsDataComponent(m_unitSphereMesh);

	m_terrainMesh = m_renderingServer->AddMeshDataComponent("TerrainMesh/");
	g_pModuleManager->getAssetSystem()->addTerrain(*m_terrainMesh);
	m_terrainMesh->m_meshPrimitiveTopology = MeshPrimitiveTopology::Triangle;
	m_terrainMesh->m_objectStatus = ObjectStatus::Created;
	g_pModuleManager->getPhysicsSystem()->generatePhysicsDataComponent(m_terrainMesh);

	m_renderingServer->InitializeMeshDataComponent(m_unitLineMesh);
	m_renderingServer->InitializeMeshDataComponent(m_unitQuadMesh);
	m_renderingServer->InitializeMeshDataComponent(m_unitCubeMesh);
	m_renderingServer->InitializeMeshDataComponent(m_unitSphereMesh);
	m_renderingServer->InitializeMeshDataComponent(m_terrainMesh);

	m_renderingServer->InitializeTextureDataComponent(m_basicNormalTexture);
	m_renderingServer->InitializeTextureDataComponent(m_basicAlbedoTexture);
	m_renderingServer->InitializeTextureDataComponent(m_basicMetallicTexture);
	m_renderingServer->InitializeTextureDataComponent(m_basicRoughnessTexture);
	m_renderingServer->InitializeTextureDataComponent(m_basicAOTexture);

	m_renderingServer->InitializeTextureDataComponent(m_iconTemplate_OBJ);
	m_renderingServer->InitializeTextureDataComponent(m_iconTemplate_PNG);
	m_renderingServer->InitializeTextureDataComponent(m_iconTemplate_SHADER);
	m_renderingServer->InitializeTextureDataComponent(m_iconTemplate_UNKNOWN);

	m_renderingServer->InitializeTextureDataComponent(m_iconTemplate_DirectionalLight);
	m_renderingServer->InitializeTextureDataComponent(m_iconTemplate_PointLight);
	m_renderingServer->InitializeTextureDataComponent(m_iconTemplate_SphereLight);

	m_renderingServer->InitializeMaterialDataComponent(m_basicMaterial);

	return true;
}

bool InnoRenderingFrontendNS::initialize()
{
	if (InnoRenderingFrontendNS::m_objectStatus == ObjectStatus::Created)
	{
		loadDefaultAssets();

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
	auto l_cameraComponents = GetComponentManager(CameraComponent)->GetAllComponents();
	auto l_mainCamera = l_cameraComponents[0];
	auto l_mainCameraTransformComponent = GetComponent(TransformComponent, l_mainCamera->m_parentEntity);

	auto l_p = l_mainCamera->m_projectionMatrix;

	CameraGPUData l_CameraGPUData;

	l_CameraGPUData.p_original = l_p;
	l_CameraGPUData.p_jittered = l_p;

	if (m_renderingConfig.useTAA)
	{
		//TAA jitter for projection matrix
		auto& l_currentHaltonStep = currentHaltonStep;
		if (l_currentHaltonStep >= 16)
		{
			l_currentHaltonStep = 0;
		}
		l_CameraGPUData.p_jittered.m02 = m_haltonSampler[l_currentHaltonStep].x / m_screenResolution.x;
		l_CameraGPUData.p_jittered.m12 = m_haltonSampler[l_currentHaltonStep].y / m_screenResolution.y;
		l_currentHaltonStep += 1;
	}

	l_CameraGPUData.r =
		InnoMath::getInvertRotationMatrix(
			l_mainCameraTransformComponent->m_globalTransformVector.m_rot
		);

	l_CameraGPUData.t =
		InnoMath::getInvertTranslationMatrix(
			l_mainCameraTransformComponent->m_globalTransformVector.m_pos
		);

	l_CameraGPUData.r_prev = l_mainCameraTransformComponent->m_globalTransformMatrix_prev.m_rotationMat.inverse();
	l_CameraGPUData.t_prev = l_mainCameraTransformComponent->m_globalTransformMatrix_prev.m_translationMat.inverse();

	l_CameraGPUData.globalPos = l_mainCameraTransformComponent->m_globalTransformVector.m_pos;

	l_CameraGPUData.WHRatio = l_mainCamera->m_WHRatio;
	l_CameraGPUData.zNear = l_mainCamera->m_zNear;
	l_CameraGPUData.zFar = l_mainCamera->m_zFar;

	m_cameraGPUData.SetValue(l_CameraGPUData);

	return true;
}

bool InnoRenderingFrontendNS::updateSunData()
{
	auto l_directionalLightComponents = GetComponentManager(DirectionalLightComponent)->GetAllComponents();
	auto l_directionalLight = l_directionalLightComponents[0];
	auto l_directionalLightTransformComponent = GetComponent(TransformComponent, l_directionalLight->m_parentEntity);
	auto l_lightRotMat = l_directionalLightTransformComponent->m_globalTransformMatrix.m_rotationMat.inverse();

	SunGPUData l_SunGPUData;

	l_SunGPUData.dir = InnoMath::getDirection(direction::BACKWARD, l_directionalLightTransformComponent->m_globalTransformVector.m_rot);
	l_SunGPUData.luminance = l_directionalLight->m_color * l_directionalLight->m_luminousFlux;
	l_SunGPUData.r = InnoMath::getInvertRotationMatrix(l_directionalLightTransformComponent->m_globalTransformVector.m_rot);

	m_sunGPUData.SetValue(l_SunGPUData);

	auto l_SplitAABB = GetComponentManager(DirectionalLightComponent)->GetSplitAABB();
	auto l_ProjectionMatrices = GetComponentManager(DirectionalLightComponent)->GetProjectionMatrices();
	auto l_CSMSplitCount = l_ProjectionMatrices.size();

	m_CSMGPUData.clear();
	for (size_t j = 0; j < l_CSMSplitCount; j++)
	{
		m_CSMGPUData.emplace_back();

		m_CSMGPUData[j].p = l_ProjectionMatrices[j];
		m_CSMGPUData[j].AABBMax = l_SplitAABB[j].m_boundMax;
		m_CSMGPUData[j].AABBMin = l_SplitAABB[j].m_boundMin;
		m_CSMGPUData[j].v = l_lightRotMat;
	}

	return true;
}

bool InnoRenderingFrontendNS::updateSkyData()
{
	SkyGPUData l_SkyGPUData;

	l_SkyGPUData.p_inv = m_cameraGPUData.GetValue().p_original.inverse();
	l_SkyGPUData.r_inv = m_cameraGPUData.GetValue().r.inverse();
	l_SkyGPUData.viewportSize.x = (float)m_screenResolution.x;
	l_SkyGPUData.viewportSize.y = (float)m_screenResolution.y;

	m_skyGPUData.SetValue(l_SkyGPUData);

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

					if (l_cullingData.visiblilityType == VisiblilityType::Opaque)
					{
						OpaquePassGPUData l_opaquePassGPUData;

						l_opaquePassGPUData.mesh = l_cullingData.mesh;
						l_opaquePassGPUData.material = l_cullingData.material;

						MaterialGPUData l_materialGPUData;

						l_materialGPUData.useNormalTexture = !(l_opaquePassGPUData.material->m_normalTexture == nullptr);
						l_materialGPUData.useAlbedoTexture = !(l_opaquePassGPUData.material->m_albedoTexture == nullptr);
						l_materialGPUData.useMetallicTexture = !(l_opaquePassGPUData.material->m_metallicTexture == nullptr);
						l_materialGPUData.useRoughnessTexture = !(l_opaquePassGPUData.material->m_roughnessTexture == nullptr);
						l_materialGPUData.useAOTexture = !(l_opaquePassGPUData.material->m_aoTexture == nullptr);
						l_materialGPUData.materialType = int(l_cullingData.meshUsageType);
						l_materialGPUData.customMaterial = l_cullingData.material->m_meshCustomMaterial;

						m_opaquePassGPUData[l_opaquePassIndex] = l_opaquePassGPUData;
						m_opaquePassMeshGPUData[l_opaquePassIndex] = l_meshGPUData;
						m_opaquePassMaterialGPUData[l_opaquePassIndex] = l_materialGPUData;
						l_opaquePassIndex++;
					}
					else if (l_cullingData.visiblilityType == VisiblilityType::Transparent)
					{
						TransparentPassGPUData l_transparentPassGPUData;

						l_transparentPassGPUData.mesh = l_cullingData.mesh;
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
	auto l_t = m_cameraGPUData.GetValue().t;
	auto l_r = m_cameraGPUData.GetValue().r;

	std::sort(l_sortedTransparentPassGPUData.begin(), l_sortedTransparentPassGPUData.end(), [&](TransparentPassGPUData a, TransparentPassGPUData b) {
		auto m_a_InViewSpace = l_t * l_r * m_transparentPassMeshGPUData[a.meshGPUDataIndex].m;
		auto m_b_InViewSpace = l_t * l_r * m_transparentPassMeshGPUData[b.meshGPUDataIndex].m;
		return m_a_InViewSpace.m23 < m_b_InViewSpace.m23;
	});

	m_transparentPassGPUData = l_sortedTransparentPassGPUData;

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
		l_billboardPAssGPUData.distanceToCamera = (m_cameraGPUData.GetValue().globalPos - l_billboardPAssGPUData.globalPos).length();
		l_billboardPAssGPUData.iconType = WorldEditorIconType::DIRECTIONAL_LIGHT;

		m_billboardPassGPUData.emplace_back(l_billboardPAssGPUData);
		l_index++;
	}

	for (auto i : GetComponentManager(PointLightComponent)->GetAllComponents())
	{
		BillboardPassGPUData l_billboardPAssGPUData;
		l_billboardPAssGPUData.globalPos = GetComponent(TransformComponent, i->m_parentEntity)->m_globalTransformVector.m_pos;
		l_billboardPAssGPUData.distanceToCamera = (m_cameraGPUData.GetValue().globalPos - l_billboardPAssGPUData.globalPos).length();
		l_billboardPAssGPUData.iconType = WorldEditorIconType::POINT_LIGHT;

		m_billboardPassGPUData.emplace_back(l_billboardPAssGPUData);
		l_index++;
	}

	for (auto i : GetComponentManager(SphereLightComponent)->GetAllComponents())
	{
		BillboardPassGPUData l_billboardPAssGPUData;
		l_billboardPAssGPUData.globalPos = GetComponent(TransformComponent, i->m_parentEntity)->m_globalTransformVector.m_pos;
		l_billboardPAssGPUData.distanceToCamera = (m_cameraGPUData.GetValue().globalPos - l_billboardPAssGPUData.globalPos).length();
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
		if (visibleComponent->m_visiblilityType == VisiblilityType::Opaque
			&& visibleComponent->m_objectStatus == ObjectStatus::Activated
			&& visibleComponent->m_meshUsageType == MeshUsageType::Static
			)
		{
			auto l_transformComponent = GetComponent(TransformComponent, visibleComponent->m_parentEntity);
			auto l_globalTm = l_transformComponent->m_globalTransformMatrix.m_transformationMat;
			if (visibleComponent->m_PDC)
			{
				for (auto& l_modelPair : visibleComponent->m_modelMap)
				{
					OpaquePassGPUData l_GIPassGPUData;

					l_GIPassGPUData.mesh = l_modelPair.first;
					l_GIPassGPUData.material = l_modelPair.second;

					MeshGPUData l_meshGPUData;

					l_meshGPUData.m = l_transformComponent->m_globalTransformMatrix.m_transformationMat;
					l_meshGPUData.m_prev = l_transformComponent->m_globalTransformMatrix_prev.m_transformationMat;
					l_meshGPUData.normalMat = l_transformComponent->m_globalTransformMatrix.m_rotationMat;
					l_meshGPUData.UUID = (float)visibleComponent->m_UUID;

					MaterialGPUData l_materialGPUData;

					l_materialGPUData.useNormalTexture = !(l_GIPassGPUData.material->m_normalTexture == nullptr);
					l_materialGPUData.useAlbedoTexture = !(l_GIPassGPUData.material->m_albedoTexture == nullptr);
					l_materialGPUData.useMetallicTexture = !(l_GIPassGPUData.material->m_metallicTexture == nullptr);
					l_materialGPUData.useRoughnessTexture = !(l_GIPassGPUData.material->m_roughnessTexture == nullptr);
					l_materialGPUData.useAOTexture = !(l_GIPassGPUData.material->m_aoTexture == nullptr);

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

		updateSkyData();

		// copy culling data pack for local scope
		auto l_cullingDataPack = g_pModuleManager->getPhysicsSystem()->getCullingDataPack();
		if (l_cullingDataPack.has_value())
		{
			if ((*l_cullingDataPack).size() > 0)
			{
				m_cullingDataPack.setRawData(std::move(*l_cullingDataPack));
			}
		}

		updateMeshData();

		updateBillboardPassData();

		updateDebuggerPassData();

		while (m_uninitializedMeshes.size() > 0)
		{
			MeshDataComponent* l_Mesh;
			m_uninitializedMeshes.tryPop(l_Mesh);

			if (l_Mesh)
			{
				auto l_result = m_renderingServer->InitializeMeshDataComponent(l_Mesh);
				if (!l_result)
				{
					g_pModuleManager->getLogSystem()->printLog(LogType::INNO_ERROR, "InnoRenderingFrontend: can't initialize MeshDataComponent for " + std::string(l_Mesh->m_parentEntity->m_entityName.c_str()) + "!");
				}
			}
		}

		while (m_uninitializedMaterials.size() > 0)
		{
			MaterialDataComponent* l_Mesh;
			m_uninitializedMaterials.tryPop(l_Mesh);

			if (l_Mesh)
			{
				auto l_result = m_renderingServer->InitializeMaterialDataComponent(l_Mesh);
				if (!l_result)
				{
					g_pModuleManager->getLogSystem()->printLog(LogType::INNO_ERROR, "InnoRenderingFrontend: can't initialize MaterialDataComponent for " + std::string(l_Mesh->m_parentEntity->m_entityName.c_str()) + "!");
				}
			}
		}

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

bool InnoRenderingFrontend::setup(IRenderingServer* renderingServer)
{
	return InnoRenderingFrontendNS::setup(renderingServer);
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

bool InnoRenderingFrontend::runRayTrace()
{
	return InnoRenderingFrontendNS::m_rayTracer->Execute();
}

MeshDataComponent * InnoRenderingFrontend::addMeshDataComponent()
{
	return InnoRenderingFrontendNS::m_renderingServer->AddMeshDataComponent();
}

TextureDataComponent * InnoRenderingFrontend::addTextureDataComponent()
{
	return InnoRenderingFrontendNS::m_renderingServer->AddTextureDataComponent();
}

MaterialDataComponent * InnoRenderingFrontend::addMaterialDataComponent()
{
	return InnoRenderingFrontendNS::m_renderingServer->AddMaterialDataComponent();
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

MeshDataComponent * InnoRenderingFrontend::getMeshDataComponent(MeshShapeType meshShapeType)
{
	switch (meshShapeType)
	{
	case MeshShapeType::Line:
		return InnoRenderingFrontendNS::m_unitLineMesh; break;
	case MeshShapeType::Quad:
		return InnoRenderingFrontendNS::m_unitQuadMesh; break;
	case MeshShapeType::Cube:
		return InnoRenderingFrontendNS::m_unitCubeMesh; break;
	case MeshShapeType::Sphere:
		return InnoRenderingFrontendNS::m_unitSphereMesh; break;
	case MeshShapeType::Terrain:
		return InnoRenderingFrontendNS::m_terrainMesh; break;
	case MeshShapeType::Custom:
		g_pModuleManager->getLogSystem()->printLog(LogType::INNO_ERROR, "RenderingFrontend: wrong MeshShapeType!");
		return nullptr; break;
	default:
		return nullptr; break;
	}
}

TextureDataComponent * InnoRenderingFrontend::getTextureDataComponent(TextureUsageType textureUsageType)
{
	switch (textureUsageType)
	{
	case TextureUsageType::Invisible:
		return nullptr; break;
	case TextureUsageType::Normal:
		return InnoRenderingFrontendNS::m_basicNormalTexture; break;
	case TextureUsageType::Albedo:
		return InnoRenderingFrontendNS::m_basicAlbedoTexture; break;
	case TextureUsageType::Metallic:
		return InnoRenderingFrontendNS::m_basicMetallicTexture; break;
	case TextureUsageType::Roughness:
		return InnoRenderingFrontendNS::m_basicRoughnessTexture; break;
	case TextureUsageType::AmbientOcclusion:
		return InnoRenderingFrontendNS::m_basicAOTexture; break;
	case TextureUsageType::ColorAttachment:
		return nullptr; break;
	default:
		return nullptr; break;
	}
}

TextureDataComponent * InnoRenderingFrontend::getTextureDataComponent(FileExplorerIconType iconType)
{
	switch (iconType)
	{
	case FileExplorerIconType::OBJ:
		return InnoRenderingFrontendNS::m_iconTemplate_OBJ; break;
	case FileExplorerIconType::PNG:
		return InnoRenderingFrontendNS::m_iconTemplate_PNG; break;
	case FileExplorerIconType::SHADER:
		return InnoRenderingFrontendNS::m_iconTemplate_SHADER; break;
	case FileExplorerIconType::UNKNOWN:
		return InnoRenderingFrontendNS::m_iconTemplate_UNKNOWN; break;
	default:
		return nullptr; break;
	}
}

TextureDataComponent * InnoRenderingFrontend::getTextureDataComponent(WorldEditorIconType iconType)
{
	switch (iconType)
	{
	case WorldEditorIconType::DIRECTIONAL_LIGHT:
		return InnoRenderingFrontendNS::m_iconTemplate_DirectionalLight; break;
	case WorldEditorIconType::POINT_LIGHT:
		return InnoRenderingFrontendNS::m_iconTemplate_PointLight; break;
	case WorldEditorIconType::SPHERE_LIGHT:
		return InnoRenderingFrontendNS::m_iconTemplate_SphereLight; break;
	default:
		return nullptr; break;
	}
}

MaterialDataComponent * InnoRenderingFrontend::getDefaultMaterialDataComponent()
{
	return InnoRenderingFrontendNS::m_basicMaterial;
}

bool InnoRenderingFrontend::registerMeshDataComponent(MeshDataComponent * rhs)
{
	InnoRenderingFrontendNS::m_uninitializedMeshes.push(rhs);

	return true;
}

bool InnoRenderingFrontend::registerMaterialDataComponent(MaterialDataComponent * rhs)
{
	InnoRenderingFrontendNS::m_uninitializedMaterials.push(rhs);

	return true;
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

RenderPassDesc InnoRenderingFrontend::getDefaultRenderPassDesc()
{
	return InnoRenderingFrontendNS::m_DefaultRenderPassDesc;
}

CameraGPUData InnoRenderingFrontend::getCameraGPUData()
{
	return InnoRenderingFrontendNS::m_cameraGPUData.GetValue();
}

SunGPUData InnoRenderingFrontend::getSunGPUData()
{
	return InnoRenderingFrontendNS::m_sunGPUData.GetValue();
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
	return InnoRenderingFrontendNS::m_skyGPUData.GetValue();
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