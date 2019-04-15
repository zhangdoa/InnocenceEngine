#include "GLRenderingSystem.h"

#include "GLRenderingSystemUtilities.h"

#include "GLEnvironmentRenderPass.h"
#include "GLShadowRenderPass.h"

#include "GLEarlyZPass.h"
#include "GLOpaquePass.h"
#include "GLSSAONoisePass.h"
#include "GLSSAOBlurPass.h"
#include "GLTerrainPass.h"

#include "GLLightPass.h"

#include "GLSkyPass.h"
#include "GLPreTAAPass.h"
#include "GLTransparentPass.h"

#include "GLTAAPass.h"
#include "GLPostTAAPass.h"
#include "GLMotionBlurPass.h"
#include "GLBloomExtractPass.h"
#include "GLBloomBlurPass.h"
#include "GLBloomMergePass.h"
#include "GLBillboardPass.h"
#include "GLDebuggerPass.h"
#include "GLFinalBlendPass.h"

#include "../../component/GLRenderingSystemComponent.h"

#include "../ICoreSystem.h"

extern ICoreSystem* g_pCoreSystem;

INNO_PRIVATE_SCOPE GLRenderingSystemNS
{
	bool setup(IRenderingFrontendSystem* renderingFrontend);
	bool initialize();
	bool update();
	bool terminate();

	bool resize();

	void initializeDefaultAssets();

	void prepareRenderingData();
	bool prepareGeometryPassData();
	bool prepareLightPassData();
	bool prepareBillboardPassData();
	bool prepareDebuggerPassData();

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
			g_pCoreSystem->getLogSystem()->printLog(l_logType, "GLRenderingSystem: " + l_typeStr + std::to_string(id) + ": " + l_message);
		}
	}

	void getGLError()
	{
		GLenum err;
		while ((err = glGetError()) != GL_NO_ERROR)
		{
			g_pCoreSystem->getLogSystem()->printLog(LogType::INNO_ERROR, std::to_string(err));
		}
	}

	CameraDataPack m_cameraDataPack;
	SunDataPack m_sunDataPack;
	std::vector<MeshDataPack> m_meshDataPack;

	ObjectStatus m_objectStatus = ObjectStatus::SHUTDOWN;

	IRenderingFrontendSystem* m_renderingFrontendSystem;
}

