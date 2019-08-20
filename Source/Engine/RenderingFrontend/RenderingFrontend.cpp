#include "RenderingFrontend.h"
#include "../Common/CommonMacro.inl"
#include "../ComponentManager/ITransformComponentManager.h"
#include "../ComponentManager/IVisibleComponentManager.h"
#include "../ComponentManager/IDirectionalLightComponentManager.h"
#include "../ComponentManager/IPointLightComponentManager.h"
#include "../ComponentManager/ISphereLightComponentManager.h"
#include "../ComponentManager/ICameraComponentManager.h"

#include "../Core/InnoLogger.h"

#include "../ModuleManager/IModuleManager.h"
extern IModuleManager* g_pModuleManager;

#include "../RayTracer/RayTracer.h"

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

	DoubleBuffer<CameraGPUData, true> m_cameraGPUData;
	DoubleBuffer<SunGPUData, true> m_sunGPUData;
	DoubleBuffer<std::vector<CSMGPUData>, true> m_CSMGPUData;
	DoubleBuffer<std::vector<PointLightGPUData>, true> m_pointLightGPUData;
	DoubleBuffer<std::vector<SphereLightGPUData>, true> m_sphereLightGPUData;
	DoubleBuffer<SkyGPUData, true> m_skyGPUData;

	unsigned int m_opaquePassDrawCallCount = 0;
	DoubleBuffer<std::vector<OpaquePassDrawCallData>, true> m_opaquePassDrawCallData;
	DoubleBuffer<std::vector<MeshGPUData>, true> m_opaquePassMeshGPUData;
	DoubleBuffer<std::vector<MaterialGPUData>, true> m_opaquePassMaterialGPUData;

	unsigned int m_transparentPassDrawCallCount = 0;
	std::vector<TransparentPassDrawCallData> m_transparentPassGPUData;
	std::vector<MeshGPUData> m_transparentPassMeshGPUData;
	std::vector<MaterialGPUData> m_transparentPassMaterialGPUData;

	DoubleBuffer<std::vector<BillboardPassDrawCallData>, true> m_billboardPassDrawCallData;
	DoubleBuffer<std::vector<MeshGPUData>, true> m_billboardMeshGPUData;

	unsigned int m_debuggerPassDrawCallCount = 0;
	std::vector<DebuggerPassGPUData> m_debuggerPassGPUData;

	ThreadSafeVector<CullingDataPack> m_cullingDataPack;

	std::vector<Plane> m_debugPlanes;
	std::vector<Sphere> m_debugSpheres;

	std::vector<vec2> m_haltonSampler;
	int currentHaltonStep = 0;

	std::function<void(RenderPassType)> f_reloadShader;
	std::function<void()> f_sceneLoadingStartCallback;
	std::function<void()> f_sceneLoadingFinishCallback;

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
	bool updatePointLightData();
	bool updateSphereLightData();
	bool updateMeshData();
	bool updateBillboardPassData();
	bool updateDebuggerPassData();
}

