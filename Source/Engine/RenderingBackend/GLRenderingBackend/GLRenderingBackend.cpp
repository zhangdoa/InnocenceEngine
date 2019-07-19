#include "GLRenderingBackend.h"

#include "GLRenderingBackendUtilities.h"

#include "GLBRDFLUTPass.h"
#include "GLEnvironmentCapturePass.h"
#include "GLEnvironmentConvolutionPass.h"
#include "GLEnvironmentPreFilterPass.h"

#include "GLSHPass.h"

#include "GLVXGIPass.h"

#include "GLShadowPass.h"

#include "GLEarlyZPass.h"
#include "GLOpaquePass.h"
#include "GLTerrainPass.h"
#include "GLSSAONoisePass.h"
#include "GLSSAOBlurPass.h"

#include "GLLightCullingPass.h"
#include "GLLightPass.h"

#include "GLSkyPass.h"
#include "GLPreTAAPass.h"
#include "GLTransparentPass.h"

#include "GLTAAPass.h"
#include "GLPostTAAPass.h"
#include "GLMotionBlurPass.h"
#include "GLBloomExtractPass.h"
#include "GLGaussianBlurPass.h"
#include "GLBloomMergePass.h"
#include "GLBillboardPass.h"
#include "GLDebuggerPass.h"
#include "GLFinalBlendPass.h"

#include "../../Component/GLRenderingBackendComponent.h"

#include "../../ModuleManager/IModuleManager.h"

extern IModuleManager* g_pModuleManager;

INNO_PRIVATE_SCOPE GLRenderingBackendNS
{
	void MessageCallback(GLenum source,
		GLenum type,
		GLuint id,
		GLenum severity,
		GLsizei length,
		const GLchar* message,
		const void* userParam)
	{
		if (severity == GL_DEBUG_SEVERITY_HIGH)
		{
			LogType l_logType;
			std::string l_typeStr;
			if (type == GL_DEBUG_TYPE_ERROR)
			{
				l_logType = LogType::INNO_ERROR;
				l_typeStr = "GL_DEBUG_TYPE_ERROR: ID: ";
			}
			else if (type == GL_DEBUG_TYPE_UNDEFINED_BEHAVIOR)
			{
				l_logType = LogType::INNO_ERROR;
				l_typeStr = "GL_DEBUG_TYPE_UNDEFINED_BEHAVIOR: ID: ";
			}
			else if (type == GL_DEBUG_TYPE_DEPRECATED_BEHAVIOR)
			{
				l_logType = LogType::INNO_ERROR;
				l_typeStr = "GL_DEBUG_TYPE_DEPRECATED_BEHAVIOR: ID: ";
			}
			else if (type == GL_DEBUG_TYPE_PERFORMANCE)
			{
				l_logType = LogType::INNO_ERROR;
				l_typeStr = "GL_DEBUG_TYPE_PERFORMANCE: ID: ";
			}
			else if (type == GL_DEBUG_TYPE_PORTABILITY)
			{
				l_logType = LogType::INNO_ERROR;
				l_typeStr = "GL_DEBUG_TYPE_PORTABILITY: ID: ";
			}
			else if (type == GL_DEBUG_TYPE_OTHER)
			{
				l_logType = LogType::INNO_ERROR;
				l_typeStr = "GL_DEBUG_TYPE_OTHER: ID: ";
			}
			else
			{
				l_logType = LogType::INNO_DEV_VERBOSE;
			}

			std::string l_message = message;
			g_pModuleManager->getLogSystem()->printLog(l_logType, "GLRenderingBackend: " + l_typeStr + std::to_string(id) + ": " + l_message);
		}
	}

	ObjectStatus m_objectStatus = ObjectStatus::Terminated;

	std::function<void()> f_sceneLoadingFinishCallback;

	bool m_isBaked = false;
	bool m_visualizeVXGI = false;

	std::function<void()> f_toggleVisualizeVXGI;

	void* m_MeshDataComponentPool;
	void* m_MaterialDataComponentPool;
	void* m_TextureDataComponentPool;

	ThreadSafeQueue<GLMeshDataComponent*> m_uninitializedMeshes;
	ThreadSafeQueue<GLMaterialDataComponent*> m_uninitializedMaterials;

	GLTextureDataComponent* m_iconTemplate_OBJ;
	GLTextureDataComponent* m_iconTemplate_PNG;
	GLTextureDataComponent* m_iconTemplate_SHADER;
	GLTextureDataComponent* m_iconTemplate_UNKNOWN;

	GLTextureDataComponent* m_iconTemplate_DirectionalLight;
	GLTextureDataComponent* m_iconTemplate_PointLight;
	GLTextureDataComponent* m_iconTemplate_SphereLight;

	GLMeshDataComponent* m_unitLineMDC;
	GLMeshDataComponent* m_unitQuadMDC;
	GLMeshDataComponent* m_unitCubeMDC;
	GLMeshDataComponent* m_unitSphereMDC;
	GLMeshDataComponent* m_terrainMDC;

	GLTextureDataComponent* m_basicNormalTDC;
	GLTextureDataComponent* m_basicAlbedoTDC;
	GLTextureDataComponent* m_basicMetallicTDC;
	GLTextureDataComponent* m_basicRoughnessTDC;
	GLTextureDataComponent* m_basicAOTDC;
}