bool GLRenderingSystemNS::setup(IRenderingFrontendSystem* renderingFrontend)
{
	m_renderingFrontendSystem = renderingFrontend;

	initializeComponentPool();

	auto l_screenResolution = m_renderingFrontendSystem->getScreenResolution();

	GLRenderingSystemComponent::get().depthOnlyPassFBDesc.renderBufferAttachmentType = GL_DEPTH_ATTACHMENT;
	GLRenderingSystemComponent::get().depthOnlyPassFBDesc.renderBufferInternalFormat = GL_DEPTH_COMPONENT24;
	GLRenderingSystemComponent::get().depthOnlyPassFBDesc.sizeX = l_screenResolution.x;
	GLRenderingSystemComponent::get().depthOnlyPassFBDesc.sizeY = l_screenResolution.y;
	GLRenderingSystemComponent::get().depthOnlyPassFBDesc.drawColorBuffers = false;

	GLRenderingSystemComponent::get().depthOnlyPassTextureDesc.samplerType = TextureSamplerType::SAMPLER_2D;
	GLRenderingSystemComponent::get().depthOnlyPassTextureDesc.usageType = TextureUsageType::RENDER_TARGET;
	GLRenderingSystemComponent::get().depthOnlyPassTextureDesc.colorComponentsFormat = TextureColorComponentsFormat::DEPTH_COMPONENT;
	GLRenderingSystemComponent::get().depthOnlyPassTextureDesc.pixelDataFormat = TexturePixelDataFormat::DEPTH_COMPONENT;
	GLRenderingSystemComponent::get().depthOnlyPassTextureDesc.minFilterMethod = TextureFilterMethod::NEAREST;
	GLRenderingSystemComponent::get().depthOnlyPassTextureDesc.magFilterMethod = TextureFilterMethod::NEAREST;
	GLRenderingSystemComponent::get().depthOnlyPassTextureDesc.wrapMethod = TextureWrapMethod::CLAMP_TO_EDGE;
	GLRenderingSystemComponent::get().depthOnlyPassTextureDesc.width = GLRenderingSystemComponent::get().depthOnlyPassFBDesc.sizeX;
	GLRenderingSystemComponent::get().depthOnlyPassTextureDesc.height = GLRenderingSystemComponent::get().depthOnlyPassFBDesc.sizeY;
	GLRenderingSystemComponent::get().depthOnlyPassTextureDesc.pixelDataType = TexturePixelDataType::FLOAT;

	GLRenderingSystemComponent::get().deferredPassFBDesc.renderBufferAttachmentType = GL_DEPTH_STENCIL_ATTACHMENT;
	GLRenderingSystemComponent::get().deferredPassFBDesc.renderBufferInternalFormat = GL_DEPTH24_STENCIL8;
	GLRenderingSystemComponent::get().deferredPassFBDesc.sizeX = l_screenResolution.x;
	GLRenderingSystemComponent::get().deferredPassFBDesc.sizeY = l_screenResolution.y;
	GLRenderingSystemComponent::get().deferredPassFBDesc.drawColorBuffers = true;

	GLRenderingSystemComponent::get().deferredPassTextureDesc.samplerType = TextureSamplerType::SAMPLER_2D;
	GLRenderingSystemComponent::get().deferredPassTextureDesc.usageType = TextureUsageType::RENDER_TARGET;
	GLRenderingSystemComponent::get().deferredPassTextureDesc.colorComponentsFormat = TextureColorComponentsFormat::RGBA16F;
	GLRenderingSystemComponent::get().deferredPassTextureDesc.pixelDataFormat = TexturePixelDataFormat::RGBA;
	GLRenderingSystemComponent::get().deferredPassTextureDesc.minFilterMethod = TextureFilterMethod::NEAREST;
	GLRenderingSystemComponent::get().deferredPassTextureDesc.magFilterMethod = TextureFilterMethod::NEAREST;
	GLRenderingSystemComponent::get().deferredPassTextureDesc.wrapMethod = TextureWrapMethod::CLAMP_TO_EDGE;
	GLRenderingSystemComponent::get().deferredPassTextureDesc.width = GLRenderingSystemComponent::get().deferredPassFBDesc.sizeX;
	GLRenderingSystemComponent::get().deferredPassTextureDesc.height = GLRenderingSystemComponent::get().deferredPassFBDesc.sizeY;
	GLRenderingSystemComponent::get().deferredPassTextureDesc.pixelDataType = TexturePixelDataType::FLOAT;

	if (m_renderingFrontendSystem->getRenderingConfig().MSAAdepth)
	{
		// antialiasing
		// MSAA
		glEnable(GL_MULTISAMPLE);
	}

	glEnable(GL_DEBUG_OUTPUT);
	glDebugMessageCallback(MessageCallback, 0);

	// enable seamless cubemap sampling for lower mip levels in the pre-filter map.
	glEnable(GL_TEXTURE_CUBE_MAP_SEAMLESS);
	glEnable(GL_PROGRAM_POINT_SIZE);

	m_objectStatus = ObjectStatus::ALIVE;
	g_pCoreSystem->getLogSystem()->printLog(LogType::INNO_DEV_SUCCESS, "GLRenderingSystem setup finished.");
	return true;
}

bool GLRenderingSystemNS::initialize()
{
	initializeDefaultAssets();

	// UBO
	GLRenderingSystemComponent::get().m_cameraUBO = generateUBO(sizeof(GPassCameraUBOData));

	GLRenderingSystemComponent::get().m_meshUBO = generateUBO(sizeof(GPassMeshUBOData));

	GLRenderingSystemComponent::get().m_textureUBO = generateUBO(sizeof(GPassTextureUBOData));

	GLEnvironmentRenderPass::initialize();
	GLShadowRenderPass::initialize();

	GLEarlyZPass::initialize();
	GLOpaquePass::initialize();
	GLSSAONoisePass::initialize();
	GLSSAOBlurPass::initialize();
	GLTerrainPass::initialize();

	GLLightPass::initialize();

	GLSkyPass::initialize();
	GLPreTAAPass::initialize();
	GLTransparentPass::initialize();

	GLTAAPass::initialize();
	GLPostTAAPass::initialize();
	GLMotionBlurPass::initialize();
	GLBloomExtractPass::initialize();
	GLBloomBlurPass::initialize();
	GLBloomMergePass::initialize();
	GLBillboardPass::initialize();
	GLDebuggerPass::initialize();
	GLFinalBlendPass::initialize();

	return true;
}

