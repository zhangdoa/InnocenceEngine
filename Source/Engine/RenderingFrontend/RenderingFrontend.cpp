#include "RenderingFrontend.h"
#include "../Common/CommonMacro.inl"
#include "../ComponentManager/ITransformComponentManager.h"
#include "../ComponentManager/IVisibleComponentManager.h"
#include "../ComponentManager/ILightComponentManager.h"
#include "../ComponentManager/ICameraComponentManager.h"

#include "../Core/InnoLogger.h"
#include "../Core/InnoMemory.h"

#include "../Interface/IModuleManager.h"
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

	DoubleBuffer<PerFrameConstantBuffer, true> m_perFrameCB;

	DoubleBuffer<std::vector<CSMConstantBuffer>, true> m_CSMCBVector;

	DoubleBuffer<std::vector<PointLightConstantBuffer>, true> m_pointLightCBVector;
	DoubleBuffer<std::vector<SphereLightConstantBuffer>, true> m_sphereLightCBVector;

	uint32_t m_drawCallCount = 0;
	DoubleBuffer<std::vector<DrawCallInfo>, true> m_drawCallInfoVector;
	DoubleBuffer<std::vector<PerObjectConstantBuffer>, true> m_perObjectCBVector;
	DoubleBuffer<std::vector<MaterialConstantBuffer>, true> m_materialCBVector;

	DoubleBuffer<std::vector<PerObjectConstantBuffer>, true> m_directionalLightPerObjectCB;
	DoubleBuffer<std::vector<PerObjectConstantBuffer>, true> m_pointLightPerObjectCB;
	DoubleBuffer<std::vector<PerObjectConstantBuffer>, true> m_sphereLightPerObjectCB;

	DoubleBuffer<std::vector<BillboardPassDrawCallInfo>, true> m_billboardPassDrawCallInfoVector;
	DoubleBuffer<std::vector<PerObjectConstantBuffer>, true> m_billboardPassPerObjectCB;

	DoubleBuffer<std::vector<DebugPassDrawCallInfo>, true> m_debugPassDrawCallInfoVector;
	DoubleBuffer<std::vector<PerObjectConstantBuffer>, true> m_debugPassPerObjectCB;

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

	MaterialDataComponent* m_defaultMaterial;

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

	m_CSMCBVector.Reserve(m_renderingCapability.maxCSMSplits);

	m_drawCallInfoVector.Reserve(m_renderingCapability.maxMeshes);
	m_perObjectCBVector.Reserve(m_renderingCapability.maxMeshes);
	m_materialCBVector.Reserve(m_renderingCapability.maxMaterials);

	m_pointLightCBVector.Reserve(m_renderingCapability.maxPointLights);
	m_sphereLightCBVector.Reserve(m_renderingCapability.maxSphereLights);

	m_directionalLightPerObjectCB.Reserve(8192);
	m_pointLightPerObjectCB.Reserve(8192);
	m_sphereLightPerObjectCB.Reserve(8192);
	m_billboardPassPerObjectCB.Reserve(8192);

	f_sceneLoadingStartCallback = [&]() {
		m_cullingData.clear();

		m_drawCallCount = 0;
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

	m_SkeletonDataComponentPool = InnoMemory::CreateObjectPool<SkeletonDataComponent>(2048);
	m_AnimationDataComponentPool = InnoMemory::CreateObjectPool<AnimationDataComponent>(16384);

	m_rayTracer->Setup();

	m_ObjectStatus = ObjectStatus::Created;
	return true;
}