using namespace InnoRenderingFrontendNS;

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
	m_renderingConfig.CSMFitToScene = false;
	m_renderingConfig.CSMAdjustDrawDistance = true;
	m_renderingConfig.CSMAdjustSidePlane = false;

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

		m_opaquePassDrawCallCount = 0;

		m_transparentPassGPUData.clear();
		m_transparentPassMeshGPUData.clear();
		m_transparentPassMaterialGPUData.clear();
		m_transparentPassDrawCallCount = 0;

		m_debuggerPassGPUData.clear();
	};

	f_sceneLoadingFinishCallback = [&]() {
		m_transparentPassGPUData.resize(m_renderingCapability.maxMeshes);
		m_transparentPassMeshGPUData.resize(m_renderingCapability.maxMeshes);
		m_transparentPassMaterialGPUData.resize(m_renderingCapability.maxMaterials);
	};

	g_pModuleManager->getFileSystem()->addSceneLoadingStartCallback(&f_sceneLoadingStartCallback);
	g_pModuleManager->getFileSystem()->addSceneLoadingFinishCallback(&f_sceneLoadingFinishCallback);

	m_SkeletonDataComponentPool = g_pModuleManager->getMemorySystem()->allocateMemoryPool(sizeof(SkeletonDataComponent), 2048);
	m_AnimationDataComponentPool = g_pModuleManager->getMemorySystem()->allocateMemoryPool(sizeof(AnimationDataComponent), 16384);

	m_rayTracer->Setup();

	m_objectStatus = ObjectStatus::Created;
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

	m_unitQuadMesh = m_renderingServer->AddMeshDataComponent("UnitQuadMesh/");
	g_pModuleManager->getAssetSystem()->addUnitQuad(*m_unitQuadMesh);
	m_unitQuadMesh->m_meshPrimitiveTopology = MeshPrimitiveTopology::Triangle;
	m_unitQuadMesh->m_meshShapeType = MeshShapeType::Quad;
	m_unitQuadMesh->m_objectStatus = ObjectStatus::Created;

	m_unitCubeMesh = m_renderingServer->AddMeshDataComponent("UnitCubeMesh/");
	g_pModuleManager->getAssetSystem()->addUnitCube(*m_unitCubeMesh);
	m_unitCubeMesh->m_meshPrimitiveTopology = MeshPrimitiveTopology::Triangle;
	m_unitCubeMesh->m_meshShapeType = MeshShapeType::Cube;
	m_unitCubeMesh->m_objectStatus = ObjectStatus::Created;

	m_unitSphereMesh = m_renderingServer->AddMeshDataComponent("UnitSphereMesh/");
	g_pModuleManager->getAssetSystem()->addUnitSphere(*m_unitSphereMesh);
	m_unitSphereMesh->m_meshPrimitiveTopology = MeshPrimitiveTopology::Triangle;
	m_unitSphereMesh->m_meshShapeType = MeshShapeType::Sphere;
	m_unitSphereMesh->m_objectStatus = ObjectStatus::Created;

	m_terrainMesh = m_renderingServer->AddMeshDataComponent("TerrainMesh/");
	g_pModuleManager->getAssetSystem()->addTerrain(*m_terrainMesh);
	m_terrainMesh->m_meshPrimitiveTopology = MeshPrimitiveTopology::Triangle;
	m_terrainMesh->m_objectStatus = ObjectStatus::Created;

	auto l_DefaultAssetInitializeTask = g_pModuleManager->getTaskSystem()->submit("DefaultAssetInitializeTask", 2, nullptr,
		[&]() {
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
	});

	l_DefaultAssetInitializeTask->Wait();

	return true;
}

bool InnoRenderingFrontendNS::initialize()
{
	if (m_objectStatus == ObjectStatus::Created)
	{
		loadDefaultAssets();

		initializeHaltonSampler();
		m_rayTracer->Initialize();

		m_objectStatus = ObjectStatus::Activated;
		InnoLogger::Log(LogLevel::Success, "RenderingFrontend has been initialized.");
		return true;
	}
	else
	{
		InnoLogger::Log(LogLevel::Error, "RenderingFrontend: Object is not created!");
		return false;
	}
}

bool InnoRenderingFrontendNS::updateCameraData()
{
	auto l_mainCamera = GetComponentManager(CameraComponent)->GetMainCamera();

	if (l_mainCamera == nullptr)
	{
		return false;
	}

	auto l_mainCameraTransformComponent = GetComponent(TransformComponent, l_mainCamera->m_parentEntity);

	if (l_mainCameraTransformComponent == nullptr)
	{
		return false;
	}

	CameraGPUData l_CameraGPUData;

	auto l_p = l_mainCamera->m_projectionMatrix;

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

	m_cameraGPUData.SetValue(std::move(l_CameraGPUData));

	return true;
}