void  GLRenderingSystemNS::initializeDefaultAssets()
{
	GLRenderingSystemComponent::get().m_UnitLineGLMDC = generateGLMeshDataComponent(g_pCoreSystem->getAssetSystem()->getMeshDataComponent(MeshShapeType::LINE));
	GLRenderingSystemComponent::get().m_UnitQuadGLMDC = generateGLMeshDataComponent(g_pCoreSystem->getAssetSystem()->getMeshDataComponent(MeshShapeType::QUAD));
	GLRenderingSystemComponent::get().m_UnitCubeGLMDC = generateGLMeshDataComponent(g_pCoreSystem->getAssetSystem()->getMeshDataComponent(MeshShapeType::CUBE));
	GLRenderingSystemComponent::get().m_UnitSphereGLMDC = generateGLMeshDataComponent(g_pCoreSystem->getAssetSystem()->getMeshDataComponent(MeshShapeType::SPHERE));
	GLRenderingSystemComponent::get().m_terrainGLMDC = generateGLMeshDataComponent(g_pCoreSystem->getAssetSystem()->getMeshDataComponent(MeshShapeType::TERRAIN));

	GLRenderingSystemComponent::get().m_basicNormalGLTDC = generateGLTextureDataComponent(g_pCoreSystem->getAssetSystem()->getTextureDataComponent(TextureUsageType::NORMAL));
	GLRenderingSystemComponent::get().m_basicAlbedoGLTDC = generateGLTextureDataComponent(g_pCoreSystem->getAssetSystem()->getTextureDataComponent(TextureUsageType::ALBEDO));
	GLRenderingSystemComponent::get().m_basicMetallicGLTDC = generateGLTextureDataComponent(g_pCoreSystem->getAssetSystem()->getTextureDataComponent(TextureUsageType::METALLIC));
	GLRenderingSystemComponent::get().m_basicRoughnessGLTDC = generateGLTextureDataComponent(g_pCoreSystem->getAssetSystem()->getTextureDataComponent(TextureUsageType::ROUGHNESS));
	GLRenderingSystemComponent::get().m_basicAOGLTDC = generateGLTextureDataComponent(g_pCoreSystem->getAssetSystem()->getTextureDataComponent(TextureUsageType::AMBIENT_OCCLUSION));

	GLRenderingSystemComponent::get().m_iconTemplate_OBJ = generateGLTextureDataComponent(g_pCoreSystem->getAssetSystem()->getTextureDataComponent(FileExplorerIconType::OBJ));
	GLRenderingSystemComponent::get().m_iconTemplate_PNG = generateGLTextureDataComponent(g_pCoreSystem->getAssetSystem()->getTextureDataComponent(FileExplorerIconType::PNG));
	GLRenderingSystemComponent::get().m_iconTemplate_SHADER = generateGLTextureDataComponent(g_pCoreSystem->getAssetSystem()->getTextureDataComponent(FileExplorerIconType::SHADER));
	GLRenderingSystemComponent::get().m_iconTemplate_UNKNOWN = generateGLTextureDataComponent(g_pCoreSystem->getAssetSystem()->getTextureDataComponent(FileExplorerIconType::UNKNOWN));

	GLRenderingSystemComponent::get().m_iconTemplate_DirectionalLight = generateGLTextureDataComponent(g_pCoreSystem->getAssetSystem()->getTextureDataComponent(WorldEditorIconType::DIRECTIONAL_LIGHT));
	GLRenderingSystemComponent::get().m_iconTemplate_PointLight = generateGLTextureDataComponent(g_pCoreSystem->getAssetSystem()->getTextureDataComponent(WorldEditorIconType::POINT_LIGHT));
	GLRenderingSystemComponent::get().m_iconTemplate_SphereLight = generateGLTextureDataComponent(g_pCoreSystem->getAssetSystem()->getTextureDataComponent(WorldEditorIconType::SPHERE_LIGHT));
}

