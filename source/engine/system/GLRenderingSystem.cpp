#include "GLRenderingSystem.h"

#include <sstream>

#include "../component/GLEnvironmentRenderPassComponent.h"
#include "../component/GLShadowRenderPassComponent.h"
#include "../component/GLGeometryRenderPassComponent.h"
#include "../component/GLTerrainRenderPassComponent.h"
#include "../component/GLLightRenderPassComponent.h"
#include "../component/GLFinalRenderPassComponent.h"

#include "../component/AssetSystemComponent.h"
#include "../component/GameSystemComponent.h"
#include "../component/WindowSystemComponent.h"
#include "../component/RenderingSystemComponent.h"
#include "../component/GLRenderingSystemComponent.h"
#include "../component/PhysicsSystemComponent.h"

#include "../component/MeshDataComponent.h"
#include "../component/TextureDataComponent.h"
#include "../component/GLMeshDataComponent.h"
#include "../component/GLTextureDataComponent.h"
#include "../component/GLFrameBufferComponent.h"
#include "../component/GLShaderProgramComponent.h"

#include "ICoreSystem.h"

extern ICoreSystem* g_pCoreSystem;

INNO_PRIVATE_SCOPE GLRenderingSystemNS
{
	bool setup();
	bool initialize();

	void initializeDefaultAssets();

	float radicalInverse(unsigned int n, unsigned int base);
	void initializeHaltonSampler();

	void initializeEnvironmentPass();
	void initializeBRDFLUTPass();

	void initializeShadowPass();

	void initializeGeometryPass();

	void initializeOpaquePass();
	void initializeOpaquePassShaders();
	void bindOpaquePassUniformLocations(GLShaderProgramComponent* rhs);

	void initializeTransparentPass();
	void initializeTransparentPassShaders();
	void bindTransparentPassUniformLocations(GLShaderProgramComponent* rhs);

	void initializeTerrainPass();
	void initializeTerrainPassShaders();
	void bindTerrainPassUniformLocations(GLShaderProgramComponent* rhs);

	void initializeLightPass();
	void initializeLightPassShaders();
	void bindLightPassUniformLocations(GLShaderProgramComponent* rhs);

	void initializeFinalPass();

	void initializeSkyPass();
	void initializeTAAPass();
	void initializeBloomExtractPass();
	void initializeBloomBlurPass();
	void initializeMotionBlurPass();
	void initializeBillboardPass();
	void initializeDebuggerPass();
	void initializeFinalBlendPass();

	GLRenderPassComponent* addGLRenderPassComponent(unsigned int RTNum, GLFrameBufferDesc glFrameBufferDesc, TextureDataDesc RTDesc);

	bool resizeGLRenderPassComponent(GLRenderPassComponent* GLRPC, GLFrameBufferDesc glFrameBufferDesc);

	GLMeshDataComponent* generateGLMeshDataComponent(MeshDataComponent* rhs);
	GLTextureDataComponent* generateGLTextureDataComponent(TextureDataComponent* rhs);

	bool initializeGLShaderProgramComponent(GLShaderProgramComponent* rhs, const ShaderFilePaths& shaderFilePaths);
	bool initializeGLMeshDataComponent(GLMeshDataComponent * rhs, const std::vector<Vertex>& vertices, const std::vector<Index>& indices);
	bool initializeGLTextureDataComponent(GLTextureDataComponent * rhs, TextureDataDesc textureDataDesc, const std::vector<void*>& textureData);

	GLShaderProgramComponent* addGLShaderProgramComponent(EntityID rhs);

	bool reloadGLShaderProgramComponent(RenderPassType renderPassType);

	GLMeshDataComponent* addGLMeshDataComponent(EntityID rhs);
	GLTextureDataComponent* addGLTextureDataComponent(EntityID rhs);

	GLMeshDataComponent* getGLMeshDataComponent(EntityID rhs);
	GLTextureDataComponent* getGLTextureDataComponent(EntityID rhs);

	void prepareRenderingData();

	void updateEnvironmentRenderPass();
	void updateBRDFLUTRenderPass();

	void updateShadowRenderPass();

	void updateGeometryRenderPass();
	void updateOpaqueRenderPass();
	void updateTransparentRenderPass();

	void updateTerrainRenderPass();

	void updateLightRenderPass();

	void updateFinalRenderPass();

	GLTextureDataComponent* updateSkyPass();
	GLTextureDataComponent* updatePreTAAPass();
	GLTextureDataComponent* updateTAAPass(GLTextureDataComponent* inputGLTDC);
	GLTextureDataComponent* updateTAASharpenPass(GLTextureDataComponent* inputGLTDC);
	GLTextureDataComponent* updateBloomExtractPass(GLTextureDataComponent* inputGLTDC);
	GLTextureDataComponent* updateBloomBlurPass(GLTextureDataComponent* inputGLTDC);
	GLTextureDataComponent* updateMotionBlurPass(GLTextureDataComponent * inputGLTDC);
	GLTextureDataComponent* updateBillboardPass();
	GLTextureDataComponent* updateDebuggerPass();
	GLTextureDataComponent* updateFinalBlendPass();

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

	GLuint getUniformLocation(GLuint shaderProgram, const std::string& uniformName);
	GLuint getUniformBlockIndex(GLuint shaderProgram, const std::string& uniformBlockName);
	GLuint generateUBO(GLuint UBOSize);
	void bindUniformBlock(GLuint UBO, GLuint UBOSize, GLuint program, const std::string & uniformBlockName, GLuint uniformBlockBindingPoint);
	void updateTextureUniformLocations(GLuint program, const std::vector<std::string>& UniformNames);

	template<typename T>
	void updateUBO(const GLint& UBO, const T& UBOValue);

	void updateUniform(const GLint uniformLocation, bool uniformValue);
	void updateUniform(const GLint uniformLocation, int uniformValue);
	void updateUniform(const GLint uniformLocation, float uniformValue);
	void updateUniform(const GLint uniformLocation, float x, float y);
	void updateUniform(const GLint uniformLocation, float x, float y, float z);
	void updateUniform(const GLint uniformLocation, float x, float y, float z, float w);
	void updateUniform(const GLint uniformLocation, const mat4& mat);

	void attachTextureToFramebuffer(TextureDataComponent* TDC, GLTextureDataComponent* GLTextureDataComponent, GLFrameBufferComponent* GLFrameBufferComponent, int colorAttachmentIndex, int textureIndex, int mipLevel);
	void activateShaderProgram(GLShaderProgramComponent* GLShaderProgramComponent);
	void drawMesh(EntityID rhs);
	void drawMesh(MeshDataComponent* MDC);
	void drawMesh(size_t indicesSize, MeshPrimitiveTopology MeshPrimitiveTopology, GLMeshDataComponent* GLMDC);
	void activateTexture(TextureDataComponent* TDC, int activateIndex);
	void activateTexture(GLTextureDataComponent* GLTDC, int activateIndex);

	static GameSystemComponent* g_GameSystemComponent;
	static RenderingSystemComponent* g_RenderingSystemComponent;
	static GLRenderingSystemComponent* g_GLRenderingSystemComponent;

	std::unordered_map<EntityID, GLMeshDataComponent*> m_initializedGLMDC;
	std::unordered_map<EntityID, GLTextureDataComponent*> m_initializedGLTDC;

	ObjectStatus m_objectStatus = ObjectStatus::SHUTDOWN;

	GLMeshDataComponent* m_UnitLineGLMDC;
	GLMeshDataComponent* m_UnitQuadGLMDC;
	GLMeshDataComponent* m_UnitCubeGLMDC;
	GLMeshDataComponent* m_UnitSphereGLMDC;
	GLMeshDataComponent* m_terrainGLMDC;

	GLTextureDataComponent* m_basicNormalGLTDC;
	GLTextureDataComponent* m_basicAlbedoGLTDC;
	GLTextureDataComponent* m_basicMetallicGLTDC;
	GLTextureDataComponent* m_basicRoughnessGLTDC;
	GLTextureDataComponent* m_basicAOGLTDC;

	GLFrameBufferDesc deferredPassFBDesc = GLFrameBufferDesc();
	GLFrameBufferDesc shadowPassFBDesc = GLFrameBufferDesc();

	TextureDataDesc deferredPassTextureDesc = TextureDataDesc();
	TextureDataDesc shadowPassTextureDesc = TextureDataDesc();

	mat4 m_CamProjOriginal;
	mat4 m_CamProjJittered;
	mat4 m_CamRot;
	mat4 m_CamTrans;
	mat4 m_CamRot_prev;
	mat4 m_CamTrans_prev;
	vec4 m_CamGlobalPos;

	vec4 m_sunDir;
	vec4 m_sunColor;
	mat4 m_sunRot;

	std::vector<mat4> m_CSMProjs;
	std::vector<vec4> m_CSMSplitCorners;

	struct GPassCameraUBOData
	{
		mat4 m_CamProjOriginal;
		mat4 m_CamProjJittered;
		mat4 m_CamRot;
		mat4 m_CamTrans;
		mat4 m_CamRot_prev;
		mat4 m_CamTrans_prev;
	};

	GPassCameraUBOData m_GPassCameraUBOData;

	struct GPassLightUBOData
	{
		mat4 uni_p_light_0;
		mat4 uni_p_light_1;
		mat4 uni_p_light_2;
		mat4 uni_p_light_3;
		mat4 uni_v_light;
	};

	GPassLightUBOData m_GPassLightUBOData;

	struct GPassMeshUBOData
	{
		mat4 m;
		mat4 m_prev;
	};

	// glsl's bool is 4 bytes, so use int here
	struct GPassTextureUBOData
	{
		vec4 albedo;
		vec4 MRA;
		int useNormalTexture = true;
		int useAlbedoTexture = true;
		int useMetallicTexture = true;
		int useRoughnessTexture = true;
		int useAOTexture = true;
	};

	struct GPassOpaqueRenderDataPack
	{
		size_t indiceSize;
		GPassMeshUBOData m_GPassMeshUBOData;
		GLMeshDataComponent* GLMDC;
		MeshPrimitiveTopology m_meshDrawMethod;
		GPassTextureUBOData m_GPassTextureUBOData;
		GLTextureDataComponent* m_basicNormalGLTDC;
		GLTextureDataComponent* m_basicAlbedoGLTDC;
		GLTextureDataComponent* m_basicMetallicGLTDC;
		GLTextureDataComponent* m_basicRoughnessGLTDC;
		GLTextureDataComponent* m_basicAOGLTDC;
		VisiblilityType visiblilityType;
	};

	std::queue<GPassOpaqueRenderDataPack> m_GPassOpaqueRenderDataQueue;

	struct GPassTransparentRenderDataPack
	{
		size_t indiceSize;
		GPassMeshUBOData m_GPassMeshUBOData;
		GLMeshDataComponent* GLMDC;
		MeshPrimitiveTopology m_meshDrawMethod;
		MeshCustomMaterial meshCustomMaterial;
		VisiblilityType visiblilityType;
	};

	std::queue<GPassTransparentRenderDataPack> m_GPassTransparentRenderDataQueue;

	struct PointLightData
	{
		vec4 pos;
		vec4 luminance;
		float attenuationRadius;
	};

	std::vector<PointLightData> m_PointLightDatas;

	struct SphereLightData
	{
		vec4 pos;
		vec4 luminance;
		float sphereRadius;
	};

	std::vector<SphereLightData> m_SphereLightDatas;

	std::function<void(GLFrameBufferComponent*)> f_bindFBC;
	std::function<void(GLFrameBufferComponent*)> f_cleanFBC;
	std::function<void(GLFrameBufferComponent*, GLFrameBufferComponent*)> f_copyDepthBuffer;
	std::function<void(GLFrameBufferComponent*, GLFrameBufferComponent*)> f_copyStencilBuffer;
	std::function<void(GLFrameBufferComponent*, GLFrameBufferComponent*)> f_copyColorBuffer;
}

bool GLRenderingSystem::setup()
{
	return GLRenderingSystemNS::setup();
}

bool GLRenderingSystem::initialize()
{
	return GLRenderingSystemNS::initialize();
}

bool GLRenderingSystemNS::setup()
{
	g_GameSystemComponent = &GameSystemComponent::get();
	g_RenderingSystemComponent = &RenderingSystemComponent::get();
	g_GLRenderingSystemComponent = &GLRenderingSystemComponent::get();

	deferredPassFBDesc.renderBufferAttachmentType = GL_DEPTH_STENCIL_ATTACHMENT;
	deferredPassFBDesc.renderBufferInternalFormat = GL_DEPTH24_STENCIL8;
	deferredPassFBDesc.sizeX = WindowSystemComponent::get().m_windowResolution.x;
	deferredPassFBDesc.sizeY = WindowSystemComponent::get().m_windowResolution.y;

	shadowPassFBDesc.renderBufferAttachmentType = GL_DEPTH_ATTACHMENT;
	shadowPassFBDesc.renderBufferInternalFormat = GL_DEPTH_COMPONENT32;
	shadowPassFBDesc.sizeX = 2048;
	shadowPassFBDesc.sizeY = 2048;

	deferredPassTextureDesc.textureUsageType = TextureUsageType::RENDER_BUFFER_SAMPLER;
	deferredPassTextureDesc.textureColorComponentsFormat = TextureColorComponentsFormat::RGBA16F;
	deferredPassTextureDesc.texturePixelDataFormat = TexturePixelDataFormat::RGBA;
	deferredPassTextureDesc.textureMinFilterMethod = TextureFilterMethod::NEAREST;
	deferredPassTextureDesc.textureMagFilterMethod = TextureFilterMethod::NEAREST;
	deferredPassTextureDesc.textureWrapMethod = TextureWrapMethod::CLAMP_TO_EDGE;
	deferredPassTextureDesc.textureWidth = deferredPassFBDesc.sizeX;
	deferredPassTextureDesc.textureHeight = deferredPassFBDesc.sizeY;
	deferredPassTextureDesc.texturePixelDataType = TexturePixelDataType::FLOAT;

	shadowPassTextureDesc.textureUsageType = TextureUsageType::SHADOWMAP;
	shadowPassTextureDesc.textureColorComponentsFormat = TextureColorComponentsFormat::DEPTH_COMPONENT;
	shadowPassTextureDesc.texturePixelDataFormat = TexturePixelDataFormat::DEPTH_COMPONENT;
	shadowPassTextureDesc.textureMinFilterMethod = TextureFilterMethod::NEAREST;
	shadowPassTextureDesc.textureMagFilterMethod = TextureFilterMethod::NEAREST;
	shadowPassTextureDesc.textureWrapMethod = TextureWrapMethod::CLAMP_TO_BORDER;
	shadowPassTextureDesc.textureWidth = shadowPassFBDesc.sizeX;
	shadowPassTextureDesc.textureHeight = shadowPassFBDesc.sizeY;
	shadowPassTextureDesc.texturePixelDataType = TexturePixelDataType::FLOAT;

	g_RenderingSystemComponent->f_reloadShader =
		[&](RenderPassType renderPassType) {
		reloadGLShaderProgramComponent(renderPassType);
	};

	f_bindFBC = [&](GLFrameBufferComponent* val) {
		f_cleanFBC(val);

		glRenderbufferStorage(GL_RENDERBUFFER, val->m_GLFrameBufferDesc.renderBufferInternalFormat, val->m_GLFrameBufferDesc.sizeX, val->m_GLFrameBufferDesc.sizeY);
		glViewport(0, 0, val->m_GLFrameBufferDesc.sizeX, val->m_GLFrameBufferDesc.sizeY);
	};

	f_cleanFBC = [&](GLFrameBufferComponent* val) {
		glBindFramebuffer(GL_FRAMEBUFFER, val->m_FBO);
		glBindRenderbuffer(GL_RENDERBUFFER, val->m_RBO);

		glClearColor(0.0f, 0.0f, 0.0f, 0.0f);
		glClear(GL_COLOR_BUFFER_BIT);
		glClear(GL_DEPTH_BUFFER_BIT);
		glClear(GL_STENCIL_BUFFER_BIT);
	};

	f_copyDepthBuffer = [&](GLFrameBufferComponent* src, GLFrameBufferComponent* dest) {
		glBindFramebuffer(GL_READ_FRAMEBUFFER, src->m_FBO);
		glBindFramebuffer(GL_DRAW_FRAMEBUFFER, dest->m_FBO);
		glBlitFramebuffer(0, 0, src->m_GLFrameBufferDesc.sizeX, src->m_GLFrameBufferDesc.sizeY, 0, 0, dest->m_GLFrameBufferDesc.sizeX, dest->m_GLFrameBufferDesc.sizeY, GL_DEPTH_BUFFER_BIT, GL_NEAREST);
	};

	f_copyStencilBuffer = [&](GLFrameBufferComponent* src, GLFrameBufferComponent* dest) {
		glBindFramebuffer(GL_READ_FRAMEBUFFER, src->m_FBO);
		glBindFramebuffer(GL_DRAW_FRAMEBUFFER, dest->m_FBO);
		glBlitFramebuffer(0, 0, src->m_GLFrameBufferDesc.sizeX, src->m_GLFrameBufferDesc.sizeY, 0, 0, dest->m_GLFrameBufferDesc.sizeX, dest->m_GLFrameBufferDesc.sizeY, GL_STENCIL_BUFFER_BIT, GL_NEAREST);
	};

	f_copyColorBuffer = [&](GLFrameBufferComponent* src, GLFrameBufferComponent* dest) {
		glBindFramebuffer(GL_READ_FRAMEBUFFER, src->m_FBO);
		glBindFramebuffer(GL_DRAW_FRAMEBUFFER, dest->m_FBO);
		glBlitFramebuffer(0, 0, src->m_GLFrameBufferDesc.sizeX, src->m_GLFrameBufferDesc.sizeY, 0, 0, dest->m_GLFrameBufferDesc.sizeX, dest->m_GLFrameBufferDesc.sizeY, GL_COLOR_BUFFER_BIT, GL_NEAREST);
	};

	if (g_RenderingSystemComponent->m_MSAAdepth)
	{
		// antialiasing
		glfwWindowHint(GLFW_SAMPLES, g_RenderingSystemComponent->m_MSAAdepth);
		// MSAA
		glEnable(GL_MULTISAMPLE);
	}
	
	glEnable(GL_DEBUG_OUTPUT);
	glDebugMessageCallback(MessageCallback, 0);

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
	initializeEnvironmentPass();
	initializeShadowPass();
	initializeGeometryPass();
	initializeTerrainPass();
	initializeLightPass();
	initializeFinalPass();

	return true;
}

