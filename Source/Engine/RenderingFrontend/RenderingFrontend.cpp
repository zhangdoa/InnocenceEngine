#include "RenderingFrontend.h"

#include "../Core/Logger.h"

#include "../Interface/IEngine.h"
using namespace Inno;
extern IEngine* g_Engine;

#include "../RayTracer/RayTracer.h"

namespace RenderingFrontendNS
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

	DoubleBuffer<std::vector<AnimationDrawCallInfo>, true> m_animationDrawCallInfoVector;
	DoubleBuffer<std::vector<AnimationConstantBuffer>, true> m_animationCBVector;

	DoubleBuffer<std::vector<PerObjectConstantBuffer>, true> m_directionalLightPerObjectCB;
	DoubleBuffer<std::vector<PerObjectConstantBuffer>, true> m_pointLightPerObjectCB;
	DoubleBuffer<std::vector<PerObjectConstantBuffer>, true> m_sphereLightPerObjectCB;

	DoubleBuffer<std::vector<BillboardPassDrawCallInfo>, true> m_billboardPassDrawCallInfoVector;
	DoubleBuffer<std::vector<PerObjectConstantBuffer>, true> m_billboardPassPerObjectCB;

	DoubleBuffer<std::vector<DebugPassDrawCallInfo>, true> m_debugPassDrawCallInfoVector;
	DoubleBuffer<std::vector<PerObjectConstantBuffer>, true> m_debugPassPerObjectCB;

	ThreadSafeUnorderedMap<uint64_t, AnimationInstance> m_animationInstanceMap;

	std::vector<CullingData> m_cullingData;

	std::vector<Vec2> m_haltonSampler;
	int32_t m_currentHaltonStep = 0;
	int64_t m_previousTime = 0;
	int64_t m_currentTime = 0;

	std::function<void()> f_sceneLoadingStartCallback;
	std::function<void()> f_sceneLoadingFinishCallback;

	RenderingConfig m_renderingConfig = RenderingConfig();

	ThreadSafeQueue<MeshComponent*> m_uninitializedMeshes;
	ThreadSafeQueue<MaterialComponent*> m_uninitializedMaterials;
	ThreadSafeQueue<AnimationComponent*> m_uninitializedAnimations;

	ThreadSafeUnorderedMap<std::string, AnimationData> m_animationDataInfosLUT;

	TextureComponent* m_iconTemplate_DirectionalLight;
	TextureComponent* m_iconTemplate_PointLight;
	TextureComponent* m_iconTemplate_SphereLight;

	MeshComponent* m_unitTriangleMesh;
	MeshComponent* m_unitSquareMesh;
	MeshComponent* m_unitPentagonMesh;
	MeshComponent* m_unitHexagonMesh;

	MeshComponent* m_unitTetrahedronMesh;
	MeshComponent* m_unitCubeMesh;
	MeshComponent* m_unitOctahedronMesh;
	MeshComponent* m_unitDodecahedronMesh;
	MeshComponent* m_unitIcosahedronMesh;
	MeshComponent* m_unitSphereMesh;
	MeshComponent* m_terrainMesh;

	MaterialComponent* m_defaultMaterial;

	bool Setup(ISystemConfig* systemConfig);
	bool loadDefaultAssets();
	bool Initialize();
	bool Update();
	bool Terminate();

	float radicalInverse(uint32_t n, uint32_t base);
	void initializeHaltonSampler();
	void initializeAnimation(AnimationComponent* rhs);
	AnimationData getAnimationData(const char* animationName);

	bool updatePerFrameConstantBuffer();
	bool updateLightData();

	bool updateMeshData();
	bool simulateAnimation();
	bool updateBillboardPassData();
	bool updateDebuggerPassData();
}

using namespace RenderingFrontendNS;

float RenderingFrontendNS::radicalInverse(uint32_t n, uint32_t base)
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

void RenderingFrontendNS::initializeHaltonSampler()
{
	// in NDC space
	for (uint32_t i = 0; i < 16; i++)
	{
		m_haltonSampler.emplace_back(Vec2(radicalInverse(i, 3) * 2.0f - 1.0f, radicalInverse(i, 4) * 2.0f - 1.0f));
	}
}

void RenderingFrontendNS::initializeAnimation(AnimationComponent* rhs)
{
	std::string l_name = rhs->m_InstanceName.c_str();

	auto l_keyData = g_Engine->getRenderingServer()->AddGPUBufferComponent((l_name + "_KeyData").c_str());
	l_keyData->m_Owner = rhs->m_Owner;
	l_keyData->m_ElementCount = rhs->m_KeyData.capacity();
	l_keyData->m_ElementSize = sizeof(KeyData);
	l_keyData->m_GPUAccessibility = Accessibility::ReadWrite;

	g_Engine->getRenderingServer()->InitializeGPUBufferComponent(l_keyData);
	g_Engine->getRenderingServer()->UploadGPUBufferComponent(l_keyData, &rhs->m_KeyData[0]);

	rhs->m_ObjectStatus = ObjectStatus::Activated;

	AnimationData l_info;
	l_info.ADC = rhs;
	l_info.keyData = l_keyData;

	m_animationDataInfosLUT.emplace(rhs->m_InstanceName.c_str(), l_info);
}

AnimationData RenderingFrontendNS::getAnimationData(const char* animationName)
{
	auto l_result = m_animationDataInfosLUT.find(animationName);
	if (l_result != m_animationDataInfosLUT.end())
	{
		return l_result->second;
	}
	else
	{
		return AnimationData();
	}
}

