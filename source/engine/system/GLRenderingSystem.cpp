#include "GLRenderingSystem.h"

#include "GLRenderingSystemUtilities.h"

#include "GLEnvironmentRenderingPassUtilities.h"
#include "GLShadowRenderingPassUtilities.h"
#include "GLGeometryRenderingPassUtilities.h"
#include "GLLightRenderingPassUtilities.h"
#include "GLFinalRenderingPassUtilities.h"

#include "../component/FileSystemComponent.h"
#include "../component/GameSystemComponent.h"
#include "../component/WindowSystemComponent.h"
#include "../component/RenderingSystemComponent.h"
#include "../component/GLRenderingSystemComponent.h"

#include "ICoreSystem.h"

extern ICoreSystem* g_pCoreSystem;

INNO_PRIVATE_SCOPE GLRenderingSystemNS
{
	bool setup();
	bool initialize();
	bool update();
	bool terminate();

	bool resize();

	void initializeDefaultAssets();

	float radicalInverse(unsigned int n, unsigned int base);
	void initializeHaltonSampler();

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
		fprintf(stderr, "GL CALLBACK: %s type = 0x%x, severity = 0x%x, message = %s\n",
			(type == GL_DEBUG_TYPE_ERROR ? "** GL ERROR **" : ""),
			type, severity, message);
	}

	void getGLError()
	{
		GLenum err;
		while ((err = glGetError()) != GL_NO_ERROR)
		{
			g_pCoreSystem->getLogSystem()->printLog(LogType::INNO_ERROR, std::to_string(err));
		}
	}

	std::vector<RenderDataPack> m_renderDataPack;
	
	ObjectStatus m_objectStatus = ObjectStatus::SHUTDOWN;
}

INNO_SYSTEM_EXPORT bool GLRenderingSystem::setup()
{
	return GLRenderingSystemNS::setup();
}

INNO_SYSTEM_EXPORT bool GLRenderingSystem::initialize()
{
	return GLRenderingSystemNS::initialize();
}

INNO_SYSTEM_EXPORT bool GLRenderingSystem::update()
{
	return GLRenderingSystemNS::update();
}

INNO_SYSTEM_EXPORT bool GLRenderingSystem::terminate()
{
	return GLRenderingSystemNS::terminate();
}

INNO_SYSTEM_EXPORT ObjectStatus GLRenderingSystem::getStatus()
{
	return GLRenderingSystemNS::m_objectStatus;
}

INNO_SYSTEM_EXPORT bool GLRenderingSystem::resize()
{
	return GLRenderingSystemNS::resize();
}