bool InnoRenderingFrontendNS::updateSunData()
{
	std::vector<CSMGPUData> l_CSMGPUData;

	l_CSMGPUData.resize(m_renderingCapability.maxCSMSplits);

	auto l_directionalLightComponents = GetComponentManager(DirectionalLightComponent)->GetAllComponents();

	if (l_directionalLightComponents.size() > 0)
	{
		auto l_directionalLight = l_directionalLightComponents[0];
		auto l_directionalLightTransformComponent = GetComponent(TransformComponent, l_directionalLight->m_parentEntity);

		if (l_directionalLightTransformComponent == nullptr)
		{
			return false;
		}

		auto l_lightRotMat = l_directionalLightTransformComponent->m_globalTransformMatrix.m_rotationMat.inverse();

		SunGPUData l_SunGPUData;

		l_SunGPUData.dir = InnoMath::getDirection(direction::BACKWARD, l_directionalLightTransformComponent->m_globalTransformVector.m_rot);
		l_SunGPUData.luminance = l_directionalLight->m_RGBColor * l_directionalLight->m_LuminousFlux;
		l_SunGPUData.r = InnoMath::getInvertRotationMatrix(l_directionalLightTransformComponent->m_globalTransformVector.m_rot);

		m_sunGPUData.SetValue(std::move(l_SunGPUData));

		auto l_SplitAABB = GetComponentManager(DirectionalLightComponent)->GetSplitAABB();
		auto l_ProjectionMatrices = GetComponentManager(DirectionalLightComponent)->GetProjectionMatrices();
		if (l_SplitAABB.size() > 0 && l_ProjectionMatrices.size() > 0)
		{
			for (size_t j = 0; j < m_renderingCapability.maxCSMSplits; j++)
			{
				l_CSMGPUData[j].p = l_ProjectionMatrices[j];
				l_CSMGPUData[j].AABBMax = l_SplitAABB[j].m_boundMax;
				l_CSMGPUData[j].AABBMin = l_SplitAABB[j].m_boundMin;
				l_CSMGPUData[j].v = l_lightRotMat;
			}

			m_CSMGPUData.SetValue(std::move(l_CSMGPUData));
		}
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

	m_skyGPUData.SetValue(std::move(l_SkyGPUData));

	return true;
}

bool InnoRenderingFrontendNS::updatePointLightData()
{
	auto l_pointLightComponents = GetComponentManager(PointLightComponent)->GetAllComponents();
	auto l_pointLightComponentCount = l_pointLightComponents.size();
	std::vector<PointLightGPUData> l_PointLightGPUData;

	if (l_pointLightComponentCount > 0)
	{
		l_PointLightGPUData.resize(l_pointLightComponentCount);

		for (size_t i = 0; i < l_pointLightComponentCount; i++)
		{
			auto l_transformCompoent = GetComponent(TransformComponent, l_pointLightComponents[i]->m_parentEntity);
			if (l_transformCompoent != nullptr)
			{
				l_PointLightGPUData[i].pos = l_transformCompoent->m_globalTransformVector.m_pos;
				l_PointLightGPUData[i].luminance = l_pointLightComponents[i]->m_RGBColor * l_pointLightComponents[i]->m_LuminousFlux;
				l_PointLightGPUData[i].luminance.w = l_pointLightComponents[i]->m_attenuationRadius;
			}
		}

		m_pointLightGPUData.SetValue(std::move(l_PointLightGPUData));
	}

	return true;
}

bool InnoRenderingFrontendNS::updateSphereLightData()
{
	auto l_sphereLightComponents = GetComponentManager(SphereLightComponent)->GetAllComponents();
	auto l_sphereLightComponentCount = l_sphereLightComponents.size();
	std::vector<SphereLightGPUData> l_SphereLightGPUData;

	if (l_sphereLightComponentCount > 0)
	{
		l_SphereLightGPUData.resize(l_sphereLightComponentCount);

		for (size_t i = 0; i < l_sphereLightComponentCount; i++)
		{
			auto l_transformCompoent = GetComponent(TransformComponent, l_sphereLightComponents[i]->m_parentEntity);
			if (l_transformCompoent != nullptr)
			{
				l_SphereLightGPUData[i].pos = l_transformCompoent->m_globalTransformVector.m_pos;
				l_SphereLightGPUData[i].luminance = l_sphereLightComponents[i]->m_RGBColor * l_sphereLightComponents[i]->m_LuminousFlux;
				l_SphereLightGPUData[i].luminance.w = l_sphereLightComponents[i]->m_sphereRadius;
			}
		}

		m_sphereLightGPUData.SetValue(std::move(l_SphereLightGPUData));
	}

	return true;
}

bool InnoRenderingFrontendNS::updateMeshData()
{
	unsigned int l_opaquePassIndex = 0;
	unsigned int l_transparentPassIndex = 0;

	auto l_cullingDataSize = m_cullingDataPack.size();

	std::vector<OpaquePassDrawCallData> l_opaquePassDrawCallDataVector;
	l_opaquePassDrawCallDataVector.resize(l_cullingDataSize);
	std::vector<MeshGPUData> l_opaquePassMeshGPUData;
	l_opaquePassMeshGPUData.resize(l_cullingDataSize);
	std::vector<MaterialGPUData> l_opaquePassMaterialGPUData;
	l_opaquePassMaterialGPUData.resize(l_cullingDataSize);

	std::vector<TransparentPassDrawCallData> l_sortedTransparentPassDrawCallData;

	for (size_t i = 0; i < l_cullingDataSize; i++)
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
						OpaquePassDrawCallData l_opaquePassDrawCallData;

						l_opaquePassDrawCallData.mesh = l_cullingData.mesh;
						l_opaquePassDrawCallData.material = l_cullingData.material;

						MaterialGPUData l_materialGPUData;

						l_materialGPUData.useNormalTexture = !(l_opaquePassDrawCallData.material->m_normalTexture == nullptr);
						l_materialGPUData.useAlbedoTexture = !(l_opaquePassDrawCallData.material->m_albedoTexture == nullptr);
						l_materialGPUData.useMetallicTexture = !(l_opaquePassDrawCallData.material->m_metallicTexture == nullptr);
						l_materialGPUData.useRoughnessTexture = !(l_opaquePassDrawCallData.material->m_roughnessTexture == nullptr);
						l_materialGPUData.useAOTexture = !(l_opaquePassDrawCallData.material->m_aoTexture == nullptr);
						l_materialGPUData.materialType = int(l_cullingData.meshUsageType);
						l_materialGPUData.customMaterial = l_cullingData.material->m_meshCustomMaterial;

						l_opaquePassDrawCallDataVector[l_opaquePassIndex] = l_opaquePassDrawCallData;
						l_opaquePassMeshGPUData[l_opaquePassIndex] = l_meshGPUData;
						l_opaquePassMaterialGPUData[l_opaquePassIndex] = l_materialGPUData;

						l_opaquePassIndex++;
					}
					else if (l_cullingData.visiblilityType == VisiblilityType::Transparent)
					{
						TransparentPassDrawCallData l_transparentPassGPUData;

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

						l_sortedTransparentPassDrawCallData.emplace_back(l_transparentPassGPUData);
						m_transparentPassMeshGPUData[l_transparentPassIndex] = l_meshGPUData;
						m_transparentPassMaterialGPUData[l_transparentPassIndex] = l_materialGPUData;
						l_transparentPassIndex++;
					}
				}
			}
		}
	}

	m_opaquePassDrawCallData.SetValue(std::move(l_opaquePassDrawCallDataVector));
	m_opaquePassMeshGPUData.SetValue(std::move(l_opaquePassMeshGPUData));
	m_opaquePassMaterialGPUData.SetValue(std::move(l_opaquePassMaterialGPUData));

	m_opaquePassDrawCallCount = l_opaquePassIndex;
	m_transparentPassDrawCallCount = l_transparentPassIndex;

	// @TODO: use GPU to do OIT
	auto l_t = m_cameraGPUData.GetValue().t;
	auto l_r = m_cameraGPUData.GetValue().r;

	std::sort(l_sortedTransparentPassDrawCallData.begin(), l_sortedTransparentPassDrawCallData.end(), [&](TransparentPassDrawCallData a, TransparentPassDrawCallData b) {
		auto m_a_InViewSpace = l_t * l_r * m_transparentPassMeshGPUData[a.meshGPUDataIndex].m;
		auto m_b_InViewSpace = l_t * l_r * m_transparentPassMeshGPUData[b.meshGPUDataIndex].m;
		return m_a_InViewSpace.m23 < m_b_InViewSpace.m23;
	});

	m_transparentPassGPUData = l_sortedTransparentPassDrawCallData;

	return true;
}