bool GLRenderingSystemNS::update()
{
	// @TODO: too many states
	bool l_meshStatus = true;
	l_meshStatus = l_meshStatus & m_renderingFrontendSystem->anyUninitializedMeshDataComponent();

	bool l_textureStatus = true;
	l_textureStatus = l_textureStatus & m_renderingFrontendSystem->anyUninitializedTextureDataComponent();

	if (l_meshStatus)
	{
		auto l_MDC = m_renderingFrontendSystem->acquireUninitializedMeshDataComponent();
		if (l_MDC)
		{
			auto l_result = generateGLMeshDataComponent(l_MDC);
			if (l_result == nullptr)
			{
				g_pCoreSystem->getLogSystem()->printLog(LogType::INNO_ERROR, "GLRenderingSystem: can't create GLMeshDataComponent for " + l_result->m_parentEntity + "!");
			}
		}
	}
	if (l_textureStatus)
	{
		auto l_TDC = m_renderingFrontendSystem->acquireUninitializedTextureDataComponent();
		if (l_TDC)
		{
			auto l_result = generateGLTextureDataComponent(l_TDC);
			if (l_result == nullptr)
			{
				g_pCoreSystem->getLogSystem()->printLog(LogType::INNO_ERROR, "GLRenderingSystem: can't create GLTextureDataComponent for " + l_result->m_parentEntity + "!");
			}
		}
	}

	prepareRenderingData();

	if (!l_meshStatus && !l_textureStatus)
	{
		GLEnvironmentRenderPass::update();
		//GLEnvironmentRenderPass::draw();
	}

	GLShadowRenderPass::update();

	GLEarlyZPass::update();
	GLOpaquePass::update();
	GLSSAONoisePass::update();
	GLSSAOBlurPass::update();

	GLTerrainPass::update();

	GLLightPass::update();

	auto l_renderingConfig = g_pCoreSystem->getVisionSystem()->getRenderingFrontend()->getRenderingConfig();

	GLSkyPass::update();
	GLPreTAAPass::update();
	GLTransparentPass::update();

	auto l_canvasGLRPC = GLPreTAAPass::getGLRPC();

	if (l_renderingConfig.useBloom)
	{
		GLBloomExtractPass::update(l_canvasGLRPC);

		GLBloomBlurPass::update(GLBloomExtractPass::getGLRPC(0));

		GLBloomBlurPass::update(GLBloomExtractPass::getGLRPC(1));

		GLBloomBlurPass::update(GLBloomExtractPass::getGLRPC(2));

		GLBloomBlurPass::update(GLBloomExtractPass::getGLRPC(3));

		GLBloomMergePass::update();

		l_canvasGLRPC = GLBloomMergePass::getGLRPC();
	}
	else
	{
		cleanRenderBuffers(GLBloomExtractPass::getGLRPC(0));
		cleanRenderBuffers(GLBloomExtractPass::getGLRPC(1));
		cleanRenderBuffers(GLBloomExtractPass::getGLRPC(2));
		cleanRenderBuffers(GLBloomExtractPass::getGLRPC(3));

		cleanRenderBuffers(GLBloomBlurPass::getGLRPC(0));
		cleanRenderBuffers(GLBloomBlurPass::getGLRPC(1));

		cleanRenderBuffers(GLBloomMergePass::getGLRPC());
	}

	if (l_renderingConfig.useTAA)
	{
		GLTAAPass::update(l_canvasGLRPC);
		GLPostTAAPass::update();

		l_canvasGLRPC = GLPostTAAPass::getGLRPC();
	}

	if (l_renderingConfig.useMotionBlur)
	{
		GLMotionBlurPass::update();

		l_canvasGLRPC = GLMotionBlurPass::getGLRPC();
	}

	GLBillboardPass::update();

	if (l_renderingConfig.drawDebugObject)
	{
		GLDebuggerPass::update();
	}
	else
	{
		cleanRenderBuffers(GLDebuggerPass::getGLRPC());
	}
	GLFinalBlendPass::update(l_canvasGLRPC);

	return true;
}

void GLRenderingSystemNS::prepareRenderingData()
{
	// copy camera data pack for local scope
	auto l_cameraDataPack = g_pCoreSystem->getVisionSystem()->getRenderingFrontend()->getCameraDataPack();
	if (l_cameraDataPack.has_value())
	{
		m_cameraDataPack = l_cameraDataPack.value();
	}

	// copy mesh data pack for local scope
	auto l_meshDataPack = m_renderingFrontendSystem->getMeshDataPack();
	if (l_meshDataPack.has_value())
	{
		m_meshDataPack = l_meshDataPack.value();
	}

	prepareGeometryPassData();

	prepareLightPassData();

	prepareBillboardPassData();

	prepareDebuggerPassData();
}