bool GLRenderingSystemNS::setup()
{
	GLRenderingSystemComponent::get().depthOnlyPassFBDesc.renderBufferAttachmentType = GL_DEPTH_ATTACHMENT;
	GLRenderingSystemComponent::get().depthOnlyPassFBDesc.renderBufferInternalFormat = GL_DEPTH_COMPONENT24;
	GLRenderingSystemComponent::get().depthOnlyPassFBDesc.sizeX = WindowSystemComponent::get().m_windowResolution.x;
	GLRenderingSystemComponent::get().depthOnlyPassFBDesc.sizeY = WindowSystemComponent::get().m_windowResolution.y;
	GLRenderingSystemComponent::get().depthOnlyPassFBDesc.drawColorBuffers = false;

	GLRenderingSystemComponent::get().depthOnlyPassTextureDesc.textureUsageType = TextureUsageType::RENDER_TARGET;
	GLRenderingSystemComponent::get().depthOnlyPassTextureDesc.textureColorComponentsFormat = TextureColorComponentsFormat::DEPTH_COMPONENT;
	GLRenderingSystemComponent::get().depthOnlyPassTextureDesc.texturePixelDataFormat = TexturePixelDataFormat::DEPTH_COMPONENT;
	GLRenderingSystemComponent::get().depthOnlyPassTextureDesc.textureMinFilterMethod = TextureFilterMethod::NEAREST;
	GLRenderingSystemComponent::get().depthOnlyPassTextureDesc.textureMagFilterMethod = TextureFilterMethod::NEAREST;
	GLRenderingSystemComponent::get().depthOnlyPassTextureDesc.textureWrapMethod = TextureWrapMethod::CLAMP_TO_EDGE;
	GLRenderingSystemComponent::get().depthOnlyPassTextureDesc.textureWidth = GLRenderingSystemComponent::get().depthOnlyPassFBDesc.sizeX;
	GLRenderingSystemComponent::get().depthOnlyPassTextureDesc.textureHeight = GLRenderingSystemComponent::get().depthOnlyPassFBDesc.sizeY;
	GLRenderingSystemComponent::get().depthOnlyPassTextureDesc.texturePixelDataType = TexturePixelDataType::FLOAT;

	GLRenderingSystemComponent::get().deferredPassFBDesc.renderBufferAttachmentType = GL_DEPTH_STENCIL_ATTACHMENT;
	GLRenderingSystemComponent::get().deferredPassFBDesc.renderBufferInternalFormat = GL_DEPTH24_STENCIL8;
	GLRenderingSystemComponent::get().deferredPassFBDesc.sizeX = WindowSystemComponent::get().m_windowResolution.x;
	GLRenderingSystemComponent::get().deferredPassFBDesc.sizeY = WindowSystemComponent::get().m_windowResolution.y;
	GLRenderingSystemComponent::get().deferredPassFBDesc.drawColorBuffers = true;

	GLRenderingSystemComponent::get().deferredPassTextureDesc.textureUsageType = TextureUsageType::RENDER_TARGET;
	GLRenderingSystemComponent::get().deferredPassTextureDesc.textureColorComponentsFormat = TextureColorComponentsFormat::RGBA16F;
	GLRenderingSystemComponent::get().deferredPassTextureDesc.texturePixelDataFormat = TexturePixelDataFormat::RGBA;
	GLRenderingSystemComponent::get().deferredPassTextureDesc.textureMinFilterMethod = TextureFilterMethod::NEAREST;
	GLRenderingSystemComponent::get().deferredPassTextureDesc.textureMagFilterMethod = TextureFilterMethod::NEAREST;
	GLRenderingSystemComponent::get().deferredPassTextureDesc.textureWrapMethod = TextureWrapMethod::CLAMP_TO_EDGE;
	GLRenderingSystemComponent::get().deferredPassTextureDesc.textureWidth = GLRenderingSystemComponent::get().deferredPassFBDesc.sizeX;
	GLRenderingSystemComponent::get().deferredPassTextureDesc.textureHeight = GLRenderingSystemComponent::get().deferredPassFBDesc.sizeY;
	GLRenderingSystemComponent::get().deferredPassTextureDesc.texturePixelDataType = TexturePixelDataType::FLOAT;

	RenderingSystemComponent::get().f_reloadShader =
		[&](RenderPassType renderPassType) {
		switch (renderPassType)
		{
		case RenderPassType::OpaquePass:
			GLGeometryRenderingPassUtilities::reloadOpaquePassShaders();
			break;
		case RenderPassType::TransparentPass:
			GLGeometryRenderingPassUtilities::reloadTransparentPassShaders();
			break;
		case RenderPassType::TerrainPass:
			GLGeometryRenderingPassUtilities::reloadTerrainPassShaders();
			break;
		case RenderPassType::LightPass:
			GLLightRenderingPassUtilities::reloadLightPassShaders();
			break;
		case RenderPassType::FinalPass:
			GLFinalRenderingPassUtilities::reloadFinalPassShaders();
			break;
		default: break;
		}
	};

	RenderingSystemComponent::get().f_captureEnvironment =
		[]() {
		GLEnvironmentRenderingPassUtilities::update();
	};

	if (RenderingSystemComponent::get().m_MSAAdepth)
	{
		// antialiasing
		glfwWindowHint(GLFW_SAMPLES, RenderingSystemComponent::get().m_MSAAdepth);
		// MSAA
		glEnable(GL_MULTISAMPLE);
	}

	// Thanks Jobs left us these nice piece of codes
#ifndef INNO_PLATFORM_MAC
	glEnable(GL_DEBUG_OUTPUT);
	glDebugMessageCallback(MessageCallback, 0);
#endif // !INNO_PLATFORM_MAC

	// enable seamless cubemap sampling for lower mip levels in the pre-filter map.
	glEnable(GL_TEXTURE_CUBE_MAP_SEAMLESS);
	glEnable(GL_PROGRAM_POINT_SIZE);
	glEnable(GL_TEXTURE_2D);

	return true;
}