bool InnoRenderingFrontendNS::updateBillboardPassData()
{
	auto l_directionalLightComponents = GetComponentManager(DirectionalLightComponent)->GetAllComponents();
	auto l_pointLightComponents = GetComponentManager(PointLightComponent)->GetAllComponents();
	auto l_sphereLightComponents = GetComponentManager(SphereLightComponent)->GetAllComponents();

	std::vector<BillboardPassDrawCallData> l_billboardPassDrawCallDataVector;
	l_billboardPassDrawCallDataVector.resize(3);
	l_billboardPassDrawCallDataVector[0].iconTexture = g_pModuleManager->getRenderingFrontend()->getTextureDataComponent(WorldEditorIconType::DIRECTIONAL_LIGHT);
	l_billboardPassDrawCallDataVector[0].instanceCount = (unsigned int)l_directionalLightComponents.size();
	l_billboardPassDrawCallDataVector[1].iconTexture = g_pModuleManager->getRenderingFrontend()->getTextureDataComponent(WorldEditorIconType::POINT_LIGHT);
	l_billboardPassDrawCallDataVector[1].instanceCount = (unsigned int)l_pointLightComponents.size();
	l_billboardPassDrawCallDataVector[2].iconTexture = g_pModuleManager->getRenderingFrontend()->getTextureDataComponent(WorldEditorIconType::SPHERE_LIGHT);
	l_billboardPassDrawCallDataVector[2].instanceCount = (unsigned int)l_sphereLightComponents.size();

	auto l_totalBillboardDrawCallCount = l_directionalLightComponents.size() + l_pointLightComponents.size() + l_sphereLightComponents.size();

	std::vector<MeshGPUData> l_billboardPassMeshGPUData;
	l_billboardPassMeshGPUData.reserve(l_totalBillboardDrawCallCount);

	unsigned int l_index = 0;

	l_billboardPassDrawCallDataVector[0].meshGPUDataOffset = l_index;
	for (auto i : l_directionalLightComponents)
	{
		MeshGPUData l_meshGPUData;

		auto l_transformCompoent = GetComponent(TransformComponent, i->m_parentEntity);
		if (l_transformCompoent != nullptr)
		{
			l_meshGPUData.m = InnoMath::toTranslationMatrix(l_transformCompoent->m_globalTransformVector.m_pos);
		}

		l_billboardPassMeshGPUData.emplace_back(l_meshGPUData);

		l_index++;
	}

	l_billboardPassDrawCallDataVector[1].meshGPUDataOffset = l_index;
	for (auto i : l_pointLightComponents)
	{
		MeshGPUData l_meshGPUData;

		auto l_transformCompoent = GetComponent(TransformComponent, i->m_parentEntity);
		if (l_transformCompoent != nullptr)
		{
			l_meshGPUData.m = InnoMath::toTranslationMatrix(l_transformCompoent->m_globalTransformVector.m_pos);
		}

		l_billboardPassMeshGPUData.emplace_back(l_meshGPUData);

		l_index++;
	}

	l_billboardPassDrawCallDataVector[2].meshGPUDataOffset = l_index;
	for (auto i : l_sphereLightComponents)
	{
		MeshGPUData l_meshGPUData;

		auto l_transformCompoent = GetComponent(TransformComponent, i->m_parentEntity);
		if (l_transformCompoent != nullptr)
		{
			l_meshGPUData.m = InnoMath::toTranslationMatrix(l_transformCompoent->m_globalTransformVector.m_pos);
		}

		l_billboardPassMeshGPUData.emplace_back(l_meshGPUData);

		l_index++;
	}

	m_billboardPassDrawCallData.SetValue(std::move(l_billboardPassDrawCallDataVector));
	m_billboardMeshGPUData.SetValue(std::move(l_billboardPassMeshGPUData));

	return true;
}