bool RenderingFrontendNS::Setup(ISystemConfig* systemConfig)
{
	auto l_renderingFrontendConfig = reinterpret_cast<IRenderingFrontendConfig*>(systemConfig);

	m_renderingServer = l_renderingFrontendConfig->m_RenderingServer;
	m_rayTracer = new RayTracer();

	m_renderingConfig.useCSM = true;
	m_renderingConfig.useMotionBlur = true;
	m_renderingConfig.useTAA = true;
	//m_renderingConfig.useBloom = true;
	m_renderingConfig.drawSky = true;
	//m_renderingConfig.drawTerrain = true;
	m_renderingConfig.drawDebugObject = true;
	m_renderingConfig.CSMFitToScene = true;
	m_renderingConfig.CSMAdjustDrawDistance = true;
	m_renderingConfig.CSMAdjustSidePlane = false;

	m_renderingCapability.maxCSMSplits = 4;
	m_renderingCapability.maxPointLights = 1024;
	m_renderingCapability.maxSphereLights = 128;
	m_renderingCapability.maxMeshes = 4096;
	m_renderingCapability.maxMaterials = 4096;
	m_renderingCapability.maxTextures = 4096;

	m_DefaultRenderPassDesc.m_UseMultiFrames = false;
	m_DefaultRenderPassDesc.m_RenderTargetCount = 1;
	m_DefaultRenderPassDesc.m_RenderTargetDesc.Sampler = TextureSampler::Sampler2D;
	m_DefaultRenderPassDesc.m_RenderTargetDesc.Usage = TextureUsage::ColorAttachment;
	m_DefaultRenderPassDesc.m_RenderTargetDesc.PixelDataFormat = TexturePixelDataFormat::RGBA;
	m_DefaultRenderPassDesc.m_RenderTargetDesc.Width = m_screenResolution.x;
	m_DefaultRenderPassDesc.m_RenderTargetDesc.Height = m_screenResolution.y;
	m_DefaultRenderPassDesc.m_RenderTargetDesc.DepthOrArraySize = 1;
	m_DefaultRenderPassDesc.m_RenderTargetDesc.PixelDataType = TexturePixelDataType::Float16;

	m_DefaultRenderPassDesc.m_GraphicsPipelineDesc.m_ViewportDesc.m_Width = (float)m_screenResolution.x;
	m_DefaultRenderPassDesc.m_GraphicsPipelineDesc.m_ViewportDesc.m_Height = (float)m_screenResolution.y;

	m_previousTime = g_Engine->getTimeSystem()->getCurrentTimeFromEpoch();
	m_currentTime = g_Engine->getTimeSystem()->getCurrentTimeFromEpoch();

	m_CSMCBVector.Reserve(m_renderingCapability.maxCSMSplits);

	m_drawCallInfoVector.Reserve(m_renderingCapability.maxMeshes);
	m_perObjectCBVector.Reserve(m_renderingCapability.maxMeshes);
	m_materialCBVector.Reserve(m_renderingCapability.maxMaterials);
	m_animationDrawCallInfoVector.Reserve(512);
	m_animationCBVector.Reserve(512);

	m_pointLightCBVector.Reserve(m_renderingCapability.maxPointLights);
	m_sphereLightCBVector.Reserve(m_renderingCapability.maxSphereLights);

	m_directionalLightPerObjectCB.Reserve(4096);
	m_pointLightPerObjectCB.Reserve(4096);
	m_sphereLightPerObjectCB.Reserve(4096);
	m_billboardPassPerObjectCB.Reserve(4096);

	f_sceneLoadingStartCallback = [&]() {
		m_cullingData.clear();

		m_drawCallCount = 0;
	};

	f_sceneLoadingFinishCallback = [&]()
	{
		// @TODO:
		std::vector<BillboardPassDrawCallInfo> l_billboardPassDrawCallInfoVectorA(3);
		l_billboardPassDrawCallInfoVectorA[0].iconTexture = g_Engine->getRenderingFrontend()->GetTextureComponent(WorldEditorIconType::DIRECTIONAL_LIGHT);
		l_billboardPassDrawCallInfoVectorA[1].iconTexture = g_Engine->getRenderingFrontend()->GetTextureComponent(WorldEditorIconType::POINT_LIGHT);
		l_billboardPassDrawCallInfoVectorA[2].iconTexture = g_Engine->getRenderingFrontend()->GetTextureComponent(WorldEditorIconType::SPHERE_LIGHT);
		auto l_billboardPassDrawCallInfoVectorB = l_billboardPassDrawCallInfoVectorA;

		m_billboardPassDrawCallInfoVector.SetValue(std::move(l_billboardPassDrawCallInfoVectorA));
		m_billboardPassDrawCallInfoVector.SetValue(std::move(l_billboardPassDrawCallInfoVectorB));
	};

	g_Engine->getSceneSystem()->addSceneLoadingStartCallback(&f_sceneLoadingStartCallback);
	g_Engine->getSceneSystem()->addSceneLoadingFinishCallback(&f_sceneLoadingFinishCallback);

	m_rayTracer->Setup();

	m_ObjectStatus = ObjectStatus::Created;
	return true;
}