void  GLRenderingSystemNS::initializeDefaultAssets()
{
	m_UnitLineGLMDC = generateGLMeshDataComponent(g_pCoreSystem->getAssetSystem()->getMeshDataComponent(MeshShapeType::LINE));
	m_UnitQuadGLMDC = generateGLMeshDataComponent(g_pCoreSystem->getAssetSystem()->getMeshDataComponent(MeshShapeType::QUAD));
	m_UnitCubeGLMDC = generateGLMeshDataComponent(g_pCoreSystem->getAssetSystem()->getMeshDataComponent(MeshShapeType::CUBE));
	m_UnitSphereGLMDC = generateGLMeshDataComponent(g_pCoreSystem->getAssetSystem()->getMeshDataComponent(MeshShapeType::SPHERE));
	m_terrainGLMDC = generateGLMeshDataComponent(g_pCoreSystem->getAssetSystem()->getMeshDataComponent(MeshShapeType::TERRAIN));

	m_basicNormalGLTDC = generateGLTextureDataComponent(g_pCoreSystem->getAssetSystem()->getTextureDataComponent(TextureUsageType::NORMAL));
	m_basicAlbedoGLTDC = generateGLTextureDataComponent(g_pCoreSystem->getAssetSystem()->getTextureDataComponent(TextureUsageType::ALBEDO));
	m_basicMetallicGLTDC = generateGLTextureDataComponent(g_pCoreSystem->getAssetSystem()->getTextureDataComponent(TextureUsageType::METALLIC));
	m_basicRoughnessGLTDC = generateGLTextureDataComponent(g_pCoreSystem->getAssetSystem()->getTextureDataComponent(TextureUsageType::ROUGHNESS));
	m_basicAOGLTDC = generateGLTextureDataComponent(g_pCoreSystem->getAssetSystem()->getTextureDataComponent(TextureUsageType::AMBIENT_OCCLUSION));

	g_GLRenderingSystemComponent->m_iconTemplate_OBJ = generateGLTextureDataComponent(g_pCoreSystem->getAssetSystem()->getTextureDataComponent(IconType::OBJ));
	g_GLRenderingSystemComponent->m_iconTemplate_PNG = generateGLTextureDataComponent(g_pCoreSystem->getAssetSystem()->getTextureDataComponent(IconType::PNG));
	g_GLRenderingSystemComponent->m_iconTemplate_SHADER = generateGLTextureDataComponent(g_pCoreSystem->getAssetSystem()->getTextureDataComponent(IconType::SHADER));
	g_GLRenderingSystemComponent->m_iconTemplate_UNKNOWN = generateGLTextureDataComponent(g_pCoreSystem->getAssetSystem()->getTextureDataComponent(IconType::UNKNOWN));
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
		g_RenderingSystemComponent->HaltonSampler.emplace_back(vec2(radicalInverse(i, 2) * 2.0f - 1.0f, radicalInverse(i, 3) * 2.0f - 1.0f));
	}
}

GLRenderPassComponent* GLRenderingSystemNS::addGLRenderPassComponent(unsigned int RTNum, GLFrameBufferDesc glFrameBufferDesc, TextureDataDesc RTDesc)
{
	auto l_GLRPC = g_pCoreSystem->getMemorySystem()->spawn<GLRenderPassComponent>();

	// generate and bind framebuffer
	auto l_FBC = g_pCoreSystem->getMemorySystem()->spawn<GLFrameBufferComponent>();
	l_GLRPC->m_GLFBC = l_FBC;

	l_GLRPC->m_GLFBC->m_GLFrameBufferDesc.sizeX = glFrameBufferDesc.sizeX;
	l_GLRPC->m_GLFBC->m_GLFrameBufferDesc.sizeY = glFrameBufferDesc.sizeY;
	l_GLRPC->m_GLFBC->m_GLFrameBufferDesc.renderBufferAttachmentType = glFrameBufferDesc.renderBufferAttachmentType;
	l_GLRPC->m_GLFBC->m_GLFrameBufferDesc.renderBufferInternalFormat = glFrameBufferDesc.renderBufferInternalFormat;

	glGenFramebuffers(1, &l_FBC->m_FBO);
	glBindFramebuffer(GL_FRAMEBUFFER, l_FBC->m_FBO);

	// generate and bind renderbuffer
	glGenRenderbuffers(1, &l_FBC->m_RBO);
	glBindRenderbuffer(GL_RENDERBUFFER, l_FBC->m_RBO);

	glFramebufferRenderbuffer(GL_FRAMEBUFFER, l_GLRPC->m_GLFBC->m_GLFrameBufferDesc.renderBufferAttachmentType, GL_RENDERBUFFER, l_FBC->m_RBO);
	glRenderbufferStorage(GL_RENDERBUFFER, l_GLRPC->m_GLFBC->m_GLFrameBufferDesc.renderBufferInternalFormat, l_GLRPC->m_GLFBC->m_GLFrameBufferDesc.sizeX, l_GLRPC->m_GLFBC->m_GLFrameBufferDesc.sizeY);

	// generate and bind texture
	l_GLRPC->m_TDCs.reserve(RTNum);

	for (unsigned int i = 0; i < RTNum; i++)
	{
		auto l_TDC = g_pCoreSystem->getMemorySystem()->spawn<TextureDataComponent>();

		l_TDC->m_textureDataDesc.textureUsageType = RTDesc.textureUsageType;
		l_TDC->m_textureDataDesc.textureColorComponentsFormat = RTDesc.textureColorComponentsFormat;
		l_TDC->m_textureDataDesc.texturePixelDataFormat = RTDesc.texturePixelDataFormat;
		l_TDC->m_textureDataDesc.textureMinFilterMethod = RTDesc.textureMinFilterMethod;
		l_TDC->m_textureDataDesc.textureMagFilterMethod = RTDesc.textureMagFilterMethod;
		l_TDC->m_textureDataDesc.textureWrapMethod = RTDesc.textureWrapMethod;
		l_TDC->m_textureDataDesc.textureWidth = RTDesc.textureWidth;
		l_TDC->m_textureDataDesc.textureHeight = RTDesc.textureHeight;
		l_TDC->m_textureDataDesc.texturePixelDataType = RTDesc.texturePixelDataType;
		l_TDC->m_textureData = { nullptr };

		l_GLRPC->m_TDCs.emplace_back(l_TDC);
	}

	l_GLRPC->m_GLTDCs.reserve(RTNum);

	for (unsigned int i = 0; i < RTNum; i++)
	{
		auto l_TDC = l_GLRPC->m_TDCs[i];
		auto l_GLTDC = generateGLTextureDataComponent(l_TDC);

		attachTextureToFramebuffer(
			l_TDC,
			l_GLTDC,
			l_FBC,
			(int)i, 0, 0
		);

		l_GLRPC->m_GLTDCs.emplace_back(l_GLTDC);
	}

	if (RTDesc.textureUsageType == TextureUsageType::SHADOWMAP)
	{
		glDrawBuffer(GL_NONE);
		glReadBuffer(GL_NONE);
	}
	else
	{
		std::vector<unsigned int> l_colorAttachments;
		for (unsigned int i = 0; i < RTNum; ++i)
		{
			l_colorAttachments.emplace_back(GL_COLOR_ATTACHMENT0 + i);
		}
		glDrawBuffers((GLsizei)l_colorAttachments.size(), &l_colorAttachments[0]);
	}

	// finally check if framebuffer is complete
	if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
	{
		g_pCoreSystem->getLogSystem()->printLog(LogType::INNO_ERROR, "GLRenderingSystem: Framebuffer is not completed!");
	}

	glBindRenderbuffer(GL_RENDERBUFFER, 0);
	glBindFramebuffer(GL_FRAMEBUFFER, 0);

	return l_GLRPC;
}

bool GLRenderingSystemNS::resizeGLRenderPassComponent(GLRenderPassComponent * GLRPC, GLFrameBufferDesc glFrameBufferDesc)
{
	GLRPC->m_GLFBC->m_GLFrameBufferDesc.sizeX = glFrameBufferDesc.sizeX;
	GLRPC->m_GLFBC->m_GLFrameBufferDesc.sizeY = glFrameBufferDesc.sizeY;

	glBindFramebuffer(GL_FRAMEBUFFER, GLRPC->m_GLFBC->m_FBO);
	glBindRenderbuffer(GL_RENDERBUFFER, GLRPC->m_GLFBC->m_RBO);

	for (unsigned int i = 0; i < GLRPC->m_GLTDCs.size(); i++)
	{
		GLRPC->m_TDCs[i]->m_textureDataDesc.textureWidth = GLRPC->m_GLFBC->m_GLFrameBufferDesc.sizeX;
		GLRPC->m_TDCs[i]->m_textureDataDesc.textureHeight = GLRPC->m_GLFBC->m_GLFrameBufferDesc.sizeY;
		GLRPC->m_TDCs[i]->m_objectStatus = ObjectStatus::STANDBY;
		glDeleteTextures(1, &GLRPC->m_GLTDCs[i]->m_TAO);
		initializeGLTextureDataComponent(GLRPC->m_GLTDCs[i], GLRPC->m_TDCs[i]->m_textureDataDesc, GLRPC->m_TDCs[i]->m_textureData);
		attachTextureToFramebuffer(
			GLRPC->m_TDCs[i],
			GLRPC->m_GLTDCs[i],
			GLRPC->m_GLFBC,
			(int)i, 0, 0
		);
	}

	glBindRenderbuffer(GL_RENDERBUFFER, 0);
	glBindFramebuffer(GL_FRAMEBUFFER, 0);

	return true;
}

bool GLRenderingSystemNS::reloadGLShaderProgramComponent(RenderPassType renderPassType)
{
	GLShaderProgramComponent* l_GLSPC;
	ShaderFilePaths l_shaderFilePaths;
	std::function<void(GLShaderProgramComponent*)> f_bindUniformLocations;

	switch (renderPassType)
	{
	case RenderPassType::OpaquePass: 
		l_GLSPC = GLGeometryRenderPassComponent::get().m_opaquePass_GLSPC; 
		l_shaderFilePaths = GLGeometryRenderPassComponent::get().m_opaquePass_shaderFilePaths;
		f_bindUniformLocations = [](GLShaderProgramComponent* rhs) { bindOpaquePassUniformLocations(rhs); };
		break;
	case RenderPassType::TransparentPass:
		l_GLSPC = GLGeometryRenderPassComponent::get().m_transparentPass_GLSPC;
		l_shaderFilePaths = GLGeometryRenderPassComponent::get().m_transparentPass_shaderFilePaths;
		f_bindUniformLocations = [](GLShaderProgramComponent* rhs) { bindTransparentPassUniformLocations(rhs); };
		break;
	case RenderPassType::TerrainPass: 
		l_GLSPC = GLTerrainRenderPassComponent::get().m_GLSPC;
		l_shaderFilePaths = GLTerrainRenderPassComponent::get().m_shaderFilePaths;
		f_bindUniformLocations = [](GLShaderProgramComponent* rhs) { bindTerrainPassUniformLocations(rhs); };

		break;
	case RenderPassType::LightPass: l_GLSPC = GLLightRenderPassComponent::get().m_GLSPC;
		l_shaderFilePaths = GLLightRenderPassComponent::get().m_shaderFilePaths;
		f_bindUniformLocations = [](GLShaderProgramComponent* rhs) { bindLightPassUniformLocations(rhs); };
		break;
	default: break;
	}

	if (l_GLSPC->m_VSID)
	{
		glDetachShader(l_GLSPC->m_program, l_GLSPC->m_VSID);
		glDeleteShader(l_GLSPC->m_VSID);
	}
	if (l_GLSPC->m_GSID)
	{
		glDetachShader(l_GLSPC->m_program, l_GLSPC->m_GSID);
		glDeleteShader(l_GLSPC->m_GSID);
	}
	if (l_GLSPC->m_FSID)
	{
		glDetachShader(l_GLSPC->m_program, l_GLSPC->m_FSID);
		glDeleteShader(l_GLSPC->m_FSID);
	}

	glDeleteProgram(l_GLSPC->m_program);

	initializeGLShaderProgramComponent(l_GLSPC, l_shaderFilePaths);

	f_bindUniformLocations(l_GLSPC);

	g_pCoreSystem->getLogSystem()->printLog(LogType::INNO_DEV_SUCCESS, "GLRenderingSystem: innoShader: shader reloaded.");

	return true;
}

void GLRenderingSystemNS::initializeEnvironmentPass()
{
	initializeBRDFLUTPass();
}

void GLRenderingSystemNS::initializeBRDFLUTPass()
{
	// generate and bind framebuffer
	auto l_FBC = g_pCoreSystem->getMemorySystem()->spawn<GLFrameBufferComponent>();

	glGenFramebuffers(1, &l_FBC->m_FBO);
	glBindFramebuffer(GL_FRAMEBUFFER, l_FBC->m_FBO);

	// generate and bind renderbuffer
	glGenRenderbuffers(1, &l_FBC->m_RBO);
	glBindRenderbuffer(GL_RENDERBUFFER, l_FBC->m_RBO);

	glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, l_FBC->m_RBO);
	glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT32, 512, 512);

	GLEnvironmentRenderPassComponent::get().m_BRDFLUTPassFBC = l_FBC;

	// generate and bind texture
	auto l_TDC = g_pCoreSystem->getMemorySystem()->spawn<TextureDataComponent>();

	l_TDC->m_textureDataDesc.textureUsageType = TextureUsageType::RENDER_BUFFER_SAMPLER;
	l_TDC->m_textureDataDesc.textureColorComponentsFormat = TextureColorComponentsFormat::RGBA16F;
	l_TDC->m_textureDataDesc.texturePixelDataFormat = TexturePixelDataFormat::RGBA;
	l_TDC->m_textureDataDesc.textureMinFilterMethod = TextureFilterMethod::LINEAR;
	l_TDC->m_textureDataDesc.textureMagFilterMethod = TextureFilterMethod::LINEAR;
	l_TDC->m_textureDataDesc.textureWrapMethod = TextureWrapMethod::CLAMP_TO_EDGE;
	l_TDC->m_textureDataDesc.textureWidth = 512;
	l_TDC->m_textureDataDesc.textureHeight = 512;
	l_TDC->m_textureDataDesc.texturePixelDataType = TexturePixelDataType::FLOAT;
	l_TDC->m_textureData = { nullptr };

	GLEnvironmentRenderPassComponent::get().m_BRDFSplitSumLUTPassTDC = l_TDC;

	auto l_GLTDC = generateGLTextureDataComponent(l_TDC);

	GLEnvironmentRenderPassComponent::get().m_BRDFSplitSumLUTPassGLTDC = l_GLTDC;

	////
	l_TDC = g_pCoreSystem->getMemorySystem()->spawn<TextureDataComponent>();

	l_TDC->m_textureDataDesc.textureUsageType = TextureUsageType::RENDER_BUFFER_SAMPLER;
	l_TDC->m_textureDataDesc.textureColorComponentsFormat = TextureColorComponentsFormat::RG16F;
	l_TDC->m_textureDataDesc.texturePixelDataFormat = TexturePixelDataFormat::RG;
	l_TDC->m_textureDataDesc.textureMinFilterMethod = TextureFilterMethod::LINEAR;
	l_TDC->m_textureDataDesc.textureMagFilterMethod = TextureFilterMethod::LINEAR;
	l_TDC->m_textureDataDesc.textureWrapMethod = TextureWrapMethod::CLAMP_TO_EDGE;
	l_TDC->m_textureDataDesc.textureWidth = 512;
	l_TDC->m_textureDataDesc.textureHeight = 512;
	l_TDC->m_textureDataDesc.texturePixelDataType = TexturePixelDataType::FLOAT;
	l_TDC->m_textureData = { nullptr };

	GLEnvironmentRenderPassComponent::get().m_BRDFMSAverageLUTPassTDC = l_TDC;

	l_GLTDC = generateGLTextureDataComponent(l_TDC);

	GLEnvironmentRenderPassComponent::get().m_BRDFMSAverageLUTPassGLTDC = l_GLTDC;

	// finally check if framebuffer is complete
	if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
	{
		g_pCoreSystem->getLogSystem()->printLog(LogType::INNO_ERROR, "GLRenderingSystem: BRDFLUTRenderPass Framebuffer is not completed!");
	}

	glBindRenderbuffer(GL_RENDERBUFFER, 0);
	glBindFramebuffer(GL_FRAMEBUFFER, 0);

	// shader programs and shaders
	ShaderFilePaths m_ShaderFilePaths;

	////
	m_ShaderFilePaths.m_VSPath = "GL4.0//BRDFLUTPassVertex.sf";
	m_ShaderFilePaths.m_FSPath = "GL4.0//BRDFLUTPassFragment.sf";

	auto rhs = addGLShaderProgramComponent(0); initializeGLShaderProgramComponent(rhs, m_ShaderFilePaths);

	GLEnvironmentRenderPassComponent::get().m_BRDFSplitSumLUTPassSPC = rhs;

	////
	m_ShaderFilePaths.m_VSPath = "GL4.0//BRDFLUTMSPassVertex.sf";
	m_ShaderFilePaths.m_FSPath = "GL4.0//BRDFLUTMSPassFragment.sf";

	rhs = addGLShaderProgramComponent(0); initializeGLShaderProgramComponent(rhs, m_ShaderFilePaths);

	GLEnvironmentRenderPassComponent::get().m_BRDFMSAverageLUTPass_uni_brdfLUT = getUniformLocation(
		rhs->m_program,
		"uni_brdfLUT");
	updateUniform(
		GLEnvironmentRenderPassComponent::get().m_BRDFMSAverageLUTPass_uni_brdfLUT,
		0);

	GLEnvironmentRenderPassComponent::get().m_BRDFMSAverageLUTPassSPC = rhs;
}

