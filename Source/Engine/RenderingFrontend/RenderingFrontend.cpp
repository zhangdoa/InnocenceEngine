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

	DoubleBuffer<PerFrameConstantBuffer, true> m_perFrameCBuffer;

	DoubleBuffer<std::vector<CSMConstantBuffer>, true> m_CSMConstantBufferVector;

	DoubleBuffer<std::vector<PointLightConstantBuffer>, true> m_pointLightConstantBuffer;
	DoubleBuffer<std::vector<SphereLightConstantBuffer>, true> m_sphereLightConstantBuffer;

	uint32_t m_sunShadowPassDrawCallCount = 0;
	DoubleBuffer<std::vector<OpaquePassDrawCallInfo>, true> m_sunShadowPassDrawCallInfoVector;
	DoubleBuffer<std::vector<PerObjectConstantBuffer>, true> m_sunShadowPassPerObjectConstantBuffer;

	uint32_t m_opaquePassDrawCallCount = 0;
	DoubleBuffer<std::vector<OpaquePassDrawCallInfo>, true> m_opaquePassDrawCallInfoVector;
	DoubleBuffer<std::vector<PerObjectConstantBuffer>, true> m_opaquePassPerObjectConstantBuffer;
	DoubleBuffer<std::vector<MaterialConstantBuffer>, true> m_opaquePassMaterialConstantBuffer;

	uint32_t m_transparentPassDrawCallCount = 0;
	DoubleBuffer<std::vector<TransparentPassDrawCallInfo>, true> m_transparentPassDrawCallInfoVector;
	DoubleBuffer<std::vector<PerObjectConstantBuffer>, true> m_transparentPassPerObjectConstantBuffer;
	DoubleBuffer<std::vector<MaterialConstantBuffer>, true> m_transparentPassMaterialConstantBuffer;

	DoubleBuffer<std::vector<PerObjectConstantBuffer>, true> m_directionalLightPerObjectConstantBuffer;
	DoubleBuffer<std::vector<PerObjectConstantBuffer>, true> m_pointLightPerObjectConstantBuffer;
	DoubleBuffer<std::vector<PerObjectConstantBuffer>, true> m_sphereLightPerObjectConstantBuffer;

	DoubleBuffer<std::vector<BillboardPassDrawCallInfo>, true> m_billboardPassDrawCallInfoVector;
	DoubleBuffer<std::vector<PerObjectConstantBuffer>, true> m_billboardPassPerObjectConstantBuffer;

	uint32_t m_debugPassDrawCallCount = 0;
	DoubleBuffer<std::vector<DebugPassDrawCallInfo>, true> m_debugPassDrawCallInfoVector;
	DoubleBuffer<std::vector<PerObjectConstantBuffer>, true> m_debugPassPerObjectConstantBuffer;

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

	bool updatePerFrameConstantBuffer();
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
	//m_renderingConfig.useTAA = true;
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
	m_DefaultRenderPassDesc.m_RenderTargetDesc.Width = m_screenResolution.x;
	m_DefaultRenderPassDesc.m_RenderTargetDesc.Height = m_screenResolution.y;
	m_DefaultRenderPassDesc.m_RenderTargetDesc.PixelDataType = TexturePixelDataType::FLOAT16;

	m_DefaultRenderPassDesc.m_GraphicsPipelineDesc.m_ViewportDesc.m_Width = (float)m_screenResolution.x;
	m_DefaultRenderPassDesc.m_GraphicsPipelineDesc.m_ViewportDesc.m_Height = (float)m_screenResolution.y;

	m_CSMConstantBufferVector.Reserve(m_renderingCapability.maxCSMSplits);

	m_transparentPassDrawCallInfoVector.Reserve(m_renderingCapability.maxMeshes);
	m_transparentPassPerObjectConstantBuffer.Reserve(m_renderingCapability.maxMeshes);
	m_transparentPassMaterialConstantBuffer.Reserve(m_renderingCapability.maxMaterials);

	m_opaquePassDrawCallInfoVector.Reserve(m_renderingCapability.maxMeshes);
	m_opaquePassPerObjectConstantBuffer.Reserve(m_renderingCapability.maxMeshes);
	m_opaquePassMaterialConstantBuffer.Reserve(m_renderingCapability.maxMaterials);

	m_sunShadowPassDrawCallInfoVector.Reserve(m_renderingCapability.maxMeshes);
	m_sunShadowPassPerObjectConstantBuffer.Reserve(m_renderingCapability.maxMeshes);

	m_pointLightConstantBuffer.Reserve(m_renderingCapability.maxPointLights);
	m_sphereLightConstantBuffer.Reserve(m_renderingCapability.maxSphereLights);

	m_directionalLightPerObjectConstantBuffer.Reserve(8192);
	m_pointLightPerObjectConstantBuffer.Reserve(8192);
	m_sphereLightPerObjectConstantBuffer.Reserve(8192);
	m_billboardPassPerObjectConstantBuffer.Reserve(8192);

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
		std::vector<BillboardPassDrawCallInfo> l_billboardPassDrawCallInfoVectorA(3);
		l_billboardPassDrawCallInfoVectorA[0].iconTexture = g_pModuleManager->getRenderingFrontend()->getTextureDataComponent(WorldEditorIconType::DIRECTIONAL_LIGHT);
		l_billboardPassDrawCallInfoVectorA[1].iconTexture = g_pModuleManager->getRenderingFrontend()->getTextureDataComponent(WorldEditorIconType::POINT_LIGHT);
		l_billboardPassDrawCallInfoVectorA[2].iconTexture = g_pModuleManager->getRenderingFrontend()->getTextureDataComponent(WorldEditorIconType::SPHERE_LIGHT);
		auto l_billboardPassDrawCallInfoVectorB = l_billboardPassDrawCallInfoVectorA;

		m_billboardPassDrawCallInfoVector.SetValue(std::move(l_billboardPassDrawCallInfoVectorA));
		m_billboardPassDrawCallInfoVector.SetValue(std::move(l_billboardPassDrawCallInfoVectorB));
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
	m_basicNormalTexture->m_textureDesc.SamplerType = TextureSamplerType::Sampler2D;
	m_basicNormalTexture->m_textureDesc.UsageType = TextureUsageType::Sample;

	m_basicAlbedoTexture = g_pModuleManager->getFileSystem()->loadTexture("Res//Textures//basic_albedo.png");
	m_basicAlbedoTexture->m_textureDesc.SamplerType = TextureSamplerType::Sampler2D;
	m_basicAlbedoTexture->m_textureDesc.UsageType = TextureUsageType::Sample;

	m_basicMetallicTexture = g_pModuleManager->getFileSystem()->loadTexture("Res//Textures//basic_metallic.png");
	m_basicMetallicTexture->m_textureDesc.SamplerType = TextureSamplerType::Sampler2D;
	m_basicMetallicTexture->m_textureDesc.UsageType = TextureUsageType::Sample;

	m_basicRoughnessTexture = g_pModuleManager->getFileSystem()->loadTexture("Res//Textures//basic_roughness.png");
	m_basicRoughnessTexture->m_textureDesc.SamplerType = TextureSamplerType::Sampler2D;
	m_basicRoughnessTexture->m_textureDesc.UsageType = TextureUsageType::Sample;

	m_basicAOTexture = g_pModuleManager->getFileSystem()->loadTexture("Res//Textures//basic_ao.png");
	m_basicAOTexture->m_textureDesc.SamplerType = TextureSamplerType::Sampler2D;
	m_basicAOTexture->m_textureDesc.UsageType = TextureUsageType::Sample;

	m_basicMaterial = m_renderingServer->AddMaterialDataComponent("BasicMaterial/");
	m_basicMaterial->m_normalTexture = m_basicNormalTexture;
	m_basicMaterial->m_albedoTexture = m_basicAlbedoTexture;
	m_basicMaterial->m_metallicTexture = m_basicMetallicTexture;
	m_basicMaterial->m_roughnessTexture = m_basicRoughnessTexture;
	m_basicMaterial->m_aoTexture = m_basicAOTexture;

	m_iconTemplate_DirectionalLight = g_pModuleManager->getFileSystem()->loadTexture("Res//Textures//InnoWorldEditorIcons_DirectionalLight.png");
	m_iconTemplate_DirectionalLight->m_textureDesc.SamplerType = TextureSamplerType::Sampler2D;
	m_iconTemplate_DirectionalLight->m_textureDesc.UsageType = TextureUsageType::Sample;

	m_iconTemplate_PointLight = g_pModuleManager->getFileSystem()->loadTexture("Res//Textures//InnoWorldEditorIcons_PointLight.png");
	m_iconTemplate_PointLight->m_textureDesc.SamplerType = TextureSamplerType::Sampler2D;
	m_iconTemplate_PointLight->m_textureDesc.UsageType = TextureUsageType::Sample;

	m_iconTemplate_SphereLight = g_pModuleManager->getFileSystem()->loadTexture("Res//Textures//InnoWorldEditorIcons_SphereLight.png");
	m_iconTemplate_SphereLight->m_textureDesc.SamplerType = TextureSamplerType::Sampler2D;
	m_iconTemplate_SphereLight->m_textureDesc.UsageType = TextureUsageType::Sample;

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

