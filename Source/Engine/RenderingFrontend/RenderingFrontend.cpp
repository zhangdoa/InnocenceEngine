#include "RenderingFrontend.h"
#include "../Common/CommonMacro.inl"
#include "../ComponentManager/ITransformComponentManager.h"
#include "../ComponentManager/IVisibleComponentManager.h"
#include "../ComponentManager/ILightComponentManager.h"
#include "../ComponentManager/ICameraComponentManager.h"

#include "../Core/InnoLogger.h"
#include "../Core/InnoMemory.h"

#include "../ModuleManager/IModuleManager.h"
extern IModuleManager* g_pModuleManager;

#include "../RayTracer/RayTracer.h"

namespace InnoRenderingFrontendNS
{
	ObjectStatus m_ObjectStatus = ObjectStatus::Terminated;

	IRenderingServer* m_renderingServer;
	IRayTracer* m_rayTracer;

	TVec2<uint32_t> m_screenResolution = TVec2<uint32_t>(1280, 720);
	std::string m_windowName;
	bool m_fullScreen = false;

	RenderingCapability m_renderingCapability;

	RenderPassDesc m_DefaultRenderPassDesc;

	DoubleBuffer<CameraGPUData, true> m_cameraGPUData;
	DoubleBuffer<SunGPUData, true> m_sunGPUData;

	DoubleBuffer<std::vector<CSMGPUData>, true> m_CSMGPUDataVector;

	DoubleBuffer<std::vector<PointLightGPUData>, true> m_pointLightGPUData;
	DoubleBuffer<std::vector<SphereLightGPUData>, true> m_sphereLightGPUData;

	DoubleBuffer<SkyGPUData, true> m_skyGPUData;

	uint32_t m_sunShadowPassDrawCallCount = 0;
	DoubleBuffer<std::vector<OpaquePassDrawCallData>, true> m_sunShadowPassDrawCallDataVector;
	DoubleBuffer<std::vector<MeshGPUData>, true> m_sunShadowPassMeshGPUData;

	uint32_t m_opaquePassDrawCallCount = 0;
	DoubleBuffer<std::vector<OpaquePassDrawCallData>, true> m_opaquePassDrawCallDataVector;
	DoubleBuffer<std::vector<MeshGPUData>, true> m_opaquePassMeshGPUData;
	DoubleBuffer<std::vector<MaterialGPUData>, true> m_opaquePassMaterialGPUData;

	uint32_t m_transparentPassDrawCallCount = 0;
	DoubleBuffer<std::vector<TransparentPassDrawCallData>, true> m_transparentPassDrawCallDataVector;
	DoubleBuffer<std::vector<MeshGPUData>, true> m_transparentPassMeshGPUData;
	DoubleBuffer<std::vector<MaterialGPUData>, true> m_transparentPassMaterialGPUData;

	DoubleBuffer<std::vector<MeshGPUData>, true> m_directionalLightMeshGPUData;
	DoubleBuffer<std::vector<MeshGPUData>, true> m_pointLightMeshGPUData;
	DoubleBuffer<std::vector<MeshGPUData>, true> m_sphereLightMeshGPUData;

	DoubleBuffer<std::vector<BillboardPassDrawCallData>, true> m_billboardPassDrawCallDataVector;
	DoubleBuffer<std::vector<MeshGPUData>, true> m_billboardPassMeshGPUData;

	uint32_t m_debugPassDrawCallCount = 0;
	DoubleBuffer<std::vector<DebugPassDrawCallData>, true> m_debugPassDrawCallDataVector;
	DoubleBuffer<std::vector<MeshGPUData>, true> m_debugPassMeshGPUData;

	std::vector<CullingData> m_cullingData;

	std::vector<Vec2> m_haltonSampler;
	int32_t m_currentHaltonStep = 0;

	std::function<void()> f_sceneLoadingStartCallback;
	std::function<void()> f_sceneLoadingFinishCallback;

	RenderingConfig m_renderingConfig = RenderingConfig();

	IObjectPool* m_SkeletonDataComponentPool;
	IObjectPool* m_AnimationDataComponentPool;

	ThreadSafeQueue<MeshDataComponent*> m_uninitializedMeshes;
	ThreadSafeQueue<MaterialDataComponent*> m_uninitializedMaterials;

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