bool RenderingFrontendNS::loadDefaultAssets()
{
	auto m_basicNormalTexture = g_Engine->getAssetSystem()->loadTexture("..//Res//Textures//basic_normal.png");
	m_basicNormalTexture->m_TextureDesc.Sampler = TextureSampler::Sampler2D;
	m_basicNormalTexture->m_TextureDesc.Usage = TextureUsage::Sample;

	auto m_basicAlbedoTexture = g_Engine->getAssetSystem()->loadTexture("..//Res//Textures//basic_albedo.png");
	m_basicAlbedoTexture->m_TextureDesc.Sampler = TextureSampler::Sampler2D;
	m_basicAlbedoTexture->m_TextureDesc.Usage = TextureUsage::Sample;

	auto m_basicMetallicTexture = g_Engine->getAssetSystem()->loadTexture("..//Res//Textures//basic_metallic.png");
	m_basicMetallicTexture->m_TextureDesc.Sampler = TextureSampler::Sampler2D;
	m_basicMetallicTexture->m_TextureDesc.Usage = TextureUsage::Sample;

	auto m_basicRoughnessTexture = g_Engine->getAssetSystem()->loadTexture("..//Res//Textures//basic_roughness.png");
	m_basicRoughnessTexture->m_TextureDesc.Sampler = TextureSampler::Sampler2D;
	m_basicRoughnessTexture->m_TextureDesc.Usage = TextureUsage::Sample;

	auto m_basicAOTexture = g_Engine->getAssetSystem()->loadTexture("..//Res//Textures//basic_ao.png");
	m_basicAOTexture->m_TextureDesc.Sampler = TextureSampler::Sampler2D;
	m_basicAOTexture->m_TextureDesc.Usage = TextureUsage::Sample;

	m_defaultMaterial = m_renderingServer->AddMaterialComponent("BasicMaterial/");
	m_defaultMaterial->m_TextureSlots[0].m_Texture = m_basicNormalTexture;
	m_defaultMaterial->m_TextureSlots[1].m_Texture = m_basicAlbedoTexture;
	m_defaultMaterial->m_TextureSlots[2].m_Texture = m_basicMetallicTexture;
	m_defaultMaterial->m_TextureSlots[3].m_Texture = m_basicRoughnessTexture;
	m_defaultMaterial->m_TextureSlots[4].m_Texture = m_basicAOTexture;
	m_defaultMaterial->m_TextureSlots[5].m_Texture = m_basicAOTexture;
	m_defaultMaterial->m_TextureSlots[6].m_Texture = m_basicAOTexture;
	m_defaultMaterial->m_TextureSlots[7].m_Texture = m_basicAOTexture;
	m_defaultMaterial->m_ShaderModel = ShaderModel::Opaque;

	m_iconTemplate_DirectionalLight = g_Engine->getAssetSystem()->loadTexture("..//Res//Textures//WorldEditorIcons_DirectionalLight.png");
	m_iconTemplate_DirectionalLight->m_TextureDesc.Sampler = TextureSampler::Sampler2D;
	m_iconTemplate_DirectionalLight->m_TextureDesc.Usage = TextureUsage::Sample;

	m_iconTemplate_PointLight = g_Engine->getAssetSystem()->loadTexture("..//Res//Textures//WorldEditorIcons_PointLight.png");
	m_iconTemplate_PointLight->m_TextureDesc.Sampler = TextureSampler::Sampler2D;
	m_iconTemplate_PointLight->m_TextureDesc.Usage = TextureUsage::Sample;

	m_iconTemplate_SphereLight = g_Engine->getAssetSystem()->loadTexture("..//Res//Textures//WorldEditorIcons_SphereLight.png");
	m_iconTemplate_SphereLight->m_TextureDesc.Sampler = TextureSampler::Sampler2D;
	m_iconTemplate_SphereLight->m_TextureDesc.Usage = TextureUsage::Sample;

	m_unitTriangleMesh = m_renderingServer->AddMeshComponent("UnitTriangleMesh/");
	g_Engine->getAssetSystem()->generateProceduralMesh(ProceduralMeshShape::Triangle, m_unitTriangleMesh);
	m_unitTriangleMesh->m_MeshPrimitiveTopology = MeshPrimitiveTopology::Triangle;
	m_unitTriangleMesh->m_ProceduralMeshShape = ProceduralMeshShape::Triangle;
	m_unitTriangleMesh->m_ObjectStatus = ObjectStatus::Created;

	m_unitSquareMesh = m_renderingServer->AddMeshComponent("UnitSquareMesh/");
	g_Engine->getAssetSystem()->generateProceduralMesh(ProceduralMeshShape::Square, m_unitSquareMesh);
	m_unitSquareMesh->m_MeshPrimitiveTopology = MeshPrimitiveTopology::Triangle;
	m_unitSquareMesh->m_ProceduralMeshShape = ProceduralMeshShape::Square;
	m_unitSquareMesh->m_ObjectStatus = ObjectStatus::Created;

	m_unitPentagonMesh = m_renderingServer->AddMeshComponent("UnitPentagonMesh/");
	g_Engine->getAssetSystem()->generateProceduralMesh(ProceduralMeshShape::Pentagon, m_unitPentagonMesh);
	m_unitPentagonMesh->m_MeshPrimitiveTopology = MeshPrimitiveTopology::Triangle;
	m_unitPentagonMesh->m_ProceduralMeshShape = ProceduralMeshShape::Pentagon;
	m_unitPentagonMesh->m_ObjectStatus = ObjectStatus::Created;

	m_unitHexagonMesh = m_renderingServer->AddMeshComponent("UnitHexagonMesh/");
	g_Engine->getAssetSystem()->generateProceduralMesh(ProceduralMeshShape::Hexagon, m_unitHexagonMesh);
	m_unitHexagonMesh->m_MeshPrimitiveTopology = MeshPrimitiveTopology::Triangle;
	m_unitHexagonMesh->m_ProceduralMeshShape = ProceduralMeshShape::Hexagon;
	m_unitHexagonMesh->m_ObjectStatus = ObjectStatus::Created;

	m_unitTetrahedronMesh = m_renderingServer->AddMeshComponent("UnitTetrahedronMesh/");
	g_Engine->getAssetSystem()->generateProceduralMesh(ProceduralMeshShape::Tetrahedron, m_unitTetrahedronMesh);
	m_unitTetrahedronMesh->m_MeshPrimitiveTopology = MeshPrimitiveTopology::Triangle;
	m_unitTetrahedronMesh->m_ProceduralMeshShape = ProceduralMeshShape::Tetrahedron;
	m_unitTetrahedronMesh->m_ObjectStatus = ObjectStatus::Created;

	m_unitCubeMesh = m_renderingServer->AddMeshComponent("UnitCubeMesh/");
	g_Engine->getAssetSystem()->generateProceduralMesh(ProceduralMeshShape::Cube, m_unitCubeMesh);
	m_unitCubeMesh->m_MeshPrimitiveTopology = MeshPrimitiveTopology::Triangle;
	m_unitCubeMesh->m_ProceduralMeshShape = ProceduralMeshShape::Cube;
	m_unitCubeMesh->m_ObjectStatus = ObjectStatus::Created;

	m_unitOctahedronMesh = m_renderingServer->AddMeshComponent("UnitOctahedronMesh/");
	g_Engine->getAssetSystem()->generateProceduralMesh(ProceduralMeshShape::Octahedron, m_unitOctahedronMesh);
	m_unitOctahedronMesh->m_MeshPrimitiveTopology = MeshPrimitiveTopology::Triangle;
	m_unitOctahedronMesh->m_ProceduralMeshShape = ProceduralMeshShape::Octahedron;
	m_unitOctahedronMesh->m_ObjectStatus = ObjectStatus::Created;

	m_unitDodecahedronMesh = m_renderingServer->AddMeshComponent("UnitDodecahedronMesh/");
	g_Engine->getAssetSystem()->generateProceduralMesh(ProceduralMeshShape::Dodecahedron, m_unitDodecahedronMesh);
	m_unitDodecahedronMesh->m_MeshPrimitiveTopology = MeshPrimitiveTopology::Triangle;
	m_unitDodecahedronMesh->m_ProceduralMeshShape = ProceduralMeshShape::Dodecahedron;
	m_unitDodecahedronMesh->m_ObjectStatus = ObjectStatus::Created;

	m_unitIcosahedronMesh = m_renderingServer->AddMeshComponent("UnitIcosahedronMesh/");
	g_Engine->getAssetSystem()->generateProceduralMesh(ProceduralMeshShape::Icosahedron, m_unitIcosahedronMesh);
	m_unitIcosahedronMesh->m_MeshPrimitiveTopology = MeshPrimitiveTopology::Triangle;
	m_unitIcosahedronMesh->m_ProceduralMeshShape = ProceduralMeshShape::Icosahedron;
	m_unitIcosahedronMesh->m_ObjectStatus = ObjectStatus::Created;

	m_unitSphereMesh = m_renderingServer->AddMeshComponent("UnitSphereMesh/");
	g_Engine->getAssetSystem()->generateProceduralMesh(ProceduralMeshShape::Sphere, m_unitSphereMesh);
	m_unitSphereMesh->m_MeshPrimitiveTopology = MeshPrimitiveTopology::Triangle;
	m_unitSphereMesh->m_ProceduralMeshShape = ProceduralMeshShape::Sphere;
	m_unitSphereMesh->m_ObjectStatus = ObjectStatus::Created;

	auto l_DefaultAssetInitializeTask = g_Engine->getTaskSystem()->Submit("DefaultAssetInitializeTask", 2, nullptr,
		[&]() {
			m_renderingServer->InitializeMeshComponent(m_unitTriangleMesh);
			m_renderingServer->InitializeMeshComponent(m_unitSquareMesh);
			m_renderingServer->InitializeMeshComponent(m_unitPentagonMesh);
			m_renderingServer->InitializeMeshComponent(m_unitHexagonMesh);

			m_renderingServer->InitializeMeshComponent(m_unitTetrahedronMesh);
			m_renderingServer->InitializeMeshComponent(m_unitCubeMesh);
			m_renderingServer->InitializeMeshComponent(m_unitOctahedronMesh);
			m_renderingServer->InitializeMeshComponent(m_unitDodecahedronMesh);
			m_renderingServer->InitializeMeshComponent(m_unitIcosahedronMesh);
			m_renderingServer->InitializeMeshComponent(m_unitSphereMesh);

			m_renderingServer->InitializeTextureComponent(m_basicNormalTexture);
			m_renderingServer->InitializeTextureComponent(m_basicAlbedoTexture);
			m_renderingServer->InitializeTextureComponent(m_basicMetallicTexture);
			m_renderingServer->InitializeTextureComponent(m_basicRoughnessTexture);
			m_renderingServer->InitializeTextureComponent(m_basicAOTexture);

			m_renderingServer->InitializeTextureComponent(m_iconTemplate_DirectionalLight);
			m_renderingServer->InitializeTextureComponent(m_iconTemplate_PointLight);
			m_renderingServer->InitializeTextureComponent(m_iconTemplate_SphereLight);

			m_renderingServer->InitializeMaterialComponent(m_defaultMaterial);
		});

	l_DefaultAssetInitializeTask.m_Future->Get();

	return true;
}