bool InnoRenderingFrontendNS::updatePerFrameConstantBuffer()
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

	PerFrameConstantBuffer l_PerFrameConstantBuffer;

	auto l_p = l_mainCamera->m_projectionMatrix;

	l_PerFrameConstantBuffer.p_original = l_p;
	l_PerFrameConstantBuffer.p_jittered = l_p;

	if (m_renderingConfig.useTAA)
	{
		//TAA jitter for projection matrix
		auto& l_currentHaltonStep = m_currentHaltonStep;
		if (l_currentHaltonStep >= 16)
		{
			l_currentHaltonStep = 0;
		}
		l_PerFrameConstantBuffer.p_jittered.m02 = m_haltonSampler[l_currentHaltonStep].x / m_screenResolution.x;
		l_PerFrameConstantBuffer.p_jittered.m12 = m_haltonSampler[l_currentHaltonStep].y / m_screenResolution.y;
		l_currentHaltonStep += 1;
	}

	auto r =
		InnoMath::getInvertRotationMatrix(
			l_mainCameraTransformComponent->m_globalTransformVector.m_rot
		);

	auto t =
		InnoMath::getInvertTranslationMatrix(
			l_mainCameraTransformComponent->m_globalTransformVector.m_pos
		);

	l_PerFrameConstantBuffer.camera_posWS = l_mainCameraTransformComponent->m_globalTransformVector.m_pos;

	l_PerFrameConstantBuffer.v = r * t;

	auto r_prev = l_mainCameraTransformComponent->m_globalTransformMatrix_prev.m_rotationMat.inverse();
	auto t_prev = l_mainCameraTransformComponent->m_globalTransformMatrix_prev.m_translationMat.inverse();

	l_PerFrameConstantBuffer.v_prev = r_prev * t_prev;

	l_PerFrameConstantBuffer.zNear = l_mainCamera->m_zNear;
	l_PerFrameConstantBuffer.zFar = l_mainCamera->m_zFar;

	l_PerFrameConstantBuffer.p_inv = l_p.inverse();
	l_PerFrameConstantBuffer.v_inv = l_PerFrameConstantBuffer.v.inverse();
	l_PerFrameConstantBuffer.viewportSize.x = (float)m_screenResolution.x;
	l_PerFrameConstantBuffer.viewportSize.y = (float)m_screenResolution.y;
	l_PerFrameConstantBuffer.minLogLuminance = -10.0f;
	l_PerFrameConstantBuffer.maxLogLuminance = 16.0f;

	auto l_sun = GetComponentManager(LightComponent)->GetSun();

	if (l_sun == nullptr)
	{
		return false;
	}

	auto l_sunTransformComponent = GetComponent(TransformComponent, l_sun->m_ParentEntity);

	if (l_sunTransformComponent == nullptr)
	{
		return false;
	}

	auto l_lightRotMat = l_sunTransformComponent->m_globalTransformMatrix.m_rotationMat.inverse();

	l_PerFrameConstantBuffer.sun_direction = InnoMath::getDirection(Direction::Backward, l_sunTransformComponent->m_globalTransformVector.m_rot);
	l_PerFrameConstantBuffer.sun_illuminance = l_sun->m_RGBColor * l_sun->m_LuminousFlux;

	m_perFrameCBuffer.SetValue(std::move(l_PerFrameConstantBuffer));

	auto l_SplitAABB = GetComponentManager(LightComponent)->GetSunSplitAABB();
	auto l_ProjectionMatrices = GetComponentManager(LightComponent)->GetSunProjectionMatrices();

	auto& l_CSMConstantBufferVector = m_CSMConstantBufferVector.GetValue();
	l_CSMConstantBufferVector.clear();

	if (l_SplitAABB.size() > 0 && l_ProjectionMatrices.size() > 0)
	{
		for (size_t j = 0; j < m_renderingCapability.maxCSMSplits; j++)
		{
			CSMConstantBuffer l_CSMConstantBuffer;

			l_CSMConstantBuffer.p = l_ProjectionMatrices[j];
			l_CSMConstantBuffer.AABBMax = l_SplitAABB[j].m_boundMax;
			l_CSMConstantBuffer.AABBMin = l_SplitAABB[j].m_boundMin;
			l_CSMConstantBuffer.v = l_lightRotMat;

			l_CSMConstantBufferVector.emplace_back(l_CSMConstantBuffer);
		}
	}

	return true;
}