bool InnoRenderingFrontendNS::updateDebuggerPassData()
{
	unsigned int l_index = 0;
	// @TODO: Implementation
	m_debuggerPassDrawCallCount = l_index;

	return true;
}

bool InnoRenderingFrontendNS::update()
{
	if (m_objectStatus == ObjectStatus::Activated)
	{
		updateCameraData();

		updateSunData();
		updatePointLightData();
		updateSphereLightData();
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

		return true;
	}
	else
	{
		m_objectStatus = ObjectStatus::Suspended;
		return false;
	}
}

bool InnoRenderingFrontendNS::terminate()
{
	m_rayTracer->Terminate();

	m_objectStatus = ObjectStatus::Terminated;
	InnoLogger::Log(LogLevel::Success, "RenderingFrontend has been terminated.");
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
	auto l_rawPtr = g_pModuleManager->getMemorySystem()->spawnObject(m_SkeletonDataComponentPool, sizeof(SkeletonDataComponent));
	auto l_SDC = new(l_rawPtr)SkeletonDataComponent();
	auto l_parentEntity = g_pModuleManager->getEntityManager()->Spawn(ObjectSource::Runtime, ObjectUsage::Engine, ("Skeleton_" + std::to_string(skeletonCount) + "/").c_str());
	l_SDC->m_parentEntity = l_parentEntity;
	l_SDC->m_objectSource = ObjectSource::Runtime;
	l_SDC->m_objectUsage = ObjectUsage::Engine;
	l_SDC->m_ComponentType = ComponentType::SkeletonDataComponent;
	skeletonCount++;
	return l_SDC;
}