void GLRenderingSystemNS::initializeShadowPass()
{
	for (size_t i = 0; i < 4; i++)
	{
		GLShadowRenderPassComponent::get().m_GLRPCs.emplace_back(addGLRenderPassComponent(1, shadowPassFBDesc, shadowPassTextureDesc));
	}

	// shader programs and shaders
	auto rhs = addGLShaderProgramComponent(0); 
	initializeGLShaderProgramComponent(rhs, GLShadowRenderPassComponent::get().m_shaderFilePaths);

	GLShadowRenderPassComponent::get().m_shadowPass_uni_p = getUniformLocation(
		rhs->m_program,
		"uni_p");
	GLShadowRenderPassComponent::get().m_shadowPass_uni_v = getUniformLocation(
		rhs->m_program,
		"uni_v");
	GLShadowRenderPassComponent::get().m_shadowPass_uni_m = getUniformLocation(
		rhs->m_program,
		"uni_m");

	GLShadowRenderPassComponent::get().m_SPC = rhs;
}

void GLRenderingSystemNS::initializeGeometryPass()
{
	initializeOpaquePass();
	initializeTransparentPass();
}

void GLRenderingSystemNS::initializeOpaquePass()
{
	GLGeometryRenderPassComponent::get().m_opaquePass_GLRPC = addGLRenderPassComponent(8, deferredPassFBDesc, deferredPassTextureDesc);
	
	// UBO
	auto l_UBO = generateUBO(sizeof(GPassCameraUBOData));
	GLGeometryRenderPassComponent::get().m_cameraUBO = l_UBO;

	l_UBO = generateUBO(sizeof(GPassLightUBOData));
	GLGeometryRenderPassComponent::get().m_lightUBO = l_UBO;

	l_UBO = generateUBO(sizeof(GPassMeshUBOData));
	GLGeometryRenderPassComponent::get().m_meshUBO = l_UBO;

	l_UBO = generateUBO(sizeof(GPassTextureUBOData));
	GLGeometryRenderPassComponent::get().m_textureUBO = l_UBO;

	initializeOpaquePassShaders();
}

void GLRenderingSystemNS::initializeOpaquePassShaders()
{
	// shader programs and shaders
	auto rhs = addGLShaderProgramComponent(0);

	initializeGLShaderProgramComponent(rhs, GLGeometryRenderPassComponent::get().m_opaquePass_shaderFilePaths);
 	
	bindOpaquePassUniformLocations(rhs);

	GLGeometryRenderPassComponent::get().m_opaquePass_GLSPC = rhs;
}

void GLRenderingSystemNS::bindOpaquePassUniformLocations(GLShaderProgramComponent* rhs)
{
	bindUniformBlock(GLGeometryRenderPassComponent::get().m_cameraUBO, sizeof(GPassCameraUBOData), rhs->m_program, "cameraUBO", 0);

	bindUniformBlock(GLGeometryRenderPassComponent::get().m_lightUBO, sizeof(GPassLightUBOData), rhs->m_program, "lightUBO", 1);

	bindUniformBlock(GLGeometryRenderPassComponent::get().m_meshUBO, sizeof(GPassMeshUBOData), rhs->m_program, "meshUBO", 2);

	bindUniformBlock(GLGeometryRenderPassComponent::get().m_textureUBO, sizeof(GPassTextureUBOData), rhs->m_program, "textureUBO", 3);

#ifdef CookTorrance
	updateTextureUniformLocations(rhs->m_program, GLGeometryRenderPassComponent::get().m_textureUniformNames);
#elif BlinnPhong
	// @TODO: texture uniforms
#endif
}

void GLRenderingSystemNS::initializeTransparentPass()
{
	GLGeometryRenderPassComponent::get().m_transparentPass_GLRPC = addGLRenderPassComponent(1, deferredPassFBDesc, deferredPassTextureDesc);

	initializeTransparentPassShaders();
}

void GLRenderingSystemNS::initializeTransparentPassShaders()
{
	// shader programs and shaders
	auto rhs = addGLShaderProgramComponent(0);

	initializeGLShaderProgramComponent(rhs, GLGeometryRenderPassComponent::get().m_transparentPass_shaderFilePaths);

	bindTransparentPassUniformLocations(rhs);

	GLGeometryRenderPassComponent::get().m_transparentPass_GLSPC = rhs;
}

void GLRenderingSystemNS::bindTransparentPassUniformLocations(GLShaderProgramComponent* rhs)
{
	bindUniformBlock(GLGeometryRenderPassComponent::get().m_cameraUBO, sizeof(GPassCameraUBOData), rhs->m_program, "cameraUBO", 0);

	bindUniformBlock(GLGeometryRenderPassComponent::get().m_meshUBO, sizeof(GPassMeshUBOData), rhs->m_program, "meshUBO", 1);

	GLGeometryRenderPassComponent::get().m_transparentPass_uni_albedo = getUniformLocation(
		rhs->m_program,
		"uni_albedo");
	GLGeometryRenderPassComponent::get().m_transparentPass_uni_viewPos = getUniformLocation(
		rhs->m_program,
		"uni_viewPos");	
}

void GLRenderingSystemNS::initializeTerrainPass()
{
	GLTerrainRenderPassComponent::get().m_GLRPC = addGLRenderPassComponent(1, deferredPassFBDesc, deferredPassTextureDesc);

	initializeTerrainPassShaders();
}

void GLRenderingSystemNS::initializeTerrainPassShaders()
{
	// shader programs and shaders
	auto rhs = addGLShaderProgramComponent(0);

	initializeGLShaderProgramComponent(rhs, GLTerrainRenderPassComponent::get().m_shaderFilePaths);

	bindTerrainPassUniformLocations(rhs);

	GLTerrainRenderPassComponent::get().m_GLSPC = rhs;
}

void GLRenderingSystemNS::bindTerrainPassUniformLocations(GLShaderProgramComponent* rhs)
{
	GLTerrainRenderPassComponent::get().m_terrainPass_uni_p_camera = getUniformLocation(
		rhs->m_program,
		"uni_p_camera");
	GLTerrainRenderPassComponent::get().m_terrainPass_uni_r_camera = getUniformLocation(
		rhs->m_program,
		"uni_r_camera");
	GLTerrainRenderPassComponent::get().m_terrainPass_uni_t_camera = getUniformLocation(
		rhs->m_program,
		"uni_t_camera");
	GLTerrainRenderPassComponent::get().m_terrainPass_uni_m = getUniformLocation(
		rhs->m_program,
		"uni_m");
	GLTerrainRenderPassComponent::get().m_terrainPass_uni_albedoTexture = getUniformLocation(
		rhs->m_program,
		"uni_albedoTexture");
	updateUniform(
		GLTerrainRenderPassComponent::get().m_terrainPass_uni_albedoTexture,
		0);
}

void GLRenderingSystemNS::initializeLightPass()
{
	GLLightRenderPassComponent::get().m_GLRPC = addGLRenderPassComponent(1, deferredPassFBDesc, deferredPassTextureDesc);

	initializeLightPassShaders();
}

void GLRenderingSystemNS::initializeLightPassShaders()
{
	// shader programs and shaders
	auto l_GLSPC = addGLShaderProgramComponent(0);

	initializeGLShaderProgramComponent(l_GLSPC, GLLightRenderPassComponent::get().m_shaderFilePaths);

	bindLightPassUniformLocations(l_GLSPC);

	GLLightRenderPassComponent::get().m_GLSPC = l_GLSPC;
}

void GLRenderingSystemNS::bindLightPassUniformLocations(GLShaderProgramComponent* rhs)
{
	updateTextureUniformLocations(rhs->m_program, GLLightRenderPassComponent::get().m_textureUniformNames);

	for (size_t i = 0; i < 4; i++)
	{
		std::stringstream ss;
		ss << i;
		GLLightRenderPassComponent::get().m_uni_shadowSplitAreas.emplace_back(
			getUniformLocation(rhs->m_program, "uni_shadowSplitAreas[" + ss.str() + "]")
		);
	}
	GLLightRenderPassComponent::get().m_uni_viewPos = getUniformLocation(
		rhs->m_program,
		"uni_viewPos");
	GLLightRenderPassComponent::get().m_uni_dirLight_direction = getUniformLocation(
		rhs->m_program,
		"uni_dirLight.direction");
	GLLightRenderPassComponent::get().m_uni_dirLight_color = getUniformLocation(
		rhs->m_program,
		"uni_dirLight.color");

	for (auto i = (unsigned int)0; i < GLRenderingSystemNS::g_GameSystemComponent->m_PointLightComponents.size(); i++)
	{
		std::stringstream ss;
		ss << i;
		GLLightRenderPassComponent::get().m_uni_pointLights_position.emplace_back(
			getUniformLocation(rhs->m_program, "uni_pointLights[" + ss.str() + "].position")
		);
		GLLightRenderPassComponent::get().m_uni_pointLights_attenuationRadius.emplace_back(
			getUniformLocation(rhs->m_program, "uni_pointLights[" + ss.str() + "].attenuationRadius")
		);
		GLLightRenderPassComponent::get().m_uni_pointLights_luminance.emplace_back(
			getUniformLocation(rhs->m_program, "uni_pointLights[" + ss.str() + "].luminance")
		);
	}

	for (auto i = (unsigned int)0; i < GLRenderingSystemNS::g_GameSystemComponent->m_SphereLightComponents.size(); i++)
	{
		std::stringstream ss;
		ss << i;
		GLLightRenderPassComponent::get().m_uni_sphereLights_position.emplace_back(
			getUniformLocation(rhs->m_program, "uni_sphereLights[" + ss.str() + "].position")
		);
		GLLightRenderPassComponent::get().m_uni_sphereLights_sphereRadius.emplace_back(
			getUniformLocation(rhs->m_program, "uni_sphereLights[" + ss.str() + "].sphereRadius")
		);
		GLLightRenderPassComponent::get().m_uni_sphereLights_luminance.emplace_back(
			getUniformLocation(rhs->m_program, "uni_sphereLights[" + ss.str() + "].luminance")
		);
	}

	GLLightRenderPassComponent::get().m_uni_isEmissive = getUniformLocation(
		rhs->m_program,
		"uni_isEmissive");
}

void GLRenderingSystemNS::initializeFinalPass()
{
	initializeSkyPass();
	initializeTAAPass();
	initializeBloomExtractPass();
	initializeBloomBlurPass();
	initializeMotionBlurPass();
	initializeBillboardPass();
	initializeDebuggerPass();
	initializeFinalBlendPass();
}

void GLRenderingSystemNS::initializeSkyPass()
{
	GLFinalRenderPassComponent::get().m_skyPassGLRPC = addGLRenderPassComponent(1, deferredPassFBDesc, deferredPassTextureDesc);

	// shader programs and shaders
	auto rhs = addGLShaderProgramComponent(0); 
	
	initializeGLShaderProgramComponent(rhs, GLFinalRenderPassComponent::get().m_skyPassShaderFilePaths);

	GLFinalRenderPassComponent::get().m_skyPass_uni_p = getUniformLocation(
		rhs->m_program,
		"uni_p");
	GLFinalRenderPassComponent::get().m_skyPass_uni_r = getUniformLocation(
		rhs->m_program,
		"uni_r");
	GLFinalRenderPassComponent::get().m_skyPass_uni_viewportSize = getUniformLocation(
		rhs->m_program,
		"uni_viewportSize");
	GLFinalRenderPassComponent::get().m_skyPass_uni_eyePos = getUniformLocation(
		rhs->m_program,
		"uni_eyePos");
	GLFinalRenderPassComponent::get().m_skyPass_uni_lightDir = getUniformLocation(
		rhs->m_program,
		"uni_lightDir");

	GLFinalRenderPassComponent::get().m_skyPassSPC = rhs;
}

void GLRenderingSystemNS::initializeTAAPass()
{
	// pre mix pass
	GLFinalRenderPassComponent::get().m_preTAAPassGLRPC = addGLRenderPassComponent(1, deferredPassFBDesc, deferredPassTextureDesc);

	// Ping pass
	GLFinalRenderPassComponent::get().m_TAAPingPassGLRPC = addGLRenderPassComponent(1, deferredPassFBDesc, deferredPassTextureDesc);

	// Pong pass
	GLFinalRenderPassComponent::get().m_TAAPongPassGLRPC = addGLRenderPassComponent(1, deferredPassFBDesc, deferredPassTextureDesc);

	// Sharpen pass
	GLFinalRenderPassComponent::get().m_TAASharpenPassGLRPC = addGLRenderPassComponent(1, deferredPassFBDesc, deferredPassTextureDesc);

	// shader programs and shaders
	// pre mix pass
	auto rhs = addGLShaderProgramComponent(0); 
	
	initializeGLShaderProgramComponent(rhs, GLFinalRenderPassComponent::get().m_preTAAPassShaderFilePaths);

	updateTextureUniformLocations(rhs->m_program, GLFinalRenderPassComponent::get().m_preTAAPassUniformNames);

	GLFinalRenderPassComponent::get().m_preTAAPassSPC = rhs;

	// TAA pass	
	rhs = addGLShaderProgramComponent(0); 
	
	initializeGLShaderProgramComponent(rhs, GLFinalRenderPassComponent::get().m_TAAPassShaderFilePaths);

	updateTextureUniformLocations(rhs->m_program, GLFinalRenderPassComponent::get().m_TAAPassUniformNames);

	GLFinalRenderPassComponent::get().m_TAAPass_uni_renderTargetSize = getUniformLocation(
		rhs->m_program,
		"uni_renderTargetSize");

	GLFinalRenderPassComponent::get().m_TAAPassSPC = rhs;

	// Sharpen pass
	rhs = addGLShaderProgramComponent(0); 
	
	initializeGLShaderProgramComponent(rhs, GLFinalRenderPassComponent::get().m_TAASharpenPassShaderFilePaths);

	updateTextureUniformLocations(rhs->m_program, GLFinalRenderPassComponent::get().m_TAASharpenPassUniformNames);

	GLFinalRenderPassComponent::get().m_TAASharpenPass_uni_renderTargetSize = getUniformLocation(
		rhs->m_program,
		"uni_renderTargetSize");

	GLFinalRenderPassComponent::get().m_TAASharpenPassSPC = rhs;
}

void GLRenderingSystemNS::initializeBloomExtractPass()
{
	GLFinalRenderPassComponent::get().m_bloomExtractPassGLRPC = addGLRenderPassComponent(1, deferredPassFBDesc, deferredPassTextureDesc);

	// shader programs and shaders
	ShaderFilePaths m_ShaderFilePaths;

	m_ShaderFilePaths.m_VSPath = "GL4.0//bloomExtractPassVertex.sf";
	m_ShaderFilePaths.m_FSPath = "GL4.0//bloomExtractPassFragment.sf";

	auto rhs = addGLShaderProgramComponent(0); initializeGLShaderProgramComponent(rhs, m_ShaderFilePaths);

	GLFinalRenderPassComponent::get().m_bloomExtractPass_uni_TAAPassRT0 = getUniformLocation(
		rhs->m_program,
		"uni_TAAPassRT0");
	updateUniform(
		GLFinalRenderPassComponent::get().m_bloomExtractPass_uni_TAAPassRT0,
		0);

	GLFinalRenderPassComponent::get().m_bloomExtractPassSPC = rhs;
}

void GLRenderingSystemNS::initializeBloomBlurPass()
{
	//Ping pass
	GLFinalRenderPassComponent::get().m_bloomBlurPingPassGLRPC = addGLRenderPassComponent(1, deferredPassFBDesc, deferredPassTextureDesc);

	//Pong pass
	GLFinalRenderPassComponent::get().m_bloomBlurPongPassGLRPC = addGLRenderPassComponent(1, deferredPassFBDesc, deferredPassTextureDesc);

	// shader programs and shaders
	ShaderFilePaths m_ShaderFilePaths;

	m_ShaderFilePaths.m_VSPath = "GL4.0//bloomBlurPassVertex.sf";
	m_ShaderFilePaths.m_FSPath = "GL4.0//bloomBlurPassFragment.sf";

	auto rhs = addGLShaderProgramComponent(0); initializeGLShaderProgramComponent(rhs, m_ShaderFilePaths);

	GLFinalRenderPassComponent::get().m_bloomBlurPass_uni_bloomExtractPassRT0 = getUniformLocation(
		rhs->m_program,
		"uni_bloomExtractPassRT0");
	updateUniform(
		GLFinalRenderPassComponent::get().m_bloomBlurPass_uni_bloomExtractPassRT0,
		0);
	GLFinalRenderPassComponent::get().m_bloomBlurPass_uni_horizontal = getUniformLocation(
		rhs->m_program,
		"uni_horizontal");

	GLFinalRenderPassComponent::get().m_bloomBlurPassSPC = rhs;
}

void GLRenderingSystemNS::initializeMotionBlurPass()
{
	GLFinalRenderPassComponent::get().m_motionBlurPassGLRPC = addGLRenderPassComponent(1, deferredPassFBDesc, deferredPassTextureDesc);

	// shader programs and shaders
	ShaderFilePaths m_ShaderFilePaths;

	m_ShaderFilePaths.m_VSPath = "GL4.0//motionBlurPassVertex.sf";
	m_ShaderFilePaths.m_FSPath = "GL4.0//motionBlurPassFragment.sf";

	auto rhs = addGLShaderProgramComponent(0); initializeGLShaderProgramComponent(rhs, m_ShaderFilePaths);

	GLFinalRenderPassComponent::get().m_motionBlurPass_uni_motionVectorTexture = getUniformLocation(
		rhs->m_program,
		"uni_motionVectorTexture");
	updateUniform(
		GLFinalRenderPassComponent::get().m_motionBlurPass_uni_motionVectorTexture,
		0);
	GLFinalRenderPassComponent::get().m_motionBlurPass_uni_TAAPassRT0 = getUniformLocation(
		rhs->m_program,
		"uni_TAAPassRT0");
	updateUniform(
		GLFinalRenderPassComponent::get().m_motionBlurPass_uni_TAAPassRT0,
		1);

	GLFinalRenderPassComponent::get().m_motionBlurPassSPC = rhs;
}