bool InnoRenderingFrontendNS::updateLightData()
{
	auto& l_PointLightConstantBuffer = m_pointLightConstantBuffer.GetValue();
	auto& l_SphereLightConstantBuffer = m_sphereLightConstantBuffer.GetValue();

	l_PointLightConstantBuffer.clear();
	l_SphereLightConstantBuffer.clear();

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
					PointLightConstantBuffer l_data;
					l_data.pos = l_transformCompoent->m_globalTransformVector.m_pos;
					l_data.luminance = l_lightComponents[i]->m_RGBColor * l_lightComponents[i]->m_LuminousFlux;
					l_data.luminance.w = l_lightComponents[i]->m_Shape.x;
					l_PointLightConstantBuffer.emplace_back(l_data);
				}
				else if (l_lightComponents[i]->m_LightType == LightType::Sphere)
				{
					SphereLightConstantBuffer l_data;
					l_data.pos = l_transformCompoent->m_globalTransformVector.m_pos;
					l_data.luminance = l_lightComponents[i]->m_RGBColor * l_lightComponents[i]->m_LuminousFlux;
					l_data.luminance.w = l_lightComponents[i]->m_Shape.x;
					l_SphereLightConstantBuffer.emplace_back(l_data);
				}
			}
		}
	}

	return true;
}