bool GLRenderingSystemNS::initialize()
{
	initializeDefaultAssets();
	initializeHaltonSampler();
	GLEnvironmentRenderingPassUtilities::initialize();
	GLShadowRenderingPassUtilities::initialize();
	GLGeometryRenderingPassUtilities::initialize();
	GLLightRenderingPassUtilities::initialize();
	GLFinalRenderingPassUtilities::initialize();

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

float GLRenderingSystemNS::radicalInverse(unsigned int n, unsigned int base)
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

void GLRenderingSystemNS::initializeHaltonSampler()
{
	// in NDC space
	for (unsigned int i = 0; i < 16; i++)
	{
		RenderingSystemComponent::get().HaltonSampler.emplace_back(vec2(radicalInverse(i, 2) * 2.0f - 1.0f, radicalInverse(i, 3) * 2.0f - 1.0f));
	}
}

bool GLRenderingSystemNS::update()
{
	if (FileSystemComponent::get().m_uninitializedMeshComponents.size() > 0)
	{
		MeshDataComponent* l_meshDataComponent;
		if (FileSystemComponent::get().m_uninitializedMeshComponents.tryPop(l_meshDataComponent))
		{
			generateGLMeshDataComponent(l_meshDataComponent);
		}
	}
	if (FileSystemComponent::get().m_uninitializedTextureComponents.size() > 0)
	{
		TextureDataComponent* l_textureDataComponent;
		if (FileSystemComponent::get().m_uninitializedTextureComponents.tryPop(l_textureDataComponent))
		{
			generateGLTextureDataComponent(l_textureDataComponent);
		}
	}

	prepareRenderingData();

	GLShadowRenderingPassUtilities::update();
	GLGeometryRenderingPassUtilities::update();
	GLLightRenderingPassUtilities::update();
	GLFinalRenderingPassUtilities::update();

	return true;
}

void GLRenderingSystemNS::prepareRenderingData()
{
	prepareGeometryPassData();

	prepareLightPassData();

	prepareBillboardPassData();

	prepareDebuggerPassData();

	// copy for environment capture
	GLRenderingSystemComponent::get().m_opaquePassDataQueue_copy = GLRenderingSystemComponent::get().m_opaquePassDataQueue;
}

bool GLRenderingSystemNS::prepareGeometryPassData()
{
	//UBO
	GLRenderingSystemComponent::get().m_GPassCameraUBOData.m_CamProjJittered = RenderingSystemComponent::get().m_CamProjJittered;
	GLRenderingSystemComponent::get().m_GPassCameraUBOData.m_CamProjOriginal = RenderingSystemComponent::get().m_CamProjOriginal;
	GLRenderingSystemComponent::get().m_GPassCameraUBOData.m_CamRot = RenderingSystemComponent::get().m_CamRot;
	GLRenderingSystemComponent::get().m_GPassCameraUBOData.m_CamTrans = RenderingSystemComponent::get().m_CamTrans;
	GLRenderingSystemComponent::get().m_GPassCameraUBOData.m_CamRot_prev = RenderingSystemComponent::get().m_CamRot_prev;
	GLRenderingSystemComponent::get().m_GPassCameraUBOData.m_CamTrans_prev = RenderingSystemComponent::get().m_CamTrans_prev;

	if (RenderingSystemComponent::get().m_isRenderDataPackValid)
	{
		GLRenderingSystemNS::m_renderDataPack = RenderingSystemComponent::get().m_renderDataPack.getRawData();
	}

	for (auto& i : GLRenderingSystemNS::m_renderDataPack)
	{
		auto l_GLMDC = getGLMeshDataComponent(i.MDC->m_parentEntity);
		if (l_GLMDC)
		{
			if (i.visiblilityType == VisiblilityType::INNO_OPAQUE || i.visiblilityType == VisiblilityType::INNO_EMISSIVE)
			{
				OpaquePassDataPack l_GLRenderDataPack;

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
				GLRenderingSystemComponent::get().m_transparentPassDataQueue.push(l_GLRenderDataPack);
			}
		}
	}

	return true;
}

bool GLRenderingSystemNS::prepareLightPassData()
{
	// point light
	GLRenderingSystemComponent::get().m_PointLightDatas.clear();
	GLRenderingSystemComponent::get().m_PointLightDatas.reserve(GameSystemComponent::get().m_PointLightComponents.size());

	for (auto i : GameSystemComponent::get().m_PointLightComponents)
	{
		PointLightData l_PointLightData;
		l_PointLightData.pos = g_pCoreSystem->getGameSystem()->get<TransformComponent>(i->m_parentEntity)->m_globalTransformVector.m_pos;
		l_PointLightData.luminance = i->m_color * i->m_luminousFlux;
		l_PointLightData.attenuationRadius = i->m_attenuationRadius;
		GLRenderingSystemComponent::get().m_PointLightDatas.emplace_back(l_PointLightData);
	}

	// sphere light
	GLRenderingSystemComponent::get().m_SphereLightDatas.clear();
	GLRenderingSystemComponent::get().m_SphereLightDatas.reserve(GameSystemComponent::get().m_SphereLightComponents.size());

	for (auto i : GameSystemComponent::get().m_SphereLightComponents)
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
	for (auto i : GameSystemComponent::get().m_DirectionalLightComponents)
	{
		BillboardPassDataPack l_GLRenderDataPack;
		l_GLRenderDataPack.globalPos = g_pCoreSystem->getGameSystem()->get<TransformComponent>(i->m_parentEntity)->m_globalTransformVector.m_pos;
		l_GLRenderDataPack.distanceToCamera = (RenderingSystemComponent::get().m_CamGlobalPos - l_GLRenderDataPack.globalPos).length();
		l_GLRenderDataPack.iconType = WorldEditorIconType::DIRECTIONAL_LIGHT;

		GLRenderingSystemComponent::get().m_billboardPassDataQueue.emplace(l_GLRenderDataPack);
	}

	for (auto i : GameSystemComponent::get().m_PointLightComponents)
	{
		BillboardPassDataPack l_GLRenderDataPack;
		l_GLRenderDataPack.globalPos = g_pCoreSystem->getGameSystem()->get<TransformComponent>(i->m_parentEntity)->m_globalTransformVector.m_pos;
		l_GLRenderDataPack.distanceToCamera = (RenderingSystemComponent::get().m_CamGlobalPos - l_GLRenderDataPack.globalPos).length();
		l_GLRenderDataPack.iconType = WorldEditorIconType::POINT_LIGHT;

		GLRenderingSystemComponent::get().m_billboardPassDataQueue.emplace(l_GLRenderDataPack);
	}

	for (auto i : GameSystemComponent::get().m_SphereLightComponents)
	{
		BillboardPassDataPack l_GLRenderDataPack;
		l_GLRenderDataPack.globalPos = g_pCoreSystem->getGameSystem()->get<TransformComponent>(i->m_parentEntity)->m_globalTransformVector.m_pos;
		l_GLRenderDataPack.distanceToCamera = (RenderingSystemComponent::get().m_CamGlobalPos - l_GLRenderDataPack.globalPos).length();
		l_GLRenderDataPack.iconType = WorldEditorIconType::SPHERE_LIGHT;

		GLRenderingSystemComponent::get().m_billboardPassDataQueue.emplace(l_GLRenderDataPack);
	}

	return true;
}

bool GLRenderingSystemNS::prepareDebuggerPassData()
{
	if (RenderingSystemComponent::get().m_selectedVisibleComponent)
	{
		for (auto i : RenderingSystemComponent::get().m_selectedVisibleComponent->m_modelMap)
		{
			DebuggerPassDataPack l_GLRenderDataPack;

			auto l_transformComponent = g_pCoreSystem->getGameSystem()->get<TransformComponent>(RenderingSystemComponent::get().m_selectedVisibleComponent->m_parentEntity);
			auto l_globalTm = l_transformComponent->m_globalTransformMatrix.m_transformationMat;

			l_GLRenderDataPack.m = l_globalTm;
			l_GLRenderDataPack.GLMDC = getGLMeshDataComponent(i.first->m_parentEntity);
			l_GLRenderDataPack.indiceSize = i.first->m_indicesSize;
			l_GLRenderDataPack.meshPrimitiveTopology = i.first->m_meshPrimitiveTopology;

			GLRenderingSystemComponent::get().m_debuggerPassDataQueue.emplace(l_GLRenderDataPack);
		}
	}

	auto l_sphereMDC = g_pCoreSystem->getAssetSystem()->getMeshDataComponent(MeshShapeType::SPHERE);

	if (RenderingSystemComponent::get().m_debugSpheres.size() > 0)
	{
		for (auto i : RenderingSystemComponent::get().m_debugSpheres)
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

	auto l_planeMDC = g_pCoreSystem->getAssetSystem()->getMeshDataComponent(MeshShapeType::QUAD);

	if (RenderingSystemComponent::get().m_debugPlanes.size() > 0)
	{
		for (auto i : RenderingSystemComponent::get().m_debugPlanes)
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
	GLRenderingSystemComponent::get().depthOnlyPassFBDesc.sizeX = WindowSystemComponent::get().m_windowResolution.x;
	GLRenderingSystemComponent::get().depthOnlyPassFBDesc.sizeY = WindowSystemComponent::get().m_windowResolution.y;

	GLRenderingSystemComponent::get().deferredPassFBDesc.sizeX = WindowSystemComponent::get().m_windowResolution.x;
	GLRenderingSystemComponent::get().deferredPassFBDesc.sizeY = WindowSystemComponent::get().m_windowResolution.y;

	GLGeometryRenderingPassUtilities::resize();
	GLLightRenderingPassUtilities::resize();
	GLFinalRenderingPassUtilities::resize();

	return true;
}