AnimationDataComponent * InnoRenderingFrontend::addAnimationDataComponent()
{
	static std::atomic<unsigned int> animationCount = 0;
	auto l_rawPtr = g_pModuleManager->getMemorySystem()->spawnObject(m_AnimationDataComponentPool, sizeof(AnimationDataComponent));
	auto l_ADC = new(l_rawPtr)AnimationDataComponent();
	auto l_parentEntity = g_pModuleManager->getEntityManager()->Spawn(ObjectSource::Runtime, ObjectUsage::Engine, ("Animation_" + std::to_string(animationCount) + "/").c_str());
	l_ADC->m_parentEntity = l_parentEntity;
	l_ADC->m_objectSource = ObjectSource::Runtime;
	l_ADC->m_objectUsage = ObjectUsage::Engine;
	l_ADC->m_ComponentType = ComponentType::AnimationDataComponent;
	animationCount++;
	return l_ADC;
}

MeshDataComponent * InnoRenderingFrontend::getMeshDataComponent(MeshShapeType meshShapeType)
{
	switch (meshShapeType)
	{
	case MeshShapeType::Line:
		return m_unitLineMesh; break;
	case MeshShapeType::Quad:
		return m_unitQuadMesh; break;
	case MeshShapeType::Cube:
		return m_unitCubeMesh; break;
	case MeshShapeType::Sphere:
		return m_unitSphereMesh; break;
	case MeshShapeType::Terrain:
		return m_terrainMesh; break;
	case MeshShapeType::Custom:
		InnoLogger::Log(LogLevel::Error, "RenderingFrontend: wrong MeshShapeType!");
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
		return m_basicNormalTexture; break;
	case TextureUsageType::Albedo:
		return m_basicAlbedoTexture; break;
	case TextureUsageType::Metallic:
		return m_basicMetallicTexture; break;
	case TextureUsageType::Roughness:
		return m_basicRoughnessTexture; break;
	case TextureUsageType::AmbientOcclusion:
		return m_basicAOTexture; break;
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
		return m_iconTemplate_OBJ; break;
	case FileExplorerIconType::PNG:
		return m_iconTemplate_PNG; break;
	case FileExplorerIconType::SHADER:
		return m_iconTemplate_SHADER; break;
	case FileExplorerIconType::UNKNOWN:
		return m_iconTemplate_UNKNOWN; break;
	default:
		return nullptr; break;
	}
}

TextureDataComponent * InnoRenderingFrontend::getTextureDataComponent(WorldEditorIconType iconType)
{
	switch (iconType)
	{
	case WorldEditorIconType::DIRECTIONAL_LIGHT:
		return m_iconTemplate_DirectionalLight; break;
	case WorldEditorIconType::POINT_LIGHT:
		return m_iconTemplate_PointLight; break;
	case WorldEditorIconType::SPHERE_LIGHT:
		return m_iconTemplate_SphereLight; break;
	default:
		return nullptr; break;
	}
}

MaterialDataComponent * InnoRenderingFrontend::getDefaultMaterialDataComponent()
{
	return m_basicMaterial;
}

bool InnoRenderingFrontend::transferDataToGPU()
{
	while (m_uninitializedMeshes.size() > 0)
	{
		MeshDataComponent* l_Mesh;
		m_uninitializedMeshes.tryPop(l_Mesh);

		if (l_Mesh)
		{
			auto l_result = m_renderingServer->InitializeMeshDataComponent(l_Mesh);
		}
	}

	while (m_uninitializedMaterials.size() > 0)
	{
		MaterialDataComponent* l_Mesh;
		m_uninitializedMaterials.tryPop(l_Mesh);

		if (l_Mesh)
		{
			auto l_result = m_renderingServer->InitializeMaterialDataComponent(l_Mesh);
		}
	}

	return true;
}

bool InnoRenderingFrontend::registerMeshDataComponent(MeshDataComponent * rhs, bool AsyncUploadToGPU)
{
	if (AsyncUploadToGPU)
	{
		m_uninitializedMeshes.push(rhs);
	}
	else
	{
		auto l_MeshDataComponentInitializeTask = g_pModuleManager->getTaskSystem()->submit("MeshDataComponentInitializeTask", 2, nullptr,
			[=]() {m_renderingServer->InitializeMeshDataComponent(rhs); });
		l_MeshDataComponentInitializeTask->Wait();
	}

	return true;
}