void GLRenderingSystemNS::initializeBillboardPass()
{
	GLFinalRenderPassComponent::get().m_billboardPassGLRPC = addGLRenderPassComponent(1, deferredPassFBDesc, deferredPassTextureDesc);

	// shader programs and shaders
	ShaderFilePaths m_ShaderFilePaths;

	m_ShaderFilePaths.m_VSPath = "GL4.0//billboardPassVertex.sf";
	m_ShaderFilePaths.m_FSPath = "GL4.0//billboardPassFragment.sf";

	auto rhs = addGLShaderProgramComponent(0); initializeGLShaderProgramComponent(rhs, m_ShaderFilePaths);

	GLFinalRenderPassComponent::get().m_billboardPass_uni_texture = getUniformLocation(
		rhs->m_program,
		"uni_texture");
	updateUniform(
		GLFinalRenderPassComponent::get().m_billboardPass_uni_texture,
		0);
	GLFinalRenderPassComponent::get().m_billboardPass_uni_p = getUniformLocation(
		rhs->m_program,
		"uni_p");
	GLFinalRenderPassComponent::get().m_billboardPass_uni_r = getUniformLocation(
		rhs->m_program,
		"uni_r");
	GLFinalRenderPassComponent::get().m_billboardPass_uni_t = getUniformLocation(
		rhs->m_program,
		"uni_t");
	GLFinalRenderPassComponent::get().m_billboardPass_uni_pos = getUniformLocation(
		rhs->m_program,
		"uni_pos");
	GLFinalRenderPassComponent::get().m_billboardPass_uni_albedo = getUniformLocation(
		rhs->m_program,
		"uni_albedo");
	GLFinalRenderPassComponent::get().m_billboardPass_uni_size = getUniformLocation(
		rhs->m_program,
		"uni_size");

	GLFinalRenderPassComponent::get().m_billboardPassSPC = rhs;
}

void GLRenderingSystemNS::initializeDebuggerPass()
{
	GLFinalRenderPassComponent::get().m_debuggerPassGLRPC = addGLRenderPassComponent(1, deferredPassFBDesc, deferredPassTextureDesc);

	// shader programs and shaders
	ShaderFilePaths m_ShaderFilePaths;

	//m_ShaderFilePaths.m_VSPath = "GL4.0//debuggerPassVertex.sf";
	//m_ShaderFilePaths.m_GSPath = "GL4.0//debuggerPassGeometry.sf";
	//m_ShaderFilePaths.m_FSPath = "GL4.0//debuggerPassFragment.sf";

	m_ShaderFilePaths.m_VSPath = "GL4.0//wireframeOverlayPassVertex.sf";
	m_ShaderFilePaths.m_FSPath = "GL4.0//wireframeOverlayPassFragment.sf";

	auto rhs = addGLShaderProgramComponent(0); initializeGLShaderProgramComponent(rhs, m_ShaderFilePaths);

	//GLFinalRenderPassComponent::get().m_debuggerPass_uni_normalTexture = getUniformLocation(
	//	rhs->m_program,
	//	"uni_normalTexture");
	//updateUniform(
	//	GLFinalRenderPassComponent::get().m_debuggerPass_uni_normalTexture,
	//	0);
	GLFinalRenderPassComponent::get().m_debuggerPass_uni_p = getUniformLocation(
		rhs->m_program,
		"uni_p");
	GLFinalRenderPassComponent::get().m_debuggerPass_uni_r = getUniformLocation(
		rhs->m_program,
		"uni_r");
	GLFinalRenderPassComponent::get().m_debuggerPass_uni_t = getUniformLocation(
		rhs->m_program,
		"uni_t");
	GLFinalRenderPassComponent::get().m_debuggerPass_uni_m = getUniformLocation(
		rhs->m_program,
		"uni_m");

	GLFinalRenderPassComponent::get().m_debuggerPassSPC = rhs;
}

void GLRenderingSystemNS::initializeFinalBlendPass()
{
	GLFinalRenderPassComponent::get().m_finalBlendPassGLRPC = addGLRenderPassComponent(1, deferredPassFBDesc, deferredPassTextureDesc);

	// shader programs and shaders
	ShaderFilePaths m_ShaderFilePaths;

	m_ShaderFilePaths.m_VSPath = "GL4.0//finalBlendPassVertex.sf";
	m_ShaderFilePaths.m_FSPath = "GL4.0//finalBlendPassFragment.sf";

	auto rhs = addGLShaderProgramComponent(0); initializeGLShaderProgramComponent(rhs, m_ShaderFilePaths);

	GLFinalRenderPassComponent::get().m_uni_motionBlurPassRT0 = getUniformLocation(
		rhs->m_program,
		"uni_motionBlurPassRT0");
	updateUniform(
		GLFinalRenderPassComponent::get().m_uni_motionBlurPassRT0,
		0);
	GLFinalRenderPassComponent::get().m_uni_bloomPassRT0 = getUniformLocation(
		rhs->m_program,
		"uni_bloomPassRT0");
	updateUniform(
		GLFinalRenderPassComponent::get().m_uni_bloomPassRT0,
		1);
	GLFinalRenderPassComponent::get().m_uni_billboardPassRT0 = getUniformLocation(
		rhs->m_program,
		"uni_billboardPassRT0");
	updateUniform(
		GLFinalRenderPassComponent::get().m_uni_billboardPassRT0,
		2);
	GLFinalRenderPassComponent::get().m_uni_debuggerPassRT0 = getUniformLocation(
		rhs->m_program,
		"uni_debuggerPassRT0");
	updateUniform(
		GLFinalRenderPassComponent::get().m_uni_debuggerPassRT0,
		3);
	GLFinalRenderPassComponent::get().m_finalBlendPassSPC = rhs;
}

GLShaderProgramComponent * GLRenderingSystemNS::addGLShaderProgramComponent(EntityID rhs)
{
	GLShaderProgramComponent* newShaderProgram = g_pCoreSystem->getMemorySystem()->spawn<GLShaderProgramComponent>();
	newShaderProgram->m_parentEntity = rhs;
	return newShaderProgram;
}

GLMeshDataComponent * GLRenderingSystemNS::addGLMeshDataComponent(EntityID rhs)
{
	GLMeshDataComponent* newMesh = g_pCoreSystem->getMemorySystem()->spawn<GLMeshDataComponent>();
	newMesh->m_parentEntity = rhs;
	auto l_meshMap = &GLRenderingSystemNS::g_GLRenderingSystemComponent->m_meshMap;
	l_meshMap->emplace(std::pair<EntityID, GLMeshDataComponent*>(rhs, newMesh));
	return newMesh;
}

GLTextureDataComponent * GLRenderingSystemNS::addGLTextureDataComponent(EntityID rhs)
{
	GLTextureDataComponent* newTexture = g_pCoreSystem->getMemorySystem()->spawn<GLTextureDataComponent>();
	newTexture->m_parentEntity = rhs;
	auto l_textureMap = &GLRenderingSystemNS::g_GLRenderingSystemComponent->m_textureMap;
	l_textureMap->emplace(std::pair<EntityID, GLTextureDataComponent*>(rhs, newTexture));
	return newTexture;
}

GLMeshDataComponent * GLRenderingSystemNS::getGLMeshDataComponent(EntityID rhs)
{
	auto result = GLRenderingSystemNS::g_GLRenderingSystemComponent->m_meshMap.find(rhs);
	if (result != GLRenderingSystemNS::g_GLRenderingSystemComponent->m_meshMap.end())
	{
		return result->second;
	}
	else
	{
		return nullptr;
	}
}

GLTextureDataComponent * GLRenderingSystemNS::getGLTextureDataComponent(EntityID rhs)
{
	auto result = GLRenderingSystemNS::g_GLRenderingSystemComponent->m_textureMap.find(rhs);
	if (result != GLRenderingSystemNS::g_GLRenderingSystemComponent->m_textureMap.end())
	{
		return result->second;
	}
	else
	{
		return nullptr;
	}
}

GLMeshDataComponent* GLRenderingSystemNS::generateGLMeshDataComponent(MeshDataComponent* rhs)
{
	if (rhs->m_objectStatus == ObjectStatus::ALIVE)
	{
		return getGLMeshDataComponent(rhs->m_parentEntity);
	}
	else
	{
		auto l_ptr = addGLMeshDataComponent(rhs->m_parentEntity);

		initializeGLMeshDataComponent(l_ptr, rhs->m_vertices, rhs->m_indices);

		rhs->m_objectStatus = ObjectStatus::ALIVE;

		return l_ptr;
	}
}

GLTextureDataComponent* GLRenderingSystemNS::generateGLTextureDataComponent(TextureDataComponent * rhs)
{
	if (rhs->m_objectStatus == ObjectStatus::ALIVE)
	{
		return getGLTextureDataComponent(rhs->m_parentEntity);
	}
	else
	{
		if (rhs->m_textureDataDesc.textureUsageType == TextureUsageType::INVISIBLE)
		{
			return nullptr;
		}
		else
		{
			auto l_ptr = addGLTextureDataComponent(rhs->m_parentEntity);

			initializeGLTextureDataComponent(l_ptr, rhs->m_textureDataDesc, rhs->m_textureData);

			rhs->m_objectStatus = ObjectStatus::ALIVE;

			return l_ptr;
		}
	}
}

bool GLRenderingSystemNS::initializeGLShaderProgramComponent(GLShaderProgramComponent* rhs, const ShaderFilePaths& ShaderFilePaths)
{
	rhs->m_program = glCreateProgram();

	std::function<void(GLuint& shaderProgram, GLuint& shaderID, GLuint ShaderType, const std::string & shaderFilePath)> f_addShader =
		[&](GLuint& shaderProgram, GLuint& shaderID, GLuint shaderType, const std::string & shaderFilePath) {
		shaderID = glCreateShader(shaderType);

		if (shaderID == 0) {
			g_pCoreSystem->getLogSystem()->printLog(LogType::INNO_ERROR, "GLRenderingSystem: innoShader: Shader creation failed! memory location invaild when adding shader!");
			return;
		}

		auto l_shaderCodeContent = g_pCoreSystem->getAssetSystem()->loadShader(shaderFilePath);
		const char* l_sourcePointer = l_shaderCodeContent.c_str();

		if (l_sourcePointer == nullptr || l_shaderCodeContent.empty())
		{
			g_pCoreSystem->getLogSystem()->printLog(LogType::INNO_ERROR, "GLRenderingSystem: innoShader: " + shaderFilePath + " loading failed!");
			return;
		}

		// compile shader
		g_pCoreSystem->getLogSystem()->printLog(LogType::INNO_DEV_VERBOSE, "GLRenderingSystem: innoShader: " + shaderFilePath + " is compiling ...");

		glShaderSource(shaderID, 1, &l_sourcePointer, NULL);
		glCompileShader(shaderID);

		GLint l_validationResult = GL_FALSE;
		GLint l_infoLogLength = 0;
		GLint l_shaderFileLength = 0;
		glGetShaderiv(shaderID, GL_COMPILE_STATUS, &l_validationResult);

		if (!l_validationResult)
		{
			g_pCoreSystem->getLogSystem()->printLog(LogType::INNO_ERROR, "GLRenderingSystem: innoShader: " + shaderFilePath + " compile failed!");
			glGetShaderiv(shaderID, GL_SHADER_SOURCE_LENGTH, &l_shaderFileLength);
			g_pCoreSystem->getLogSystem()->printLog(LogType::INNO_DEV_VERBOSE, "GLRenderingSystem: innoShader: " + shaderFilePath + " file length is: " + std::to_string(l_shaderFileLength));
			glGetShaderiv(shaderID, GL_INFO_LOG_LENGTH, &l_infoLogLength);

			if (l_infoLogLength > 0) {
				std::vector<char> l_shaderErrorMessage(l_infoLogLength + 1);
				glGetShaderInfoLog(shaderID, l_infoLogLength, NULL, &l_shaderErrorMessage[0]);
				g_pCoreSystem->getLogSystem()->printLog(LogType::INNO_DEV_VERBOSE, "GLRenderingSystem: innoShader: " + shaderFilePath + " compile error: " + &l_shaderErrorMessage[0] + "\n -- --------------------------------------------------- -- ");
				return;
			}

			return;
		}

		g_pCoreSystem->getLogSystem()->printLog(LogType::INNO_DEV_SUCCESS, "GLRenderingSystem: innoShader: " + shaderFilePath + " Shader has been compiled.");

		// Link shader to program
		g_pCoreSystem->getLogSystem()->printLog(LogType::INNO_DEV_VERBOSE, "GLRenderingSystem: innoShader: " + shaderFilePath + " is linking ...");

		glAttachShader(shaderProgram, shaderID);
		glLinkProgram(shaderProgram);
		glValidateProgram(shaderProgram);

		glGetProgramiv(shaderProgram, GL_LINK_STATUS, &l_validationResult);
		if (!l_validationResult)
		{
			g_pCoreSystem->getLogSystem()->printLog(LogType::INNO_ERROR, "GLRenderingSystem: innoShader: " + shaderFilePath + " link failed!");
			glGetProgramiv(shaderProgram, GL_INFO_LOG_LENGTH, &l_infoLogLength);

			if (l_infoLogLength > 0) {
				std::vector<char> l_shaderErrorMessage(l_infoLogLength + 1);
				glGetProgramInfoLog(shaderProgram, l_infoLogLength, NULL, &l_shaderErrorMessage[0]);
				g_pCoreSystem->getLogSystem()->printLog(LogType::INNO_DEV_VERBOSE, "GLRenderingSystem: innoShader: " + shaderFilePath + " link error: " + &l_shaderErrorMessage[0] + "\n -- --------------------------------------------------- -- ");
				return;
			}

			return;
		}

		g_pCoreSystem->getLogSystem()->printLog(LogType::INNO_DEV_SUCCESS, "GLRenderingSystem: innoShader: " + shaderFilePath + " Shader has been linked.");
	};

	if (!ShaderFilePaths.m_VSPath.empty())
	{
		f_addShader(rhs->m_program, rhs->m_VSID, GL_VERTEX_SHADER, ShaderFilePaths.m_VSPath);
	}
	if (!ShaderFilePaths.m_GSPath.empty())
	{
		f_addShader(rhs->m_program, rhs->m_GSID, GL_GEOMETRY_SHADER, ShaderFilePaths.m_GSPath);
	}
	if (!ShaderFilePaths.m_FSPath.empty())
	{
		f_addShader(rhs->m_program, rhs->m_FSID, GL_FRAGMENT_SHADER, ShaderFilePaths.m_FSPath);
	}

	rhs->m_objectStatus = ObjectStatus::ALIVE;
	return rhs;
}

bool GLRenderingSystemNS::initializeGLMeshDataComponent(GLMeshDataComponent * rhs, const std::vector<Vertex>& vertices, const std::vector<Index>& indices)
{
	std::vector<float> l_verticesBuffer;
	auto l_containerSize = vertices.size() * 8;
	l_verticesBuffer.reserve(l_containerSize);

	std::for_each(vertices.begin(), vertices.end(), [&](Vertex val)
	{
		l_verticesBuffer.emplace_back((float)val.m_pos.x);
		l_verticesBuffer.emplace_back((float)val.m_pos.y);
		l_verticesBuffer.emplace_back((float)val.m_pos.z);
		l_verticesBuffer.emplace_back((float)val.m_texCoord.x);
		l_verticesBuffer.emplace_back((float)val.m_texCoord.y);
		l_verticesBuffer.emplace_back((float)val.m_normal.x);
		l_verticesBuffer.emplace_back((float)val.m_normal.y);
		l_verticesBuffer.emplace_back((float)val.m_normal.z);
	});

	glGenVertexArrays(1, &rhs->m_VAO);
	glGenBuffers(1, &rhs->m_VBO);
	glGenBuffers(1, &rhs->m_IBO);

	glBindVertexArray(rhs->m_VAO);

	glBindBuffer(GL_ARRAY_BUFFER, rhs->m_VBO);
	glBufferData(GL_ARRAY_BUFFER, l_verticesBuffer.size() * sizeof(float), &l_verticesBuffer[0], GL_STATIC_DRAW);

	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, rhs->m_IBO);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices.size() * sizeof(unsigned int), &indices[0], GL_STATIC_DRAW);

	// position attribute, 1st attribution with 3 * sizeof(float) bits of data
	glEnableVertexAttribArray(0);
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)0);

	// texture attribute, 2nd attribution with 2 * sizeof(float) bits of data
	glEnableVertexAttribArray(1);
	glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(3 * sizeof(float)));

	// normal coord attribute, 3rd attribution with 3 * sizeof(float) bits of data
	glEnableVertexAttribArray(2);
	glVertexAttribPointer(2, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(5 * sizeof(float)));

	rhs->m_objectStatus = ObjectStatus::ALIVE;

	m_initializedGLMDC.emplace(rhs->m_parentEntity, rhs);

	g_pCoreSystem->getLogSystem()->printLog(LogType::INNO_DEV_VERBOSE, "GLRenderingSystem: VAO " + std::to_string(rhs->m_VAO) + " is initialized.");

	return true;
}