bool GLRenderingBackendNS::setup()
{
	initializeComponentPool();

	f_sceneLoadingFinishCallback = [&]() {
		m_isBaked = false;
	};

	f_toggleVisualizeVXGI = [&]() {
		m_visualizeVXGI = !m_visualizeVXGI;
	};
	g_pModuleManager->getEventSystem()->addButtonStatusCallback(ButtonData{ INNO_KEY_G, ButtonStatus::PRESSED }, &f_toggleVisualizeVXGI);

	g_pModuleManager->getFileSystem()->addSceneLoadingFinishCallback(&f_sceneLoadingFinishCallback);

	auto l_screenResolution = g_pModuleManager->getRenderingFrontend()->getScreenResolution();

	// general render pass desc
	GLRenderingBackendComponent::get().m_deferredRenderPassDesc.RTNumber = 1;
	GLRenderingBackendComponent::get().m_deferredRenderPassDesc.RTDesc.samplerType = TextureSamplerType::SAMPLER_2D;
	GLRenderingBackendComponent::get().m_deferredRenderPassDesc.RTDesc.usageType = TextureUsageType::COLOR_ATTACHMENT;
	GLRenderingBackendComponent::get().m_deferredRenderPassDesc.RTDesc.pixelDataFormat = TexturePixelDataFormat::RGBA;
	GLRenderingBackendComponent::get().m_deferredRenderPassDesc.RTDesc.minFilterMethod = TextureFilterMethod::NEAREST;
	GLRenderingBackendComponent::get().m_deferredRenderPassDesc.RTDesc.magFilterMethod = TextureFilterMethod::NEAREST;
	GLRenderingBackendComponent::get().m_deferredRenderPassDesc.RTDesc.wrapMethod = TextureWrapMethod::CLAMP_TO_EDGE;
	GLRenderingBackendComponent::get().m_deferredRenderPassDesc.RTDesc.width = l_screenResolution.x;
	GLRenderingBackendComponent::get().m_deferredRenderPassDesc.RTDesc.height = l_screenResolution.y;
	GLRenderingBackendComponent::get().m_deferredRenderPassDesc.RTDesc.pixelDataType = TexturePixelDataType::FLOAT16;

	if (g_pModuleManager->getRenderingFrontend()->getRenderingConfig().MSAAdepth)
	{
		// antialiasing
		// MSAA
		glEnable(GL_MULTISAMPLE);
	}

	glEnable(GL_DEBUG_OUTPUT);
	glEnable(GL_DEBUG_OUTPUT_SYNCHRONOUS);
	glDebugMessageCallback(MessageCallback, 0);

	// enable seamless cubemap sampling for lower mip levels in the pre-filter map.
	glEnable(GL_TEXTURE_CUBE_MAP_SEAMLESS);
	glEnable(GL_PROGRAM_POINT_SIZE);

	m_objectStatus = ObjectStatus::Created;
	g_pModuleManager->getLogSystem()->printLog(LogType::INNO_DEV_SUCCESS, "GLRenderingBackend setup finished.");
	return true;
}

bool GLRenderingBackendNS::initialize()
{
	if (GLRenderingBackendNS::m_objectStatus == ObjectStatus::Created)
	{
		auto l_renderingCapability = g_pModuleManager->getRenderingFrontend()->getRenderingCapability();

		m_MeshDataComponentPool = g_pModuleManager->getMemorySystem()->allocateMemoryPool(sizeof(GLMeshDataComponent), l_renderingCapability.maxMeshes);
		m_MaterialDataComponentPool = g_pModuleManager->getMemorySystem()->allocateMemoryPool(sizeof(GLMaterialDataComponent), l_renderingCapability.maxMaterials);
		m_TextureDataComponentPool = g_pModuleManager->getMemorySystem()->allocateMemoryPool(sizeof(GLTextureDataComponent), l_renderingCapability.maxTextures);

		loadDefaultAssets();

		generateGPUBuffers();

		GLBRDFLUTPass::initialize();
		GLEnvironmentCapturePass::initialize();
		GLEnvironmentConvolutionPass::initialize();
		GLEnvironmentPreFilterPass::initialize();
		GLSHPass::initialize();
		GLVXGIPass::initialize();

		GLShadowPass::initialize();

		GLEarlyZPass::initialize();
		GLOpaquePass::initialize();
		GLTerrainPass::initialize();
		GLSSAONoisePass::initialize();
		GLSSAOBlurPass::initialize();

		GLLightCullingPass::initialize();
		GLLightPass::initialize();

		GLSkyPass::initialize();
		GLPreTAAPass::initialize();
		GLTransparentPass::initialize();

		GLTAAPass::initialize();
		GLPostTAAPass::initialize();
		GLMotionBlurPass::initialize();
		GLBloomExtractPass::initialize();
		GLGaussianBlurPass::initialize();
		GLBloomMergePass::initialize();
		GLBillboardPass::initialize();
		GLDebuggerPass::initialize();
		GLFinalBlendPass::initialize();

		GLRenderingBackendNS::m_objectStatus = ObjectStatus::Activated;
		g_pModuleManager->getLogSystem()->printLog(LogType::INNO_DEV_SUCCESS, "GLRenderingBackend has been initialized.");
		return true;
	}
	else
	{
		g_pModuleManager->getLogSystem()->printLog(LogType::INNO_ERROR, "GLRenderingBackend: Object is not created!");
		return false;
	}
}