bool RenderingFrontendNS::Initialize()
{
	if (m_ObjectStatus == ObjectStatus::Created)
	{
		loadDefaultAssets();

		initializeHaltonSampler();
		m_rayTracer->Initialize();

		m_ObjectStatus = ObjectStatus::Activated;
		Logger::Log(LogLevel::Success, "RenderingFrontend has been initialized.");
		return true;
	}
	else
	{
		Logger::Log(LogLevel::Error, "RenderingFrontend: Object is not created!");
		return false;
	}
}

bool RenderingFrontendNS::updatePerFrameConstantBuffer()
{
	auto l_camera = static_cast<ICameraSystem*>(g_Engine->getComponentManager()->GetComponentSystem<CameraComponent>())->GetActiveCamera();

	if (l_camera == nullptr)
	{
		return false;
	}

	auto l_cameraTransformComponent = g_Engine->getComponentManager()->Find<TransformComponent>(l_camera->m_Owner);

	if (l_cameraTransformComponent == nullptr)
	{
		return false;
	}

	PerFrameConstantBuffer l_PerFrameCB;

	auto l_p = l_camera->m_projectionMatrix;

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

	auto r = Math::getInvertRotationMatrix(l_cameraTransformComponent->m_globalTransformVector.m_rot);

	auto t = Math::getInvertTranslationMatrix(l_cameraTransformComponent->m_globalTransformVector.m_pos);

	l_PerFrameCB.camera_posWS = l_cameraTransformComponent->m_globalTransformVector.m_pos;

	l_PerFrameCB.v = r * t;

	auto r_prev = l_cameraTransformComponent->m_globalTransformMatrix_prev.m_rotationMat.inverse();
	auto t_prev = l_cameraTransformComponent->m_globalTransformMatrix_prev.m_translationMat.inverse();

	l_PerFrameCB.v_prev = r_prev * t_prev;

	l_PerFrameCB.zNear = l_camera->m_zNear;
	l_PerFrameCB.zFar = l_camera->m_zFar;

	l_PerFrameCB.p_inv = l_p.inverse();
	l_PerFrameCB.v_inv = l_PerFrameCB.v.inverse();
	l_PerFrameCB.viewportSize.x = (float)m_screenResolution.x;
	l_PerFrameCB.viewportSize.y = (float)m_screenResolution.y;
	l_PerFrameCB.minLogLuminance = -10.0f;
	l_PerFrameCB.maxLogLuminance = 16.0f;
	l_PerFrameCB.aperture = l_camera->m_aperture;
	l_PerFrameCB.shutterTime = l_camera->m_shutterTime;
	l_PerFrameCB.ISO = l_camera->m_ISO;

	auto l_sun = g_Engine->getComponentManager()->Get<LightComponent>(0);

	if (l_sun == nullptr)
	{
		return false;
	}

	auto l_sunTransformComponent = g_Engine->getComponentManager()->Find<TransformComponent>(l_sun->m_Owner);

	if (l_sunTransformComponent == nullptr)
	{
		return false;
	}

	l_PerFrameCB.sun_direction = Math::getDirection(Direction::Backward, l_sunTransformComponent->m_globalTransformVector.m_rot);
	l_PerFrameCB.sun_illuminance = l_sun->m_RGBColor * l_sun->m_LuminousFlux;

	static uint32_t currentCascade = 0;
	currentCascade = currentCascade < m_renderingCapability.maxCSMSplits - 1 ? ++currentCascade : 0;
	l_PerFrameCB.activeCascade = currentCascade;
	m_perFrameCB.SetValue(std::move(l_PerFrameCB));

	auto& l_SplitAABB = l_sun->m_SplitAABBWS;
	auto& l_ViewMatrices = l_sun->m_ViewMatrices;
	auto& l_ProjectionMatrices = l_sun->m_ProjectionMatrices;

	auto& l_CSMCBVector = m_CSMCBVector.GetOldValue();
	l_CSMCBVector.clear();

	if (l_SplitAABB.size() > 0 && l_ViewMatrices.size() > 0 && l_ProjectionMatrices.size() > 0)
	{
		for (size_t j = 0; j < l_SplitAABB.size(); j++)
		{
			CSMConstantBuffer l_CSMCB;

			l_CSMCB.p = l_ProjectionMatrices[j];
			l_CSMCB.v = l_ViewMatrices[j];

			l_CSMCB.AABBMax = l_SplitAABB[j].m_boundMax;
			l_CSMCB.AABBMin = l_SplitAABB[j].m_boundMin;

			l_CSMCBVector.emplace_back(l_CSMCB);
		}
	}

	m_CSMCBVector.SetValue(std::move(l_CSMCBVector));

	return true;
}