bool GLRenderingSystemNS::initializeGLTextureDataComponent(GLTextureDataComponent * rhs, TextureDataDesc textureDataDesc, const std::vector<void*>& textureData)
{
	// texture type
	if (textureDataDesc.textureUsageType == TextureUsageType::ENVIRONMENT_CAPTURE || textureDataDesc.textureUsageType == TextureUsageType::ENVIRONMENT_CONVOLUTION || textureDataDesc.textureUsageType == TextureUsageType::ENVIRONMENT_PREFILTER)
	{
		rhs->m_GLTextureDataDesc.textureType = GL_TEXTURE_CUBE_MAP;
	}
	else
	{
		rhs->m_GLTextureDataDesc.textureType = GL_TEXTURE_2D;
	}

	// set the texture wrapping parameters
	switch (textureDataDesc.textureWrapMethod)
	{
	case TextureWrapMethod::CLAMP_TO_EDGE: rhs->m_GLTextureDataDesc.textureWrapMethod = GL_CLAMP_TO_EDGE; break;
	case TextureWrapMethod::REPEAT: rhs->m_GLTextureDataDesc.textureWrapMethod = GL_REPEAT; break;
	case TextureWrapMethod::CLAMP_TO_BORDER: rhs->m_GLTextureDataDesc.textureWrapMethod = GL_CLAMP_TO_BORDER; break;
	}

	// set texture filtering parameters
	switch (textureDataDesc.textureMinFilterMethod)
	{
	case TextureFilterMethod::NEAREST: rhs->m_GLTextureDataDesc.minFilterParam = GL_NEAREST; break;
	case TextureFilterMethod::LINEAR: rhs->m_GLTextureDataDesc.minFilterParam = GL_LINEAR; break;
	case TextureFilterMethod::LINEAR_MIPMAP_LINEAR: rhs->m_GLTextureDataDesc.minFilterParam = GL_LINEAR_MIPMAP_LINEAR; break;
	}
	switch (textureDataDesc.textureMagFilterMethod)
	{
	case TextureFilterMethod::NEAREST: rhs->m_GLTextureDataDesc.magFilterParam = GL_NEAREST; break;
	case TextureFilterMethod::LINEAR: rhs->m_GLTextureDataDesc.magFilterParam = GL_LINEAR; break;
	case TextureFilterMethod::LINEAR_MIPMAP_LINEAR: rhs->m_GLTextureDataDesc.magFilterParam = GL_LINEAR_MIPMAP_LINEAR; break;
	}

	// set texture formats
	if (textureDataDesc.textureUsageType == TextureUsageType::ALBEDO)
	{
		if (textureDataDesc.texturePixelDataFormat == TexturePixelDataFormat::RGB)
		{
			rhs->m_GLTextureDataDesc.internalFormat = GL_SRGB;
		}
		else if (textureDataDesc.texturePixelDataFormat == TexturePixelDataFormat::RGBA)
		{
			rhs->m_GLTextureDataDesc.internalFormat = GL_SRGB_ALPHA;
		}
	}
	else
	{
		switch (textureDataDesc.textureColorComponentsFormat)
		{
		case TextureColorComponentsFormat::RED: rhs->m_GLTextureDataDesc.internalFormat = GL_RED; break;
		case TextureColorComponentsFormat::RG: rhs->m_GLTextureDataDesc.internalFormat = GL_RG; break;
		case TextureColorComponentsFormat::RGB: rhs->m_GLTextureDataDesc.internalFormat = GL_RGB; break;
		case TextureColorComponentsFormat::RGBA: rhs->m_GLTextureDataDesc.internalFormat = GL_RGBA; break;
		case TextureColorComponentsFormat::R8: rhs->m_GLTextureDataDesc.internalFormat = GL_R8; break;
		case TextureColorComponentsFormat::RG8: rhs->m_GLTextureDataDesc.internalFormat = GL_RG8; break;
		case TextureColorComponentsFormat::RGB8: rhs->m_GLTextureDataDesc.internalFormat = GL_RGB8; break;
		case TextureColorComponentsFormat::RGBA8: rhs->m_GLTextureDataDesc.internalFormat = GL_RGBA8; break;
		case TextureColorComponentsFormat::R16: rhs->m_GLTextureDataDesc.internalFormat = GL_R16; break;
		case TextureColorComponentsFormat::RG16: rhs->m_GLTextureDataDesc.internalFormat = GL_RG16; break;
		case TextureColorComponentsFormat::RGB16: rhs->m_GLTextureDataDesc.internalFormat = GL_RGB16; break;
		case TextureColorComponentsFormat::RGBA16: rhs->m_GLTextureDataDesc.internalFormat = GL_RGBA16; break;
		case TextureColorComponentsFormat::R16F: rhs->m_GLTextureDataDesc.internalFormat = GL_R16F; break;
		case TextureColorComponentsFormat::RG16F: rhs->m_GLTextureDataDesc.internalFormat = GL_RG16F; break;
		case TextureColorComponentsFormat::RGB16F: rhs->m_GLTextureDataDesc.internalFormat = GL_RGB16F; break;
		case TextureColorComponentsFormat::RGBA16F: rhs->m_GLTextureDataDesc.internalFormat = GL_RGBA16F; break;
		case TextureColorComponentsFormat::R32F: rhs->m_GLTextureDataDesc.internalFormat = GL_R32F; break;
		case TextureColorComponentsFormat::RG32F: rhs->m_GLTextureDataDesc.internalFormat = GL_RG32F; break;
		case TextureColorComponentsFormat::RGB32F: rhs->m_GLTextureDataDesc.internalFormat = GL_RGB32F; break;
		case TextureColorComponentsFormat::RGBA32F: rhs->m_GLTextureDataDesc.internalFormat = GL_RGBA32F; break;
		case TextureColorComponentsFormat::SRGB: rhs->m_GLTextureDataDesc.internalFormat = GL_SRGB; break;
		case TextureColorComponentsFormat::SRGBA: rhs->m_GLTextureDataDesc.internalFormat = GL_SRGB_ALPHA; break;
		case TextureColorComponentsFormat::SRGB8: rhs->m_GLTextureDataDesc.internalFormat = GL_SRGB8; break;
		case TextureColorComponentsFormat::SRGBA8: rhs->m_GLTextureDataDesc.internalFormat = GL_SRGB8_ALPHA8; break;
		case TextureColorComponentsFormat::DEPTH_COMPONENT: rhs->m_GLTextureDataDesc.internalFormat = GL_DEPTH_COMPONENT; break;
		}
	}
	switch (textureDataDesc.texturePixelDataFormat)
	{
	case TexturePixelDataFormat::RED:rhs->m_GLTextureDataDesc.pixelDataFormat = GL_RED; break;
	case TexturePixelDataFormat::RG:rhs->m_GLTextureDataDesc.pixelDataFormat = GL_RG; break;
	case TexturePixelDataFormat::RGB:rhs->m_GLTextureDataDesc.pixelDataFormat = GL_RGB; break;
	case TexturePixelDataFormat::RGBA:rhs->m_GLTextureDataDesc.pixelDataFormat = GL_RGBA; break;
	case TexturePixelDataFormat::DEPTH_COMPONENT:rhs->m_GLTextureDataDesc.pixelDataFormat = GL_DEPTH_COMPONENT; break;
	}
	switch (textureDataDesc.texturePixelDataType)
	{
	case TexturePixelDataType::UNSIGNED_BYTE:rhs->m_GLTextureDataDesc.pixelDataType = GL_UNSIGNED_BYTE; break;
	case TexturePixelDataType::BYTE:rhs->m_GLTextureDataDesc.pixelDataType = GL_BYTE; break;
	case TexturePixelDataType::UNSIGNED_SHORT:rhs->m_GLTextureDataDesc.pixelDataType = GL_UNSIGNED_SHORT; break;
	case TexturePixelDataType::SHORT:rhs->m_GLTextureDataDesc.pixelDataType = GL_SHORT; break;
	case TexturePixelDataType::UNSIGNED_INT:rhs->m_GLTextureDataDesc.pixelDataType = GL_UNSIGNED_INT; break;
	case TexturePixelDataType::INT:rhs->m_GLTextureDataDesc.pixelDataType = GL_INT; break;
	case TexturePixelDataType::FLOAT:rhs->m_GLTextureDataDesc.pixelDataType = GL_FLOAT; break;
	case TexturePixelDataType::DOUBLE:rhs->m_GLTextureDataDesc.pixelDataType = GL_FLOAT; break;
	}

	//generate and bind texture object
	glGenTextures(1, &rhs->m_TAO);

	glBindTexture(rhs->m_GLTextureDataDesc.textureType, rhs->m_TAO);

	if (rhs->m_GLTextureDataDesc.textureType == GL_TEXTURE_CUBE_MAP)
	{
		glTexParameteri(rhs->m_GLTextureDataDesc.textureType, GL_TEXTURE_WRAP_R, rhs->m_GLTextureDataDesc.textureWrapMethod);
	}
	glTexParameteri(rhs->m_GLTextureDataDesc.textureType, GL_TEXTURE_WRAP_S, rhs->m_GLTextureDataDesc.textureWrapMethod);
	glTexParameteri(rhs->m_GLTextureDataDesc.textureType, GL_TEXTURE_WRAP_T, rhs->m_GLTextureDataDesc.textureWrapMethod);

	if (textureDataDesc.textureUsageType == TextureUsageType::SHADOWMAP)
	{
		float borderColor[] = { 1.0f, 1.0f, 1.0f, 1.0f };
		glTexParameterfv(rhs->m_GLTextureDataDesc.textureType, GL_TEXTURE_BORDER_COLOR, borderColor);
	}

	glTexParameteri(rhs->m_GLTextureDataDesc.textureType, GL_TEXTURE_MIN_FILTER, rhs->m_GLTextureDataDesc.minFilterParam);
	glTexParameteri(rhs->m_GLTextureDataDesc.textureType, GL_TEXTURE_MAG_FILTER, rhs->m_GLTextureDataDesc.magFilterParam);

	if (rhs->m_GLTextureDataDesc.textureType == GL_TEXTURE_CUBE_MAP)
	{
		glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X, 0, rhs->m_GLTextureDataDesc.internalFormat, textureDataDesc.textureWidth, textureDataDesc.textureHeight, 0, rhs->m_GLTextureDataDesc.pixelDataFormat, rhs->m_GLTextureDataDesc.pixelDataType, textureData[0]);
		glTexImage2D(GL_TEXTURE_CUBE_MAP_NEGATIVE_X, 0, rhs->m_GLTextureDataDesc.internalFormat, textureDataDesc.textureWidth, textureDataDesc.textureHeight, 0, rhs->m_GLTextureDataDesc.pixelDataFormat, rhs->m_GLTextureDataDesc.pixelDataType, textureData[1]);
		glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_Y, 0, rhs->m_GLTextureDataDesc.internalFormat, textureDataDesc.textureWidth, textureDataDesc.textureHeight, 0, rhs->m_GLTextureDataDesc.pixelDataFormat, rhs->m_GLTextureDataDesc.pixelDataType, textureData[2]);
		glTexImage2D(GL_TEXTURE_CUBE_MAP_NEGATIVE_Y, 0, rhs->m_GLTextureDataDesc.internalFormat, textureDataDesc.textureWidth, textureDataDesc.textureHeight, 0, rhs->m_GLTextureDataDesc.pixelDataFormat, rhs->m_GLTextureDataDesc.pixelDataType, textureData[3]);
		glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_Z, 0, rhs->m_GLTextureDataDesc.internalFormat, textureDataDesc.textureWidth, textureDataDesc.textureHeight, 0, rhs->m_GLTextureDataDesc.pixelDataFormat, rhs->m_GLTextureDataDesc.pixelDataType, textureData[4]);
		glTexImage2D(GL_TEXTURE_CUBE_MAP_NEGATIVE_Z, 0, rhs->m_GLTextureDataDesc.internalFormat, textureDataDesc.textureWidth, textureDataDesc.textureHeight, 0, rhs->m_GLTextureDataDesc.pixelDataFormat, rhs->m_GLTextureDataDesc.pixelDataType, textureData[5]);
	}
	else
	{
		glTexImage2D(GL_TEXTURE_2D, 0, rhs->m_GLTextureDataDesc.internalFormat, textureDataDesc.textureWidth, textureDataDesc.textureHeight, 0, rhs->m_GLTextureDataDesc.pixelDataFormat, rhs->m_GLTextureDataDesc.pixelDataType, textureData[0]);
	}

	// should generate mipmap or not
	if (rhs->m_GLTextureDataDesc.minFilterParam == GL_LINEAR_MIPMAP_LINEAR)
	{
		glGenerateMipmap(rhs->m_GLTextureDataDesc.textureType);
	}

	rhs->m_objectStatus = ObjectStatus::ALIVE;

	m_initializedGLTDC.emplace(rhs->m_parentEntity, rhs);

	g_pCoreSystem->getLogSystem()->printLog(LogType::INNO_DEV_VERBOSE, "GLRenderingSystem: TAO " + std::to_string(rhs->m_TAO) + " is initialized.");

	return true;
}

bool GLRenderingSystem::update()
{
	if (AssetSystemComponent::get().m_uninitializedMeshComponents.size() > 0)
	{
		MeshDataComponent* l_meshDataComponent;
		if (AssetSystemComponent::get().m_uninitializedMeshComponents.tryPop(l_meshDataComponent))
		{
			GLRenderingSystemNS::generateGLMeshDataComponent(l_meshDataComponent);
		}
	}
	if (AssetSystemComponent::get().m_uninitializedTextureComponents.size() > 0)
	{
		TextureDataComponent* l_textureDataComponent;
		if (AssetSystemComponent::get().m_uninitializedTextureComponents.tryPop(l_textureDataComponent))
		{
			GLRenderingSystemNS::generateGLTextureDataComponent(l_textureDataComponent);
		}
	}

	GLRenderingSystemNS::prepareRenderingData();

	if (GLRenderingSystemNS::g_RenderingSystemComponent->m_shouldUpdateEnvironmentMap)
	{
		GLRenderingSystemNS::updateEnvironmentRenderPass();
	}

	GLRenderingSystemNS::updateShadowRenderPass();
	GLRenderingSystemNS::updateGeometryRenderPass();

	GLRenderingSystemNS::updateTerrainRenderPass();

	GLRenderingSystemNS::updateLightRenderPass();
	GLRenderingSystemNS::updateFinalRenderPass();

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

	m_CamProjOriginal = l_p;
	m_CamProjJittered = l_p;

	if (g_RenderingSystemComponent->m_useTAA)
	{
		//TAA jitter for projection matrix
		auto& l_currentHaltonStep = g_RenderingSystemComponent->currentHaltonStep;
		if (l_currentHaltonStep >= 16)
		{
			l_currentHaltonStep = 0;
		}
		m_CamProjJittered.m02 = g_RenderingSystemComponent->HaltonSampler[l_currentHaltonStep].x / deferredPassFBDesc.sizeX;
		m_CamProjJittered.m12 = g_RenderingSystemComponent->HaltonSampler[l_currentHaltonStep].y / deferredPassFBDesc.sizeY;
		l_currentHaltonStep += 1;
	}

	m_CamRot = l_r;
	m_CamTrans = l_t;
	m_CamRot_prev = r_prev;
	m_CamTrans_prev = t_prev;
	m_CamGlobalPos = l_mainCameraTransformComponent->m_globalTransformVector.m_pos;
	
	//UBO
	m_GPassCameraUBOData.m_CamProjJittered = m_CamProjJittered;
	m_GPassCameraUBOData.m_CamProjOriginal = m_CamProjOriginal;
	m_GPassCameraUBOData.m_CamRot = m_CamRot;
	m_GPassCameraUBOData.m_CamTrans = m_CamTrans;
	m_GPassCameraUBOData.m_CamRot_prev = m_CamRot_prev;
	m_GPassCameraUBOData.m_CamTrans_prev = m_CamTrans_prev;

	// sun/directional light
	auto l_directionalLight = GameSystemComponent::get().m_DirectionalLightComponents[0];
	auto l_directionalLightTransformComponent = g_pCoreSystem->getGameSystem()->get<TransformComponent>(l_directionalLight->m_parentEntity);

	m_sunDir = InnoMath::getDirection(direction::BACKWARD, l_directionalLightTransformComponent->m_globalTransformVector.m_rot);
	m_sunColor = l_directionalLight->m_color;
	m_sunRot = InnoMath::getInvertRotationMatrix(l_directionalLightTransformComponent->m_globalTransformVector.m_rot);

	auto l_CSMSize = l_directionalLight->m_projectionMatrices.size();

	m_CSMProjs.clear();
	m_CSMProjs.reserve(l_CSMSize);
	m_CSMSplitCorners.clear();
	m_CSMSplitCorners.reserve(l_CSMSize);

	for (size_t j = 0; j < l_directionalLight->m_projectionMatrices.size(); j++)
	{
		m_CSMProjs.emplace_back();
		m_CSMSplitCorners.emplace_back();

		auto l_shadowSplitCorner = vec4(
			l_directionalLight->m_AABBs[j].m_boundMin.x,
			l_directionalLight->m_AABBs[j].m_boundMin.z,
			l_directionalLight->m_AABBs[j].m_boundMax.x,
			l_directionalLight->m_AABBs[j].m_boundMax.z
		);

		m_CSMProjs[j] = l_directionalLight->m_projectionMatrices[j];
		m_CSMSplitCorners[j] = l_shadowSplitCorner;
	}

	//UBO
	m_GPassLightUBOData.uni_p_light_0 = l_directionalLight->m_projectionMatrices[0];
	m_GPassLightUBOData.uni_p_light_1 = l_directionalLight->m_projectionMatrices[1];
	m_GPassLightUBOData.uni_p_light_2 = l_directionalLight->m_projectionMatrices[2];
	m_GPassLightUBOData.uni_p_light_3 = l_directionalLight->m_projectionMatrices[3];
	m_GPassLightUBOData.uni_v_light = m_sunRot;

	// point light
	m_PointLightDatas.clear();
	m_PointLightDatas.reserve(GameSystemComponent::get().m_PointLightComponents.size());

	for (auto i : GameSystemComponent::get().m_PointLightComponents)
	{
		PointLightData l_PointLightData;
		l_PointLightData.pos = g_pCoreSystem->getGameSystem()->get<TransformComponent>(i->m_parentEntity)->m_globalTransformVector.m_pos;
		l_PointLightData.luminance = i->m_color * i->m_luminousFlux;
		l_PointLightData.attenuationRadius = i->m_attenuationRadius;
		m_PointLightDatas.emplace_back(l_PointLightData);
	}

	// sphere light
	m_SphereLightDatas.clear();
	m_SphereLightDatas.reserve(GameSystemComponent::get().m_SphereLightComponents.size());

	for (auto i : GameSystemComponent::get().m_SphereLightComponents)
	{
		SphereLightData l_SphereLightData;
		l_SphereLightData.pos = g_pCoreSystem->getGameSystem()->get<TransformComponent>(i->m_parentEntity)->m_globalTransformVector.m_pos;
		l_SphereLightData.luminance = i->m_color * i->m_luminousFlux;;
		l_SphereLightData.sphereRadius = i->m_sphereRadius;
		m_SphereLightDatas.emplace_back(l_SphereLightData);
	}

	// mesh
	for (auto& l_renderDataPack : RenderingSystemComponent::get().m_renderDataPack)
	{
		auto l_GLMDC = GLRenderingSystemNS::m_initializedGLMDC.find(l_renderDataPack.MDC->m_parentEntity);
		if (l_GLMDC != GLRenderingSystemNS::m_initializedGLMDC.end())
		{
			if (l_renderDataPack.visiblilityType == VisiblilityType::INNO_OPAQUE || l_renderDataPack.visiblilityType == VisiblilityType::INNO_EMISSIVE)
			{
				GPassOpaqueRenderDataPack l_GLRenderDataPack;

				l_GLRenderDataPack.indiceSize = l_renderDataPack.MDC->m_indicesSize;
				l_GLRenderDataPack.m_meshDrawMethod = l_renderDataPack.MDC->m_meshDrawMethod;
				l_GLRenderDataPack.m_GPassMeshUBOData.m = l_renderDataPack.m;
				l_GLRenderDataPack.m_GPassMeshUBOData.m_prev = l_renderDataPack.m_prev;
				l_GLRenderDataPack.GLMDC = l_GLMDC->second;

				auto l_material = l_renderDataPack.Material;
				// any normal?
				auto l_TDC = l_material->m_texturePack.m_normalTDC.second;
				if (l_TDC)
				{
					l_GLRenderDataPack.m_basicNormalGLTDC = getGLTextureDataComponent(l_TDC->m_parentEntity);
				}
				else
				{
					l_GLRenderDataPack.m_GPassTextureUBOData.useNormalTexture = false;
				}
				// any albedo?
				l_TDC = l_material->m_texturePack.m_albedoTDC.second;
				if (l_TDC)
				{
					l_GLRenderDataPack.m_basicAlbedoGLTDC = getGLTextureDataComponent(l_TDC->m_parentEntity);
				}
				else
				{
					l_GLRenderDataPack.m_GPassTextureUBOData.useAlbedoTexture = false;
				}
				// any metallic?
				l_TDC = l_material->m_texturePack.m_metallicTDC.second;
				if (l_TDC)
				{
					l_GLRenderDataPack.m_basicMetallicGLTDC = getGLTextureDataComponent(l_TDC->m_parentEntity);
				}
				else
				{
					l_GLRenderDataPack.m_GPassTextureUBOData.useMetallicTexture = false;
				}
				// any roughness?
				l_TDC = l_material->m_texturePack.m_roughnessTDC.second;
				if (l_TDC)
				{
					l_GLRenderDataPack.m_basicRoughnessGLTDC = getGLTextureDataComponent(l_TDC->m_parentEntity);
				}
				else
				{
					l_GLRenderDataPack.m_GPassTextureUBOData.useRoughnessTexture = false;
				}
				// any ao?
				l_TDC = l_material->m_texturePack.m_roughnessTDC.second;
				if (l_TDC)
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

				m_GPassOpaqueRenderDataQueue.push(l_GLRenderDataPack);
			}
			else if (l_renderDataPack.visiblilityType == VisiblilityType::INNO_TRANSPARENT)
			{
				GPassTransparentRenderDataPack l_GLRenderDataPack;

				l_GLRenderDataPack.indiceSize = l_renderDataPack.MDC->m_indicesSize;
				l_GLRenderDataPack.m_meshDrawMethod = l_renderDataPack.MDC->m_meshDrawMethod;
				l_GLRenderDataPack.m_GPassMeshUBOData.m = l_renderDataPack.m;
				l_GLRenderDataPack.m_GPassMeshUBOData.m_prev = l_renderDataPack.m_prev;
				l_GLRenderDataPack.GLMDC = l_GLMDC->second;

				auto l_material = l_renderDataPack.Material;

				l_GLRenderDataPack.meshCustomMaterial = l_material->m_meshCustomMaterial;
				l_GLRenderDataPack.visiblilityType = l_renderDataPack.visiblilityType;
				m_GPassTransparentRenderDataQueue.push(l_GLRenderDataPack);
			}
		}
	}
}