void  GLRenderingBackendNS::loadDefaultAssets()
{
	auto l_basicNormalTDC = g_pModuleManager->getAssetSystem()->loadTexture("Res//Textures//basic_normal.png", TextureSamplerType::SAMPLER_2D, TextureUsageType::NORMAL);
	auto l_basicAlbedoTDC = g_pModuleManager->getAssetSystem()->loadTexture("Res//Textures//basic_albedo.png", TextureSamplerType::SAMPLER_2D, TextureUsageType::ALBEDO);
	auto l_basicMetallicTDC = g_pModuleManager->getAssetSystem()->loadTexture("Res//Textures//basic_metallic.png", TextureSamplerType::SAMPLER_2D, TextureUsageType::METALLIC);
	auto l_basicRoughnessTDC = g_pModuleManager->getAssetSystem()->loadTexture("Res//Textures//basic_roughness.png", TextureSamplerType::SAMPLER_2D, TextureUsageType::ROUGHNESS);
	auto l_basicAOTDC = g_pModuleManager->getAssetSystem()->loadTexture("Res//Textures//basic_ao.png", TextureSamplerType::SAMPLER_2D, TextureUsageType::AMBIENT_OCCLUSION);

	auto l_iconTemplate_OBJ = g_pModuleManager->getAssetSystem()->loadTexture("Res//Textures//InnoFileTypeIcons_OBJ.png", TextureSamplerType::SAMPLER_2D, TextureUsageType::NORMAL);
	auto l_iconTemplate_PNG = g_pModuleManager->getAssetSystem()->loadTexture("Res//Textures//InnoFileTypeIcons_PNG.png", TextureSamplerType::SAMPLER_2D, TextureUsageType::NORMAL);
	auto l_iconTemplate_SHADER = g_pModuleManager->getAssetSystem()->loadTexture("Res//Textures//InnoFileTypeIcons_SHADER.png", TextureSamplerType::SAMPLER_2D, TextureUsageType::NORMAL);
	auto l_iconTemplate_UNKNOWN = g_pModuleManager->getAssetSystem()->loadTexture("Res//Textures//InnoFileTypeIcons_UNKNOWN.png", TextureSamplerType::SAMPLER_2D, TextureUsageType::NORMAL);

	auto l_iconTemplate_DirectionalLight = g_pModuleManager->getAssetSystem()->loadTexture("Res//Textures//InnoWorldEditorIcons_DirectionalLight.png", TextureSamplerType::SAMPLER_2D, TextureUsageType::NORMAL);
	auto l_iconTemplate_PointLight = g_pModuleManager->getAssetSystem()->loadTexture("Res//Textures//InnoWorldEditorIcons_PointLight.png", TextureSamplerType::SAMPLER_2D, TextureUsageType::NORMAL);
	auto l_iconTemplate_SphereLight = g_pModuleManager->getAssetSystem()->loadTexture("Res//Textures//InnoWorldEditorIcons_SphereLight.png", TextureSamplerType::SAMPLER_2D, TextureUsageType::NORMAL);

	m_basicNormalTDC = reinterpret_cast<GLTextureDataComponent*>(l_basicNormalTDC);
	m_basicAlbedoTDC = reinterpret_cast<GLTextureDataComponent*>(l_basicAlbedoTDC);
	m_basicMetallicTDC = reinterpret_cast<GLTextureDataComponent*>(l_basicMetallicTDC);
	m_basicRoughnessTDC = reinterpret_cast<GLTextureDataComponent*>(l_basicRoughnessTDC);
	m_basicAOTDC = reinterpret_cast<GLTextureDataComponent*>(l_basicAOTDC);

	m_iconTemplate_OBJ = reinterpret_cast<GLTextureDataComponent*>(l_iconTemplate_OBJ);
	m_iconTemplate_PNG = reinterpret_cast<GLTextureDataComponent*>(l_iconTemplate_PNG);
	m_iconTemplate_SHADER = reinterpret_cast<GLTextureDataComponent*>(l_iconTemplate_SHADER);
	m_iconTemplate_UNKNOWN = reinterpret_cast<GLTextureDataComponent*>(l_iconTemplate_UNKNOWN);

	m_iconTemplate_DirectionalLight = reinterpret_cast<GLTextureDataComponent*>(l_iconTemplate_DirectionalLight);
	m_iconTemplate_PointLight = reinterpret_cast<GLTextureDataComponent*>(l_iconTemplate_PointLight);
	m_iconTemplate_SphereLight = reinterpret_cast<GLTextureDataComponent*>(l_iconTemplate_SphereLight);

	m_unitLineMDC = addGLMeshDataComponent();
	g_pModuleManager->getAssetSystem()->addUnitLine(*m_unitLineMDC);
	m_unitLineMDC->m_meshPrimitiveTopology = MeshPrimitiveTopology::TRIANGLE_STRIP;
	m_unitLineMDC->m_meshShapeType = MeshShapeType::LINE;
	m_unitLineMDC->m_objectStatus = ObjectStatus::Created;
	g_pModuleManager->getPhysicsSystem()->generatePhysicsDataComponent(m_unitLineMDC);

	m_unitQuadMDC = addGLMeshDataComponent();
	g_pModuleManager->getAssetSystem()->addUnitQuad(*m_unitQuadMDC);
	m_unitQuadMDC->m_meshPrimitiveTopology = MeshPrimitiveTopology::TRIANGLE;
	m_unitQuadMDC->m_meshShapeType = MeshShapeType::QUAD;
	m_unitQuadMDC->m_objectStatus = ObjectStatus::Created;
	g_pModuleManager->getPhysicsSystem()->generatePhysicsDataComponent(m_unitQuadMDC);

	m_unitCubeMDC = addGLMeshDataComponent();
	g_pModuleManager->getAssetSystem()->addUnitCube(*m_unitCubeMDC);
	m_unitCubeMDC->m_meshPrimitiveTopology = MeshPrimitiveTopology::TRIANGLE;
	m_unitCubeMDC->m_meshShapeType = MeshShapeType::CUBE;
	m_unitCubeMDC->m_objectStatus = ObjectStatus::Created;
	g_pModuleManager->getPhysicsSystem()->generatePhysicsDataComponent(m_unitCubeMDC);

	m_unitSphereMDC = addGLMeshDataComponent();
	g_pModuleManager->getAssetSystem()->addUnitSphere(*m_unitSphereMDC);
	m_unitSphereMDC->m_meshPrimitiveTopology = MeshPrimitiveTopology::TRIANGLE;
	m_unitSphereMDC->m_meshShapeType = MeshShapeType::SPHERE;
	m_unitSphereMDC->m_objectStatus = ObjectStatus::Created;
	g_pModuleManager->getPhysicsSystem()->generatePhysicsDataComponent(m_unitSphereMDC);

	m_terrainMDC = addGLMeshDataComponent();
	g_pModuleManager->getAssetSystem()->addTerrain(*m_terrainMDC);
	m_terrainMDC->m_meshPrimitiveTopology = MeshPrimitiveTopology::TRIANGLE;
	m_terrainMDC->m_objectStatus = ObjectStatus::Created;
	g_pModuleManager->getPhysicsSystem()->generatePhysicsDataComponent(m_terrainMDC);

	initializeGLMeshDataComponent(m_unitLineMDC);
	initializeGLMeshDataComponent(m_unitQuadMDC);
	initializeGLMeshDataComponent(m_unitCubeMDC);
	initializeGLMeshDataComponent(m_unitSphereMDC);
	initializeGLMeshDataComponent(m_terrainMDC);

	initializeGLTextureDataComponent(m_basicNormalTDC);
	initializeGLTextureDataComponent(m_basicAlbedoTDC);
	initializeGLTextureDataComponent(m_basicMetallicTDC);
	initializeGLTextureDataComponent(m_basicRoughnessTDC);
	initializeGLTextureDataComponent(m_basicAOTDC);

	initializeGLTextureDataComponent(m_iconTemplate_OBJ);
	initializeGLTextureDataComponent(m_iconTemplate_PNG);
	initializeGLTextureDataComponent(m_iconTemplate_SHADER);
	initializeGLTextureDataComponent(m_iconTemplate_UNKNOWN);

	initializeGLTextureDataComponent(m_iconTemplate_DirectionalLight);
	initializeGLTextureDataComponent(m_iconTemplate_PointLight);
	initializeGLTextureDataComponent(m_iconTemplate_SphereLight);
}