bool GLRenderingSystemNS::prepareGeometryPassData()
{
	GLRenderingSystemComponent::get().m_GPassCameraUBOData.p_original = m_cameraDataPack.p_original;
	GLRenderingSystemComponent::get().m_GPassCameraUBOData.p_jittered = m_cameraDataPack.p_jittered;
	GLRenderingSystemComponent::get().m_GPassCameraUBOData.r = m_cameraDataPack.r;
	GLRenderingSystemComponent::get().m_GPassCameraUBOData.t = m_cameraDataPack.t;
	GLRenderingSystemComponent::get().m_GPassCameraUBOData.r_prev = m_cameraDataPack.r_prev;
	GLRenderingSystemComponent::get().m_GPassCameraUBOData.t_prev = m_cameraDataPack.t_prev;

	std::vector<TransparentPassDataPack> l_sortedTransparentPassDataPack;

	for (auto i : m_meshDataPack)
	{
		auto l_GLMDC = getGLMeshDataComponent(i.MDC->m_parentEntity);
		if (l_GLMDC)
		{
			if (i.visiblilityType == VisiblilityType::INNO_OPAQUE || i.visiblilityType == VisiblilityType::INNO_EMISSIVE)
			{
				OpaquePassDataPack l_GLRenderDataPack;

				l_GLRenderDataPack.UUID = i.m_UUID;
				l_GLRenderDataPack.indiceSize = i.MDC->m_indicesSize;
				l_GLRenderDataPack.meshPrimitiveTopology = i.MDC->m_meshPrimitiveTopology;
				l_GLRenderDataPack.meshShapeType = i.MDC->m_meshShapeType;
				l_GLRenderDataPack.meshUBOData.m = i.m;
				l_GLRenderDataPack.meshUBOData.m_prev = i.m_prev;
				l_GLRenderDataPack.GLMDC = l_GLMDC;

				auto l_material = i.material;
				// any normal?
				auto l_TDC = l_material->m_texturePack.m_normalTDC.second;
				if (l_TDC && l_TDC->m_objectStatus == ObjectStatus::ALIVE)
				{
					l_GLRenderDataPack.normalGLTDC = getGLTextureDataComponent(l_TDC->m_parentEntity);
				}
				else
				{
					l_GLRenderDataPack.textureUBOData.useNormalTexture = false;
				}
				// any albedo?
				l_TDC = l_material->m_texturePack.m_albedoTDC.second;
				if (l_TDC && l_TDC->m_objectStatus == ObjectStatus::ALIVE)
				{
					l_GLRenderDataPack.albedoGLTDC = getGLTextureDataComponent(l_TDC->m_parentEntity);
				}
				else
				{
					l_GLRenderDataPack.textureUBOData.useAlbedoTexture = false;
				}
				// any metallic?
				l_TDC = l_material->m_texturePack.m_metallicTDC.second;
				if (l_TDC && l_TDC->m_objectStatus == ObjectStatus::ALIVE)
				{
					l_GLRenderDataPack.metallicGLTDC = getGLTextureDataComponent(l_TDC->m_parentEntity);
				}
				else
				{
					l_GLRenderDataPack.textureUBOData.useMetallicTexture = false;
				}
				// any roughness?
				l_TDC = l_material->m_texturePack.m_roughnessTDC.second;
				if (l_TDC && l_TDC->m_objectStatus == ObjectStatus::ALIVE)
				{
					l_GLRenderDataPack.roughnessGLTDC = getGLTextureDataComponent(l_TDC->m_parentEntity);
				}
				else
				{
					l_GLRenderDataPack.textureUBOData.useRoughnessTexture = false;
				}
				// any ao?
				l_TDC = l_material->m_texturePack.m_roughnessTDC.second;
				if (l_TDC && l_TDC->m_objectStatus == ObjectStatus::ALIVE)
				{
					l_GLRenderDataPack.AOGLTDC = getGLTextureDataComponent(l_TDC->m_parentEntity);
				}
				else
				{
					l_GLRenderDataPack.textureUBOData.useAOTexture = false;
				}

				l_GLRenderDataPack.textureUBOData.albedo = vec4(
					l_material->m_meshCustomMaterial.albedo_r,
					l_material->m_meshCustomMaterial.albedo_g,
					l_material->m_meshCustomMaterial.albedo_b,
					1.0f
				);
				l_GLRenderDataPack.textureUBOData.MRA = vec4(
					l_material->m_meshCustomMaterial.metallic,
					l_material->m_meshCustomMaterial.roughness,
					l_material->m_meshCustomMaterial.ao,
					1.0f
				);

				l_GLRenderDataPack.visiblilityType = i.visiblilityType;

				GLRenderingSystemComponent::get().m_opaquePassDataQueue.push(l_GLRenderDataPack);
			}
			else if (i.visiblilityType == VisiblilityType::INNO_TRANSPARENT)
			{
				TransparentPassDataPack l_GLRenderDataPack;

				l_GLRenderDataPack.indiceSize = i.MDC->m_indicesSize;
				l_GLRenderDataPack.meshPrimitiveTopology = i.MDC->m_meshPrimitiveTopology;
				l_GLRenderDataPack.meshShapeType = i.MDC->m_meshShapeType;
				l_GLRenderDataPack.meshUBOData.m = i.m;
				l_GLRenderDataPack.meshUBOData.m_prev = i.m_prev;
				l_GLRenderDataPack.GLMDC = l_GLMDC;

				auto l_material = i.material;

				l_GLRenderDataPack.meshCustomMaterial = l_material->m_meshCustomMaterial;
				l_GLRenderDataPack.visiblilityType = i.visiblilityType;

				l_sortedTransparentPassDataPack.emplace_back(l_GLRenderDataPack);
			}
		}
	}

	// @TODO: use GPU to do OIT
	std::sort(l_sortedTransparentPassDataPack.begin(), l_sortedTransparentPassDataPack.end(), [](TransparentPassDataPack a, TransparentPassDataPack b) {
		auto m_a_InViewSpace = m_cameraDataPack.t * m_cameraDataPack.r * a.meshUBOData.m;
		auto m_b_InViewSpace = m_cameraDataPack.t * m_cameraDataPack.r * b.meshUBOData.m;
		return m_a_InViewSpace.m23 < m_b_InViewSpace.m23;
	});

	for (auto i : l_sortedTransparentPassDataPack)
	{
		GLRenderingSystemComponent::get().m_transparentPassDataQueue.push(i);
	}
	return true;
}