void GLRenderingSystemNS::updateBRDFLUTRenderPass()
{
	glDisable(GL_DEPTH_TEST);
	glDisable(GL_CULL_FACE);

	// bind to framebuffer
	auto l_FBC = GLEnvironmentRenderPassComponent::get().m_BRDFLUTPassFBC;
	glBindFramebuffer(GL_FRAMEBUFFER, l_FBC->m_FBO);
	glBindRenderbuffer(GL_RENDERBUFFER, l_FBC->m_RBO);
	glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT32, 512, 512);
	glViewport(0, 0, 512, 512);

	// draw split-Sum LUT
	activateShaderProgram(GLEnvironmentRenderPassComponent::get().m_BRDFSplitSumLUTPassSPC);
	auto l_SSTDC = GLEnvironmentRenderPassComponent::get().m_BRDFSplitSumLUTPassTDC;
	auto l_SSGLTDC = GLEnvironmentRenderPassComponent::get().m_BRDFSplitSumLUTPassGLTDC;
	auto l_MDC = g_pCoreSystem->getAssetSystem()->getMeshDataComponent(MeshShapeType::QUAD);
	attachTextureToFramebuffer(l_SSTDC, l_SSGLTDC, l_FBC, 0, 0, 0);
	glClear(GL_COLOR_BUFFER_BIT);
	glClear(GL_DEPTH_BUFFER_BIT);
	drawMesh(l_MDC);

	// draw averange RsF1 LUT
	activateShaderProgram(GLEnvironmentRenderPassComponent::get().m_BRDFMSAverageLUTPassSPC);
	auto l_MSTDC = GLEnvironmentRenderPassComponent::get().m_BRDFMSAverageLUTPassTDC;
	auto l_MSGLTDC = GLEnvironmentRenderPassComponent::get().m_BRDFMSAverageLUTPassGLTDC;
	attachTextureToFramebuffer(l_MSTDC, l_MSGLTDC, l_FBC, 0, 0, 0);
	glClear(GL_COLOR_BUFFER_BIT);
	glClear(GL_DEPTH_BUFFER_BIT);
	activateTexture(l_SSGLTDC, 0);
	drawMesh(l_MDC);

	RenderingSystemComponent::get().m_shouldUpdateEnvironmentMap = false;
}

void GLRenderingSystemNS::updateEnvironmentRenderPass()
{
	updateBRDFLUTRenderPass();
}

void GLRenderingSystemNS::updateShadowRenderPass()
{
	glEnable(GL_DEPTH_TEST);
	glDepthFunc(GL_LESS);

	glEnable(GL_CULL_FACE);
	glFrontFace(GL_CCW);
	glCullFace(GL_FRONT);

	activateShaderProgram(GLShadowRenderPassComponent::get().m_SPC);
	updateUniform(
		GLShadowRenderPassComponent::get().m_shadowPass_uni_v,
		m_sunRot);

	// draw each lightComponent's shadowmap
	for (size_t i = 0; i < GLShadowRenderPassComponent::get().m_GLRPCs.size(); i++)
	{
		f_bindFBC(GLShadowRenderPassComponent::get().m_GLRPCs[i]->m_GLFBC);
		updateUniform(
			GLShadowRenderPassComponent::get().m_shadowPass_uni_p,
			m_CSMProjs[i]);

		// draw each visibleComponent
		for (auto& l_visibleComponent : GLRenderingSystemNS::g_GameSystemComponent->m_VisibleComponents)
		{
			if (l_visibleComponent->m_visiblilityType == VisiblilityType::INNO_OPAQUE)
			{
				updateUniform(
					GLShadowRenderPassComponent::get().m_shadowPass_uni_m,
					g_pCoreSystem->getGameSystem()->get<TransformComponent>(l_visibleComponent->m_parentEntity)->m_globalTransformMatrix.m_transformationMat);

				// draw each graphic data of visibleComponent
				for (auto l_modelPair : l_visibleComponent->m_modelMap)
				{
					// draw meshes
					auto l_MDC = l_modelPair.first;
					if (l_MDC)
					{
						// draw meshes
						drawMesh(l_MDC);
					}
				}
			}
		}

	}

	glDisable(GL_CULL_FACE);
	glDisable(GL_DEPTH_TEST);
}


void GLRenderingSystemNS::updateGeometryRenderPass()
{
	updateOpaqueRenderPass();
	updateTransparentRenderPass();
}

void GLRenderingSystemNS::updateOpaqueRenderPass()
{
	glEnable(GL_DEPTH_TEST);
	glDepthFunc(GL_LESS);
	glEnable(GL_DEPTH_CLAMP);

	glEnable(GL_STENCIL_TEST);
	glStencilOp(GL_KEEP, GL_KEEP, GL_REPLACE);
	glStencilMask(0xFF);

	//glEnable(GL_CULL_FACE);
	//glFrontFace(GL_CCW);
	//glCullFace(GL_BACK);

	// bind to framebuffer
	auto l_FBC = GLGeometryRenderPassComponent::get().m_opaquePass_GLRPC->m_GLFBC;
	f_bindFBC(l_FBC);

	activateShaderProgram(GLGeometryRenderPassComponent::get().m_opaquePass_GLSPC);

	updateUBO(GLGeometryRenderPassComponent::get().m_cameraUBO, m_GPassCameraUBOData);

#ifdef CookTorrance
	//Cook-Torrance
	updateUBO(GLGeometryRenderPassComponent::get().m_lightUBO, m_GPassLightUBOData);

	while (GLRenderingSystemNS::m_GPassOpaqueRenderDataQueue.size() > 0)
	{
		auto l_renderPack = GLRenderingSystemNS::m_GPassOpaqueRenderDataQueue.front();
		if (l_renderPack.visiblilityType == VisiblilityType::INNO_OPAQUE)
		{
			glStencilFunc(GL_ALWAYS, 0x01, 0xFF);

			// any normal?
			if (l_renderPack.m_GPassTextureUBOData.useNormalTexture)
			{
				activateTexture(l_renderPack.m_basicNormalGLTDC, 0);
			}
			// any albedo?
			if (l_renderPack.m_GPassTextureUBOData.useAlbedoTexture)
			{
				activateTexture(l_renderPack.m_basicAlbedoGLTDC, 1);
			}
			// any metallic?
			if (l_renderPack.m_GPassTextureUBOData.useMetallicTexture)
			{
				activateTexture(l_renderPack.m_basicMetallicGLTDC, 2);
			}
			// any roughness?
			if (l_renderPack.m_GPassTextureUBOData.useRoughnessTexture)
			{
				activateTexture(l_renderPack.m_basicRoughnessGLTDC, 3);
			}
			// any ao?
			if (l_renderPack.m_GPassTextureUBOData.useAOTexture)
			{
				activateTexture(l_renderPack.m_basicAOGLTDC, 4);
			}

			updateUBO(GLGeometryRenderPassComponent::get().m_meshUBO, l_renderPack.m_GPassMeshUBOData);
			updateUBO(GLGeometryRenderPassComponent::get().m_textureUBO, l_renderPack.m_GPassTextureUBOData);
			drawMesh(l_renderPack.indiceSize, l_renderPack.m_meshDrawMethod, l_renderPack.GLMDC);
		}
		else if (l_renderPack.visiblilityType == VisiblilityType::INNO_EMISSIVE)
		{
			glStencilFunc(GL_ALWAYS, 0x02, 0xFF);

			updateUBO(GLGeometryRenderPassComponent::get().m_meshUBO, l_renderPack.m_GPassMeshUBOData);
			updateUBO(GLGeometryRenderPassComponent::get().m_textureUBO, l_renderPack.m_GPassTextureUBOData);

			drawMesh(l_renderPack.indiceSize, l_renderPack.m_meshDrawMethod, l_renderPack.GLMDC);
		}
		else
		{
			glStencilFunc(GL_ALWAYS, 0x00, 0xFF);
		}
		GLRenderingSystemNS::m_GPassOpaqueRenderDataQueue.pop();
	}

	//glDisable(GL_CULL_FACE);
	glDisable(GL_STENCIL_TEST);
	glDisable(GL_DEPTH_CLAMP);
	glDisable(GL_DEPTH_TEST);

#elif BlinnPhong
#endif
}

void GLRenderingSystemNS::updateTransparentRenderPass()
{
	glEnable(GL_DEPTH_TEST);
	glDepthFunc(GL_LESS);
	glEnable(GL_DEPTH_CLAMP);

	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_COLOR, GL_ONE_MINUS_SRC1_COLOR);

	// bind to framebuffer
	auto l_FBC = GLGeometryRenderPassComponent::get().m_transparentPass_GLRPC->m_GLFBC;
	f_bindFBC(l_FBC);

	f_copyDepthBuffer(GLGeometryRenderPassComponent::get().m_opaquePass_GLRPC->m_GLFBC, l_FBC);

	activateShaderProgram(GLGeometryRenderPassComponent::get().m_transparentPass_GLSPC);

	updateUBO(GLGeometryRenderPassComponent::get().m_cameraUBO, m_GPassCameraUBOData);

	updateUniform(
		GLGeometryRenderPassComponent::get().m_transparentPass_uni_viewPos,
		m_CamGlobalPos.x, m_CamGlobalPos.y, m_CamGlobalPos.z);

	while (GLRenderingSystemNS::m_GPassTransparentRenderDataQueue.size() > 0)
	{
		auto l_renderPack = GLRenderingSystemNS::m_GPassTransparentRenderDataQueue.front();

		updateUBO(GLGeometryRenderPassComponent::get().m_meshUBO, l_renderPack.m_GPassMeshUBOData);

		updateUniform(GLGeometryRenderPassComponent::get().m_transparentPass_uni_albedo, l_renderPack.meshCustomMaterial.albedo_r, l_renderPack.meshCustomMaterial.albedo_g, l_renderPack.meshCustomMaterial.albedo_b, l_renderPack.meshCustomMaterial.alpha);

		drawMesh(l_renderPack.indiceSize, l_renderPack.m_meshDrawMethod, l_renderPack.GLMDC);

		GLRenderingSystemNS::m_GPassTransparentRenderDataQueue.pop();
	}

	glDisable(GL_BLEND);
	glDisable(GL_DEPTH_CLAMP);
	glDisable(GL_DEPTH_TEST);
}

void GLRenderingSystemNS::updateTerrainRenderPass()
{
	if (GLRenderingSystemNS::g_RenderingSystemComponent->m_drawTerrain)
	{
		glEnable(GL_DEPTH_TEST);

		// bind to framebuffer
		auto l_FBC = GLTerrainRenderPassComponent::get().m_GLRPC->m_GLFBC;
		f_bindFBC(l_FBC);

		f_copyDepthBuffer(GLGeometryRenderPassComponent::get().m_opaquePass_GLRPC->m_GLFBC, l_FBC);

		activateShaderProgram(GLTerrainRenderPassComponent::get().m_GLSPC);

		mat4 m = InnoMath::generateIdentityMatrix<float>();

		updateUniform(
			GLTerrainRenderPassComponent::get().m_terrainPass_uni_p_camera,
			m_CamProjOriginal);
		updateUniform(
			GLTerrainRenderPassComponent::get().m_terrainPass_uni_r_camera,
			m_CamRot);
		updateUniform(
			GLTerrainRenderPassComponent::get().m_terrainPass_uni_t_camera,
			m_CamTrans);
		updateUniform(
			GLTerrainRenderPassComponent::get().m_terrainPass_uni_m,
			m);

		auto l_MDC = g_pCoreSystem->getAssetSystem()->getMeshDataComponent(MeshShapeType::TERRAIN);
		activateTexture(m_basicAlbedoGLTDC, 0);

		drawMesh(l_MDC);

		glDisable(GL_DEPTH_TEST);
	}
	else
	{
		GLRenderingSystemNS::f_cleanFBC(GLTerrainRenderPassComponent::get().m_GLRPC->m_GLFBC);
	}
}