bool GLRenderingBackendNS::generateGPUBuffers()
{
	auto l_renderingCapability = g_pModuleManager->getRenderingFrontend()->getRenderingCapability();

	GLRenderingBackendComponent::get().m_cameraUBO = generateUBO(sizeof(CameraGPUData), 0, "cameraUBO");
	GLRenderingBackendComponent::get().m_meshUBO = generateUBO(sizeof(MeshGPUData) * l_renderingCapability.maxMeshes, 1, "meshUBO");
	GLRenderingBackendComponent::get().m_materialUBO = generateUBO(sizeof(MaterialGPUData) * l_renderingCapability.maxMaterials, 2, "materialUBO");
	GLRenderingBackendComponent::get().m_sunUBO = generateUBO(sizeof(SunGPUData), 3, "sunUBO");
	GLRenderingBackendComponent::get().m_pointLightUBO = generateUBO(sizeof(PointLightGPUData) * l_renderingCapability.maxPointLights, 4, "pointLightUBO");
	GLRenderingBackendComponent::get().m_sphereLightUBO = generateUBO(sizeof(SphereLightGPUData) * l_renderingCapability.maxSphereLights, 5, "sphereLightUBO");
	GLRenderingBackendComponent::get().m_CSMUBO = generateUBO(sizeof(CSMGPUData) * l_renderingCapability.maxCSMSplits, 6, "CSMUBO");
	GLRenderingBackendComponent::get().m_skyUBO = generateUBO(sizeof(SkyGPUData), 7, "skyUBO");
	GLRenderingBackendComponent::get().m_dispatchParamsUBO = generateUBO(sizeof(DispatchParamsGPUData), 8, "dispatchParamsUBO");

	return true;
}