bool GLRenderingSystemNS::prepareLightPassData()
{
	// point light
	GLRenderingSystemComponent::get().m_PointLightDatas.clear();
	GLRenderingSystemComponent::get().m_PointLightDatas.reserve(g_pCoreSystem->getGameSystem()->get<PointLightComponent>().size());

	for (auto i : g_pCoreSystem->getGameSystem()->get<PointLightComponent>())
	{
		PointLightData l_PointLightData;
		l_PointLightData.pos = g_pCoreSystem->getGameSystem()->get<TransformComponent>(i->m_parentEntity)->m_globalTransformVector.m_pos;
		l_PointLightData.luminance = i->m_color * i->m_luminousFlux;
		l_PointLightData.attenuationRadius = i->m_attenuationRadius;
		GLRenderingSystemComponent::get().m_PointLightDatas.emplace_back(l_PointLightData);
	}

	// sphere light
	GLRenderingSystemComponent::get().m_SphereLightDatas.clear();
	GLRenderingSystemComponent::get().m_SphereLightDatas.reserve(g_pCoreSystem->getGameSystem()->get<SphereLightComponent>().size());

	for (auto i : g_pCoreSystem->getGameSystem()->get<SphereLightComponent>())
	{
		SphereLightData l_SphereLightData;
		l_SphereLightData.pos = g_pCoreSystem->getGameSystem()->get<TransformComponent>(i->m_parentEntity)->m_globalTransformVector.m_pos;
		l_SphereLightData.luminance = i->m_color * i->m_luminousFlux;;
		l_SphereLightData.sphereRadius = i->m_sphereRadius;
		GLRenderingSystemComponent::get().m_SphereLightDatas.emplace_back(l_SphereLightData);
	}

	return true;
}

bool GLRenderingSystemNS::prepareBillboardPassData()
{
	auto l_cameraDataPack = m_renderingFrontendSystem->getCameraDataPack();

	for (auto i : g_pCoreSystem->getGameSystem()->get<DirectionalLightComponent>())
	{
		BillboardPassDataPack l_GLRenderDataPack;
		l_GLRenderDataPack.globalPos = g_pCoreSystem->getGameSystem()->get<TransformComponent>(i->m_parentEntity)->m_globalTransformVector.m_pos;
		l_GLRenderDataPack.distanceToCamera = (m_cameraDataPack.globalPos - l_GLRenderDataPack.globalPos).length();
		l_GLRenderDataPack.iconType = WorldEditorIconType::DIRECTIONAL_LIGHT;

		GLRenderingSystemComponent::get().m_billboardPassDataQueue.emplace(l_GLRenderDataPack);
	}

	for (auto i : g_pCoreSystem->getGameSystem()->get<PointLightComponent>())
	{
		BillboardPassDataPack l_GLRenderDataPack;
		l_GLRenderDataPack.globalPos = g_pCoreSystem->getGameSystem()->get<TransformComponent>(i->m_parentEntity)->m_globalTransformVector.m_pos;
		l_GLRenderDataPack.distanceToCamera = (m_cameraDataPack.globalPos - l_GLRenderDataPack.globalPos).length();
		l_GLRenderDataPack.iconType = WorldEditorIconType::POINT_LIGHT;

		GLRenderingSystemComponent::get().m_billboardPassDataQueue.emplace(l_GLRenderDataPack);
	}

	for (auto i : g_pCoreSystem->getGameSystem()->get<SphereLightComponent>())
	{
		BillboardPassDataPack l_GLRenderDataPack;
		l_GLRenderDataPack.globalPos = g_pCoreSystem->getGameSystem()->get<TransformComponent>(i->m_parentEntity)->m_globalTransformVector.m_pos;
		l_GLRenderDataPack.distanceToCamera = (m_cameraDataPack.globalPos - l_GLRenderDataPack.globalPos).length();
		l_GLRenderDataPack.iconType = WorldEditorIconType::SPHERE_LIGHT;

		GLRenderingSystemComponent::get().m_billboardPassDataQueue.emplace(l_GLRenderDataPack);
	}

	return true;
}