void GLRenderingSystemNS::updateLightRenderPass()
{
	glDisable(GL_DEPTH_TEST);

	glEnable(GL_STENCIL_TEST);
	glStencilOp(GL_KEEP, GL_KEEP, GL_KEEP);
	glStencilFunc(GL_EQUAL, 0x01, 0xFF);
	glStencilMask(0x00);

	glDisable(GL_CULL_FACE);

	// bind to framebuffer
	auto l_FBC = GLLightRenderPassComponent::get().m_GLRPC->m_GLFBC;
	f_bindFBC(l_FBC);

	// 1. opaque objects
	// copy stencil buffer of opaque objects from G-Pass
	f_copyStencilBuffer(GLGeometryRenderPassComponent::get().m_opaquePass_GLRPC->m_GLFBC, l_FBC);

	activateShaderProgram(GLLightRenderPassComponent::get().m_GLSPC);

#ifdef CookTorrance
	// Cook-Torrance
	// world space position + metallic
	activateTexture(
		GLGeometryRenderPassComponent::get().m_opaquePass_GLRPC->m_GLTDCs[0],
		0);
	// normal + roughness
	activateTexture(
		GLGeometryRenderPassComponent::get().m_opaquePass_GLRPC->m_GLTDCs[1],
		1);
	// albedo + ambient occlusion
	activateTexture(
		GLGeometryRenderPassComponent::get().m_opaquePass_GLRPC->m_GLTDCs[2],
		2);
	// motion vector + transparency
	activateTexture(
		GLGeometryRenderPassComponent::get().m_opaquePass_GLRPC->m_GLTDCs[3],
		3);
	// light space position 0
	activateTexture(
		GLGeometryRenderPassComponent::get().m_opaquePass_GLRPC->m_GLTDCs[4],
		4);
	// light space position 1
	activateTexture(
		GLGeometryRenderPassComponent::get().m_opaquePass_GLRPC->m_GLTDCs[5],
		5);
	// light space position 2
	activateTexture(
		GLGeometryRenderPassComponent::get().m_opaquePass_GLRPC->m_GLTDCs[6],
		6);
	// light space position 3
	activateTexture(
		GLGeometryRenderPassComponent::get().m_opaquePass_GLRPC->m_GLTDCs[7],
		7);
	// shadow map 0
	activateTexture(
		GLShadowRenderPassComponent::get().m_GLRPCs[0]->m_GLTDCs[0],
		8);
	// shadow map 1
	activateTexture(
		GLShadowRenderPassComponent::get().m_GLRPCs[1]->m_GLTDCs[0],
		9);
	// shadow map 2
	activateTexture(
		GLShadowRenderPassComponent::get().m_GLRPCs[2]->m_GLTDCs[0],
		10);
	// shadow map 3
	activateTexture(
		GLShadowRenderPassComponent::get().m_GLRPCs[3]->m_GLTDCs[0],
		11);
	// BRDF look-up table 1
	activateTexture(
		GLEnvironmentRenderPassComponent::get().m_BRDFSplitSumLUTPassGLTDC,
		12);
	// BRDF look-up table 2
	activateTexture(
		GLEnvironmentRenderPassComponent::get().m_BRDFMSAverageLUTPassGLTDC,
		13);
#endif

	updateUniform(
		GLLightRenderPassComponent::get().m_uni_isEmissive,
		false);

	updateUniform(
		GLLightRenderPassComponent::get().m_uni_viewPos,
		m_CamGlobalPos.x, m_CamGlobalPos.y, m_CamGlobalPos.z);

	updateUniform(
		GLLightRenderPassComponent::get().m_uni_dirLight_direction,
		m_sunDir.x, m_sunDir.y, m_sunDir.z);
	updateUniform(
		GLLightRenderPassComponent::get().m_uni_dirLight_color,
		m_sunColor.x, m_sunColor.y, m_sunColor.z);

	for (size_t j = 0; j < 4; j++)
	{
		updateUniform(
			GLLightRenderPassComponent::get().m_uni_shadowSplitAreas[j],
			m_CSMSplitCorners[j].x, m_CSMSplitCorners[j].y, m_CSMSplitCorners[j].z, m_CSMSplitCorners[j].w);
	}

	for (size_t i = 0; i < m_PointLightDatas.size(); i++)
	{
		auto l_pos = m_PointLightDatas[i].pos;
		auto l_luminance = m_PointLightDatas[i].luminance;
		auto l_attenuationRadius = m_PointLightDatas[i].attenuationRadius;

		updateUniform(
			GLLightRenderPassComponent::get().m_uni_pointLights_position[i],
			l_pos.x, l_pos.y, l_pos.z);
		updateUniform(
			GLLightRenderPassComponent::get().m_uni_pointLights_attenuationRadius[i],
			l_attenuationRadius);
		updateUniform(
			GLLightRenderPassComponent::get().m_uni_pointLights_luminance[i],
			l_luminance.x, l_luminance.y, l_luminance.z);
	}

	for (size_t i = 0; i < m_SphereLightDatas.size(); i++)
	{
		auto l_pos = m_SphereLightDatas[i].pos;
		auto l_luminance = m_SphereLightDatas[i].luminance;
		auto l_sphereRadius = m_SphereLightDatas[i].sphereRadius;

		updateUniform(
			GLLightRenderPassComponent::get().m_uni_sphereLights_position[i],
			l_pos.x, l_pos.y, l_pos.z);
		updateUniform(
			GLLightRenderPassComponent::get().m_uni_sphereLights_sphereRadius[i],
			l_sphereRadius);
		updateUniform(
			GLLightRenderPassComponent::get().m_uni_sphereLights_luminance[i],
			l_luminance.x, l_luminance.y, l_luminance.z);
	}

	// draw light pass rectangle
	auto l_MDC = g_pCoreSystem->getAssetSystem()->getMeshDataComponent(MeshShapeType::QUAD);
	drawMesh(l_MDC);

	// 2. draw emissive objects
	glStencilOp(GL_KEEP, GL_KEEP, GL_KEEP);
	glStencilFunc(GL_EQUAL, 0x02, 0xFF);
	glStencilMask(0x00);

	glClear(GL_STENCIL_BUFFER_BIT);

	// copy stencil buffer of emmisive objects from G-Pass
	f_copyStencilBuffer(GLGeometryRenderPassComponent::get().m_opaquePass_GLRPC->m_GLFBC, l_FBC);

	updateUniform(
		GLLightRenderPassComponent::get().m_uni_isEmissive,
		true);

	// draw light pass rectangle
	l_MDC = g_pCoreSystem->getAssetSystem()->getMeshDataComponent(MeshShapeType::QUAD);
	drawMesh(l_MDC);

	glDisable(GL_STENCIL_TEST);
}

GLTextureDataComponent* GLRenderingSystemNS::updateSkyPass()
{
	if (GLRenderingSystemNS::g_RenderingSystemComponent->m_drawSky)
	{
		glEnable(GL_DEPTH_TEST);
		glDepthFunc(GL_LEQUAL);

		//glEnable(GL_CULL_FACE);
		//glFrontFace(GL_CW);
		//glCullFace(GL_FRONT);

		// bind to framebuffer
		auto l_FBC = GLFinalRenderPassComponent::get().m_skyPassGLRPC->m_GLFBC;
		f_bindFBC(l_FBC);

		activateShaderProgram(GLFinalRenderPassComponent::get().m_skyPassSPC);

		updateUniform(
			GLFinalRenderPassComponent::get().m_skyPass_uni_p,
			m_CamProjOriginal);
		updateUniform(
			GLFinalRenderPassComponent::get().m_skyPass_uni_r,
			m_CamRot);
		updateUniform(
			GLFinalRenderPassComponent::get().m_skyPass_uni_viewportSize,
			(float)deferredPassFBDesc.sizeX, (float)deferredPassFBDesc.sizeY);
		updateUniform(
			GLFinalRenderPassComponent::get().m_skyPass_uni_eyePos,
			m_CamGlobalPos.x, m_CamGlobalPos.y, m_CamGlobalPos.z);
		updateUniform(
			GLFinalRenderPassComponent::get().m_skyPass_uni_lightDir,
			m_sunDir.x, m_sunDir.y, m_sunDir.z);

		auto l_MDC = g_pCoreSystem->getAssetSystem()->getMeshDataComponent(MeshShapeType::CUBE);
		drawMesh(l_MDC);

		glDisable(GL_CULL_FACE);
		glDisable(GL_DEPTH_TEST);
	}
	else
	{
		GLRenderingSystemNS::f_cleanFBC(GLFinalRenderPassComponent::get().m_skyPassGLRPC->m_GLFBC);
	}

	return GLFinalRenderPassComponent::get().m_skyPassGLRPC->m_GLTDCs[0];
}

GLTextureDataComponent* GLRenderingSystemNS::updatePreTAAPass()
{
	auto l_FBC = GLFinalRenderPassComponent::get().m_preTAAPassGLRPC->m_GLFBC;
	f_bindFBC(l_FBC);

	activateShaderProgram(GLFinalRenderPassComponent::get().m_preTAAPassSPC);

	activateTexture(
		GLLightRenderPassComponent::get().m_GLRPC->m_GLTDCs[0],
		0);
	activateTexture(
		GLGeometryRenderPassComponent::get().m_transparentPass_GLRPC->m_GLTDCs[0],
		1);
	activateTexture(
		GLFinalRenderPassComponent::get().m_skyPassGLRPC->m_GLTDCs[0],
		2);
	activateTexture(
		GLTerrainRenderPassComponent::get().m_GLRPC->m_GLTDCs[0],
		3);

	auto l_MDC = g_pCoreSystem->getAssetSystem()->getMeshDataComponent(MeshShapeType::QUAD);
	drawMesh(l_MDC);

	return GLFinalRenderPassComponent::get().m_preTAAPassGLRPC->m_GLTDCs[0];
}

GLTextureDataComponent* GLRenderingSystemNS::updateTAAPass(GLTextureDataComponent* inputGLTDC)
{
	GLTextureDataComponent* l_currentFrameTAAGLTDC = GLFinalRenderPassComponent::get().m_TAAPingPassGLRPC->m_GLTDCs[0];
	GLTextureDataComponent* l_lastFrameTAAGLTDC = GLFinalRenderPassComponent::get().m_TAAPongPassGLRPC->m_GLTDCs[0];

	activateShaderProgram(GLFinalRenderPassComponent::get().m_TAAPassSPC);

	GLFrameBufferComponent* l_FBC;

	if (g_RenderingSystemComponent->m_isTAAPingPass)
	{
		l_currentFrameTAAGLTDC = GLFinalRenderPassComponent::get().m_TAAPingPassGLRPC->m_GLTDCs[0];
		l_lastFrameTAAGLTDC = GLFinalRenderPassComponent::get().m_TAAPongPassGLRPC->m_GLTDCs[0];

		l_FBC = GLFinalRenderPassComponent::get().m_TAAPingPassGLRPC->m_GLFBC;

		g_RenderingSystemComponent->m_isTAAPingPass = false;
	}
	else
	{
		l_currentFrameTAAGLTDC = GLFinalRenderPassComponent::get().m_TAAPongPassGLRPC->m_GLTDCs[0];
		l_lastFrameTAAGLTDC = GLFinalRenderPassComponent::get().m_TAAPingPassGLRPC->m_GLTDCs[0];

		l_FBC = GLFinalRenderPassComponent::get().m_TAAPongPassGLRPC->m_GLFBC;

		g_RenderingSystemComponent->m_isTAAPingPass = true;
	}

	f_bindFBC(l_FBC);

	activateTexture(inputGLTDC,
		0);
	activateTexture(
		l_lastFrameTAAGLTDC,
		1);
	activateTexture(
		GLGeometryRenderPassComponent::get().m_opaquePass_GLRPC->m_GLTDCs[3],
		2);

	updateUniform(
		GLFinalRenderPassComponent::get().m_TAAPass_uni_renderTargetSize,
		(float)deferredPassFBDesc.sizeX, (float)deferredPassFBDesc.sizeY);

	auto l_MDC = g_pCoreSystem->getAssetSystem()->getMeshDataComponent(MeshShapeType::QUAD);
	drawMesh(l_MDC);

	return l_currentFrameTAAGLTDC;
}

GLTextureDataComponent* GLRenderingSystemNS::updateTAASharpenPass(GLTextureDataComponent * inputGLTDC)
{
	auto l_FBC = GLFinalRenderPassComponent::get().m_TAASharpenPassGLRPC->m_GLFBC;
	f_bindFBC(l_FBC);
	activateShaderProgram(GLFinalRenderPassComponent::get().m_TAASharpenPassSPC);

	activateTexture(
		inputGLTDC,
		0);

	updateUniform(
		GLFinalRenderPassComponent::get().m_TAASharpenPass_uni_renderTargetSize,
		(float)deferredPassFBDesc.sizeX, (float)deferredPassFBDesc.sizeY);

	auto l_MDC = g_pCoreSystem->getAssetSystem()->getMeshDataComponent(MeshShapeType::QUAD);
	drawMesh(l_MDC);

	return GLFinalRenderPassComponent::get().m_TAASharpenPassGLRPC->m_GLTDCs[0];
}

GLTextureDataComponent* GLRenderingSystemNS::updateBloomExtractPass(GLTextureDataComponent * inputGLTDC)
{
	auto l_FBC = GLFinalRenderPassComponent::get().m_bloomExtractPassGLRPC->m_GLFBC;
	f_bindFBC(l_FBC);

	activateShaderProgram(GLFinalRenderPassComponent::get().m_bloomExtractPassSPC);

	activateTexture(inputGLTDC, 0);

	auto l_MDC = g_pCoreSystem->getAssetSystem()->getMeshDataComponent(MeshShapeType::QUAD);
	drawMesh(l_MDC);

	return GLFinalRenderPassComponent::get().m_bloomExtractPassGLRPC->m_GLTDCs[0];
}

GLTextureDataComponent* GLRenderingSystemNS::updateBloomBlurPass(GLTextureDataComponent * inputGLTDC)
{
	GLTextureDataComponent* l_currentFrameBloomBlurGLTDC = GLFinalRenderPassComponent::get().m_bloomBlurPingPassGLRPC->m_GLTDCs[0];
	GLTextureDataComponent* l_lastFrameBloomBlurGLTDC = GLFinalRenderPassComponent::get().m_bloomBlurPongPassGLRPC->m_GLTDCs[0];

	activateShaderProgram(GLFinalRenderPassComponent::get().m_bloomBlurPassSPC);

	bool l_isPing = true;
	bool l_isFirstIteration = true;

	auto l_MDC = g_pCoreSystem->getAssetSystem()->getMeshDataComponent(MeshShapeType::QUAD);

	for (size_t i = 0; i < 5; i++)
	{
		if (l_isPing)
		{
			l_currentFrameBloomBlurGLTDC = GLFinalRenderPassComponent::get().m_bloomBlurPingPassGLRPC->m_GLTDCs[0];
			l_lastFrameBloomBlurGLTDC = GLFinalRenderPassComponent::get().m_bloomBlurPongPassGLRPC->m_GLTDCs[0];

			auto l_FBC = GLFinalRenderPassComponent::get().m_bloomBlurPingPassGLRPC->m_GLFBC;
			f_bindFBC(l_FBC);

			updateUniform(
				GLFinalRenderPassComponent::get().m_bloomBlurPass_uni_horizontal,
				true);

			if (l_isFirstIteration)
			{
				activateTexture(
					inputGLTDC,
					0);
				l_isFirstIteration = false;
			}
			else
			{
				activateTexture(
					l_lastFrameBloomBlurGLTDC,
					0);
			}

			drawMesh(l_MDC);

			l_isPing = false;
		}
		else
		{
			l_currentFrameBloomBlurGLTDC = GLFinalRenderPassComponent::get().m_bloomBlurPongPassGLRPC->m_GLTDCs[0];
			l_lastFrameBloomBlurGLTDC = GLFinalRenderPassComponent::get().m_bloomBlurPingPassGLRPC->m_GLTDCs[0];

			auto l_FBC = GLFinalRenderPassComponent::get().m_bloomBlurPongPassGLRPC->m_GLFBC;
			f_bindFBC(l_FBC);

			updateUniform(
				GLFinalRenderPassComponent::get().m_bloomBlurPass_uni_horizontal,
				false);

			activateTexture(
				l_lastFrameBloomBlurGLTDC,
				0);

			drawMesh(l_MDC);

			l_isPing = true;
		}
	}

	return l_currentFrameBloomBlurGLTDC;
}

GLTextureDataComponent* GLRenderingSystemNS::updateMotionBlurPass(GLTextureDataComponent * inputGLTDC)
{
	auto l_FBC = GLFinalRenderPassComponent::get().m_motionBlurPassGLRPC->m_GLFBC;
	f_bindFBC(l_FBC);

	activateShaderProgram(GLFinalRenderPassComponent::get().m_motionBlurPassSPC);

	activateTexture(
		GLGeometryRenderPassComponent::get().m_opaquePass_GLRPC->m_GLTDCs[3],
		0);
	activateTexture(inputGLTDC, 1);

	auto l_MDC = g_pCoreSystem->getAssetSystem()->getMeshDataComponent(MeshShapeType::QUAD);
	drawMesh(l_MDC);

	return GLFinalRenderPassComponent::get().m_motionBlurPassGLRPC->m_GLTDCs[0];
}

GLTextureDataComponent* GLRenderingSystemNS::updateBillboardPass()
{
	glEnable(GL_DEPTH_TEST);
	glDepthFunc(GL_LESS);

	auto l_FBC = GLFinalRenderPassComponent::get().m_billboardPassGLRPC->m_GLFBC;
	f_bindFBC(l_FBC);

	// copy depth buffer from G-Pass
	f_copyDepthBuffer(GLGeometryRenderPassComponent::get().m_opaquePass_GLRPC->m_GLFBC, l_FBC);

	activateShaderProgram(GLFinalRenderPassComponent::get().m_billboardPassSPC);

	updateUniform(
		GLFinalRenderPassComponent::get().m_billboardPass_uni_p,
		m_CamProjOriginal);
	updateUniform(
		GLFinalRenderPassComponent::get().m_billboardPass_uni_r,
		m_CamRot);
	updateUniform(
		GLFinalRenderPassComponent::get().m_billboardPass_uni_t,
		m_CamTrans);

	if (g_GameSystemComponent->m_VisibleComponents.size() > 0)
	{
		// draw each visibleComponent
		for (auto& l_visibleComponent : g_GameSystemComponent->m_VisibleComponents)
		{
			if (l_visibleComponent->m_visiblilityType == VisiblilityType::INNO_BILLBOARD)
			{
				auto l_GlobalPos = g_pCoreSystem->getGameSystem()->get<TransformComponent>(l_visibleComponent->m_parentEntity)->m_globalTransformVector.m_pos;

				updateUniform(
					GLFinalRenderPassComponent::get().m_billboardPass_uni_pos,
					l_GlobalPos.x, l_GlobalPos.y, l_GlobalPos.z);

				auto l_distanceToCamera = (m_CamGlobalPos - l_GlobalPos).length();
				if (l_distanceToCamera > 1.0f)
				{
					updateUniform(
						GLFinalRenderPassComponent::get().m_billboardPass_uni_size,
						(1.0f / l_distanceToCamera) * (9.0f / 16.0f), (1.0f / l_distanceToCamera));
				}
				else
				{
					updateUniform(
						GLFinalRenderPassComponent::get().m_billboardPass_uni_size,
						(9.0f / 16.0f), 1.0f);
				}
				// draw each graphic data of visibleComponent
				for (auto& l_modelPair : l_visibleComponent->m_modelMap)
				{
					// draw meshes
					auto l_MDC = l_modelPair.first;
					if (l_MDC)
					{
						auto l_textureMap = l_modelPair.second;
						// any normal?
						auto l_TDC = l_textureMap->m_texturePack.m_normalTDC.second;
						if (l_TDC)
						{
							activateTexture(l_TDC, 0);
						}
						else
						{
							activateTexture(m_basicNormalGLTDC, 0);
						}
						drawMesh(l_MDC);
					}
				}
			}
		}
	}

	glDisable(GL_DEPTH_TEST);

	return GLFinalRenderPassComponent::get().m_billboardPassGLRPC->m_GLTDCs[0];
}