bool InnoRenderingFrontend::registerMaterialDataComponent(MaterialDataComponent * rhs, bool AsyncUploadToGPU)
{
	if (AsyncUploadToGPU)
	{
		m_uninitializedMaterials.push(rhs);
	}
	else
	{
		auto l_MaterialDataComponentInitializeTask = g_pModuleManager->getTaskSystem()->submit("MaterialDataComponentInitializeTask", 2, nullptr,
			[=]() {m_renderingServer->InitializeMaterialDataComponent(rhs); });
		l_MaterialDataComponentInitializeTask->Wait();
	}

	return true;
}

TVec2<unsigned int> InnoRenderingFrontend::getScreenResolution()
{
	return m_screenResolution;
}

bool InnoRenderingFrontend::setScreenResolution(TVec2<unsigned int> screenResolution)
{
	m_screenResolution = screenResolution;
	return true;
}

RenderingConfig InnoRenderingFrontend::getRenderingConfig()
{
	return m_renderingConfig;
}

bool InnoRenderingFrontend::setRenderingConfig(RenderingConfig renderingConfig)
{
	m_renderingConfig = renderingConfig;
	return true;
}

RenderingCapability InnoRenderingFrontend::getRenderingCapability()
{
	return m_renderingCapability;
}

RenderPassDesc InnoRenderingFrontend::getDefaultRenderPassDesc()
{
	return m_DefaultRenderPassDesc;
}

const CameraGPUData& InnoRenderingFrontend::getCameraGPUData()
{
	return m_cameraGPUData.GetValue();
}

const SunGPUData& InnoRenderingFrontend::getSunGPUData()
{
	return m_sunGPUData.GetValue();
}

const std::vector<CSMGPUData>& InnoRenderingFrontend::getCSMGPUData()
{
	return m_CSMGPUData.GetValue();
}

const std::vector<PointLightGPUData>& InnoRenderingFrontend::getPointLightGPUData()
{
	return m_pointLightGPUData.GetValue();
}

const std::vector<SphereLightGPUData>& InnoRenderingFrontend::getSphereLightGPUData()
{
	return m_sphereLightGPUData.GetValue();
}

const SkyGPUData& InnoRenderingFrontend::getSkyGPUData()
{
	return m_skyGPUData.GetValue();
}

unsigned int InnoRenderingFrontend::getOpaquePassDrawCallCount()
{
	return m_opaquePassDrawCallCount;
}

const std::vector<OpaquePassDrawCallData>& InnoRenderingFrontend::getOpaquePassDrawCallData()
{
	return m_opaquePassDrawCallData.GetValue();
}

const std::vector<MeshGPUData>& InnoRenderingFrontend::getOpaquePassMeshGPUData()
{
	return m_opaquePassMeshGPUData.GetValue();
}

const std::vector<MaterialGPUData>& InnoRenderingFrontend::getOpaquePassMaterialGPUData()
{
	return m_opaquePassMaterialGPUData.GetValue();
}

unsigned int InnoRenderingFrontend::getTransparentPassDrawCallCount()
{
	return m_transparentPassDrawCallCount;
}

const std::vector<TransparentPassDrawCallData>& InnoRenderingFrontend::getTransparentPassDrawCallData()
{
	return m_transparentPassGPUData;
}

const std::vector<MeshGPUData>& InnoRenderingFrontend::getTransparentPassMeshGPUData()
{
	return m_transparentPassMeshGPUData;
}

const std::vector<MaterialGPUData>& InnoRenderingFrontend::getTransparentPassMaterialGPUData()
{
	return m_transparentPassMaterialGPUData;
}

const std::vector<BillboardPassDrawCallData>& InnoRenderingFrontend::getBillboardPassDrawCallData()
{
	return m_billboardPassDrawCallData.GetValue();
}

const std::vector<MeshGPUData>& InnoRenderingFrontend::getBillboardPassMeshGPUData()
{
	return m_billboardMeshGPUData.GetValue();
}

unsigned int InnoRenderingFrontend::getDebuggerPassDrawCallCount()
{
	return m_debuggerPassDrawCallCount;
}

const std::vector<DebuggerPassGPUData>& InnoRenderingFrontend::getDebuggerPassGPUData()
{
	return m_debuggerPassGPUData;
}