bool GLRenderingSystemNS::prepareDebuggerPassData()
{
	//if (RenderingSystemComponent::get().m_selectedVisibleComponent)
	//{
	//	for (auto i : RenderingSystemComponent::get().m_selectedVisibleComponent->m_modelMap)
	//	{
	//		DebuggerPassDataPack l_GLRenderDataPack;

	//		auto l_transformComponent = g_pCoreSystem->getGameSystem()->get<TransformComponent>(RenderingSystemComponent::get().m_selectedVisibleComponent->m_parentEntity);
	//		auto l_globalTm = l_transformComponent->m_globalTransformMatrix.m_transformationMat;

	//		l_GLRenderDataPack.m = l_globalTm;
	//		l_GLRenderDataPack.GLMDC = getGLMeshDataComponent(i.first->m_parentEntity);
	//		l_GLRenderDataPack.indiceSize = i.first->m_indicesSize;
	//		l_GLRenderDataPack.meshPrimitiveTopology = i.first->m_meshPrimitiveTopology;

	//		GLRenderingSystemComponent::get().m_debuggerPassDataQueue.emplace(l_GLRenderDataPack);
	//	}
	//}

	auto l_planeMDC = g_pCoreSystem->getAssetSystem()->getMeshDataComponent(MeshShapeType::QUAD);
	auto l_debugPlanes = m_renderingFrontendSystem->getDebugPlane();

	if (l_debugPlanes.size() > 0)
	{
		for (auto i : l_debugPlanes)
		{
			DebuggerPassDataPack l_GLRenderDataPack;

			auto l_p = i.m_normal * i.m_distance;
			auto l_t = InnoMath::toTranslationMatrix(l_p);
			//@TODO: forward vector to rot

			auto l_r = InnoMath::toRotationMatrix(i.m_normal);
			auto l_m = l_t * l_r;

			l_GLRenderDataPack.m = l_m;
			l_GLRenderDataPack.GLMDC = GLRenderingSystemComponent::get().m_UnitQuadGLMDC;
			l_GLRenderDataPack.indiceSize = l_planeMDC->m_indicesSize;
			l_GLRenderDataPack.meshPrimitiveTopology = l_planeMDC->m_meshPrimitiveTopology;

			GLRenderingSystemComponent::get().m_debuggerPassDataQueue.emplace(l_GLRenderDataPack);
		}
	}

	auto l_sphereMDC = g_pCoreSystem->getAssetSystem()->getMeshDataComponent(MeshShapeType::SPHERE);
	auto l_debugSpheres = m_renderingFrontendSystem->getDebugSphere();

	if (l_debugSpheres.size() > 0)
	{
		for (auto i : l_debugSpheres)
		{
			DebuggerPassDataPack l_GLRenderDataPack;

			auto l_t = InnoMath::toTranslationMatrix(i.m_center);
			auto l_s = InnoMath::toScaleMatrix(vec4(i.m_radius, i.m_radius, i.m_radius, 1.0f));
			auto l_m = l_t * l_s;

			l_GLRenderDataPack.m = l_m;
			l_GLRenderDataPack.GLMDC = GLRenderingSystemComponent::get().m_UnitSphereGLMDC;
			l_GLRenderDataPack.indiceSize = l_sphereMDC->m_indicesSize;
			l_GLRenderDataPack.meshPrimitiveTopology = l_sphereMDC->m_meshPrimitiveTopology;

			GLRenderingSystemComponent::get().m_debuggerPassDataQueue.emplace(l_GLRenderDataPack);
		}
	}

	return true;
}