bool RenderingFrontendNS::updateLightData()
{
	auto& l_PointLightCB = m_pointLightCBVector.GetOldValue();
	auto& l_SphereLightCB = m_sphereLightCBVector.GetOldValue();

	l_PointLightCB.clear();
	l_SphereLightCB.clear();

	auto& l_lightComponents = g_Engine->getComponentManager()->GetAll<LightComponent>();
	auto l_lightComponentCount = l_lightComponents.size();

	if (l_lightComponentCount > 0)
	{
		for (size_t i = 0; i < l_lightComponentCount; i++)
		{
			auto l_transformCompoent = g_Engine->getComponentManager()->Find<TransformComponent>(l_lightComponents[i]->m_Owner);
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

	m_pointLightCBVector.SetValue(std::move(l_PointLightCB));
	m_sphereLightCBVector.SetValue(std::move(l_SphereLightCB));

	return true;
}

bool RenderingFrontendNS::updateMeshData()
{
	auto& l_drawCallInfoVector = m_drawCallInfoVector.GetOldValue();
	auto& l_perObjectCBVector = m_perObjectCBVector.GetOldValue();
	auto& l_materialCBVector = m_materialCBVector.GetOldValue();
	auto& l_animationDrawCallInfoVector = m_animationDrawCallInfoVector.GetOldValue();
	auto& l_animationCBVector = m_animationCBVector.GetOldValue();

	l_drawCallInfoVector.clear();
	l_perObjectCBVector.clear();
	l_materialCBVector.clear();
	l_animationDrawCallInfoVector.clear();
	l_animationCBVector.clear();

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

					l_drawCallInfo.visibilityMask = l_cullingData.visibilityMask;
					l_drawCallInfo.meshUsage = l_cullingData.meshUsage;
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

					if (l_cullingData.meshUsage == MeshUsage::Skeletal)
					{
						auto l_result = m_animationInstanceMap.find(l_cullingData.UUID);
						if (l_result != m_animationInstanceMap.end())
						{
							AnimationDrawCallInfo animationDrawCallInfo;
							animationDrawCallInfo.animationInstance = l_result->second;
							animationDrawCallInfo.drawCallInfo = l_drawCallInfo;

							AnimationConstantBuffer l_animationCB;
							l_animationCB.duration = animationDrawCallInfo.animationInstance.animationData.ADC->m_Duration;
							l_animationCB.numChannels = animationDrawCallInfo.animationInstance.animationData.ADC->m_NumChannels;
							l_animationCB.numTicks = animationDrawCallInfo.animationInstance.animationData.ADC->m_NumTicks;
							l_animationCB.currentTime = animationDrawCallInfo.animationInstance.currentTime / l_animationCB.duration;
							l_animationCB.rootOffsetMatrix = Math::generateIdentityMatrix<float>();

							l_animationCBVector.emplace_back(l_animationCB);

							animationDrawCallInfo.animationConstantBufferIndex = (uint32_t)l_animationCBVector.size();
							l_animationDrawCallInfoVector.emplace_back(animationDrawCallInfo);
						}
					}
					else
					{
						l_drawCallInfoVector.emplace_back(l_drawCallInfo);
					}
					l_perObjectCBVector.emplace_back(l_perObjectCB);
					l_materialCBVector.emplace_back(l_materialCB);
				}
			}
		}
	}

	m_drawCallInfoVector.SetValue(std::move(l_drawCallInfoVector));
	m_perObjectCBVector.SetValue(std::move(l_perObjectCBVector));
	m_materialCBVector.SetValue(std::move(l_materialCBVector));
	m_animationDrawCallInfoVector.SetValue(std::move(l_animationDrawCallInfoVector));
	m_animationCBVector.SetValue(std::move(l_animationCBVector));

	// @TODO: use GPU to do OIT

	return true;
}