	float radicalInverse(uint32_t n, uint32_t base);
	void initializeHaltonSampler();

	bool updateCameraData();
	bool updateSunData();
	bool updateSkyData();
	bool updateLightData();

	bool updateMeshData();
	bool updateBillboardPassData();
	bool updateDebuggerPassData();
}

using namespace InnoRenderingFrontendNS;

float InnoRenderingFrontendNS::radicalInverse(uint32_t n, uint32_t base)
{
	float val = 0.0f;
	float invBase = 1.0f / base, invBi = invBase;
	while (n > 0)
	{
		auto d_i = (n % base);
		val += d_i * invBi;
		n *= (uint32_t)invBase;
		invBi *= invBase;
	}
	return val;
};

void InnoRenderingFrontendNS::initializeHaltonSampler()
{
	// in NDC space
	for (uint32_t i = 0; i < 16; i++)
	{
		m_haltonSampler.emplace_back(Vec2(radicalInverse(i, 3) * 2.0f - 1.0f, radicalInverse(i, 4) * 2.0f - 1.0f));
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
	m_renderingCapability.maxMeshes = 32768;
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

	m_CSMGPUDataVector.Reserve(m_renderingCapability.maxCSMSplits);

	m_transparentPassDrawCallDataVector.Reserve(m_renderingCapability.maxMeshes);
	m_transparentPassMeshGPUData.Reserve(m_renderingCapability.maxMeshes);
	m_transparentPassMaterialGPUData.Reserve(m_renderingCapability.maxMaterials);

	m_opaquePassDrawCallDataVector.Reserve(m_renderingCapability.maxMeshes);
	m_opaquePassMeshGPUData.Reserve(m_renderingCapability.maxMeshes);
	m_opaquePassMaterialGPUData.Reserve(m_renderingCapability.maxMaterials);

	m_sunShadowPassDrawCallDataVector.Reserve(m_renderingCapability.maxMeshes);
	m_sunShadowPassMeshGPUData.Reserve(m_renderingCapability.maxMeshes);

	m_pointLightGPUData.Reserve(m_renderingCapability.maxPointLights);
	m_sphereLightGPUData.Reserve(m_renderingCapability.maxSphereLights);

	m_directionalLightMeshGPUData.Reserve(8192);
	m_pointLightMeshGPUData.Reserve(8192);
	m_sphereLightMeshGPUData.Reserve(8192);
	m_billboardPassMeshGPUData.Reserve(8192);

	f_sceneLoadingStartCallback = [&]() {
		m_cullingData.clear();

		m_sunShadowPassDrawCallCount = 0;
		m_opaquePassDrawCallCount = 0;
		m_transparentPassDrawCallCount = 0;
		m_debugPassDrawCallCount = 0;
	};

	f_sceneLoadingFinishCallback = [&]()
	{
		// @TODO:
		std::vector<BillboardPassDrawCallData> l_billboardPassDrawCallDataVectorA(3);
		l_billboardPassDrawCallDataVectorA[0].iconTexture = g_pModuleManager->getRenderingFrontend()->getTextureDataComponent(WorldEditorIconType::DIRECTIONAL_LIGHT);
		l_billboardPassDrawCallDataVectorA[1].iconTexture = g_pModuleManager->getRenderingFrontend()->getTextureDataComponent(WorldEditorIconType::POINT_LIGHT);
		l_billboardPassDrawCallDataVectorA[2].iconTexture = g_pModuleManager->getRenderingFrontend()->getTextureDataComponent(WorldEditorIconType::SPHERE_LIGHT);
		auto l_billboardPassDrawCallDataVectorB = l_billboardPassDrawCallDataVectorA;

		m_billboardPassDrawCallDataVector.SetValue(std::move(l_billboardPassDrawCallDataVectorA));
		m_billboardPassDrawCallDataVector.SetValue(std::move(l_billboardPassDrawCallDataVectorB));
	};

	g_pModuleManager->getFileSystem()->addSceneLoadingStartCallback(&f_sceneLoadingStartCallback);
	g_pModuleManager->getFileSystem()->addSceneLoadingFinishCallback(&f_sceneLoadingFinishCallback);

	m_SkeletonDataComponentPool = InnoMemory::CreateObjectPool(sizeof(SkeletonDataComponent), 2048);
	m_AnimationDataComponentPool = InnoMemory::CreateObjectPool(sizeof(AnimationDataComponent), 16384);

	m_rayTracer->Setup();

	m_ObjectStatus = ObjectStatus::Created;
	return true;
}

bool InnoRenderingFrontendNS::loadDefaultAssets()
{
	m_basicNormalTexture = g_pModuleManager->getFileSystem()->loadTexture("Res//Textures//basic_normal.png");
	m_basicNormalTexture->m_textureDataDesc.SamplerType = TextureSamplerType::Sampler2D;
	m_basicNormalTexture->m_textureDataDesc.UsageType = TextureUsageType::Sample;

	m_basicAlbedoTexture = g_pModuleManager->getFileSystem()->loadTexture("Res//Textures//basic_albedo.png");
	m_basicAlbedoTexture->m_textureDataDesc.SamplerType = TextureSamplerType::Sampler2D;
	m_basicAlbedoTexture->m_textureDataDesc.UsageType = TextureUsageType::Sample;

	m_basicMetallicTexture = g_pModuleManager->getFileSystem()->loadTexture("Res//Textures//basic_metallic.png");
	m_basicMetallicTexture->m_textureDataDesc.SamplerType = TextureSamplerType::Sampler2D;
	m_basicMetallicTexture->m_textureDataDesc.UsageType = TextureUsageType::Sample;

	m_basicRoughnessTexture = g_pModuleManager->getFileSystem()->loadTexture("Res//Textures//basic_roughness.png");
	m_basicRoughnessTexture->m_textureDataDesc.SamplerType = TextureSamplerType::Sampler2D;
	m_basicRoughnessTexture->m_textureDataDesc.UsageType = TextureUsageType::Sample;

	m_basicAOTexture = g_pModuleManager->getFileSystem()->loadTexture("Res//Textures//basic_ao.png");
	m_basicAOTexture->m_textureDataDesc.SamplerType = TextureSamplerType::Sampler2D;
	m_basicAOTexture->m_textureDataDesc.UsageType = TextureUsageType::Sample;

	m_basicMaterial = m_renderingServer->AddMaterialDataComponent("BasicMaterial/");
	m_basicMaterial->m_normalTexture = m_basicNormalTexture;
	m_basicMaterial->m_albedoTexture = m_basicAlbedoTexture;
	m_basicMaterial->m_metallicTexture = m_basicMetallicTexture;
	m_basicMaterial->m_roughnessTexture = m_basicRoughnessTexture;
	m_basicMaterial->m_aoTexture = m_basicAOTexture;

	m_iconTemplate_DirectionalLight = g_pModuleManager->getFileSystem()->loadTexture("Res//Textures//InnoWorldEditorIcons_DirectionalLight.png");
	m_iconTemplate_DirectionalLight->m_textureDataDesc.SamplerType = TextureSamplerType::Sampler2D;
	m_iconTemplate_DirectionalLight->m_textureDataDesc.UsageType = TextureUsageType::Sample;

	m_iconTemplate_PointLight = g_pModuleManager->getFileSystem()->loadTexture("Res//Textures//InnoWorldEditorIcons_PointLight.png");
	m_iconTemplate_PointLight->m_textureDataDesc.SamplerType = TextureSamplerType::Sampler2D;
	m_iconTemplate_PointLight->m_textureDataDesc.UsageType = TextureUsageType::Sample;

	m_iconTemplate_SphereLight = g_pModuleManager->getFileSystem()->loadTexture("Res//Textures//InnoWorldEditorIcons_SphereLight.png");
	m_iconTemplate_SphereLight->m_textureDataDesc.SamplerType = TextureSamplerType::Sampler2D;
	m_iconTemplate_SphereLight->m_textureDataDesc.UsageType = TextureUsageType::Sample;

	m_unitLineMesh = m_renderingServer->AddMeshDataComponent("UnitLineMesh/");
	g_pModuleManager->getAssetSystem()->addUnitLine(*m_unitLineMesh);
	m_unitLineMesh->m_meshPrimitiveTopology = MeshPrimitiveTopology::TriangleStrip;
	m_unitLineMesh->m_meshShapeType = MeshShapeType::Line;
	m_unitLineMesh->m_ObjectStatus = ObjectStatus::Created;

	m_unitQuadMesh = m_renderingServer->AddMeshDataComponent("UnitQuadMesh/");
	g_pModuleManager->getAssetSystem()->addUnitQuad(*m_unitQuadMesh);
	m_unitQuadMesh->m_meshPrimitiveTopology = MeshPrimitiveTopology::Triangle;
	m_unitQuadMesh->m_meshShapeType = MeshShapeType::Quad;
	m_unitQuadMesh->m_ObjectStatus = ObjectStatus::Created;

	m_unitCubeMesh = m_renderingServer->AddMeshDataComponent("UnitCubeMesh/");
	g_pModuleManager->getAssetSystem()->addUnitCube(*m_unitCubeMesh);
	m_unitCubeMesh->m_meshPrimitiveTopology = MeshPrimitiveTopology::Triangle;
	m_unitCubeMesh->m_meshShapeType = MeshShapeType::Cube;
	m_unitCubeMesh->m_ObjectStatus = ObjectStatus::Created;

	m_unitSphereMesh = m_renderingServer->AddMeshDataComponent("UnitSphereMesh/");
	g_pModuleManager->getAssetSystem()->addUnitSphere(*m_unitSphereMesh);
	m_unitSphereMesh->m_meshPrimitiveTopology = MeshPrimitiveTopology::Triangle;
	m_unitSphereMesh->m_meshShapeType = MeshShapeType::Sphere;
	m_unitSphereMesh->m_ObjectStatus = ObjectStatus::Created;

	m_terrainMesh = m_renderingServer->AddMeshDataComponent("TerrainMesh/");
	g_pModuleManager->getAssetSystem()->addTerrain(*m_terrainMesh);
	m_terrainMesh->m_meshPrimitiveTopology = MeshPrimitiveTopology::Triangle;
	m_terrainMesh->m_ObjectStatus = ObjectStatus::Created;

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
	if (m_ObjectStatus == ObjectStatus::Created)
	{
		loadDefaultAssets();

		initializeHaltonSampler();
		m_rayTracer->Initialize();

		m_ObjectStatus = ObjectStatus::Activated;
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

	auto l_mainCameraTransformComponent = GetComponent(TransformComponent, l_mainCamera->m_ParentEntity);

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
		auto& l_currentHaltonStep = m_currentHaltonStep;
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
	auto l_sun = GetComponentManager(LightComponent)->GetSun();

	if (l_sun)
	{
		auto l_sunTransformComponent = GetComponent(TransformComponent, l_sun->m_ParentEntity);

		if (l_sunTransformComponent == nullptr)
		{
			return false;
		}

		auto l_lightRotMat = l_sunTransformComponent->m_globalTransformMatrix.m_rotationMat.inverse();

		SunGPUData l_SunGPUData;

		l_SunGPUData.dir = InnoMath::getDirection(Direction::Backward, l_sunTransformComponent->m_globalTransformVector.m_rot);
		l_SunGPUData.luminance = l_sun->m_RGBColor * l_sun->m_LuminousFlux;
		l_SunGPUData.r = InnoMath::getInvertRotationMatrix(l_sunTransformComponent->m_globalTransformVector.m_rot);

		m_sunGPUData.SetValue(std::move(l_SunGPUData));

		auto l_SplitAABB = GetComponentManager(LightComponent)->GetSunSplitAABB();
		auto l_ProjectionMatrices = GetComponentManager(LightComponent)->GetSunProjectionMatrices();

		auto& l_CSMGPUDataVector = m_CSMGPUDataVector.GetValue();
		l_CSMGPUDataVector.clear();

		if (l_SplitAABB.size() > 0 && l_ProjectionMatrices.size() > 0)
		{
			for (size_t j = 0; j < m_renderingCapability.maxCSMSplits; j++)
			{
				CSMGPUData l_CSMGPUData;

				l_CSMGPUData.p = l_ProjectionMatrices[j];
				l_CSMGPUData.AABBMax = l_SplitAABB[j].m_boundMax;
				l_CSMGPUData.AABBMin = l_SplitAABB[j].m_boundMin;
				l_CSMGPUData.v = l_lightRotMat;

				l_CSMGPUDataVector.emplace_back(l_CSMGPUData);
			}
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

bool InnoRenderingFrontendNS::updateLightData()
{
	auto& l_PointLightGPUData = m_pointLightGPUData.GetValue();
	auto& l_SphereLightGPUData = m_sphereLightGPUData.GetValue();

	l_PointLightGPUData.clear();
	l_SphereLightGPUData.clear();

	auto& l_lightComponents = GetComponentManager(LightComponent)->GetAllComponents();
	auto l_lightComponentCount = l_lightComponents.size();

	if (l_lightComponentCount > 0)
	{
		for (size_t i = 0; i < l_lightComponentCount; i++)
		{
			auto l_transformCompoent = GetComponent(TransformComponent, l_lightComponents[i]->m_ParentEntity);
			if (l_transformCompoent != nullptr)
			{
				if (l_lightComponents[i]->m_LightType == LightType::Point)
				{
					PointLightGPUData l_data;
					l_data.pos = l_transformCompoent->m_globalTransformVector.m_pos;
					l_data.luminance = l_lightComponents[i]->m_RGBColor * l_lightComponents[i]->m_LuminousFlux;
					l_data.luminance.w = l_lightComponents[i]->m_Shape.x;
					l_PointLightGPUData.emplace_back(l_data);
				}
				else if (l_lightComponents[i]->m_LightType == LightType::Sphere)
				{
					SphereLightGPUData l_data;
					l_data.pos = l_transformCompoent->m_globalTransformVector.m_pos;
					l_data.luminance = l_lightComponents[i]->m_RGBColor * l_lightComponents[i]->m_LuminousFlux;
					l_data.luminance.w = l_lightComponents[i]->m_Shape.x;
					l_SphereLightGPUData.emplace_back(l_data);
				}
			}
		}
	}

	return true;
}

bool InnoRenderingFrontendNS::updateMeshData()
{
	auto& l_opaquePassDrawCallDataVector = m_opaquePassDrawCallDataVector.GetValue();
	auto& l_opaquePassMeshGPUData = m_opaquePassMeshGPUData.GetValue();
	auto& l_opaquePassMaterialGPUData = m_opaquePassMaterialGPUData.GetValue();

	auto& l_sunShadowPassDrawCallDataVector = m_sunShadowPassDrawCallDataVector.GetValue();
	auto& l_sunShadowPassMeshGPUData = m_sunShadowPassMeshGPUData.GetValue();

	auto& l_transparentPassDrawCallDataVector = m_transparentPassDrawCallDataVector.GetValue();
	auto& l_transparentPassMeshGPUData = m_transparentPassMeshGPUData.GetValue();
	auto& l_transparentPassMaterialGPUData = m_transparentPassMaterialGPUData.GetValue();

	l_opaquePassDrawCallDataVector.clear();
	l_opaquePassMeshGPUData.clear();
	l_opaquePassMaterialGPUData.clear();

	l_sunShadowPassDrawCallDataVector.clear();
	l_sunShadowPassMeshGPUData.clear();

	l_transparentPassDrawCallDataVector.clear();
	l_transparentPassMeshGPUData.clear();
	l_transparentPassMaterialGPUData.clear();

	auto l_cullingDataSize = m_cullingData.size();
	uint32_t l_sunShadowPassIndex = 0;
	uint32_t l_opaquePassIndex = 0;
	uint32_t l_transparentPassIndex = 0;

	for (size_t i = 0; i < l_cullingDataSize; i++)
	{
		auto l_cullingData = m_cullingData[i];
		if (l_cullingData.mesh != nullptr)
		{
			if (l_cullingData.mesh->m_ObjectStatus == ObjectStatus::Activated)
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
						l_materialGPUData.materialType = int32_t(l_cullingData.meshUsageType);
						l_materialGPUData.customMaterial = l_cullingData.material->m_meshCustomMaterial;

						if (l_cullingData.cullingDataChannel != CullingDataChannel::Shadow)
						{
							l_opaquePassDrawCallDataVector.emplace_back(l_opaquePassDrawCallData);
							l_opaquePassMeshGPUData.emplace_back(l_meshGPUData);
							l_opaquePassMaterialGPUData.emplace_back(l_materialGPUData);
							l_opaquePassIndex++;
						}

						l_sunShadowPassDrawCallDataVector.emplace_back(l_opaquePassDrawCallData);
						l_sunShadowPassMeshGPUData.emplace_back(l_meshGPUData);
						l_sunShadowPassIndex++;
					}
					else if (l_cullingData.visiblilityType == VisiblilityType::Transparent)
					{
						TransparentPassDrawCallData l_transparentPassDrawCallData;

						l_transparentPassDrawCallData.mesh = l_cullingData.mesh;
						l_transparentPassDrawCallData.meshGPUDataIndex = l_transparentPassIndex;
						l_transparentPassDrawCallData.materialGPUDataIndex = l_transparentPassIndex;

						MaterialGPUData l_materialGPUData;

						l_materialGPUData.useNormalTexture = false;
						l_materialGPUData.useAlbedoTexture = false;
						l_materialGPUData.useMetallicTexture = false;
						l_materialGPUData.useRoughnessTexture = false;
						l_materialGPUData.useAOTexture = false;
						l_materialGPUData.materialType = int32_t(l_cullingData.meshUsageType);
						l_materialGPUData.customMaterial = l_cullingData.material->m_meshCustomMaterial;

						l_transparentPassDrawCallDataVector.emplace_back(l_transparentPassDrawCallData);
						l_transparentPassMeshGPUData.emplace_back(l_meshGPUData);
						l_transparentPassMaterialGPUData.emplace_back(l_materialGPUData);
						l_transparentPassIndex++;
					}
				}
			}
		}
	}

	m_sunShadowPassDrawCallCount = l_sunShadowPassIndex;
	m_opaquePassDrawCallCount = l_opaquePassIndex;
	m_transparentPassDrawCallCount = l_transparentPassIndex;

	// @TODO: use GPU to do OIT
	auto l_t = m_cameraGPUData.GetValue().t;
	auto l_r = m_cameraGPUData.GetValue().r;

	std::sort(l_transparentPassDrawCallDataVector.begin(), l_transparentPassDrawCallDataVector.end(), [&](TransparentPassDrawCallData a, TransparentPassDrawCallData b) {
		auto m_a_InViewSpace = l_t * l_r * l_transparentPassMeshGPUData[a.meshGPUDataIndex].m;
		auto m_b_InViewSpace = l_t * l_r * l_transparentPassMeshGPUData[b.meshGPUDataIndex].m;
		return m_a_InViewSpace.m23 < m_b_InViewSpace.m23;
	});

	return true;
}

bool InnoRenderingFrontendNS::updateBillboardPassData()
{
	auto& l_lightComponents = GetComponentManager(LightComponent)->GetAllComponents();

	auto l_totalBillboardDrawCallCount = l_lightComponents.size();

	if (l_totalBillboardDrawCallCount == 0)
	{
		return false;
	}

	auto& l_billboardPassDrawCallDataVector = m_billboardPassDrawCallDataVector.GetValue();
	auto& l_billboardPassMeshGPUData = m_billboardPassMeshGPUData.GetValue();
	auto& l_directionalLightMeshGPUData = m_directionalLightMeshGPUData.GetValue();
	auto& l_pointLightMeshGPUData = m_pointLightMeshGPUData.GetValue();
	auto& l_sphereLightMeshGPUData = m_sphereLightMeshGPUData.GetValue();

	auto l_billboardPassDrawCallDataCount = l_billboardPassDrawCallDataVector.size();
	for (size_t i = 0; i < l_billboardPassDrawCallDataCount; i++)
	{
		l_billboardPassDrawCallDataVector[i].instanceCount = 0;
	}

	l_billboardPassMeshGPUData.clear();

	l_directionalLightMeshGPUData.clear();
	l_pointLightMeshGPUData.clear();
	l_sphereLightMeshGPUData.clear();

	for (auto i : l_lightComponents)
	{
		MeshGPUData l_meshGPUData;

		auto l_transformCompoent = GetComponent(TransformComponent, i->m_ParentEntity);
		if (l_transformCompoent != nullptr)
		{
			l_meshGPUData.m = InnoMath::toTranslationMatrix(l_transformCompoent->m_globalTransformVector.m_pos);
		}

		switch (i->m_LightType)
		{
		case LightType::Directional:
			l_directionalLightMeshGPUData.emplace_back(l_meshGPUData);
			l_billboardPassDrawCallDataVector[0].instanceCount++;
			break;
		case LightType::Point:
			l_pointLightMeshGPUData.emplace_back(l_meshGPUData);
			l_billboardPassDrawCallDataVector[1].instanceCount++;
			break;
		case LightType::Spot:
			break;
		case LightType::Sphere:
			l_sphereLightMeshGPUData.emplace_back(l_meshGPUData);
			l_billboardPassDrawCallDataVector[2].instanceCount++;
			break;
		case LightType::Disk:
			break;
		case LightType::Tube:
			break;
		case LightType::Rectangle:
			break;
		default:
			break;
		}
	}

	l_billboardPassDrawCallDataVector[0].meshGPUDataOffset = 0;
	l_billboardPassDrawCallDataVector[1].meshGPUDataOffset = (uint32_t)l_directionalLightMeshGPUData.size();
	l_billboardPassDrawCallDataVector[2].meshGPUDataOffset = (uint32_t)(l_directionalLightMeshGPUData.size() + l_pointLightMeshGPUData.size());

	l_billboardPassMeshGPUData.insert(l_billboardPassMeshGPUData.end(), l_directionalLightMeshGPUData.begin(), l_directionalLightMeshGPUData.end());
	l_billboardPassMeshGPUData.insert(l_billboardPassMeshGPUData.end(), l_pointLightMeshGPUData.begin(), l_pointLightMeshGPUData.end());
	l_billboardPassMeshGPUData.insert(l_billboardPassMeshGPUData.end(), l_sphereLightMeshGPUData.begin(), l_sphereLightMeshGPUData.end());

	return true;
}

bool InnoRenderingFrontendNS::updateDebuggerPassData()
{
	uint32_t l_index = 0;
	// @TODO: Implementation
	m_debugPassDrawCallCount = l_index;

	return true;
}

bool InnoRenderingFrontendNS::update()
{
	if (m_ObjectStatus == ObjectStatus::Activated)
	{
		updateCameraData();

		updateSunData();
		updateLightData();
		updateSkyData();

		// copy culling data pack for local scope
		m_cullingData = g_pModuleManager->getPhysicsSystem()->getCullingData();

		updateMeshData();

		updateBillboardPassData();

		updateDebuggerPassData();

		return true;
	}
	else
	{
		m_ObjectStatus = ObjectStatus::Suspended;
		return false;
	}
}

bool InnoRenderingFrontendNS::terminate()
{
	m_rayTracer->Terminate();

	m_ObjectStatus = ObjectStatus::Terminated;
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
	return InnoRenderingFrontendNS::m_ObjectStatus;
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
	static std::atomic<uint32_t> skeletonCount = 0;
	auto l_rawPtr = m_SkeletonDataComponentPool->Spawn();
	auto l_SDC = new(l_rawPtr)SkeletonDataComponent();
	auto l_parentEntity = g_pModuleManager->getEntityManager()->Spawn(ObjectSource::Runtime, ObjectOwnership::Engine, ("Skeleton_" + std::to_string(skeletonCount) + "/").c_str());
	l_SDC->m_ParentEntity = l_parentEntity;
	l_SDC->m_ObjectSource = ObjectSource::Runtime;
	l_SDC->m_ObjectOwnership = ObjectOwnership::Engine;
	l_SDC->m_ComponentType = ComponentType::SkeletonDataComponent;
	skeletonCount++;
	return l_SDC;
}

AnimationDataComponent * InnoRenderingFrontend::addAnimationDataComponent()
{
	static std::atomic<uint32_t> animationCount = 0;
	auto l_rawPtr = m_AnimationDataComponentPool->Spawn();
	auto l_ADC = new(l_rawPtr)AnimationDataComponent();
	auto l_parentEntity = g_pModuleManager->getEntityManager()->Spawn(ObjectSource::Runtime, ObjectOwnership::Engine, ("Animation_" + std::to_string(animationCount) + "/").c_str());
	l_ADC->m_ParentEntity = l_parentEntity;
	l_ADC->m_ObjectSource = ObjectSource::Runtime;
	l_ADC->m_ObjectOwnership = ObjectOwnership::Engine;
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
		InnoLogger::Log(LogLevel::Error, "RenderingFrontend: Wrong MeshShapeType!");
		return nullptr; break;
	default:
		return nullptr; break;
	}
}

TextureDataComponent * InnoRenderingFrontend::getTextureDataComponent(TextureAttributeType textureAttributeType)
{
	switch (textureAttributeType)
	{
	case TextureAttributeType::Normal:
		return m_basicNormalTexture; break;
	case TextureAttributeType::Albedo:
		return m_basicAlbedoTexture; break;
	case TextureAttributeType::Metallic:
		return m_basicMetallicTexture; break;
	case TextureAttributeType::Roughness:
		return m_basicRoughnessTexture; break;
	case TextureAttributeType::AmbientOcclusion:
		return m_basicAOTexture; break;
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

TVec2<uint32_t> InnoRenderingFrontend::getScreenResolution()
{
	return m_screenResolution;
}

bool InnoRenderingFrontend::setScreenResolution(TVec2<uint32_t> screenResolution)
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
	return m_CSMGPUDataVector.GetValue();
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

uint32_t InnoRenderingFrontend::getSunShadowPassDrawCallCount()
{
	return m_sunShadowPassDrawCallCount;
}

const std::vector<OpaquePassDrawCallData>& InnoRenderingFrontend::getSunShadowPassDrawCallData()
{
	return m_sunShadowPassDrawCallDataVector.GetValue();
}

const std::vector<MeshGPUData>& InnoRenderingFrontend::getSunShadowPassMeshGPUData()
{
	return m_sunShadowPassMeshGPUData.GetValue();
}

uint32_t InnoRenderingFrontend::getOpaquePassDrawCallCount()
{
	return m_opaquePassDrawCallCount;
}

const std::vector<OpaquePassDrawCallData>& InnoRenderingFrontend::getOpaquePassDrawCallData()
{
	return m_opaquePassDrawCallDataVector.GetValue();
}

const std::vector<MeshGPUData>& InnoRenderingFrontend::getOpaquePassMeshGPUData()
{
	return m_opaquePassMeshGPUData.GetValue();
}

const std::vector<MaterialGPUData>& InnoRenderingFrontend::getOpaquePassMaterialGPUData()
{
	return m_opaquePassMaterialGPUData.GetValue();
}

uint32_t InnoRenderingFrontend::getTransparentPassDrawCallCount()
{
	return m_transparentPassDrawCallCount;
}

const std::vector<TransparentPassDrawCallData>& InnoRenderingFrontend::getTransparentPassDrawCallData()
{
	return m_transparentPassDrawCallDataVector.GetValue();
}

const std::vector<MeshGPUData>& InnoRenderingFrontend::getTransparentPassMeshGPUData()
{
	return m_transparentPassMeshGPUData.GetValue();
}

const std::vector<MaterialGPUData>& InnoRenderingFrontend::getTransparentPassMaterialGPUData()
{
	return m_transparentPassMaterialGPUData.GetValue();
}

const std::vector<BillboardPassDrawCallData>& InnoRenderingFrontend::getBillboardPassDrawCallData()
{
	return m_billboardPassDrawCallDataVector.GetValue();
}

const std::vector<MeshGPUData>& InnoRenderingFrontend::getBillboardPassMeshGPUData()
{
	return m_billboardPassMeshGPUData.GetValue();
}

uint32_t InnoRenderingFrontend::getDebugPassDrawCallCount()
{
	return m_debugPassDrawCallCount;
}

const std::vector<DebugPassDrawCallData>& InnoRenderingFrontend::getDebugPassDrawCallData()
{
	return m_debugPassDrawCallDataVector.GetValue();
}

const std::vector<MeshGPUData>& InnoRenderingFrontend::getDebugPassMeshGPUData()
{
	return m_debugPassMeshGPUData.GetValue();
}