bool InnoRenderingFrontendNS::loadDefaultAssets()
{
	auto m_basicNormalTexture = g_pModuleManager->getFileSystem()->loadTexture("..//Res//Textures//basic_normal.png");
	m_basicNormalTexture->m_TextureDesc.SamplerType = TextureSamplerType::Sampler2D;
	m_basicNormalTexture->m_TextureDesc.UsageType = TextureUsageType::Sample;

	auto m_basicAlbedoTexture = g_pModuleManager->getFileSystem()->loadTexture("..//Res//Textures//basic_albedo.png");
	m_basicAlbedoTexture->m_TextureDesc.SamplerType = TextureSamplerType::Sampler2D;
	m_basicAlbedoTexture->m_TextureDesc.UsageType = TextureUsageType::Sample;

	auto m_basicMetallicTexture = g_pModuleManager->getFileSystem()->loadTexture("..//Res//Textures//basic_metallic.png");
	m_basicMetallicTexture->m_TextureDesc.SamplerType = TextureSamplerType::Sampler2D;
	m_basicMetallicTexture->m_TextureDesc.UsageType = TextureUsageType::Sample;

	auto m_basicRoughnessTexture = g_pModuleManager->getFileSystem()->loadTexture("..//Res//Textures//basic_roughness.png");
	m_basicRoughnessTexture->m_TextureDesc.SamplerType = TextureSamplerType::Sampler2D;
	m_basicRoughnessTexture->m_TextureDesc.UsageType = TextureUsageType::Sample;

	auto m_basicAOTexture = g_pModuleManager->getFileSystem()->loadTexture("..//Res//Textures//basic_ao.png");
	m_basicAOTexture->m_TextureDesc.SamplerType = TextureSamplerType::Sampler2D;
	m_basicAOTexture->m_TextureDesc.UsageType = TextureUsageType::Sample;

	m_defaultMaterial = m_renderingServer->AddMaterialDataComponent("BasicMaterial/");
	m_defaultMaterial->m_TextureSlots[0].m_Texture = m_basicNormalTexture;
	m_defaultMaterial->m_TextureSlots[1].m_Texture = m_basicAlbedoTexture;
	m_defaultMaterial->m_TextureSlots[2].m_Texture = m_basicMetallicTexture;
	m_defaultMaterial->m_TextureSlots[3].m_Texture = m_basicRoughnessTexture;
	m_defaultMaterial->m_TextureSlots[4].m_Texture = m_basicAOTexture;
	m_defaultMaterial->m_TextureSlots[5].m_Texture = m_basicAOTexture;
	m_defaultMaterial->m_TextureSlots[6].m_Texture = m_basicAOTexture;
	m_defaultMaterial->m_TextureSlots[7].m_Texture = m_basicAOTexture;

	m_iconTemplate_DirectionalLight = g_pModuleManager->getFileSystem()->loadTexture("..//Res//Textures//InnoWorldEditorIcons_DirectionalLight.png");
	m_iconTemplate_DirectionalLight->m_TextureDesc.SamplerType = TextureSamplerType::Sampler2D;
	m_iconTemplate_DirectionalLight->m_TextureDesc.UsageType = TextureUsageType::Sample;

	m_iconTemplate_PointLight = g_pModuleManager->getFileSystem()->loadTexture("..//Res//Textures//InnoWorldEditorIcons_PointLight.png");
	m_iconTemplate_PointLight->m_TextureDesc.SamplerType = TextureSamplerType::Sampler2D;
	m_iconTemplate_PointLight->m_TextureDesc.UsageType = TextureUsageType::Sample;

	m_iconTemplate_SphereLight = g_pModuleManager->getFileSystem()->loadTexture("..//Res//Textures//InnoWorldEditorIcons_SphereLight.png");
	m_iconTemplate_SphereLight->m_TextureDesc.SamplerType = TextureSamplerType::Sampler2D;
	m_iconTemplate_SphereLight->m_TextureDesc.UsageType = TextureUsageType::Sample;

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

		m_renderingServer->InitializeMaterialDataComponent(m_defaultMaterial);
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

	PerFrameConstantBuffer l_PerFrameCB;

	auto l_p = l_mainCamera->m_projectionMatrix;

	l_PerFrameCB.p_original = l_p;
	l_PerFrameCB.p_jittered = l_p;

	if (m_renderingConfig.useTAA)
	{
		//TAA jitter for projection matrix
		auto& l_currentHaltonStep = m_currentHaltonStep;
		if (l_currentHaltonStep >= 16)
		{
			l_currentHaltonStep = 0;
		}
		l_PerFrameCB.p_jittered.m02 = m_haltonSampler[l_currentHaltonStep].x / m_screenResolution.x;
		l_PerFrameCB.p_jittered.m12 = m_haltonSampler[l_currentHaltonStep].y / m_screenResolution.y;
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

	l_PerFrameCB.camera_posWS = l_mainCameraTransformComponent->m_globalTransformVector.m_pos;

	l_PerFrameCB.v = r * t;

	auto r_prev = l_mainCameraTransformComponent->m_globalTransformMatrix_prev.m_rotationMat.inverse();
	auto t_prev = l_mainCameraTransformComponent->m_globalTransformMatrix_prev.m_translationMat.inverse();

	l_PerFrameCB.v_prev = r_prev * t_prev;

	l_PerFrameCB.zNear = l_mainCamera->m_zNear;
	l_PerFrameCB.zFar = l_mainCamera->m_zFar;

	l_PerFrameCB.p_inv = l_p.inverse();
	l_PerFrameCB.v_inv = l_PerFrameCB.v.inverse();
	l_PerFrameCB.viewportSize.x = (float)m_screenResolution.x;
	l_PerFrameCB.viewportSize.y = (float)m_screenResolution.y;
	l_PerFrameCB.minLogLuminance = -10.0f;
	l_PerFrameCB.maxLogLuminance = 16.0f;

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

	l_PerFrameCB.sun_direction = InnoMath::getDirection(Direction::Backward, l_sunTransformComponent->m_globalTransformVector.m_rot);
	l_PerFrameCB.sun_illuminance = l_sun->m_RGBColor * l_sun->m_LuminousFlux;

	m_perFrameCB.SetValue(std::move(l_PerFrameCB));

	auto l_SplitAABB = GetComponentManager(LightComponent)->GetSunSplitAABB();
	auto l_ProjectionMatrices = GetComponentManager(LightComponent)->GetSunProjectionMatrices();

	auto& l_CSMCBVector = m_CSMCBVector.GetValue();
	l_CSMCBVector.clear();

	if (l_SplitAABB.size() > 0 && l_ProjectionMatrices.size() > 0)
	{
		for (size_t j = 0; j < m_renderingCapability.maxCSMSplits; j++)
		{
			CSMConstantBuffer l_CSMCB;

			l_CSMCB.p = l_ProjectionMatrices[j];
			l_CSMCB.AABBMax = l_SplitAABB[j].m_boundMax;
			l_CSMCB.AABBMin = l_SplitAABB[j].m_boundMin;
			l_CSMCB.v = l_lightRotMat;

			l_CSMCBVector.emplace_back(l_CSMCB);
		}
	}

	return true;
}

bool InnoRenderingFrontendNS::updateLightData()
{
	auto& l_PointLightCB = m_pointLightCBVector.GetValue();
	auto& l_SphereLightCB = m_sphereLightCBVector.GetValue();

	l_PointLightCB.clear();
	l_SphereLightCB.clear();

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
					l_PointLightCB.emplace_back(l_data);
				}
				else if (l_lightComponents[i]->m_LightType == LightType::Sphere)
				{
					SphereLightConstantBuffer l_data;
					l_data.pos = l_transformCompoent->m_globalTransformVector.m_pos;
					l_data.luminance = l_lightComponents[i]->m_RGBColor * l_lightComponents[i]->m_LuminousFlux;
					l_data.luminance.w = l_lightComponents[i]->m_Shape.x;
					l_SphereLightCB.emplace_back(l_data);
				}
			}
		}
	}

	return true;
}