bool RenderingFrontendNS::simulateAnimation()
{
	m_currentTime = g_Engine->getTimeSystem()->getCurrentTimeFromEpoch();

	float l_tickTime = float(m_currentTime - m_previousTime) / 1000.0f;

	if (m_animationInstanceMap.size())
	{
		for (auto& i : m_animationInstanceMap)
		{
			if (!i.second.isFinished)
			{
				if (i.second.currentTime < i.second.animationData.ADC->m_Duration)
				{
					i.second.currentTime += l_tickTime / 60.0f;
				}
				else
				{
					if (i.second.isLooping)
					{
						i.second.currentTime -= i.second.animationData.ADC->m_Duration;
					}
					else
					{
						i.second.isFinished = true;
					}
				}
			}
		}

		m_animationInstanceMap.erase_if([](auto it) { return it.second.isFinished; });
	}

	m_previousTime = m_currentTime;

	return true;
}

bool RenderingFrontendNS::updateBillboardPassData()
{
	auto& l_lightComponents = g_Engine->getComponentManager()->GetAll<LightComponent>();

	auto l_totalBillboardDrawCallCount = l_lightComponents.size();

	if (l_totalBillboardDrawCallCount == 0)
	{
		return false;
	}

	auto& l_billboardPassDrawCallInfoVector = m_billboardPassDrawCallInfoVector.GetOldValue();
	auto& l_billboardPassPerObjectCB = m_billboardPassPerObjectCB.GetOldValue();
	auto& l_directionalLightPerObjectCB = m_directionalLightPerObjectCB.GetOldValue();
	auto& l_pointLightPerObjectCB = m_pointLightPerObjectCB.GetOldValue();
	auto& l_sphereLightPerObjectCB = m_sphereLightPerObjectCB.GetOldValue();

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

		auto l_transformCompoent = g_Engine->getComponentManager()->Find<TransformComponent>(i->m_Owner);
		if (l_transformCompoent != nullptr)
		{
			l_meshCB.m = Math::toTranslationMatrix(l_transformCompoent->m_globalTransformVector.m_pos);
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

bool RenderingFrontendNS::updateDebuggerPassData()
{
	// @TODO: Implementation

	return true;
}

bool RenderingFrontendNS::Update()
{
	if (m_ObjectStatus == ObjectStatus::Activated)
	{
		simulateAnimation();

		updatePerFrameConstantBuffer();

		updateLightData();

		// copy culling data pack for local scope
		m_cullingData = g_Engine->getPhysicsSystem()->getCullingData();

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

bool RenderingFrontendNS::Terminate()
{
	m_rayTracer->Terminate();

	m_ObjectStatus = ObjectStatus::Terminated;
	Logger::Log(LogLevel::Success, "RenderingFrontend has been terminated.");
	return true;
}

bool RenderingFrontend::Setup(ISystemConfig* systemConfig)
{	
	g_Engine->getComponentManager()->RegisterType<SkeletonComponent>(2048, this);
	g_Engine->getComponentManager()->RegisterType<AnimationComponent>(16384, this);
	
	return RenderingFrontendNS::Setup(systemConfig);
}

bool RenderingFrontend::Initialize()
{
	return RenderingFrontendNS::Initialize();
}

bool RenderingFrontend::Update()
{
	return RenderingFrontendNS::Update();
}

bool RenderingFrontend::Terminate()
{
	return RenderingFrontendNS::Terminate();
}

ObjectStatus RenderingFrontend::GetStatus()
{
	return RenderingFrontendNS::m_ObjectStatus;
}

bool RenderingFrontend::RunRayTracing()
{
	return RenderingFrontendNS::m_rayTracer->Execute();
}

MeshComponent* RenderingFrontend::AddMeshComponent()
{
	return RenderingFrontendNS::m_renderingServer->AddMeshComponent();
}

TextureComponent* RenderingFrontend::AddTextureComponent()
{
	return RenderingFrontendNS::m_renderingServer->AddTextureComponent();
}

MaterialComponent* RenderingFrontend::AddMaterialComponent()
{
	return RenderingFrontendNS::m_renderingServer->AddMaterialComponent();
}

SkeletonComponent* RenderingFrontend::AddSkeletonComponent()
{
	static std::atomic<uint32_t> skeletonCount = 0;
	auto l_parentEntity = g_Engine->getEntityManager()->Spawn(false, ObjectLifespan::Persistence, ("Skeleton_" + std::to_string(skeletonCount) + "/").c_str());
	auto l_SDC = g_Engine->getComponentManager()->Spawn<SkeletonComponent>(l_parentEntity, false, ObjectLifespan::Persistence);
	l_SDC->m_Owner = l_parentEntity;
	l_SDC->m_Serializable = false;
	l_SDC->m_ObjectStatus = ObjectStatus::Created;
	l_SDC->m_ObjectLifespan = ObjectLifespan::Persistence;
	skeletonCount++;
	return l_SDC;
}

AnimationComponent* RenderingFrontend::AddAnimationComponent()
{
	static std::atomic<uint32_t> animationCount = 0;
	auto l_parentEntity = g_Engine->getEntityManager()->Spawn(false, ObjectLifespan::Persistence, ("Animation_" + std::to_string(animationCount) + "/").c_str());
	auto l_ADC = g_Engine->getComponentManager()->Spawn<AnimationComponent>(l_parentEntity, false, ObjectLifespan::Persistence);
	l_ADC->m_Owner = l_parentEntity;
	l_ADC->m_Serializable = false;
	l_ADC->m_ObjectStatus = ObjectStatus::Created;
	l_ADC->m_ObjectLifespan = ObjectLifespan::Persistence;
	animationCount++;
	return l_ADC;
}

MeshComponent* RenderingFrontend::GetMeshComponent(ProceduralMeshShape shape)
{
	switch (shape)
	{
	case Type::ProceduralMeshShape::Triangle:
		return m_unitTriangleMesh;
		break;
	case Type::ProceduralMeshShape::Square:
		return m_unitSquareMesh;
		break;
	case Type::ProceduralMeshShape::Pentagon:
		return m_unitPentagonMesh;
		break;
	case Type::ProceduralMeshShape::Hexagon:
		return m_unitHexagonMesh;
		break;
	case Type::ProceduralMeshShape::Tetrahedron:
		return m_unitTetrahedronMesh;
		break;
	case Type::ProceduralMeshShape::Cube:
		return m_unitCubeMesh;
		break;
	case Type::ProceduralMeshShape::Octahedron:
		return m_unitOctahedronMesh;
		break;
	case Type::ProceduralMeshShape::Dodecahedron:
		return m_unitDodecahedronMesh;
		break;
	case Type::ProceduralMeshShape::Icosahedron:
		return m_unitIcosahedronMesh;
		break;
	case Type::ProceduralMeshShape::Sphere:
		return m_unitSphereMesh;
		break;
	default:
		Logger::Log(LogLevel::Error, "RenderingFrontend: Invalid ProceduralMeshShape!");
		return nullptr;
		break;
	}
}

TextureComponent* RenderingFrontend::GetTextureComponent(WorldEditorIconType iconType)
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

MaterialComponent* RenderingFrontend::GetDefaultMaterialComponent()
{
	return m_defaultMaterial;
}

bool RenderingFrontend::TransferDataToGPU()
{
	while (m_uninitializedMeshes.size() > 0)
	{
		MeshComponent* l_Mesh;
		m_uninitializedMeshes.tryPop(l_Mesh);

		if (l_Mesh)
		{
			auto l_result = m_renderingServer->InitializeMeshComponent(l_Mesh);
		}
	}

	while (m_uninitializedMaterials.size() > 0)
	{
		MaterialComponent* l_Material;
		m_uninitializedMaterials.tryPop(l_Material);

		if (l_Material)
		{
			auto l_result = m_renderingServer->InitializeMaterialComponent(l_Material);
		}
	}

	while (m_uninitializedAnimations.size() > 0)
	{
		AnimationComponent* l_Animations;
		m_uninitializedAnimations.tryPop(l_Animations);

		if (l_Animations)
		{
			initializeAnimation(l_Animations);
		}
	}

	return true;
}

bool RenderingFrontend::InitializeMeshComponent(MeshComponent* rhs, bool AsyncUploadToGPU)
{
	if (AsyncUploadToGPU)
	{
		m_uninitializedMeshes.push(rhs);
	}
	else
	{
		auto l_MeshComponentInitializeTask = g_Engine->getTaskSystem()->Submit("MeshComponentInitializeTask", 2, nullptr,
			[=]() { m_renderingServer->InitializeMeshComponent(rhs); });
		l_MeshComponentInitializeTask.m_Future->Get();
	}

	return true;
}

bool RenderingFrontend::InitializeMaterialComponent(MaterialComponent* rhs, bool AsyncUploadToGPU)
{
	if (AsyncUploadToGPU)
	{
		m_uninitializedMaterials.push(rhs);
	}
	else
	{
		auto l_MaterialComponentInitializeTask = g_Engine->getTaskSystem()->Submit("MaterialComponentInitializeTask", 2, nullptr,
			[=]() { m_renderingServer->InitializeMaterialComponent(rhs); });
		l_MaterialComponentInitializeTask.m_Future->Get();
	}

	return true;
}

bool RenderingFrontend::InitializeSkeletonComponent(SkeletonComponent* rhs, bool AsyncUploadToGPU)
{
	rhs->m_ObjectStatus = ObjectStatus::Activated;

	return true;
}

bool RenderingFrontend::InitializeAnimationComponent(AnimationComponent* rhs, bool AsyncUploadToGPU)
{
	if (AsyncUploadToGPU)
	{
		m_uninitializedAnimations.push(rhs);
	}
	else
	{
		auto l_AnimationComponentInitializeTask = g_Engine->getTaskSystem()->Submit("AnimationComponentInitializeTask", 2, nullptr,
			[=]() { initializeAnimation(rhs); });
		l_AnimationComponentInitializeTask.m_Future->Get();
	}

	return true;
}

TVec2<uint32_t> RenderingFrontend::GetScreenResolution()
{
	return m_screenResolution;
}

bool RenderingFrontend::SetScreenResolution(TVec2<uint32_t> screenResolution)
{
	m_screenResolution = screenResolution;
	return true;
}

RenderingConfig RenderingFrontend::GetRenderingConfig()
{
	return m_renderingConfig;
}

bool RenderingFrontend::SetRenderingConfig(RenderingConfig renderingConfig)
{
	m_renderingConfig = renderingConfig;
	return true;
}

RenderingCapability RenderingFrontend::GetRenderingCapability()
{
	return m_renderingCapability;
}

RenderPassDesc RenderingFrontend::GetDefaultRenderPassDesc()
{
	return m_DefaultRenderPassDesc;
}

bool RenderingFrontend::PlayAnimation(VisibleComponent* rhs, const char* animationName, bool isLooping)
{
	auto l_animationData = getAnimationData(animationName);

	if (l_animationData.ADC != nullptr)
	{
		AnimationInstance l_instance;

		l_instance.animationData = l_animationData;
		l_instance.currentTime = 0.0f;
		l_instance.isLooping = isLooping;
		l_instance.isFinished = false;

		m_animationInstanceMap.emplace(rhs->m_UUID, l_instance);

		return true;
	}

	return false;
}

bool RenderingFrontend::StopAnimation(VisibleComponent* rhs, const char* animationName)
{
	auto l_result = m_animationInstanceMap.find(rhs->m_UUID);
	if (l_result != m_animationInstanceMap.end())
	{
		m_animationInstanceMap.erase(l_result->first);

		return true;
	}

	return false;
}

const PerFrameConstantBuffer& RenderingFrontend::GetPerFrameConstantBuffer()
{
	return m_perFrameCB.GetNewValue();
}

const std::vector<CSMConstantBuffer>& RenderingFrontend::GetCSMConstantBuffer()
{
	return m_CSMCBVector.GetNewValue();
}

const std::vector<PointLightConstantBuffer>& RenderingFrontend::GetPointLightConstantBuffer()
{
	return m_pointLightCBVector.GetNewValue();
}

const std::vector<SphereLightConstantBuffer>& RenderingFrontend::GetSphereLightConstantBuffer()
{
	return m_sphereLightCBVector.GetNewValue();
}

const std::vector<DrawCallInfo>& RenderingFrontend::GetDrawCallInfo()
{
	return m_drawCallInfoVector.GetNewValue();
}

const std::vector<PerObjectConstantBuffer>& RenderingFrontend::GetPerObjectConstantBuffer()
{
	return m_perObjectCBVector.GetNewValue();
}

const std::vector<MaterialConstantBuffer>& RenderingFrontend::GetMaterialConstantBuffer()
{
	return m_materialCBVector.GetNewValue();
}

const std::vector<AnimationDrawCallInfo>& RenderingFrontend::GetAnimationDrawCallInfo()
{
	return m_animationDrawCallInfoVector.GetNewValue();
}

const std::vector<AnimationConstantBuffer>& RenderingFrontend::GetAnimationConstantBuffer()
{
	return m_animationCBVector.GetNewValue();
}

const std::vector<BillboardPassDrawCallInfo>& RenderingFrontend::GetBillboardPassDrawCallInfo()
{
	return m_billboardPassDrawCallInfoVector.GetNewValue();
}

const std::vector<PerObjectConstantBuffer>& RenderingFrontend::GetBillboardPassPerObjectConstantBuffer()
{
	return m_billboardPassPerObjectCB.GetNewValue();
}

const std::vector<DebugPassDrawCallInfo>& RenderingFrontend::GetDebugPassDrawCallInfo()
{
	return m_debugPassDrawCallInfoVector.GetNewValue();
}

const std::vector<PerObjectConstantBuffer>& RenderingFrontend::GetDebugPassPerObjectConstantBuffer()
{
	return m_debugPassPerObjectCB.GetNewValue();
}