bool GLRenderingBackendNS::update()
{
	while (GLRenderingBackendNS::m_uninitializedMeshes.size() > 0)
	{
		GLMeshDataComponent* l_MDC;
		GLRenderingBackendNS::m_uninitializedMeshes.tryPop(l_MDC);

		if (l_MDC)
		{
			auto l_result = initializeGLMeshDataComponent(l_MDC);
			if (!l_result)
			{
				g_pModuleManager->getLogSystem()->printLog(LogType::INNO_ERROR, "GLRenderingBackend: can't initialize GLMeshDataComponent for " + std::string(l_MDC->m_parentEntity->m_entityName.c_str()) + "!");
			}
		}
	}
	while (GLRenderingBackendNS::m_uninitializedMaterials.size() > 0)
	{
		GLMaterialDataComponent* l_MDC;
		GLRenderingBackendNS::m_uninitializedMaterials.tryPop(l_MDC);

		if (l_MDC)
		{
			auto l_result = initializeGLMaterialDataComponent(l_MDC);
			if (!l_result)
			{
				g_pModuleManager->getLogSystem()->printLog(LogType::INNO_ERROR, "GLRenderingBackend: can't initialize GLTextureDataComponent for " + std::string(l_MDC->m_parentEntity->m_entityName.c_str()) + "!");
			}
		}
	}

	updateUBO(GLRenderingBackendComponent::get().m_cameraUBO, g_pModuleManager->getRenderingFrontend()->getCameraGPUData());
	updateUBO(GLRenderingBackendComponent::get().m_meshUBO, g_pModuleManager->getRenderingFrontend()->getOpaquePassMeshGPUData());
	updateUBO(GLRenderingBackendComponent::get().m_materialUBO, g_pModuleManager->getRenderingFrontend()->getOpaquePassMaterialGPUData());
	updateUBO(GLRenderingBackendComponent::get().m_sunUBO, g_pModuleManager->getRenderingFrontend()->getSunGPUData());
	updateUBO(GLRenderingBackendComponent::get().m_pointLightUBO, g_pModuleManager->getRenderingFrontend()->getPointLightGPUData());
	updateUBO(GLRenderingBackendComponent::get().m_sphereLightUBO, g_pModuleManager->getRenderingFrontend()->getSphereLightGPUData());
	updateUBO(GLRenderingBackendComponent::get().m_CSMUBO, g_pModuleManager->getRenderingFrontend()->getCSMGPUData());
	updateUBO(GLRenderingBackendComponent::get().m_skyUBO, g_pModuleManager->getRenderingFrontend()->getSkyGPUData());

	return true;
}

