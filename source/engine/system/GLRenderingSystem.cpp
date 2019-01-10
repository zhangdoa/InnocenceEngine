#include "GLRenderingSystem.h"

#include "GLRenderingSystemUtilities.h"

#include "GLEnvironmentRenderingPassUtilities.h"
#include "GLShadowRenderingPassUtilities.h"
#include "GLGeometryRenderingPassUtilities.h"
#include "GLLightRenderingPassUtilities.h"
#include "GLFinalRenderingPassUtilities.h"

#include "../component/AssetSystemComponent.h"
#include "../component/GameSystemComponent.h"
#include "../component/WindowSystemComponent.h"
#include "../component/RenderingSystemComponent.h"
#include "../component/GLRenderingSystemComponent.h"
#include "../component/PhysicsSystemComponent.h"

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

	void GLAPIENTRY
		MessageCallback(GLenum source,
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
	GLRenderingSystemComponent::get().deferredPassFBDesc.renderBufferAttachmentType = GL_DEPTH_STENCIL_ATTACHMENT;
	GLRenderingSystemComponent::get().deferredPassFBDesc.renderBufferInternalFormat = GL_DEPTH24_STENCIL8;
	GLRenderingSystemComponent::get().deferredPassFBDesc.sizeX = WindowSystemComponent::get().m_windowResolution.x;
	GLRenderingSystemComponent::get().deferredPassFBDesc.sizeY = WindowSystemComponent::get().m_windowResolution.y;

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
#ifndef INNO_PLATFORM_MACOS
	glEnable(GL_DEBUG_OUTPUT);
	glDebugMessageCallback(MessageCallback, 0);
#endif // !INNO_PLATFORM_MACOS

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
	if (AssetSystemComponent::get().m_uninitializedMeshComponents.size() > 0)
	{
		MeshDataComponent* l_meshDataComponent;
		if (AssetSystemComponent::get().m_uninitializedMeshComponents.tryPop(l_meshDataComponent))
		{
			generateGLMeshDataComponent(l_meshDataComponent);
		}
	}
	if (AssetSystemComponent::get().m_uninitializedTextureComponents.size() > 0)
	{
		TextureDataComponent* l_textureDataComponent;
		if (AssetSystemComponent::get().m_uninitializedTextureComponents.tryPop(l_textureDataComponent))
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
	// main camera
	auto l_mainCamera = GameSystemComponent::get().m_CameraComponents[0];
	auto l_mainCameraTransformComponent = g_pCoreSystem->getGameSystem()->get<TransformComponent>(l_mainCamera->m_parentEntity);

	auto l_p = l_mainCamera->m_projectionMatrix;
	auto l_r =
		InnoMath::getInvertRotationMatrix(
			l_mainCameraTransformComponent->m_globalTransformVector.m_rot
		);
	auto l_t =
		InnoMath::getInvertTranslationMatrix(
			l_mainCameraTransformComponent->m_globalTransformVector.m_pos
		);
	auto r_prev = l_mainCameraTransformComponent->m_globalTransformMatrix_prev.m_rotationMat.inverse();
	auto t_prev = l_mainCameraTransformComponent->m_globalTransformMatrix_prev.m_translationMat.inverse();

	GLRenderingSystemComponent::get().m_CamProjOriginal = l_p;
	GLRenderingSystemComponent::get().m_CamProjJittered = l_p;

	if (RenderingSystemComponent::get().m_useTAA)
	{
		//TAA jitter for projection matrix
		auto& l_currentHaltonStep = RenderingSystemComponent::get().currentHaltonStep;
		if (l_currentHaltonStep >= 16)
		{
			l_currentHaltonStep = 0;
		}
		GLRenderingSystemComponent::get().m_CamProjJittered.m02 = RenderingSystemComponent::get().HaltonSampler[l_currentHaltonStep].x / GLRenderingSystemComponent::get().deferredPassFBDesc.sizeX;
		GLRenderingSystemComponent::get().m_CamProjJittered.m12 = RenderingSystemComponent::get().HaltonSampler[l_currentHaltonStep].y / GLRenderingSystemComponent::get().deferredPassFBDesc.sizeY;
		l_currentHaltonStep += 1;
	}

	GLRenderingSystemComponent::get().m_CamRot = l_r;
	GLRenderingSystemComponent::get().m_CamTrans = l_t;
	GLRenderingSystemComponent::get().m_CamRot_prev = r_prev;
	GLRenderingSystemComponent::get().m_CamTrans_prev = t_prev;
	GLRenderingSystemComponent::get().m_CamGlobalPos = l_mainCameraTransformComponent->m_globalTransformVector.m_pos;

	prepareGeometryPassData();

	prepareLightPassData();

	prepareBillboardPassData();

	// copy for environment capture
	GLRenderingSystemComponent::get().m_GPassOpaqueRenderDataQueue_copy = GLRenderingSystemComponent::get().m_GPassOpaqueRenderDataQueue;
}


bool GLRenderingSystemNS::prepareGeometryPassData()
{
	//UBO
	GLRenderingSystemComponent::get().m_GPassCameraUBOData.m_CamProjJittered = GLRenderingSystemComponent::get().m_CamProjJittered;
	GLRenderingSystemComponent::get().m_GPassCameraUBOData.m_CamProjOriginal = GLRenderingSystemComponent::get().m_CamProjOriginal;
	GLRenderingSystemComponent::get().m_GPassCameraUBOData.m_CamRot = GLRenderingSystemComponent::get().m_CamRot;
	GLRenderingSystemComponent::get().m_GPassCameraUBOData.m_CamTrans = GLRenderingSystemComponent::get().m_CamTrans;
	GLRenderingSystemComponent::get().m_GPassCameraUBOData.m_CamRot_prev = GLRenderingSystemComponent::get().m_CamRot_prev;
	GLRenderingSystemComponent::get().m_GPassCameraUBOData.m_CamTrans_prev = GLRenderingSystemComponent::get().m_CamTrans_prev;

	for (auto& l_renderDataPack : RenderingSystemComponent::get().m_renderDataPack)
	{
		auto l_GLMDC = getGLMeshDataComponent(l_renderDataPack.MDC->m_parentEntity);
		if (l_GLMDC)
		{
			if (l_renderDataPack.visiblilityType == VisiblilityType::INNO_OPAQUE || l_renderDataPack.visiblilityType == VisiblilityType::INNO_EMISSIVE)
			{
				GPassOpaqueRenderDataPack l_GLRenderDataPack;

				l_GLRenderDataPack.indiceSize = l_renderDataPack.MDC->m_indicesSize;
				l_GLRenderDataPack.m_meshDrawMethod = l_renderDataPack.MDC->m_meshDrawMethod;
				l_GLRenderDataPack.m_GPassMeshUBOData.m = l_renderDataPack.m;
				l_GLRenderDataPack.m_GPassMeshUBOData.m_prev = l_renderDataPack.m_prev;
				l_GLRenderDataPack.GLMDC = l_GLMDC;

				auto l_material = l_renderDataPack.Material;
				// any normal?
				auto l_TDC = l_material->m_texturePack.m_normalTDC.second;
				if (l_TDC && l_TDC->m_objectStatus == ObjectStatus::ALIVE)
				{
					l_GLRenderDataPack.m_basicNormalGLTDC = getGLTextureDataComponent(l_TDC->m_parentEntity);
				}
				else
				{
					l_GLRenderDataPack.m_GPassTextureUBOData.useNormalTexture = false;
				}
				// any albedo?
				l_TDC = l_material->m_texturePack.m_albedoTDC.second;
				if (l_TDC && l_TDC->m_objectStatus == ObjectStatus::ALIVE)
				{
					l_GLRenderDataPack.m_basicAlbedoGLTDC = getGLTextureDataComponent(l_TDC->m_parentEntity);
				}
				else
				{
					l_GLRenderDataPack.m_GPassTextureUBOData.useAlbedoTexture = false;
				}
				// any metallic?
				l_TDC = l_material->m_texturePack.m_metallicTDC.second;
				if (l_TDC && l_TDC->m_objectStatus == ObjectStatus::ALIVE)
				{
					l_GLRenderDataPack.m_basicMetallicGLTDC = getGLTextureDataComponent(l_TDC->m_parentEntity);
				}
				else
				{
					l_GLRenderDataPack.m_GPassTextureUBOData.useMetallicTexture = false;
				}
				// any roughness?
				l_TDC = l_material->m_texturePack.m_roughnessTDC.second;
				if (l_TDC && l_TDC->m_objectStatus == ObjectStatus::ALIVE)
				{
					l_GLRenderDataPack.m_basicRoughnessGLTDC = getGLTextureDataComponent(l_TDC->m_parentEntity);
				}
				else
				{
					l_GLRenderDataPack.m_GPassTextureUBOData.useRoughnessTexture = false;
				}
				// any ao?
				l_TDC = l_material->m_texturePack.m_roughnessTDC.second;
				if (l_TDC && l_TDC->m_objectStatus == ObjectStatus::ALIVE)
				{
					l_GLRenderDataPack.m_basicAOGLTDC = getGLTextureDataComponent(l_TDC->m_parentEntity);
				}
				else
				{
					l_GLRenderDataPack.m_GPassTextureUBOData.useAOTexture = false;
				}

				l_GLRenderDataPack.m_GPassTextureUBOData.albedo = vec4(
					l_material->m_meshCustomMaterial.albedo_r,
					l_material->m_meshCustomMaterial.albedo_g,
					l_material->m_meshCustomMaterial.albedo_b,
					1.0f
				);
				l_GLRenderDataPack.m_GPassTextureUBOData.MRA = vec4(
					l_material->m_meshCustomMaterial.metallic,
					l_material->m_meshCustomMaterial.roughness,
					l_material->m_meshCustomMaterial.ao,
					1.0f
				);

				l_GLRenderDataPack.visiblilityType = l_renderDataPack.visiblilityType;

				GLRenderingSystemComponent::get().m_GPassOpaqueRenderDataQueue.push(l_GLRenderDataPack);
			}
			else if (l_renderDataPack.visiblilityType == VisiblilityType::INNO_TRANSPARENT)
			{
				GPassTransparentRenderDataPack l_GLRenderDataPack;

				l_GLRenderDataPack.indiceSize = l_renderDataPack.MDC->m_indicesSize;
				l_GLRenderDataPack.m_meshDrawMethod = l_renderDataPack.MDC->m_meshDrawMethod;
				l_GLRenderDataPack.m_GPassMeshUBOData.m = l_renderDataPack.m;
				l_GLRenderDataPack.m_GPassMeshUBOData.m_prev = l_renderDataPack.m_prev;
				l_GLRenderDataPack.GLMDC = l_GLMDC;

				auto l_material = l_renderDataPack.Material;

				l_GLRenderDataPack.meshCustomMaterial = l_material->m_meshCustomMaterial;
				l_GLRenderDataPack.visiblilityType = l_renderDataPack.visiblilityType;
				GLRenderingSystemComponent::get().m_GPassTransparentRenderDataQueue.push(l_GLRenderDataPack);
			}
		}
	}

	return true;
}

bool GLRenderingSystemNS::prepareLightPassData()
{
	// sun/directional light
	auto l_directionalLight = GameSystemComponent::get().m_DirectionalLightComponents[0];
	auto l_directionalLightTransformComponent = g_pCoreSystem->getGameSystem()->get<TransformComponent>(l_directionalLight->m_parentEntity);

	GLRenderingSystemComponent::get().m_sunDir = InnoMath::getDirection(direction::BACKWARD, l_directionalLightTransformComponent->m_globalTransformVector.m_rot);
	GLRenderingSystemComponent::get().m_sunColor = l_directionalLight->m_color;
	GLRenderingSystemComponent::get().m_sunRot = InnoMath::getInvertRotationMatrix(l_directionalLightTransformComponent->m_globalTransformVector.m_rot);

	auto l_CSMSize = l_directionalLight->m_projectionMatrices.size();

	GLRenderingSystemComponent::get().m_CSMProjs.clear();
	GLRenderingSystemComponent::get().m_CSMProjs.reserve(l_CSMSize);
	GLRenderingSystemComponent::get().m_CSMSplitCorners.clear();
	GLRenderingSystemComponent::get().m_CSMSplitCorners.reserve(l_CSMSize);
	GLRenderingSystemComponent::get().m_CSMViews.clear();
	GLRenderingSystemComponent::get().m_CSMViews.reserve(l_CSMSize);

	for (size_t j = 0; j < l_directionalLight->m_projectionMatrices.size(); j++)
	{
		GLRenderingSystemComponent::get().m_CSMProjs.emplace_back();
		GLRenderingSystemComponent::get().m_CSMSplitCorners.emplace_back();
		GLRenderingSystemComponent::get().m_CSMViews.emplace_back();

		auto l_shadowSplitCorner = vec4(
			l_directionalLight->m_AABBsInWorldSpace[j].m_boundMin.x,
			l_directionalLight->m_AABBsInWorldSpace[j].m_boundMin.z,
			l_directionalLight->m_AABBsInWorldSpace[j].m_boundMax.x,
			l_directionalLight->m_AABBsInWorldSpace[j].m_boundMax.z
		);

		GLRenderingSystemComponent::get().m_CSMProjs[j] = l_directionalLight->m_projectionMatrices[j];
		GLRenderingSystemComponent::get().m_CSMSplitCorners[j] = l_shadowSplitCorner;

		auto l_lightRotMat = l_directionalLightTransformComponent->m_globalTransformMatrix.m_rotationMat.inverse();

		GLRenderingSystemComponent::get().m_CSMViews[j] = l_lightRotMat;
	}

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
		BillboardPassRenderDataPack l_GLRenderDataPack;
		l_GLRenderDataPack.globalPos = g_pCoreSystem->getGameSystem()->get<TransformComponent>(i->m_parentEntity)->m_globalTransformVector.m_pos;
		l_GLRenderDataPack.distanceToCamera = (GLRenderingSystemComponent::get().m_CamGlobalPos - l_GLRenderDataPack.globalPos).length();
		l_GLRenderDataPack.iconType = WorldEditorIconType::DIRECTIONAL_LIGHT;

		GLRenderingSystemComponent::get().m_BillboardPassRenderDataQueue.emplace(l_GLRenderDataPack);
	}

	for (auto i : GameSystemComponent::get().m_PointLightComponents)
	{
		BillboardPassRenderDataPack l_GLRenderDataPack;
		l_GLRenderDataPack.globalPos = g_pCoreSystem->getGameSystem()->get<TransformComponent>(i->m_parentEntity)->m_globalTransformVector.m_pos;
		l_GLRenderDataPack.distanceToCamera = (GLRenderingSystemComponent::get().m_CamGlobalPos - l_GLRenderDataPack.globalPos).length();
		l_GLRenderDataPack.iconType = WorldEditorIconType::POINT_LIGHT;

		GLRenderingSystemComponent::get().m_BillboardPassRenderDataQueue.emplace(l_GLRenderDataPack);
	}

	for (auto i : GameSystemComponent::get().m_SphereLightComponents)
	{
		BillboardPassRenderDataPack l_GLRenderDataPack;
		l_GLRenderDataPack.globalPos = g_pCoreSystem->getGameSystem()->get<TransformComponent>(i->m_parentEntity)->m_globalTransformVector.m_pos;
		l_GLRenderDataPack.distanceToCamera = (GLRenderingSystemComponent::get().m_CamGlobalPos - l_GLRenderDataPack.globalPos).length();
		l_GLRenderDataPack.iconType = WorldEditorIconType::SPHERE_LIGHT;

		GLRenderingSystemComponent::get().m_BillboardPassRenderDataQueue.emplace(l_GLRenderDataPack);
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
	GLRenderingSystemComponent::get().deferredPassFBDesc.sizeX = WindowSystemComponent::get().m_windowResolution.x;
	GLRenderingSystemComponent::get().deferredPassFBDesc.sizeY = WindowSystemComponent::get().m_windowResolution.y;

	GLGeometryRenderingPassUtilities::resize();
	GLLightRenderingPassUtilities::resize();
	GLFinalRenderingPassUtilities::resize();

	return true;
}