bool GLRenderingSystemNS::terminate()
{
	m_objectStatus = ObjectStatus::SHUTDOWN;
	g_pCoreSystem->getLogSystem()->printLog(LogType::INNO_DEV_SUCCESS, "GLRenderingSystem has been terminated.");
	return true;
}

bool GLRenderingSystemNS::resize()
{
	auto l_screenResolution = m_renderingFrontendSystem->getScreenResolution();

	GLRenderingSystemComponent::get().depthOnlyPassFBDesc.sizeX = l_screenResolution.x;
	GLRenderingSystemComponent::get().depthOnlyPassFBDesc.sizeY = l_screenResolution.y;

	GLRenderingSystemComponent::get().deferredPassFBDesc.sizeX = l_screenResolution.x;
	GLRenderingSystemComponent::get().deferredPassFBDesc.sizeY = l_screenResolution.y;

	GLEarlyZPass::resize(l_screenResolution.x, l_screenResolution.y);
	GLOpaquePass::resize(l_screenResolution.x, l_screenResolution.y);
	GLSSAONoisePass::resize(l_screenResolution.x, l_screenResolution.y);
	GLSSAOBlurPass::resize(l_screenResolution.x, l_screenResolution.y);

	GLTerrainPass::resize(l_screenResolution.x, l_screenResolution.y);

	GLLightPass::resize(l_screenResolution.x, l_screenResolution.y);

	GLSkyPass::resize(l_screenResolution.x, l_screenResolution.y);
	GLPreTAAPass::resize(l_screenResolution.x, l_screenResolution.y);
	GLTransparentPass::resize(l_screenResolution.x, l_screenResolution.y);

	GLTAAPass::resize(l_screenResolution.x, l_screenResolution.y);
	GLPostTAAPass::resize(l_screenResolution.x, l_screenResolution.y);
	GLMotionBlurPass::resize(l_screenResolution.x, l_screenResolution.y);
	GLBloomExtractPass::resize(l_screenResolution.x, l_screenResolution.y);
	GLBloomBlurPass::resize(l_screenResolution.x, l_screenResolution.y);
	GLBloomMergePass::resize(l_screenResolution.x, l_screenResolution.y);
	GLBillboardPass::resize(l_screenResolution.x, l_screenResolution.y);
	GLDebuggerPass::resize(l_screenResolution.x, l_screenResolution.y);
	GLFinalBlendPass::resize(l_screenResolution.x, l_screenResolution.y);

	return true;
}

bool GLRenderingSystem::setup(IRenderingFrontendSystem* renderingFrontend)
{
	return GLRenderingSystemNS::setup(renderingFrontend);
}

bool GLRenderingSystem::initialize()
{
	return GLRenderingSystemNS::initialize();
}

bool GLRenderingSystem::update()
{
	return GLRenderingSystemNS::update();
}

bool GLRenderingSystem::terminate()
{
	return GLRenderingSystemNS::terminate();
}

ObjectStatus GLRenderingSystem::getStatus()
{
	return GLRenderingSystemNS::m_objectStatus;
}

bool GLRenderingSystem::resize()
{
	return GLRenderingSystemNS::resize();
}

bool GLRenderingSystem::reloadShader(RenderPassType renderPassType)
{
	switch (renderPassType)
	{
	case RenderPassType::OpaquePass:
		GLOpaquePass::reloadShader();
		break;
	case RenderPassType::TransparentPass:
		GLTransparentPass::reloadShader();
		break;
	case RenderPassType::TerrainPass:
		GLTerrainPass::reloadShader();
		break;
	case RenderPassType::LightPass:
		GLLightPass::reloadShader();
		break;
	case RenderPassType::FinalPass:
		GLSkyPass::reloadShader();
		GLPreTAAPass::reloadShader();
		GLTAAPass::reloadShader();
		GLPostTAAPass::reloadShader();
		GLMotionBlurPass::reloadShader();
		GLBloomExtractPass::reloadShader();
		GLBloomBlurPass::reloadShader();
		GLBloomMergePass::reloadShader();
		GLBillboardPass::reloadShader();
		GLDebuggerPass::reloadShader();
		GLFinalBlendPass::reloadShader();
		break;
	default: break;
	}

	return true;
}

bool GLRenderingSystem::bakeGI()
{
	return true;
}