bool GLRenderingBackendNS::render()
{
	glFrontFace(GL_CCW);

	if (!m_isBaked)
	{
		GLEnvironmentCapturePass::update();
		GLEnvironmentConvolutionPass::update(GLEnvironmentCapturePass::getGLRPC()->m_GLTDCs[0]);
		GLEnvironmentPreFilterPass::update(GLEnvironmentCapturePass::getGLRPC()->m_GLTDCs[0]);
		GLVXGIPass::update();
		m_isBaked = true;
	}

	auto l_renderingConfig = g_pModuleManager->getRenderingFrontend()->getRenderingConfig();

	GLShadowPass::update();
	//GLGaussianBlurPass::update(GLShadowPass::getGLRPC(0), 0, 2);

	GLEarlyZPass::update();
	GLOpaquePass::update();
	if (l_renderingConfig.drawTerrain)
	{
		GLTerrainPass::update();
	}
	GLSSAONoisePass::update();
	GLSSAOBlurPass::update();

	GLLightCullingPass::update();
	GLLightPass::update();

	if (l_renderingConfig.drawSky)
	{
		GLSkyPass::update();
	}
	else
	{
		cleanRenderBuffers(GLSkyPass::getGLRPC());
	}

	GLPreTAAPass::update();

	updateUBO(GLRenderingBackendComponent::get().m_meshUBO, g_pModuleManager->getRenderingFrontend()->getTransparentPassMeshGPUData());
	updateUBO(GLRenderingBackendComponent::get().m_materialUBO, g_pModuleManager->getRenderingFrontend()->getTransparentPassMaterialGPUData());

	auto l_canvasGLRPC = GLPreTAAPass::getGLRPC();

	if (l_renderingConfig.useTAA)
	{
		GLTAAPass::update(l_canvasGLRPC);
		GLPostTAAPass::update();

		l_canvasGLRPC = GLPostTAAPass::getGLRPC();
	}

	GLTransparentPass::update(l_canvasGLRPC);

	if (l_renderingConfig.useBloom)
	{
		GLBloomExtractPass::update(l_canvasGLRPC);

		GLGaussianBlurPass::update(GLBloomExtractPass::getGLRPC(0), 0, 2);

		GLGaussianBlurPass::update(GLBloomExtractPass::getGLRPC(1), 0, 2);

		GLGaussianBlurPass::update(GLBloomExtractPass::getGLRPC(2), 0, 1);

		GLGaussianBlurPass::update(GLBloomExtractPass::getGLRPC(3), 0, 0);

		GLBloomMergePass::update();

		l_canvasGLRPC = GLBloomMergePass::getGLRPC();
	}
	else
	{
		cleanRenderBuffers(GLBloomExtractPass::getGLRPC(0));
		cleanRenderBuffers(GLBloomExtractPass::getGLRPC(1));
		cleanRenderBuffers(GLBloomExtractPass::getGLRPC(2));
		cleanRenderBuffers(GLBloomExtractPass::getGLRPC(3));

		cleanRenderBuffers(GLGaussianBlurPass::getGLRPC(0));
		cleanRenderBuffers(GLGaussianBlurPass::getGLRPC(1));

		cleanRenderBuffers(GLBloomMergePass::getGLRPC());
	}

	if (l_renderingConfig.useMotionBlur)
	{
		GLMotionBlurPass::update(l_canvasGLRPC);

		l_canvasGLRPC = GLMotionBlurPass::getGLRPC();
	}
	else
	{
		cleanRenderBuffers(GLMotionBlurPass::getGLRPC());
	}

	GLBillboardPass::update();

	if (l_renderingConfig.drawDebugObject)
	{
		GLDebuggerPass::update(l_canvasGLRPC);
	}
	else
	{
		cleanRenderBuffers(GLDebuggerPass::getGLRPC(0));
		cleanRenderBuffers(GLDebuggerPass::getGLRPC(1));
		cleanRenderBuffers(GLDebuggerPass::getGLRPC(2));
		cleanRenderBuffers(GLDebuggerPass::getGLRPC(3));
		cleanRenderBuffers(GLDebuggerPass::getGLRPC(4));
	}

	if (m_visualizeVXGI)
	{
		GLVXGIPass::draw();
		l_canvasGLRPC = GLVXGIPass::getGLRPC();
	}

	GLFinalBlendPass::update(l_canvasGLRPC);

	return true;
}

bool GLRenderingBackendNS::terminate()
{
	m_objectStatus = ObjectStatus::Terminated;
	g_pModuleManager->getLogSystem()->printLog(LogType::INNO_DEV_SUCCESS, "GLRenderingBackend has been terminated.");
	return true;
}

GLMeshDataComponent* GLRenderingBackendNS::addGLMeshDataComponent()
{
	static std::atomic<unsigned int> meshCount = 0;
	meshCount++;
	auto l_rawPtr = g_pModuleManager->getMemorySystem()->spawnObject(m_MeshDataComponentPool, sizeof(GLMeshDataComponent));
	auto l_MDC = new(l_rawPtr)GLMeshDataComponent();
	auto l_parentEntity = g_pModuleManager->getEntityManager()->Spawn(ObjectSource::Runtime, ObjectUsage::Engine, ("Mesh_" + std::to_string(meshCount) + "/").c_str());
	l_MDC->m_parentEntity = l_parentEntity;
	return l_MDC;
}

GLMaterialDataComponent* GLRenderingBackendNS::addGLMaterialDataComponent()
{
	static std::atomic<unsigned int> materialCount = 0;
	materialCount++;
	auto l_rawPtr = g_pModuleManager->getMemorySystem()->spawnObject(m_MaterialDataComponentPool, sizeof(GLMaterialDataComponent));
	auto l_MDC = new(l_rawPtr)GLMaterialDataComponent();
	auto l_parentEntity = g_pModuleManager->getEntityManager()->Spawn(ObjectSource::Runtime, ObjectUsage::Engine, ("Material_" + std::to_string(materialCount) + "/").c_str());
	l_MDC->m_parentEntity = l_parentEntity;
	return l_MDC;
}