bool InnoRenderingFrontendNS::updateMeshData()
{
	auto& l_drawCallInfoVector = m_drawCallInfoVector.GetValue();
	auto& l_perObjectCBVector = m_perObjectCBVector.GetValue();
	auto& l_materialCBVector = m_materialCBVector.GetValue();

	l_drawCallInfoVector.clear();
	l_perObjectCBVector.clear();
	l_materialCBVector.clear();

	auto l_cullingDataSize = m_cullingData.size();

	for (size_t i = 0; i < l_cullingDataSize; i++)
	{
		auto l_cullingData = m_cullingData[i];
		if (l_cullingData.mesh != nullptr)
		{
			if (l_cullingData.mesh->m_ObjectStatus == ObjectStatus::Activated)
			{
				if (l_cullingData.material != nullptr)
				{
					DrawCallInfo l_drawCallInfo;

					l_drawCallInfo.mesh = l_cullingData.mesh;
					l_drawCallInfo.material = l_cullingData.material;
					// @TODO: use culled info
					l_drawCallInfo.castSunShadow = true;
					l_drawCallInfo.visibilityType = l_cullingData.visibilityType;
					l_drawCallInfo.meshConstantBufferIndex = (uint32_t)i;
					l_drawCallInfo.materialConstantBufferIndex = (uint32_t)i;

					PerObjectConstantBuffer l_perObjectCB;
					l_perObjectCB.m = l_cullingData.m;
					l_perObjectCB.m_prev = l_cullingData.m_prev;
					l_perObjectCB.normalMat = l_cullingData.normalMat;
					l_perObjectCB.UUID = (float)l_cullingData.UUID;

					MaterialConstantBuffer l_materialCB;

					for (size_t i = 0; i < 8; i++)
					{
						uint32_t l_writeMask = l_drawCallInfo.material->m_TextureSlots[i].m_Activate ? 0x00000001 : 0x00000000;
						l_writeMask = l_writeMask << i;
						l_materialCB.textureSlotMask |= l_writeMask;
					}
					l_materialCB.materialType = int32_t(l_cullingData.meshUsage);
					l_materialCB.materialAttributes = l_cullingData.material->m_materialAttributes;

					l_drawCallInfoVector.emplace_back(l_drawCallInfo);
					l_perObjectCBVector.emplace_back(l_perObjectCB);
					l_materialCBVector.emplace_back(l_materialCB);
				}
			}
		}
	}

	// @TODO: use GPU to do OIT

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
	auto& l_billboardPassPerObjectCB = m_billboardPassPerObjectCB.GetValue();
	auto& l_directionalLightPerObjectCB = m_directionalLightPerObjectCB.GetValue();
	auto& l_pointLightPerObjectCB = m_pointLightPerObjectCB.GetValue();
	auto& l_sphereLightPerObjectCB = m_sphereLightPerObjectCB.GetValue();

	auto l_billboardPassDrawCallInfoCount = l_billboardPassDrawCallInfoVector.size();
	for (size_t i = 0; i < l_billboardPassDrawCallInfoCount; i++)
	{
		l_billboardPassDrawCallInfoVector[i].instanceCount = 0;
	}

	l_billboardPassPerObjectCB.clear();

	l_directionalLightPerObjectCB.clear();
	l_pointLightPerObjectCB.clear();
	l_sphereLightPerObjectCB.clear();

	for (auto i : l_lightComponents)
	{
		PerObjectConstantBuffer l_meshCB;

		auto l_transformCompoent = GetComponent(TransformComponent, i->m_ParentEntity);
		if (l_transformCompoent != nullptr)
		{
			l_meshCB.m = InnoMath::toTranslationMatrix(l_transformCompoent->m_globalTransformVector.m_pos);
		}

		switch (i->m_LightType)
		{
		case LightType::Directional:
			l_directionalLightPerObjectCB.emplace_back(l_meshCB);
			l_billboardPassDrawCallInfoVector[0].instanceCount++;
			break;
		case LightType::Point:
			l_pointLightPerObjectCB.emplace_back(l_meshCB);
			l_billboardPassDrawCallInfoVector[1].instanceCount++;
			break;
		case LightType::Spot:
			break;
		case LightType::Sphere:
			l_sphereLightPerObjectCB.emplace_back(l_meshCB);
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
	l_billboardPassDrawCallInfoVector[1].meshConstantBufferOffset = (uint32_t)l_directionalLightPerObjectCB.size();
	l_billboardPassDrawCallInfoVector[2].meshConstantBufferOffset = (uint32_t)(l_directionalLightPerObjectCB.size() + l_pointLightPerObjectCB.size());

	l_billboardPassPerObjectCB.insert(l_billboardPassPerObjectCB.end(), l_directionalLightPerObjectCB.begin(), l_directionalLightPerObjectCB.end());
	l_billboardPassPerObjectCB.insert(l_billboardPassPerObjectCB.end(), l_pointLightPerObjectCB.begin(), l_pointLightPerObjectCB.end());
	l_billboardPassPerObjectCB.insert(l_billboardPassPerObjectCB.end(), l_sphereLightPerObjectCB.begin(), l_sphereLightPerObjectCB.end());

	return true;
}

bool InnoRenderingFrontendNS::updateDebuggerPassData()
{
	// @TODO: Implementation

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
	auto l_SDC = InnoMemory::Spawn<SkeletonDataComponent>(m_SkeletonDataComponentPool);
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
	auto l_ADC = InnoMemory::Spawn<AnimationDataComponent>(m_AnimationDataComponentPool);
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
	return m_defaultMaterial;
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
	return m_perFrameCB.GetValue();
}

const std::vector<CSMConstantBuffer>& InnoRenderingFrontend::getCSMConstantBuffer()
{
	return m_CSMCBVector.GetValue();
}

const std::vector<PointLightConstantBuffer>& InnoRenderingFrontend::getPointLightConstantBuffer()
{
	return m_pointLightCBVector.GetValue();
}

const std::vector<SphereLightConstantBuffer>& InnoRenderingFrontend::getSphereLightConstantBuffer()
{
	return m_sphereLightCBVector.GetValue();
}

const std::vector<DrawCallInfo>& InnoRenderingFrontend::getDrawCallInfo()
{
	return m_drawCallInfoVector.GetValue();
}

const std::vector<PerObjectConstantBuffer>& InnoRenderingFrontend::getPerObjectConstantBuffer()
{
	return m_perObjectCBVector.GetValue();
}

const std::vector<MaterialConstantBuffer>& InnoRenderingFrontend::getMaterialConstantBuffer()
{
	return m_materialCBVector.GetValue();
}

const std::vector<BillboardPassDrawCallInfo>& InnoRenderingFrontend::getBillboardPassDrawCallInfo()
{
	return m_billboardPassDrawCallInfoVector.GetValue();
}

const std::vector<PerObjectConstantBuffer>& InnoRenderingFrontend::getBillboardPassPerObjectConstantBuffer()
{
	return m_billboardPassPerObjectCB.GetValue();
}

const std::vector<DebugPassDrawCallInfo>& InnoRenderingFrontend::getDebugPassDrawCallInfo()
{
	return m_debugPassDrawCallInfoVector.GetValue();
}

const std::vector<PerObjectConstantBuffer>& InnoRenderingFrontend::getDebugPassPerObjectConstantBuffer()
{
	return m_debugPassPerObjectCB.GetValue();
}