bool InnoRenderingFrontendNS::updateMeshData()
{
	auto& l_opaquePassDrawCallInfoVector = m_opaquePassDrawCallInfoVector.GetValue();
	auto& l_opaquePassPerObjectConstantBuffer = m_opaquePassPerObjectConstantBuffer.GetValue();
	auto& l_opaquePassMaterialConstantBuffer = m_opaquePassMaterialConstantBuffer.GetValue();

	auto& l_sunShadowPassDrawCallInfoVector = m_sunShadowPassDrawCallInfoVector.GetValue();
	auto& l_sunShadowPassPerObjectConstantBuffer = m_sunShadowPassPerObjectConstantBuffer.GetValue();

	auto& l_transparentPassDrawCallInfoVector = m_transparentPassDrawCallInfoVector.GetValue();
	auto& l_transparentPassPerObjectConstantBuffer = m_transparentPassPerObjectConstantBuffer.GetValue();
	auto& l_transparentPassMaterialConstantBuffer = m_transparentPassMaterialConstantBuffer.GetValue();

	l_opaquePassDrawCallInfoVector.clear();
	l_opaquePassPerObjectConstantBuffer.clear();
	l_opaquePassMaterialConstantBuffer.clear();

	l_sunShadowPassDrawCallInfoVector.clear();
	l_sunShadowPassPerObjectConstantBuffer.clear();

	l_transparentPassDrawCallInfoVector.clear();
	l_transparentPassPerObjectConstantBuffer.clear();
	l_transparentPassMaterialConstantBuffer.clear();

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
					PerObjectConstantBuffer l_meshConstantBuffer;
					l_meshConstantBuffer.m = l_cullingData.m;
					l_meshConstantBuffer.m_prev = l_cullingData.m_prev;
					l_meshConstantBuffer.normalMat = l_cullingData.normalMat;
					l_meshConstantBuffer.UUID = (float)l_cullingData.UUID;

					if (l_cullingData.visiblilityType == VisiblilityType::Opaque)
					{
						OpaquePassDrawCallInfo l_opaquePassDrawCallInfo;

						l_opaquePassDrawCallInfo.mesh = l_cullingData.mesh;
						l_opaquePassDrawCallInfo.material = l_cullingData.material;

						MaterialConstantBuffer l_materialConstantBuffer;

						l_materialConstantBuffer.useNormalTexture = !(l_opaquePassDrawCallInfo.material->m_normalTexture == nullptr);
						l_materialConstantBuffer.useAlbedoTexture = !(l_opaquePassDrawCallInfo.material->m_albedoTexture == nullptr);
						l_materialConstantBuffer.useMetallicTexture = !(l_opaquePassDrawCallInfo.material->m_metallicTexture == nullptr);
						l_materialConstantBuffer.useRoughnessTexture = !(l_opaquePassDrawCallInfo.material->m_roughnessTexture == nullptr);
						l_materialConstantBuffer.useAOTexture = !(l_opaquePassDrawCallInfo.material->m_aoTexture == nullptr);
						l_materialConstantBuffer.materialType = int32_t(l_cullingData.meshUsageType);
						l_materialConstantBuffer.customMaterial = l_cullingData.material->m_meshCustomMaterial;

						if (l_cullingData.cullingDataChannel != CullingDataChannel::Shadow)
						{
							l_opaquePassDrawCallInfoVector.emplace_back(l_opaquePassDrawCallInfo);
							l_opaquePassPerObjectConstantBuffer.emplace_back(l_meshConstantBuffer);
							l_opaquePassMaterialConstantBuffer.emplace_back(l_materialConstantBuffer);
							l_opaquePassIndex++;
						}

						l_sunShadowPassDrawCallInfoVector.emplace_back(l_opaquePassDrawCallInfo);
						l_sunShadowPassPerObjectConstantBuffer.emplace_back(l_meshConstantBuffer);
						l_sunShadowPassIndex++;
					}
					else if (l_cullingData.visiblilityType == VisiblilityType::Transparent)
					{
						TransparentPassDrawCallInfo l_transparentPassDrawCallInfo;

						l_transparentPassDrawCallInfo.mesh = l_cullingData.mesh;
						l_transparentPassDrawCallInfo.meshConstantBufferIndex = l_transparentPassIndex;
						l_transparentPassDrawCallInfo.materialConstantBufferIndex = l_transparentPassIndex;

						MaterialConstantBuffer l_materialConstantBuffer;

						l_materialConstantBuffer.useNormalTexture = false;
						l_materialConstantBuffer.useAlbedoTexture = false;
						l_materialConstantBuffer.useMetallicTexture = false;
						l_materialConstantBuffer.useRoughnessTexture = false;
						l_materialConstantBuffer.useAOTexture = false;
						l_materialConstantBuffer.materialType = int32_t(l_cullingData.meshUsageType);
						l_materialConstantBuffer.customMaterial = l_cullingData.material->m_meshCustomMaterial;

						l_transparentPassDrawCallInfoVector.emplace_back(l_transparentPassDrawCallInfo);
						l_transparentPassPerObjectConstantBuffer.emplace_back(l_meshConstantBuffer);
						l_transparentPassMaterialConstantBuffer.emplace_back(l_materialConstantBuffer);
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
	auto l_v = m_perFrameCBuffer.GetValue().v;

	std::sort(l_transparentPassDrawCallInfoVector.begin(), l_transparentPassDrawCallInfoVector.end(), [&](TransparentPassDrawCallInfo a, TransparentPassDrawCallInfo b) {
		auto m_a_InViewSpace = l_v * l_transparentPassPerObjectConstantBuffer[a.meshConstantBufferIndex].m;
		auto m_b_InViewSpace = l_v * l_transparentPassPerObjectConstantBuffer[b.meshConstantBufferIndex].m;
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

	auto& l_billboardPassDrawCallInfoVector = m_billboardPassDrawCallInfoVector.GetValue();
	auto& l_billboardPassPerObjectConstantBuffer = m_billboardPassPerObjectConstantBuffer.GetValue();
	auto& l_directionalLightPerObjectConstantBuffer = m_directionalLightPerObjectConstantBuffer.GetValue();
	auto& l_pointLightPerObjectConstantBuffer = m_pointLightPerObjectConstantBuffer.GetValue();
	auto& l_sphereLightPerObjectConstantBuffer = m_sphereLightPerObjectConstantBuffer.GetValue();

	auto l_billboardPassDrawCallInfoCount = l_billboardPassDrawCallInfoVector.size();
	for (size_t i = 0; i < l_billboardPassDrawCallInfoCount; i++)
	{
		l_billboardPassDrawCallInfoVector[i].instanceCount = 0;
	}

	l_billboardPassPerObjectConstantBuffer.clear();

	l_directionalLightPerObjectConstantBuffer.clear();
	l_pointLightPerObjectConstantBuffer.clear();
	l_sphereLightPerObjectConstantBuffer.clear();

	for (auto i : l_lightComponents)
	{
		PerObjectConstantBuffer l_meshConstantBuffer;

		auto l_transformCompoent = GetComponent(TransformComponent, i->m_ParentEntity);
		if (l_transformCompoent != nullptr)
		{
			l_meshConstantBuffer.m = InnoMath::toTranslationMatrix(l_transformCompoent->m_globalTransformVector.m_pos);
		}

		switch (i->m_LightType)
		{
		case LightType::Directional:
			l_directionalLightPerObjectConstantBuffer.emplace_back(l_meshConstantBuffer);
			l_billboardPassDrawCallInfoVector[0].instanceCount++;
			break;
		case LightType::Point:
			l_pointLightPerObjectConstantBuffer.emplace_back(l_meshConstantBuffer);
			l_billboardPassDrawCallInfoVector[1].instanceCount++;
			break;
		case LightType::Spot:
			break;
		case LightType::Sphere:
			l_sphereLightPerObjectConstantBuffer.emplace_back(l_meshConstantBuffer);
			l_billboardPassDrawCallInfoVector[2].instanceCount++;
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

	l_billboardPassDrawCallInfoVector[0].meshConstantBufferOffset = 0;
	l_billboardPassDrawCallInfoVector[1].meshConstantBufferOffset = (uint32_t)l_directionalLightPerObjectConstantBuffer.size();
	l_billboardPassDrawCallInfoVector[2].meshConstantBufferOffset = (uint32_t)(l_directionalLightPerObjectConstantBuffer.size() + l_pointLightPerObjectConstantBuffer.size());

	l_billboardPassPerObjectConstantBuffer.insert(l_billboardPassPerObjectConstantBuffer.end(), l_directionalLightPerObjectConstantBuffer.begin(), l_directionalLightPerObjectConstantBuffer.end());
	l_billboardPassPerObjectConstantBuffer.insert(l_billboardPassPerObjectConstantBuffer.end(), l_pointLightPerObjectConstantBuffer.begin(), l_pointLightPerObjectConstantBuffer.end());
	l_billboardPassPerObjectConstantBuffer.insert(l_billboardPassPerObjectConstantBuffer.end(), l_sphereLightPerObjectConstantBuffer.begin(), l_sphereLightPerObjectConstantBuffer.end());

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
		updatePerFrameConstantBuffer();

		updateLightData();

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

const PerFrameConstantBuffer& InnoRenderingFrontend::getPerFrameConstantBuffer()
{
	return m_perFrameCBuffer.GetValue();
}

const std::vector<CSMConstantBuffer>& InnoRenderingFrontend::getCSMConstantBuffer()
{
	return m_CSMConstantBufferVector.GetValue();
}

const std::vector<PointLightConstantBuffer>& InnoRenderingFrontend::getPointLightConstantBuffer()
{
	return m_pointLightConstantBuffer.GetValue();
}

const std::vector<SphereLightConstantBuffer>& InnoRenderingFrontend::getSphereLightConstantBuffer()
{
	return m_sphereLightConstantBuffer.GetValue();
}

uint32_t InnoRenderingFrontend::getSunShadowPassDrawCallCount()
{
	return m_sunShadowPassDrawCallCount;
}

const std::vector<OpaquePassDrawCallInfo>& InnoRenderingFrontend::getSunShadowPassDrawCallInfo()
{
	return m_sunShadowPassDrawCallInfoVector.GetValue();
}

const std::vector<PerObjectConstantBuffer>& InnoRenderingFrontend::getSunShadowPassPerObjectConstantBuffer()
{
	return m_sunShadowPassPerObjectConstantBuffer.GetValue();
}

uint32_t InnoRenderingFrontend::getOpaquePassDrawCallCount()
{
	return m_opaquePassDrawCallCount;
}

const std::vector<OpaquePassDrawCallInfo>& InnoRenderingFrontend::getOpaquePassDrawCallInfo()
{
	return m_opaquePassDrawCallInfoVector.GetValue();
}

const std::vector<PerObjectConstantBuffer>& InnoRenderingFrontend::getOpaquePassPerObjectConstantBuffer()
{
	return m_opaquePassPerObjectConstantBuffer.GetValue();
}

const std::vector<MaterialConstantBuffer>& InnoRenderingFrontend::getOpaquePassMaterialConstantBuffer()
{
	return m_opaquePassMaterialConstantBuffer.GetValue();
}

uint32_t InnoRenderingFrontend::getTransparentPassDrawCallCount()
{
	return m_transparentPassDrawCallCount;
}

const std::vector<TransparentPassDrawCallInfo>& InnoRenderingFrontend::getTransparentPassDrawCallInfo()
{
	return m_transparentPassDrawCallInfoVector.GetValue();
}

const std::vector<PerObjectConstantBuffer>& InnoRenderingFrontend::getTransparentPassPerObjectConstantBuffer()
{
	return m_transparentPassPerObjectConstantBuffer.GetValue();
}

const std::vector<MaterialConstantBuffer>& InnoRenderingFrontend::getTransparentPassMaterialConstantBuffer()
{
	return m_transparentPassMaterialConstantBuffer.GetValue();
}

const std::vector<BillboardPassDrawCallInfo>& InnoRenderingFrontend::getBillboardPassDrawCallInfo()
{
	return m_billboardPassDrawCallInfoVector.GetValue();
}

const std::vector<PerObjectConstantBuffer>& InnoRenderingFrontend::getBillboardPassPerObjectConstantBuffer()
{
	return m_billboardPassPerObjectConstantBuffer.GetValue();
}

uint32_t InnoRenderingFrontend::getDebugPassDrawCallCount()
{
	return m_debugPassDrawCallCount;
}

const std::vector<DebugPassDrawCallInfo>& InnoRenderingFrontend::getDebugPassDrawCallInfo()
{
	return m_debugPassDrawCallInfoVector.GetValue();
}

const std::vector<PerObjectConstantBuffer>& InnoRenderingFrontend::getDebugPassPerObjectConstantBuffer()
{
	return m_debugPassPerObjectConstantBuffer.GetValue();
}