GLTextureDataComponent* GLRenderingBackendNS::addGLTextureDataComponent()
{
	static std::atomic<unsigned int> textureCount = 0;
	textureCount++;
	auto l_rawPtr = g_pModuleManager->getMemorySystem()->spawnObject(m_TextureDataComponentPool, sizeof(GLTextureDataComponent));
	auto l_TDC = new(l_rawPtr)GLTextureDataComponent();
	auto l_parentEntity = g_pModuleManager->getEntityManager()->Spawn(ObjectSource::Runtime, ObjectUsage::Engine, ("Texture_" + std::to_string(textureCount) + "/").c_str());
	l_TDC->m_parentEntity = l_parentEntity;
	return l_TDC;
}

GLMeshDataComponent* GLRenderingBackendNS::getGLMeshDataComponent(MeshShapeType meshShapeType)
{
	switch (meshShapeType)
	{
	case MeshShapeType::LINE:
		return GLRenderingBackendNS::m_unitLineMDC; break;
	case MeshShapeType::QUAD:
		return GLRenderingBackendNS::m_unitQuadMDC; break;
	case MeshShapeType::CUBE:
		return GLRenderingBackendNS::m_unitCubeMDC; break;
	case MeshShapeType::SPHERE:
		return GLRenderingBackendNS::m_unitSphereMDC; break;
	case MeshShapeType::TERRAIN:
		return GLRenderingBackendNS::m_terrainMDC; break;
	case MeshShapeType::CUSTOM:
		g_pModuleManager->getLogSystem()->printLog(LogType::INNO_ERROR, "RenderingBackend: wrong MeshShapeType passed to GLRenderingBackend::getMeshDataComponent() !");
		return nullptr; break;
	default:
		return nullptr; break;
	}
}

GLTextureDataComponent * GLRenderingBackendNS::getGLTextureDataComponent(TextureUsageType textureUsageType)
{
	switch (textureUsageType)
	{
	case TextureUsageType::INVISIBLE:
		return nullptr; break;
	case TextureUsageType::NORMAL:
		return GLRenderingBackendNS::m_basicNormalTDC; break;
	case TextureUsageType::ALBEDO:
		return GLRenderingBackendNS::m_basicAlbedoTDC; break;
	case TextureUsageType::METALLIC:
		return GLRenderingBackendNS::m_basicMetallicTDC; break;
	case TextureUsageType::ROUGHNESS:
		return GLRenderingBackendNS::m_basicRoughnessTDC; break;
	case TextureUsageType::AMBIENT_OCCLUSION:
		return GLRenderingBackendNS::m_basicAOTDC; break;
	case TextureUsageType::COLOR_ATTACHMENT:
		return nullptr; break;
	default:
		return nullptr; break;
	}
}

GLTextureDataComponent * GLRenderingBackendNS::getGLTextureDataComponent(FileExplorerIconType iconType)
{
	switch (iconType)
	{
	case FileExplorerIconType::OBJ:
		return GLRenderingBackendNS::m_iconTemplate_OBJ; break;
	case FileExplorerIconType::PNG:
		return GLRenderingBackendNS::m_iconTemplate_PNG; break;
	case FileExplorerIconType::SHADER:
		return GLRenderingBackendNS::m_iconTemplate_SHADER; break;
	case FileExplorerIconType::UNKNOWN:
		return GLRenderingBackendNS::m_iconTemplate_UNKNOWN; break;
	default:
		return nullptr; break;
	}
}

GLTextureDataComponent * GLRenderingBackendNS::getGLTextureDataComponent(WorldEditorIconType iconType)
{
	switch (iconType)
	{
	case WorldEditorIconType::DIRECTIONAL_LIGHT:
		return GLRenderingBackendNS::m_iconTemplate_DirectionalLight; break;
	case WorldEditorIconType::POINT_LIGHT:
		return GLRenderingBackendNS::m_iconTemplate_PointLight; break;
	case WorldEditorIconType::SPHERE_LIGHT:
		return GLRenderingBackendNS::m_iconTemplate_SphereLight; break;
	default:
		return nullptr; break;
	}
}

bool GLRenderingBackendNS::resize()
{
	auto l_screenResolution = g_pModuleManager->getRenderingFrontend()->getScreenResolution();

	GLRenderingBackendComponent::get().m_deferredRenderPassDesc.RTDesc.width = l_screenResolution.x;
	GLRenderingBackendComponent::get().m_deferredRenderPassDesc.RTDesc.height = l_screenResolution.y;

	GLEarlyZPass::resize(l_screenResolution.x, l_screenResolution.y);
	GLOpaquePass::resize(l_screenResolution.x, l_screenResolution.y);
	GLTerrainPass::resize(l_screenResolution.x, l_screenResolution.y);
	GLSSAONoisePass::resize(l_screenResolution.x, l_screenResolution.y);
	GLSSAOBlurPass::resize(l_screenResolution.x, l_screenResolution.y);

	GLLightCullingPass::resize(l_screenResolution.x, l_screenResolution.y);
	GLLightPass::resize(l_screenResolution.x, l_screenResolution.y);

	GLSkyPass::resize(l_screenResolution.x, l_screenResolution.y);
	GLPreTAAPass::resize(l_screenResolution.x, l_screenResolution.y);
	GLTransparentPass::resize(l_screenResolution.x, l_screenResolution.y);

	GLTAAPass::resize(l_screenResolution.x, l_screenResolution.y);
	GLPostTAAPass::resize(l_screenResolution.x, l_screenResolution.y);
	GLMotionBlurPass::resize(l_screenResolution.x, l_screenResolution.y);
	GLBloomExtractPass::resize(l_screenResolution.x, l_screenResolution.y);
	GLGaussianBlurPass::resize(l_screenResolution.x, l_screenResolution.y);
	GLBloomMergePass::resize(l_screenResolution.x, l_screenResolution.y);
	GLBillboardPass::resize(l_screenResolution.x, l_screenResolution.y);
	GLDebuggerPass::resize(l_screenResolution.x, l_screenResolution.y);
	GLFinalBlendPass::resize(l_screenResolution.x, l_screenResolution.y);

	return true;
}