GLTextureDataComponent* GLRenderingSystemNS::updateDebuggerPass()
{
	glEnable(GL_DEPTH_TEST);
	glDepthFunc(GL_LESS);

	auto l_FBC = GLFinalRenderPassComponent::get().m_debuggerPassGLRPC->m_GLFBC;
	f_bindFBC(l_FBC);

	// copy depth buffer from G-Pass
	f_copyDepthBuffer(GLGeometryRenderPassComponent::get().m_opaquePass_GLRPC->m_GLFBC, l_FBC);

	activateShaderProgram(GLFinalRenderPassComponent::get().m_debuggerPassSPC);

	updateUniform(
		GLFinalRenderPassComponent::get().m_debuggerPass_uni_p,
		m_CamProjOriginal);
	updateUniform(
		GLFinalRenderPassComponent::get().m_debuggerPass_uni_r,
		m_CamRot);
	updateUniform(
		GLFinalRenderPassComponent::get().m_debuggerPass_uni_t,
		m_CamTrans);

	glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);

	for (auto& AABBWireframeDataPack : PhysicsSystemComponent::get().m_AABBWireframeDataPack)
	{
		updateUniform(
			GLFinalRenderPassComponent::get().m_debuggerPass_uni_m,
			AABBWireframeDataPack.m);
		drawMesh(AABBWireframeDataPack.MDC);
	}

	glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
	glDisable(GL_DEPTH_TEST);

	return GLFinalRenderPassComponent::get().m_debuggerPassGLRPC->m_GLTDCs[0];
}

GLTextureDataComponent* GLRenderingSystemNS::updateFinalBlendPass()
{
	auto l_FBC = GLFinalRenderPassComponent::get().m_finalBlendPassGLRPC->m_GLFBC;
	f_bindFBC(l_FBC);

	activateShaderProgram(GLFinalRenderPassComponent::get().m_finalBlendPassSPC);

	// motion blur pass rendering target
	activateTexture(
		GLFinalRenderPassComponent::get().m_motionBlurPassGLRPC->m_GLTDCs[0],
		0);
	// bloom pass rendering target
	activateTexture(
		GLFinalRenderPassComponent::get().m_bloomBlurPongPassGLRPC->m_GLTDCs[0],
		1);
	// billboard pass rendering target
	activateTexture(
		GLFinalRenderPassComponent::get().m_billboardPassGLRPC->m_GLTDCs[0],
		2);
	// debugger pass rendering target
	activateTexture(
		GLFinalRenderPassComponent::get().m_debuggerPassGLRPC->m_GLTDCs[0],
		3);
	// draw final pass rectangle
	auto l_MDC = g_pCoreSystem->getAssetSystem()->getMeshDataComponent(MeshShapeType::QUAD);
	drawMesh(l_MDC);

	// draw again for game build
	glBindFramebuffer(GL_FRAMEBUFFER, 0);
	glBindRenderbuffer(GL_RENDERBUFFER, 0);

	drawMesh(l_MDC);

	return GLFinalRenderPassComponent::get().m_finalBlendPassGLRPC->m_GLTDCs[0];
}

void GLRenderingSystemNS::updateFinalRenderPass()
{
	auto skyPassResult = updateSkyPass();

	auto preTAAPassResult = updatePreTAAPass();

	GLTextureDataComponent* bloomInputGLTDC;

	if (g_RenderingSystemComponent->m_useTAA)
	{
		auto TAAPassResult = updateTAAPass(preTAAPassResult);

		auto TAASharpenPassResult = updateTAASharpenPass(TAAPassResult);

		bloomInputGLTDC = TAASharpenPassResult;
	}
	else
	{
		bloomInputGLTDC = preTAAPassResult;
	}

	if (g_RenderingSystemComponent->m_useBloom)
	{
		auto bloomExtractPassResult = updateBloomExtractPass(bloomInputGLTDC);

		//glEnable(GL_STENCIL_TEST);
		//glClear(GL_STENCIL_BUFFER_BIT);

		//glStencilOp(GL_KEEP, GL_KEEP, GL_KEEP);
		//glStencilFunc(GL_EQUAL, 0x02, 0xFF);
		//glStencilMask(0x00);

		//f_copyColorBuffer(GLLightRenderPassComponent::get().m_GLRPC->m_GLFBC,
		//	GLFinalRenderPassComponent::get().m_bloomExtractPassGLRPC->m_GLFBC);

		//glDisable(GL_STENCIL_TEST);

		auto bloomBlurPassResult = updateBloomBlurPass(bloomExtractPassResult);
	}
	else
	{
		f_cleanFBC(GLFinalRenderPassComponent::get().m_bloomExtractPassGLRPC->m_GLFBC);
		f_cleanFBC(GLFinalRenderPassComponent::get().m_bloomBlurPingPassGLRPC->m_GLFBC);
		f_cleanFBC(GLFinalRenderPassComponent::get().m_bloomBlurPongPassGLRPC->m_GLFBC);
	}

	updateMotionBlurPass(bloomInputGLTDC);

	updateBillboardPass();

	if (g_RenderingSystemComponent->m_drawOverlapWireframe)
	{
		updateDebuggerPass();
	}
	else
	{
		f_cleanFBC(GLFinalRenderPassComponent::get().m_debuggerPassGLRPC->m_GLFBC);
	}

	updateFinalBlendPass();
}

bool GLRenderingSystem::terminate()
{
	GLRenderingSystemNS::m_objectStatus = ObjectStatus::SHUTDOWN;
	g_pCoreSystem->getLogSystem()->printLog(LogType::INNO_DEV_SUCCESS, "GLRenderingSystem has been terminated.");
	return true;
}

ObjectStatus GLRenderingSystem::getStatus()
{
	return GLRenderingSystemNS::m_objectStatus;
}

INNO_SYSTEM_EXPORT bool GLRenderingSystem::resize()
{
	GLRenderingSystemNS::deferredPassFBDesc.sizeX = WindowSystemComponent::get().m_windowResolution.x;
	GLRenderingSystemNS::deferredPassFBDesc.sizeY = WindowSystemComponent::get().m_windowResolution.y;

	GLRenderingSystemNS::resizeGLRenderPassComponent(GLGeometryRenderPassComponent::get().m_opaquePass_GLRPC, GLRenderingSystemNS::deferredPassFBDesc);
	GLRenderingSystemNS::resizeGLRenderPassComponent(GLGeometryRenderPassComponent::get().m_transparentPass_GLRPC, GLRenderingSystemNS::deferredPassFBDesc);
	GLRenderingSystemNS::resizeGLRenderPassComponent(GLTerrainRenderPassComponent::get().m_GLRPC, GLRenderingSystemNS::deferredPassFBDesc);
	GLRenderingSystemNS::resizeGLRenderPassComponent(GLLightRenderPassComponent::get().m_GLRPC, GLRenderingSystemNS::deferredPassFBDesc);
	GLRenderingSystemNS::resizeGLRenderPassComponent(GLFinalRenderPassComponent::get().m_skyPassGLRPC, GLRenderingSystemNS::deferredPassFBDesc);
	GLRenderingSystemNS::resizeGLRenderPassComponent(GLFinalRenderPassComponent::get().m_preTAAPassGLRPC, GLRenderingSystemNS::deferredPassFBDesc);
	GLRenderingSystemNS::resizeGLRenderPassComponent(GLFinalRenderPassComponent::get().m_TAAPingPassGLRPC, GLRenderingSystemNS::deferredPassFBDesc);
	GLRenderingSystemNS::resizeGLRenderPassComponent(GLFinalRenderPassComponent::get().m_TAAPongPassGLRPC, GLRenderingSystemNS::deferredPassFBDesc);
	GLRenderingSystemNS::resizeGLRenderPassComponent(GLFinalRenderPassComponent::get().m_TAASharpenPassGLRPC, GLRenderingSystemNS::deferredPassFBDesc);
	GLRenderingSystemNS::resizeGLRenderPassComponent(GLFinalRenderPassComponent::get().m_bloomExtractPassGLRPC, GLRenderingSystemNS::deferredPassFBDesc);
	GLRenderingSystemNS::resizeGLRenderPassComponent(GLFinalRenderPassComponent::get().m_bloomBlurPingPassGLRPC, GLRenderingSystemNS::deferredPassFBDesc);
	GLRenderingSystemNS::resizeGLRenderPassComponent(GLFinalRenderPassComponent::get().m_bloomBlurPongPassGLRPC, GLRenderingSystemNS::deferredPassFBDesc);
	GLRenderingSystemNS::resizeGLRenderPassComponent(GLFinalRenderPassComponent::get().m_motionBlurPassGLRPC, GLRenderingSystemNS::deferredPassFBDesc);
	GLRenderingSystemNS::resizeGLRenderPassComponent(GLFinalRenderPassComponent::get().m_billboardPassGLRPC, GLRenderingSystemNS::deferredPassFBDesc);
	GLRenderingSystemNS::resizeGLRenderPassComponent(GLFinalRenderPassComponent::get().m_debuggerPassGLRPC, GLRenderingSystemNS::deferredPassFBDesc);
	GLRenderingSystemNS::resizeGLRenderPassComponent(GLFinalRenderPassComponent::get().m_finalBlendPassGLRPC, GLRenderingSystemNS::deferredPassFBDesc);

	return true;
}

GLuint GLRenderingSystemNS::getUniformLocation(GLuint shaderProgram, const std::string & uniformName)
{
	// @TODO: UBO
	glUseProgram(shaderProgram);
	int uniformLocation = glGetUniformLocation(shaderProgram, uniformName.c_str());
	if (uniformLocation == 0xFFFFFFFF)
	{
		g_pCoreSystem->getLogSystem()->printLog(LogType::INNO_ERROR, "GLRenderingSystem: innoShader: Uniform lost: " + uniformName);
		return -1;
	}
	return uniformLocation;
}

GLuint GLRenderingSystemNS::getUniformBlockIndex(GLuint shaderProgram, const std::string & uniformBlockName)
{
	glUseProgram(shaderProgram);
	auto uniformBlockIndex = glGetUniformBlockIndex(shaderProgram, uniformBlockName.c_str());
	if (uniformBlockIndex == 0xFFFFFFFF)
	{
		g_pCoreSystem->getLogSystem()->printLog(LogType::INNO_ERROR, "GLRenderingSystem: innoShader: Uniform Block lost: " + uniformBlockName);
		return -1;
	}
	return uniformBlockIndex;
}

GLuint GLRenderingSystemNS::generateUBO(GLuint UBOSize)
{
	GLuint l_ubo;
	glGenBuffers(1, &l_ubo);
	glBindBuffer(GL_UNIFORM_BUFFER, l_ubo);
	glBufferData(GL_UNIFORM_BUFFER, UBOSize, NULL, GL_STATIC_DRAW);
	glBindBuffer(GL_UNIFORM_BUFFER, 0);

	return l_ubo;
}

void GLRenderingSystemNS::bindUniformBlock(GLuint UBO, GLuint UBOSize, GLuint program, const std::string & uniformBlockName, GLuint uniformBlockBindingPoint)
{
	auto uniformBlockIndex = getUniformBlockIndex(program, uniformBlockName.c_str());
	glUniformBlockBinding(program, uniformBlockIndex, uniformBlockBindingPoint);
	glBindBufferRange(GL_UNIFORM_BUFFER, uniformBlockIndex, UBO, 0, UBOSize);
}

template<typename T>
void GLRenderingSystemNS::updateUBO(const GLint& UBO, const T& UBOValue)
{
	glBindBuffer(GL_UNIFORM_BUFFER, UBO);
	glBufferSubData(GL_UNIFORM_BUFFER, 0, sizeof(T), &UBOValue);
	glBindBuffer(GL_UNIFORM_BUFFER, 0);
}

void GLRenderingSystemNS::updateTextureUniformLocations(GLuint program, const std::vector<std::string>& UniformNames)
{
	for (size_t i = 0; i < UniformNames.size(); i++)
	{
		updateUniform(getUniformLocation(program, UniformNames[i]), (int)i);
	}
}

void GLRenderingSystemNS::updateUniform(const GLint uniformLocation, bool uniformValue)
{
	glUniform1i(uniformLocation, (int)uniformValue);
}

void GLRenderingSystemNS::updateUniform(const GLint uniformLocation, int uniformValue)
{
	glUniform1i(uniformLocation, uniformValue);
}

void GLRenderingSystemNS::updateUniform(const GLint uniformLocation, float uniformValue)
{
	glUniform1f(uniformLocation, (GLfloat)uniformValue);
}

void GLRenderingSystemNS::updateUniform(const GLint uniformLocation, float x, float y)
{
	glUniform2f(uniformLocation, (GLfloat)x, (GLfloat)y);
}

void GLRenderingSystemNS::updateUniform(const GLint uniformLocation, float x, float y, float z)
{
	glUniform3f(uniformLocation, (GLfloat)x, (GLfloat)y, (GLfloat)z);
}

void GLRenderingSystemNS::updateUniform(const GLint uniformLocation, float x, float y, float z, float w)
{
	glUniform4f(uniformLocation, (GLfloat)x, (GLfloat)y, (GLfloat)z, (GLfloat)w);
}

void GLRenderingSystemNS::updateUniform(const GLint uniformLocation, const mat4 & mat)
{
#ifdef USE_COLUMN_MAJOR_MEMORY_LAYOUT
	glUniformMatrix4fv(uniformLocation, 1, GL_FALSE, &mat.m00);
#endif
#ifdef USE_ROW_MAJOR_MEMORY_LAYOUT
	glUniformMatrix4fv(uniformLocation, 1, GL_TRUE, &mat.m00);
#endif
}

void GLRenderingSystemNS::attachTextureToFramebuffer(TextureDataComponent * TDC, GLTextureDataComponent * GLTDC, GLFrameBufferComponent * GLFBC, int colorAttachmentIndex, int textureIndex, int mipLevel)
{
	glBindTexture(GLTDC->m_GLTextureDataDesc.textureType, GLTDC->m_TAO);
	if (TDC->m_textureDataDesc.textureUsageType == TextureUsageType::ENVIRONMENT_CAPTURE || TDC->m_textureDataDesc.textureUsageType == TextureUsageType::ENVIRONMENT_CONVOLUTION || TDC->m_textureDataDesc.textureUsageType == TextureUsageType::ENVIRONMENT_PREFILTER)
	{
		glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0 + colorAttachmentIndex, GL_TEXTURE_CUBE_MAP_POSITIVE_X + textureIndex, GLTDC->m_TAO, mipLevel);
	}
	else if (TDC->m_textureDataDesc.textureUsageType == TextureUsageType::SHADOWMAP)
	{
		glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, GLTDC->m_TAO, mipLevel);
	}
	else
	{
		glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0 + colorAttachmentIndex, GL_TEXTURE_2D, GLTDC->m_TAO, mipLevel);
	}
}

void GLRenderingSystemNS::activateShaderProgram(GLShaderProgramComponent * GLShaderProgramComponent)
{
	glUseProgram(GLShaderProgramComponent->m_program);
}

void GLRenderingSystemNS::drawMesh(EntityID rhs)
{
	auto l_MDC = g_pCoreSystem->getAssetSystem()->getMeshDataComponent(rhs);
	if (l_MDC)
	{
		drawMesh(l_MDC);
	}
}

void GLRenderingSystemNS::drawMesh(MeshDataComponent* MDC)
{
	auto l_GLMDC = getGLMeshDataComponent(MDC->m_parentEntity);
	if (l_GLMDC)
	{
		drawMesh(MDC->m_indicesSize, MDC->m_meshDrawMethod, l_GLMDC);
	}
}

void GLRenderingSystemNS::drawMesh(size_t indicesSize, MeshPrimitiveTopology MeshPrimitiveTopology, GLMeshDataComponent* GLMDC)
{
	if (GLMDC->m_VAO)
	{
		glBindVertexArray(GLMDC->m_VAO);
		switch (MeshPrimitiveTopology)
		{
		case MeshPrimitiveTopology::TRIANGLE: glDrawElements(GL_TRIANGLES, (GLsizei)indicesSize, GL_UNSIGNED_INT, 0); break;
		case MeshPrimitiveTopology::TRIANGLE_STRIP: glDrawElements(GL_TRIANGLE_STRIP, (GLsizei)indicesSize, GL_UNSIGNED_INT, 0); break;
		default:
			break;
		}
	}
}

void GLRenderingSystemNS::activateTexture(TextureDataComponent * TDC, int activateIndex)
{
	auto l_GLTDC = getGLTextureDataComponent(TDC->m_parentEntity);
	activateTexture(l_GLTDC, activateIndex);
}

void GLRenderingSystemNS::activateTexture(GLTextureDataComponent * GLTDC, int activateIndex)
{
	glActiveTexture(GL_TEXTURE0 + activateIndex);
	glBindTexture(GLTDC->m_GLTextureDataDesc.textureType, GLTDC->m_TAO);
}