bool GLRenderingBackend::setup()
{
	return GLRenderingBackendNS::setup();
}

bool GLRenderingBackend::initialize()
{
	return GLRenderingBackendNS::initialize();
}

bool GLRenderingBackend::update()
{
	return GLRenderingBackendNS::update();
}

bool GLRenderingBackend::render()
{
	return GLRenderingBackendNS::render();
}

bool GLRenderingBackend::present()
{
	g_pModuleManager->getWindowSystem()->getWindowSurface()->swapBuffer();
	return true;
}

bool GLRenderingBackend::terminate()
{
	return GLRenderingBackendNS::terminate();
}

ObjectStatus GLRenderingBackend::getStatus()
{
	return GLRenderingBackendNS::m_objectStatus;
}

MeshDataComponent * GLRenderingBackend::addMeshDataComponent()
{
	return GLRenderingBackendNS::addGLMeshDataComponent();
}

MaterialDataComponent * GLRenderingBackend::addMaterialDataComponent()
{
	return GLRenderingBackendNS::addGLMaterialDataComponent();
}

TextureDataComponent * GLRenderingBackend::addTextureDataComponent()
{
	return GLRenderingBackendNS::addGLTextureDataComponent();
}

MeshDataComponent * GLRenderingBackend::getMeshDataComponent(MeshShapeType MeshShapeType)
{
	return GLRenderingBackendNS::getGLMeshDataComponent(MeshShapeType);
}

TextureDataComponent * GLRenderingBackend::getTextureDataComponent(TextureUsageType TextureUsageType)
{
	return GLRenderingBackendNS::getGLTextureDataComponent(TextureUsageType);
}

TextureDataComponent * GLRenderingBackend::getTextureDataComponent(FileExplorerIconType iconType)
{
	return GLRenderingBackendNS::getGLTextureDataComponent(iconType);
}

TextureDataComponent * GLRenderingBackend::getTextureDataComponent(WorldEditorIconType iconType)
{
	return GLRenderingBackendNS::getGLTextureDataComponent(iconType);
}

void GLRenderingBackend::registerUninitializedMeshDataComponent(MeshDataComponent * rhs)
{
	GLRenderingBackendNS::m_uninitializedMeshes.push(reinterpret_cast<GLMeshDataComponent*>(rhs));
}

void GLRenderingBackend::registerUninitializedMaterialDataComponent(MaterialDataComponent * rhs)
{
	GLRenderingBackendNS::m_uninitializedMaterials.push(reinterpret_cast<GLMaterialDataComponent*>(rhs));
}

bool GLRenderingBackend::resize()
{
	return GLRenderingBackendNS::resize();
}

bool GLRenderingBackend::reloadShader(RenderPassType renderPassType)
{
	switch (renderPassType)
	{
	case RenderPassType::Shadow:
		GLShadowPass::reloadShader();
		break;
	case RenderPassType::Opaque:
		GLEarlyZPass::reloadShader();
		GLOpaquePass::reloadShader();
		GLSSAONoisePass::reloadShader();
		GLSSAOBlurPass::reloadShader();
		break;
	case RenderPassType::Light:
		GLLightCullingPass::reloadShader();
		GLLightPass::reloadShader();
		break;
	case RenderPassType::Transparent:
		GLTransparentPass::reloadShader();
		break;
	case RenderPassType::Terrain:
		GLTerrainPass::reloadShader();
		break;
	case RenderPassType::PostProcessing:
		GLSkyPass::reloadShader();
		GLPreTAAPass::reloadShader();
		GLTAAPass::reloadShader();
		GLPostTAAPass::reloadShader();
		GLMotionBlurPass::reloadShader();
		GLBloomExtractPass::reloadShader();
		GLGaussianBlurPass::reloadShader();
		GLBloomMergePass::reloadShader();
		GLBillboardPass::reloadShader();
		GLDebuggerPass::reloadShader();
		GLFinalBlendPass::reloadShader();
		break;
	default: break;
	}

	return true;
}

bool GLRenderingBackend::bakeGI()
{
	GLRenderingBackendNS::m_isBaked = false;
	return true;
}