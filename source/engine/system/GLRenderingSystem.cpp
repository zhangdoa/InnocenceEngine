#include "GLRenderingSystem.h"

#include <sstream>

#include "../component/GLEnvironmentRenderPassSingletonComponent.h"
#include "../component/GLShadowRenderPassSingletonComponent.h"
#include "../component/GLGeometryRenderPassSingletonComponent.h"
#include "../component/GLTerrainRenderPassSingletonComponent.h"
#include "../component/GLLightRenderPassSingletonComponent.h"
#include "../component/GLFinalRenderPassSingletonComponent.h"

#include "../component/AssetSystemSingletonComponent.h"
#include "../component/GameSystemSingletonComponent.h"
#include "../component/WindowSystemSingletonComponent.h"
#include "../component/RenderingSystemSingletonComponent.h"
#include "../component/GLRenderingSystemSingletonComponent.h"
#include "../component/PhysicsSystemSingletonComponent.h"

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
	void initializeDefaultAssets();

	float RadicalInverse(unsigned int n, unsigned int base);
	void initializeHaltonSampler();
	void initializeEnvironmentPass();
	void initializeBRDFLUTPass();
	void initializeShadowPass();
	void initializeGeometryPass();
	void initializeGeometryPassShaders();
	void initializeTerrainPass();
	void initializeTerrainPassShaders();
	void initializeLightPass();
	void initializeLightPassShaders();
	void initializeFinalPass();

	void initializeSkyPass();
	void initializeTAAPass();
	void initializeBloomExtractPass();
	void initializeBloomBlurPass();
	void initializeMotionBlurPass();
	void initializeBillboardPass();
	void initializeDebuggerPass();
	void initializeFinalBlendPass();

	void reloadShader();

	GLRenderPassComponent* addRenderPassComponent(unsigned int RTNum);
	bool resizeGLRenderPassComponent(GLRenderPassComponent* GLRPC);

	struct shaderFilePaths
	{
		std::string m_VSPath;
		std::string m_GSPath;
		std::string m_FSPath;
	};

	GLShaderProgramComponent* addShaderProgramComponent(const shaderFilePaths& shaderFilePaths);

	GLMeshDataComponent* generateGLMeshDataComponent(MeshDataComponent* rhs);
	GLTextureDataComponent* generateGLTextureDataComponent(TextureDataComponent* rhs);


	bool initializeGLTextureDataComponent(GLTextureDataComponent * rhs, TextureDataDesc textureDataDesc, const std::vector<void*>& textureData);

	GLMeshDataComponent* addGLMeshDataComponent(EntityID rhs);
	GLTextureDataComponent* addGLTextureDataComponent(EntityID rhs);

	GLMeshDataComponent* getGLMeshDataComponent(EntityID rhs);
	GLTextureDataComponent* getGLTextureDataComponent(EntityID rhs);

	void prepareRenderingData();
	void updateEnvironmentRenderPass();
	void updateBRDFLUTRenderPass();
	void updateShadowRenderPass();
	void updateGeometryRenderPass();
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

	GLuint getUniformLocation(GLuint shaderProgram, const std::string& uniformName);

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
	void activate2DTexture(GLTextureDataComponent* GLTDC, int activateIndex);
	void activateCubemapTexture(GLTextureDataComponent* GLTDC, int activateIndex);

	static GameSystemSingletonComponent* g_GameSystemSingletonComponent;
	static RenderingSystemSingletonComponent* g_RenderingSystemSingletonComponent;
	static GLRenderingSystemSingletonComponent* g_GLRenderingSystemSingletonComponent;

	std::unordered_map<EntityID, GLMeshDataComponent*> m_initializedMeshComponents;

	ObjectStatus m_objectStatus = ObjectStatus::SHUTDOWN;

	GLMeshDataComponent* m_UnitLineTemplate;
	GLMeshDataComponent* m_UnitQuadTemplate;
	GLMeshDataComponent* m_UnitCubeTemplate;
	GLMeshDataComponent* m_UnitSphereTemplate;
	GLMeshDataComponent* m_terrainGLMDC;

	GLTextureDataComponent* m_basicNormalTemplate;
	GLTextureDataComponent* m_basicAlbedoTemplate;
	GLTextureDataComponent* m_basicMetallicTemplate;
	GLTextureDataComponent* m_basicRoughnessTemplate;
	GLTextureDataComponent* m_basicAOTemplate;

	GLsizei rtSizeX;
	GLsizei rtSizeY;

	mat4 m_CamProjOriginal;
	mat4 m_CamProjJittered;
	mat4 m_CamRot;
	mat4 m_CamTrans;
	mat4 m_CamRot_prev;
	mat4 m_CamTrans_prev;

	struct GPassCBufferData
	{
		mat4 m;
		mat4 m_prev;
		mat4 m_normalMat;
	};

	struct GPassRenderingDataPack
	{
		size_t indiceSize;
		GPassCBufferData GPassCBuffer;
		GLMeshDataComponent* GLMDC;
		MeshPrimitiveTopology m_meshDrawMethod;
		bool useNormalTexture = true;
		bool useAlbedoTexture = true;
		bool useMetallicTexture = true;
		bool useRoughnessTexture = true;
		bool useAOTexture = true;
		GLTextureDataComponent* m_basicNormalGLTDC;
		GLTextureDataComponent* m_basicAlbedoGLTDC;
		GLTextureDataComponent* m_basicMetallicGLTDC;
		GLTextureDataComponent* m_basicRoughnessGLTDC;
		GLTextureDataComponent* m_basicAOGLTDC;
		MeshCustomMaterial meshColor;
		VisiblilityType visiblilityType;
	};

	std::queue<GPassRenderingDataPack> m_GPassRenderingDataQueue;

	struct LPassCBufferData
	{
		vec4 viewPos;
		vec4 lightDir;
		vec4 color;
	};

	LPassCBufferData m_LPassCBufferData;

	std::function<void(GLFrameBufferComponent*)> f_bindDeferredFBC;
	std::function<void(GLFrameBufferComponent*)> f_cleanFBC;
	std::function<void(GLFrameBufferComponent*, GLFrameBufferComponent*)> f_copyDepthBuffer;
	std::function<void(GLFrameBufferComponent*, GLFrameBufferComponent*)> f_copyStencilBuffer;
}

bool GLRenderingSystem::setup()
{
	GLRenderingSystemNS::g_GameSystemSingletonComponent = &GameSystemSingletonComponent::getInstance();
	GLRenderingSystemNS::g_RenderingSystemSingletonComponent = &RenderingSystemSingletonComponent::getInstance();
	GLRenderingSystemNS::g_GLRenderingSystemSingletonComponent = &GLRenderingSystemSingletonComponent::getInstance();

	if (GLRenderingSystemNS::g_RenderingSystemSingletonComponent->m_MSAAdepth)
	{
		// antialiasing
		glfwWindowHint(GLFW_SAMPLES, GLRenderingSystemNS::g_RenderingSystemSingletonComponent->m_MSAAdepth);
		// MSAA
		glEnable(GL_MULTISAMPLE);
	}
	// enable seamless cubemap sampling for lower mip levels in the pre-filter map.
	glEnable(GL_TEXTURE_CUBE_MAP_SEAMLESS);
	glEnable(GL_PROGRAM_POINT_SIZE);
	glEnable(GL_TEXTURE_2D);

	GLRenderingSystemNS::rtSizeX = (GLsizei)WindowSystemSingletonComponent::getInstance().m_windowResolution.x;
	GLRenderingSystemNS::rtSizeY = (GLsizei)WindowSystemSingletonComponent::getInstance().m_windowResolution.y;

	GLRenderingSystemNS::f_bindDeferredFBC = [&](GLFrameBufferComponent* val) {
		GLRenderingSystemNS::f_cleanFBC(val);

		glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH24_STENCIL8, GLRenderingSystemNS::rtSizeX, GLRenderingSystemNS::rtSizeY);
		glViewport(0, 0, GLRenderingSystemNS::rtSizeX, GLRenderingSystemNS::rtSizeY);
	};

	GLRenderingSystemNS::f_cleanFBC = [&](GLFrameBufferComponent* val) {
		glBindFramebuffer(GL_FRAMEBUFFER, val->m_FBO);
		glBindRenderbuffer(GL_RENDERBUFFER, val->m_RBO);

		glClearColor(0.0f, 0.0f, 0.0f, 0.0f);
		glClear(GL_COLOR_BUFFER_BIT);
		glClear(GL_DEPTH_BUFFER_BIT);
		glClear(GL_STENCIL_BUFFER_BIT);
	};

	GLRenderingSystemNS::f_copyDepthBuffer = [&](GLFrameBufferComponent* src, GLFrameBufferComponent* dest) {
		glBindFramebuffer(GL_READ_FRAMEBUFFER, src->m_FBO);
		glBindFramebuffer(GL_DRAW_FRAMEBUFFER, dest->m_FBO);
		glBlitFramebuffer(0, 0, GLRenderingSystemNS::rtSizeX, GLRenderingSystemNS::rtSizeY, 0, 0, GLRenderingSystemNS::rtSizeX, GLRenderingSystemNS::rtSizeY, GL_DEPTH_BUFFER_BIT, GL_NEAREST);
	};

	GLRenderingSystemNS::f_copyStencilBuffer = [&](GLFrameBufferComponent* src, GLFrameBufferComponent* dest) {
		glBindFramebuffer(GL_READ_FRAMEBUFFER, src->m_FBO);
		glBindFramebuffer(GL_DRAW_FRAMEBUFFER, dest->m_FBO);
		glBlitFramebuffer(0, 0, GLRenderingSystemNS::rtSizeX, GLRenderingSystemNS::rtSizeY, 0, 0, GLRenderingSystemNS::rtSizeX, GLRenderingSystemNS::rtSizeY, GL_STENCIL_BUFFER_BIT, GL_NEAREST);
	};

	return true;
}

bool GLRenderingSystem::initialize()
{
	GLRenderingSystemNS::initializeDefaultAssets();
	GLRenderingSystemNS::initializeHaltonSampler();
	GLRenderingSystemNS::initializeEnvironmentPass();
	GLRenderingSystemNS::initializeShadowPass();
	GLRenderingSystemNS::initializeGeometryPass();
	GLRenderingSystemNS::initializeTerrainPass();
	GLRenderingSystemNS::initializeLightPass();
	GLRenderingSystemNS::initializeFinalPass();

	return true;
}

void  GLRenderingSystemNS::initializeDefaultAssets()
{
	m_UnitLineTemplate = generateGLMeshDataComponent(g_pCoreSystem->getAssetSystem()->getMeshDataComponent(MeshShapeType::LINE));
	m_UnitQuadTemplate = generateGLMeshDataComponent(g_pCoreSystem->getAssetSystem()->getMeshDataComponent(MeshShapeType::QUAD));
	m_UnitCubeTemplate = generateGLMeshDataComponent(g_pCoreSystem->getAssetSystem()->getMeshDataComponent(MeshShapeType::CUBE));
	m_UnitSphereTemplate = generateGLMeshDataComponent(g_pCoreSystem->getAssetSystem()->getMeshDataComponent(MeshShapeType::SPHERE));
	m_terrainGLMDC = generateGLMeshDataComponent(g_pCoreSystem->getAssetSystem()->getMeshDataComponent(MeshShapeType::TERRAIN));

	m_basicNormalTemplate = generateGLTextureDataComponent(g_pCoreSystem->getAssetSystem()->getTextureDataComponent(TextureUsageType::NORMAL));
	m_basicAlbedoTemplate = generateGLTextureDataComponent(g_pCoreSystem->getAssetSystem()->getTextureDataComponent(TextureUsageType::ALBEDO));
	m_basicMetallicTemplate = generateGLTextureDataComponent(g_pCoreSystem->getAssetSystem()->getTextureDataComponent(TextureUsageType::METALLIC));
	m_basicRoughnessTemplate = generateGLTextureDataComponent(g_pCoreSystem->getAssetSystem()->getTextureDataComponent(TextureUsageType::ROUGHNESS));
	m_basicAOTemplate = generateGLTextureDataComponent(g_pCoreSystem->getAssetSystem()->getTextureDataComponent(TextureUsageType::AMBIENT_OCCLUSION));
}

float GLRenderingSystemNS::RadicalInverse(unsigned int n, unsigned int base) {

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
		GLRenderingSystemNS::g_RenderingSystemSingletonComponent->HaltonSampler.emplace_back(vec2(RadicalInverse(i, 2) * 2.0f - 1.0f, RadicalInverse(i, 3) * 2.0f - 1.0f));
	}
}

GLRenderPassComponent* GLRenderingSystemNS::addRenderPassComponent(unsigned int RTNum)
{
	auto l_GLRPC = g_pCoreSystem->getMemorySystem()->spawn<GLRenderPassComponent>();

	// generate and bind framebuffer
	auto l_FBC = g_pCoreSystem->getMemorySystem()->spawn<GLFrameBufferComponent>();
	l_GLRPC->m_GLFBC = l_FBC;

	glGenFramebuffers(1, &l_FBC->m_FBO);
	glBindFramebuffer(GL_FRAMEBUFFER, l_FBC->m_FBO);

	// generate and bind renderbuffer
	glGenRenderbuffers(1, &l_FBC->m_RBO);
	glBindRenderbuffer(GL_RENDERBUFFER, l_FBC->m_RBO);

	glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT, GL_RENDERBUFFER, l_FBC->m_RBO);
	glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH24_STENCIL8, rtSizeX, rtSizeY);

	// generate and bind texture
	l_GLRPC->m_TDCs.reserve(RTNum);

	for (unsigned int i = 0; i < RTNum; i++)
	{
		auto l_TDC = g_pCoreSystem->getMemorySystem()->spawn<TextureDataComponent>();

		l_TDC->m_textureDataDesc.textureUsageType = TextureUsageType::RENDER_BUFFER_SAMPLER;
		l_TDC->m_textureDataDesc.textureColorComponentsFormat = TextureColorComponentsFormat::RGBA16F;
		l_TDC->m_textureDataDesc.texturePixelDataFormat = TexturePixelDataFormat::RGBA;
		l_TDC->m_textureDataDesc.textureMinFilterMethod = TextureFilterMethod::NEAREST;
		l_TDC->m_textureDataDesc.textureMagFilterMethod = TextureFilterMethod::NEAREST;
		l_TDC->m_textureDataDesc.textureWrapMethod = TextureWrapMethod::CLAMP_TO_EDGE;
		l_TDC->m_textureDataDesc.textureWidth = rtSizeX;
		l_TDC->m_textureDataDesc.textureHeight = rtSizeY;
		l_TDC->m_textureDataDesc.texturePixelDataType = TexturePixelDataType::FLOAT;
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

	std::vector<unsigned int> l_colorAttachments;
	for (unsigned int i = 0; i < RTNum; ++i)
	{
		l_colorAttachments.emplace_back(GL_COLOR_ATTACHMENT0 + i);
	}
	glDrawBuffers((GLsizei)l_colorAttachments.size(), &l_colorAttachments[0]);

	// finally check if framebuffer is complete
	if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
	{
		g_pCoreSystem->getLogSystem()->printLog(LogType::INNO_ERROR, "GLRenderingSystem: Framebuffer is not completed!");
	}

	glBindRenderbuffer(GL_RENDERBUFFER, 0);
	glBindFramebuffer(GL_FRAMEBUFFER, 0);

	return l_GLRPC;
}


bool GLRenderingSystemNS::resizeGLRenderPassComponent(GLRenderPassComponent * GLRPC)
{
	glBindFramebuffer(GL_FRAMEBUFFER, GLRPC->m_GLFBC->m_FBO);
	glBindRenderbuffer(GL_RENDERBUFFER, GLRPC->m_GLFBC->m_RBO);

	for (unsigned int i = 0; i < GLRPC->m_GLTDCs.size(); i++)
	{
		GLRPC->m_TDCs[i]->m_textureDataDesc.textureWidth = rtSizeX;
		GLRPC->m_TDCs[i]->m_textureDataDesc.textureHeight = rtSizeY;
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

GLShaderProgramComponent* GLRenderingSystemNS::addShaderProgramComponent(const shaderFilePaths& shaderFilePaths)
{
	auto l_GLSPC = g_pCoreSystem->getMemorySystem()->spawn<GLShaderProgramComponent>();

	l_GLSPC->m_program = glCreateProgram();

	std::function<void(GLuint& shaderProgram, GLuint ShaderType, const std::string & shaderFilePath)> f_addShader =
		[&](GLuint& shaderProgram, GLuint shaderType, const std::string & shaderFilePath) {
		auto shaderID = glCreateShader(shaderType);

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

		glShaderSource(shaderID, 1, &l_sourcePointer, NULL);
		glCompileShader(shaderID);

		GLint l_compileResult = GL_FALSE;
		GLint l_infoLogLength = 0;
		GLint l_shaderFileLength = 0;
		glGetShaderiv(shaderID, GL_COMPILE_STATUS, &l_compileResult);

		if (!l_compileResult)
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

		glAttachShader(shaderProgram, shaderID);
		glLinkProgram(shaderProgram);
		glValidateProgram(shaderProgram);

		g_pCoreSystem->getLogSystem()->printLog(LogType::INNO_DEV_VERBOSE, "GLRenderingSystem: innoShader: " + shaderFilePath + " is linking ...");

		glGetShaderiv(shaderID, GL_COMPILE_STATUS, &l_compileResult);
		if (!l_compileResult)
		{
			g_pCoreSystem->getLogSystem()->printLog(LogType::INNO_ERROR, "GLRenderingSystem: innoShader: " + shaderFilePath + " link failed!");
			glGetShaderiv(shaderID, GL_SHADER_SOURCE_LENGTH, &l_shaderFileLength);
			g_pCoreSystem->getLogSystem()->printLog(LogType::INNO_DEV_VERBOSE, "GLRenderingSystem: innoShader: " + shaderFilePath + " file length is: " + std::to_string(l_shaderFileLength));
			glGetShaderiv(shaderID, GL_INFO_LOG_LENGTH, &l_infoLogLength);

			if (l_infoLogLength > 0) {
				std::vector<char> l_shaderErrorMessage(l_infoLogLength + 1);
				glGetShaderInfoLog(shaderID, l_infoLogLength, NULL, &l_shaderErrorMessage[0]);
				g_pCoreSystem->getLogSystem()->printLog(LogType::INNO_DEV_VERBOSE, "GLRenderingSystem: innoShader: " + shaderFilePath + " link error: " + &l_shaderErrorMessage[0] + "\n -- --------------------------------------------------- -- ");
				return;
			}

			return;
		}

		g_pCoreSystem->getLogSystem()->printLog(LogType::INNO_DEV_SUCCESS, "GLRenderingSystem: innoShader: " + shaderFilePath + " Shader has been linked.");
	};

	if (!shaderFilePaths.m_VSPath.empty())
	{
		f_addShader(l_GLSPC->m_program, GL_VERTEX_SHADER, shaderFilePaths.m_VSPath);
	}
	if (!shaderFilePaths.m_GSPath.empty())
	{
		f_addShader(l_GLSPC->m_program, GL_GEOMETRY_SHADER, shaderFilePaths.m_GSPath);
	}
	if (!shaderFilePaths.m_FSPath.empty())
	{
		f_addShader(l_GLSPC->m_program, GL_FRAGMENT_SHADER, shaderFilePaths.m_FSPath);
	}

	return l_GLSPC;
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

	GLEnvironmentRenderPassSingletonComponent::getInstance().m_BRDFLUTPassFBC = l_FBC;

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

	GLEnvironmentRenderPassSingletonComponent::getInstance().m_BRDFSplitSumLUTPassTDC = l_TDC;

	auto l_GLTDC = generateGLTextureDataComponent(l_TDC);

	GLEnvironmentRenderPassSingletonComponent::getInstance().m_BRDFSplitSumLUTPassGLTDC = l_GLTDC;

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

	GLEnvironmentRenderPassSingletonComponent::getInstance().m_BRDFMSAverageLUTPassTDC = l_TDC;

	l_GLTDC = generateGLTextureDataComponent(l_TDC);

	GLEnvironmentRenderPassSingletonComponent::getInstance().m_BRDFMSAverageLUTPassGLTDC = l_GLTDC;

	// finally check if framebuffer is complete
	if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
	{
		g_pCoreSystem->getLogSystem()->printLog(LogType::INNO_ERROR, "GLRenderingSystem: BRDFLUTRenderPass Framebuffer is not completed!");
	}

	glBindRenderbuffer(GL_RENDERBUFFER, 0);
	glBindFramebuffer(GL_FRAMEBUFFER, 0);

	// shader programs and shaders
	shaderFilePaths m_shaderFilePaths;

	////
	m_shaderFilePaths.m_VSPath = "GL4.0//BRDFLUTPassVertex.sf";
	m_shaderFilePaths.m_FSPath = "GL4.0//BRDFLUTPassFragment.sf";

	auto l_GLSPC = addShaderProgramComponent(m_shaderFilePaths);

	GLEnvironmentRenderPassSingletonComponent::getInstance().m_BRDFSplitSumLUTPassSPC = l_GLSPC;

	////
	m_shaderFilePaths.m_VSPath = "GL4.0//BRDFLUTMSPassVertex.sf";
	m_shaderFilePaths.m_FSPath = "GL4.0//BRDFLUTMSPassFragment.sf";

	l_GLSPC = addShaderProgramComponent(m_shaderFilePaths);

	GLEnvironmentRenderPassSingletonComponent::getInstance().m_BRDFMSAverageLUTPass_uni_brdfLUT = getUniformLocation(
		l_GLSPC->m_program,
		"uni_brdfLUT");
	updateUniform(
		GLEnvironmentRenderPassSingletonComponent::getInstance().m_BRDFMSAverageLUTPass_uni_brdfLUT,
		0);

	GLEnvironmentRenderPassSingletonComponent::getInstance().m_BRDFMSAverageLUTPassSPC = l_GLSPC;
}

void GLRenderingSystemNS::initializeEnvironmentPass()
{
	initializeBRDFLUTPass();
}

void GLRenderingSystemNS::initializeShadowPass()
{
	for (size_t i = 0; i < 4; i++)
	{
		// generate and bind framebuffer
		auto l_FBC = g_pCoreSystem->getMemorySystem()->spawn<GLFrameBufferComponent>();

		glGenFramebuffers(1, &l_FBC->m_FBO);
		glBindFramebuffer(GL_FRAMEBUFFER, l_FBC->m_FBO);

		// generate and bind renderbuffer
		glGenRenderbuffers(1, &l_FBC->m_RBO);
		glBindRenderbuffer(GL_RENDERBUFFER, l_FBC->m_RBO);

		glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, l_FBC->m_RBO);
		glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT32, 2048, 2048);

		GLShadowRenderPassSingletonComponent::getInstance().m_FBCs.emplace_back(l_FBC);

		// generate and bind texture
		auto l_TDC = g_pCoreSystem->getMemorySystem()->spawn<TextureDataComponent>();

		l_TDC->m_textureDataDesc.textureUsageType = TextureUsageType::SHADOWMAP;
		l_TDC->m_textureDataDesc.textureColorComponentsFormat = TextureColorComponentsFormat::DEPTH_COMPONENT;
		l_TDC->m_textureDataDesc.texturePixelDataFormat = TexturePixelDataFormat::DEPTH_COMPONENT;
		l_TDC->m_textureDataDesc.textureMinFilterMethod = TextureFilterMethod::NEAREST;
		l_TDC->m_textureDataDesc.textureMagFilterMethod = TextureFilterMethod::NEAREST;
		l_TDC->m_textureDataDesc.textureWrapMethod = TextureWrapMethod::CLAMP_TO_BORDER;
		l_TDC->m_textureDataDesc.textureWidth = 2048;
		l_TDC->m_textureDataDesc.textureHeight = 2048;
		l_TDC->m_textureDataDesc.texturePixelDataType = TexturePixelDataType::FLOAT;
		l_TDC->m_textureData = { nullptr };

		GLShadowRenderPassSingletonComponent::getInstance().m_TDCs.emplace_back(l_TDC);

		auto l_GLTDC = generateGLTextureDataComponent(l_TDC);

		attachTextureToFramebuffer(
			l_TDC,
			l_GLTDC,
			l_FBC,
			0, 0, 0
		);

		GLShadowRenderPassSingletonComponent::getInstance().m_GLTDCs.emplace_back(l_GLTDC);

		glDrawBuffer(GL_NONE);
		glReadBuffer(GL_NONE);

		// finally check if framebuffer is complete
		if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
		{
			std::stringstream ss;
			ss << i;
			g_pCoreSystem->getLogSystem()->printLog(LogType::INNO_ERROR, "GLRenderingSystem: ShadowRenderPass level " + ss.str() + " Framebuffer is not completed!");
		}
	}

	glBindRenderbuffer(GL_RENDERBUFFER, 0);
	glBindFramebuffer(GL_FRAMEBUFFER, 0);

	// shader programs and shaders
	shaderFilePaths m_shaderFilePaths;

	m_shaderFilePaths.m_VSPath = "GL4.0//shadowPassVertex.sf";
	m_shaderFilePaths.m_FSPath = "GL4.0//shadowPassFragment.sf";

	auto l_GLSPC = addShaderProgramComponent(m_shaderFilePaths);

	GLShadowRenderPassSingletonComponent::getInstance().m_shadowPass_uni_p = getUniformLocation(
		l_GLSPC->m_program,
		"uni_p");
	GLShadowRenderPassSingletonComponent::getInstance().m_shadowPass_uni_v = getUniformLocation(
		l_GLSPC->m_program,
		"uni_v");
	GLShadowRenderPassSingletonComponent::getInstance().m_shadowPass_uni_m = getUniformLocation(
		l_GLSPC->m_program,
		"uni_m");

	GLShadowRenderPassSingletonComponent::getInstance().m_SPC = l_GLSPC;
}

void GLRenderingSystemNS::initializeGeometryPass()
{
	GLGeometryRenderPassSingletonComponent::getInstance().m_GLRPC = addRenderPassComponent(8);

	initializeGeometryPassShaders();
}

void GLRenderingSystemNS::initializeGeometryPassShaders()
{
	// shader programs and shaders
	shaderFilePaths m_shaderFilePaths;

#ifdef CookTorrance
	m_shaderFilePaths.m_VSPath = "GL4.0//geometryPassCookTorranceVertex.sf";
	m_shaderFilePaths.m_FSPath = "GL4.0//geometryPassCookTorranceFragment.sf";
#elif BlinnPhong
	m_shaderFilePaths.m_VSPath = "GL4.0//geometryPassBlinnPhongVertex.sf";
	m_shaderFilePaths.m_FSPath = "GL4.0//geometryPassBlinnPhongFragment.sf";
#endif

	auto l_GLSPC = addShaderProgramComponent(m_shaderFilePaths);

	GLGeometryRenderPassSingletonComponent::getInstance().m_geometryPass_uni_p_camera_original = getUniformLocation(
		l_GLSPC->m_program,
		"uni_p_camera_original");
	GLGeometryRenderPassSingletonComponent::getInstance().m_geometryPass_uni_p_camera_jittered = getUniformLocation(
		l_GLSPC->m_program,
		"uni_p_camera_jittered");
	GLGeometryRenderPassSingletonComponent::getInstance().m_geometryPass_uni_r_camera = getUniformLocation(
		l_GLSPC->m_program,
		"uni_r_camera");
	GLGeometryRenderPassSingletonComponent::getInstance().m_geometryPass_uni_t_camera = getUniformLocation(
		l_GLSPC->m_program,
		"uni_t_camera");
	GLGeometryRenderPassSingletonComponent::getInstance().m_geometryPass_uni_m = getUniformLocation(
		l_GLSPC->m_program,
		"uni_m");
#ifdef CookTorrance
	GLGeometryRenderPassSingletonComponent::getInstance().m_geometryPass_uni_r_camera_prev = getUniformLocation(
		l_GLSPC->m_program,
		"uni_r_camera_prev");
	GLGeometryRenderPassSingletonComponent::getInstance().m_geometryPass_uni_t_camera_prev = getUniformLocation(
		l_GLSPC->m_program,
		"uni_t_camera_prev");
	GLGeometryRenderPassSingletonComponent::getInstance().m_geometryPass_uni_m_prev = getUniformLocation(
		l_GLSPC->m_program,
		"uni_m_prev");
	GLGeometryRenderPassSingletonComponent::getInstance().m_geometryPass_uni_p_light_0 = getUniformLocation(
		l_GLSPC->m_program,
		"uni_p_light_0");
	GLGeometryRenderPassSingletonComponent::getInstance().m_geometryPass_uni_p_light_1 = getUniformLocation(
		l_GLSPC->m_program,
		"uni_p_light_1");
	GLGeometryRenderPassSingletonComponent::getInstance().m_geometryPass_uni_p_light_2 = getUniformLocation(
		l_GLSPC->m_program,
		"uni_p_light_2");
	GLGeometryRenderPassSingletonComponent::getInstance().m_geometryPass_uni_p_light_3 = getUniformLocation(
		l_GLSPC->m_program,
		"uni_p_light_3");
	GLGeometryRenderPassSingletonComponent::getInstance().m_geometryPass_uni_v_light = getUniformLocation(
		l_GLSPC->m_program,
		"uni_v_light");
	GLGeometryRenderPassSingletonComponent::getInstance().m_geometryPass_uni_normalTexture = getUniformLocation(
		l_GLSPC->m_program,
		"uni_normalTexture");
	updateUniform(
		GLGeometryRenderPassSingletonComponent::getInstance().m_geometryPass_uni_normalTexture,
		0);
	GLGeometryRenderPassSingletonComponent::getInstance().m_geometryPass_uni_albedoTexture = getUniformLocation(
		l_GLSPC->m_program,
		"uni_albedoTexture");
	updateUniform(
		GLGeometryRenderPassSingletonComponent::getInstance().m_geometryPass_uni_albedoTexture,
		1);
	GLGeometryRenderPassSingletonComponent::getInstance().m_geometryPass_uni_metallicTexture = getUniformLocation(
		l_GLSPC->m_program,
		"uni_metallicTexture");
	updateUniform(
		GLGeometryRenderPassSingletonComponent::getInstance().m_geometryPass_uni_metallicTexture,
		2);
	GLGeometryRenderPassSingletonComponent::getInstance().m_geometryPass_uni_roughnessTexture = getUniformLocation(
		l_GLSPC->m_program,
		"uni_roughnessTexture");
	updateUniform(
		GLGeometryRenderPassSingletonComponent::getInstance().m_geometryPass_uni_roughnessTexture,
		3);
	GLGeometryRenderPassSingletonComponent::getInstance().m_geometryPass_uni_aoTexture = getUniformLocation(
		l_GLSPC->m_program,
		"uni_aoTexture");
	updateUniform(
		GLGeometryRenderPassSingletonComponent::getInstance().m_geometryPass_uni_aoTexture,
		4);
	GLGeometryRenderPassSingletonComponent::getInstance().m_geometryPass_uni_useNormalTexture = getUniformLocation(
		l_GLSPC->m_program,
		"uni_useNormalTexture");
	GLGeometryRenderPassSingletonComponent::getInstance().m_geometryPass_uni_useAlbedoTexture = getUniformLocation(
		l_GLSPC->m_program,
		"uni_useAlbedoTexture");
	GLGeometryRenderPassSingletonComponent::getInstance().m_geometryPass_uni_useMetallicTexture = getUniformLocation(
		l_GLSPC->m_program,
		"uni_useMetallicTexture");
	GLGeometryRenderPassSingletonComponent::getInstance().m_geometryPass_uni_useRoughnessTexture = getUniformLocation(
		l_GLSPC->m_program,
		"uni_useRoughnessTexture");
	GLGeometryRenderPassSingletonComponent::getInstance().m_geometryPass_uni_useAOTexture = getUniformLocation(
		l_GLSPC->m_program,
		"uni_useAOTexture");
	GLGeometryRenderPassSingletonComponent::getInstance().m_geometryPass_uni_albedo = getUniformLocation(
		l_GLSPC->m_program,
		"uni_albedo");
	GLGeometryRenderPassSingletonComponent::getInstance().m_geometryPass_uni_MRA = getUniformLocation(
		l_GLSPC->m_program,
		"uni_MRA");
#elif BlinnPhong
	// @TODO: texture uniforms
#endif

	GLGeometryRenderPassSingletonComponent::getInstance().m_GLSPC = l_GLSPC;
}

void GLRenderingSystemNS::initializeTerrainPass()
{
	GLTerrainRenderPassSingletonComponent::getInstance().m_GLRPC = addRenderPassComponent(1);

	initializeTerrainPassShaders();
}

void GLRenderingSystemNS::initializeTerrainPassShaders()
{
	// shader programs and shaders
	shaderFilePaths m_shaderFilePaths;

	m_shaderFilePaths.m_VSPath = "GL4.0//terrainPassVertex.sf";
	m_shaderFilePaths.m_FSPath = "GL4.0//terrainPassFragment.sf";

	auto l_GLSPC = addShaderProgramComponent(m_shaderFilePaths);

	GLTerrainRenderPassSingletonComponent::getInstance().m_terrainPass_uni_p_camera = getUniformLocation(
		l_GLSPC->m_program,
		"uni_p_camera");
	GLTerrainRenderPassSingletonComponent::getInstance().m_terrainPass_uni_r_camera = getUniformLocation(
		l_GLSPC->m_program,
		"uni_r_camera");
	GLTerrainRenderPassSingletonComponent::getInstance().m_terrainPass_uni_t_camera = getUniformLocation(
		l_GLSPC->m_program,
		"uni_t_camera");
	GLTerrainRenderPassSingletonComponent::getInstance().m_terrainPass_uni_m = getUniformLocation(
		l_GLSPC->m_program,
		"uni_m");
	GLTerrainRenderPassSingletonComponent::getInstance().m_terrainPass_uni_albedoTexture = getUniformLocation(
		l_GLSPC->m_program,
		"uni_albedoTexture");
	updateUniform(
		GLTerrainRenderPassSingletonComponent::getInstance().m_terrainPass_uni_albedoTexture,
		0);

	GLTerrainRenderPassSingletonComponent::getInstance().m_GLSPC = l_GLSPC;
}

void GLRenderingSystemNS::initializeLightPass()
{
	GLLightRenderPassSingletonComponent::getInstance().m_GLRPC = addRenderPassComponent(1);

	initializeLightPassShaders();
}

void GLRenderingSystemNS::initializeLightPassShaders()
{
	// shader programs and shaders
	shaderFilePaths m_shaderFilePaths;

#ifdef CookTorrance
	m_shaderFilePaths.m_VSPath = "GL4.0//lightPassCookTorranceVertex.sf";
	m_shaderFilePaths.m_FSPath = "GL4.0//lightPassCookTorranceFragment.sf";
#elif BlinnPhong
	m_shaderFilePaths.m_VSPath = "GL4.0//lightPassBlinnPhongVertex.sf";
	m_shaderFilePaths.m_FSPath = "GL4.0//lightPassBlinnPhongFragment.sf";
#endif

	auto l_GLSPC = addShaderProgramComponent(m_shaderFilePaths);

	GLLightRenderPassSingletonComponent::getInstance().m_uni_geometryPassRT0 = getUniformLocation(
		l_GLSPC->m_program,
		"uni_geometryPassRT0");
	updateUniform(
		GLLightRenderPassSingletonComponent::getInstance().m_uni_geometryPassRT0,
		0);
	GLLightRenderPassSingletonComponent::getInstance().m_uni_geometryPassRT1 = getUniformLocation(
		l_GLSPC->m_program,
		"uni_geometryPassRT1");
	updateUniform(
		GLLightRenderPassSingletonComponent::getInstance().m_uni_geometryPassRT1,
		1);
	GLLightRenderPassSingletonComponent::getInstance().m_uni_geometryPassRT2 = getUniformLocation(
		l_GLSPC->m_program,
		"uni_geometryPassRT2");
	updateUniform(
		GLLightRenderPassSingletonComponent::getInstance().m_uni_geometryPassRT2,
		2);
	GLLightRenderPassSingletonComponent::getInstance().m_uni_geometryPassRT3 = getUniformLocation(
		l_GLSPC->m_program,
		"uni_geometryPassRT3");
	updateUniform(
		GLLightRenderPassSingletonComponent::getInstance().m_uni_geometryPassRT3,
		3);
	GLLightRenderPassSingletonComponent::getInstance().m_uni_geometryPassRT4 = getUniformLocation(
		l_GLSPC->m_program,
		"uni_geometryPassRT4");
	updateUniform(
		GLLightRenderPassSingletonComponent::getInstance().m_uni_geometryPassRT4,
		4);
	GLLightRenderPassSingletonComponent::getInstance().m_uni_geometryPassRT5 = getUniformLocation(
		l_GLSPC->m_program,
		"uni_geometryPassRT5");
	updateUniform(
		GLLightRenderPassSingletonComponent::getInstance().m_uni_geometryPassRT5,
		5);
	GLLightRenderPassSingletonComponent::getInstance().m_uni_geometryPassRT6 = getUniformLocation(
		l_GLSPC->m_program,
		"uni_geometryPassRT6");
	updateUniform(
		GLLightRenderPassSingletonComponent::getInstance().m_uni_geometryPassRT6,
		6);
	GLLightRenderPassSingletonComponent::getInstance().m_uni_geometryPassRT7 = getUniformLocation(
		l_GLSPC->m_program,
		"uni_geometryPassRT7");
	updateUniform(
		GLLightRenderPassSingletonComponent::getInstance().m_uni_geometryPassRT7,
		7);
	GLLightRenderPassSingletonComponent::getInstance().m_uni_shadowMap_0 = getUniformLocation(
		l_GLSPC->m_program,
		"uni_shadowMap_0");
	updateUniform(
		GLLightRenderPassSingletonComponent::getInstance().m_uni_shadowMap_0,
		8);
	GLLightRenderPassSingletonComponent::getInstance().m_uni_shadowMap_1 = getUniformLocation(
		l_GLSPC->m_program,
		"uni_shadowMap_1");
	updateUniform(
		GLLightRenderPassSingletonComponent::getInstance().m_uni_shadowMap_1,
		9);
	GLLightRenderPassSingletonComponent::getInstance().m_uni_shadowMap_2 = getUniformLocation(
		l_GLSPC->m_program,
		"uni_shadowMap_2");
	updateUniform(
		GLLightRenderPassSingletonComponent::getInstance().m_uni_shadowMap_2,
		10);
	GLLightRenderPassSingletonComponent::getInstance().m_uni_shadowMap_3 = getUniformLocation(
		l_GLSPC->m_program,
		"uni_shadowMap_3");
	updateUniform(
		GLLightRenderPassSingletonComponent::getInstance().m_uni_shadowMap_3,
		11);
	for (size_t i = 0; i < 4; i++)
	{
		std::stringstream ss;
		ss << i;
		GLLightRenderPassSingletonComponent::getInstance().m_uni_shadowSplitAreas.emplace_back(
			getUniformLocation(l_GLSPC->m_program, "uni_shadowSplitAreas[" + ss.str() + "]")
		);
	}
	GLLightRenderPassSingletonComponent::getInstance().m_uni_brdfLUT = getUniformLocation(
		l_GLSPC->m_program,
		"uni_brdfLUT");
	updateUniform(
		GLLightRenderPassSingletonComponent::getInstance().m_uni_brdfLUT,
		12);
	GLLightRenderPassSingletonComponent::getInstance().m_uni_brdfMSLUT = getUniformLocation(
		l_GLSPC->m_program,
		"uni_brdfMSLUT");
	updateUniform(
		GLLightRenderPassSingletonComponent::getInstance().m_uni_brdfMSLUT,
		13);
	GLLightRenderPassSingletonComponent::getInstance().m_uni_viewPos = getUniformLocation(
		l_GLSPC->m_program,
		"uni_viewPos");
	GLLightRenderPassSingletonComponent::getInstance().m_uni_dirLight_direction = getUniformLocation(
		l_GLSPC->m_program,
		"uni_dirLight.direction");
	GLLightRenderPassSingletonComponent::getInstance().m_uni_dirLight_color = getUniformLocation(
		l_GLSPC->m_program,
		"uni_dirLight.color");
	int l_pointLightIndexOffset = 0;
	for (auto i = (unsigned int)0; i < GLRenderingSystemNS::g_GameSystemSingletonComponent->m_LightComponents.size(); i++)
	{
		if (GLRenderingSystemNS::g_GameSystemSingletonComponent->m_LightComponents[i]->m_lightType == LightType::DIRECTIONAL)
		{
			l_pointLightIndexOffset -= 1;
		}
		if (GLRenderingSystemNS::g_GameSystemSingletonComponent->m_LightComponents[i]->m_lightType == LightType::POINT)
		{
			std::stringstream ss;
			ss << i + l_pointLightIndexOffset;
			GLLightRenderPassSingletonComponent::getInstance().m_uni_pointLights_position.emplace_back(
				getUniformLocation(l_GLSPC->m_program, "uni_pointLights[" + ss.str() + "].position")
			);
			GLLightRenderPassSingletonComponent::getInstance().m_uni_pointLights_radius.emplace_back(
				getUniformLocation(l_GLSPC->m_program, "uni_pointLights[" + ss.str() + "].radius")
			);
			GLLightRenderPassSingletonComponent::getInstance().m_uni_pointLights_color.emplace_back(
				getUniformLocation(l_GLSPC->m_program, "uni_pointLights[" + ss.str() + "].color")
			);
		}
	}
	GLLightRenderPassSingletonComponent::getInstance().m_uni_isEmissive = getUniformLocation(
		l_GLSPC->m_program,
		"uni_isEmissive");

	GLLightRenderPassSingletonComponent::getInstance().m_GLSPC = l_GLSPC;
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
	GLFinalRenderPassSingletonComponent::getInstance().m_skyPassGLRPC = addRenderPassComponent(1);

	// shader programs and shaders
	shaderFilePaths m_shaderFilePaths;

	m_shaderFilePaths.m_VSPath = "GL4.0//skyPassVertex.sf";
	m_shaderFilePaths.m_FSPath = "GL4.0//skyPassFragment.sf";

	auto l_GLSPC = addShaderProgramComponent(m_shaderFilePaths);

	GLFinalRenderPassSingletonComponent::getInstance().m_skyPass_uni_p = getUniformLocation(
		l_GLSPC->m_program,
		"uni_p");
	GLFinalRenderPassSingletonComponent::getInstance().m_skyPass_uni_r = getUniformLocation(
		l_GLSPC->m_program,
		"uni_r");
	GLFinalRenderPassSingletonComponent::getInstance().m_skyPass_uni_viewportSize = getUniformLocation(
		l_GLSPC->m_program,
		"uni_viewportSize");
	GLFinalRenderPassSingletonComponent::getInstance().m_skyPass_uni_eyePos = getUniformLocation(
		l_GLSPC->m_program,
		"uni_eyePos");
	GLFinalRenderPassSingletonComponent::getInstance().m_skyPass_uni_lightDir = getUniformLocation(
		l_GLSPC->m_program,
		"uni_lightDir");

	GLFinalRenderPassSingletonComponent::getInstance().m_skyPassSPC = l_GLSPC;
}

void GLRenderingSystemNS::initializeTAAPass()
{
	//pre mix pass
	GLFinalRenderPassSingletonComponent::getInstance().m_preTAAPassGLRPC = addRenderPassComponent(1);

	//Ping pass
	GLFinalRenderPassSingletonComponent::getInstance().m_TAAPingPassGLRPC = addRenderPassComponent(1);

	//Pong pass
	GLFinalRenderPassSingletonComponent::getInstance().m_TAAPongPassGLRPC = addRenderPassComponent(1);

	//Sharpen pass
	GLFinalRenderPassSingletonComponent::getInstance().m_TAASharpenPassGLRPC = addRenderPassComponent(1);

	// shader programs and shaders
	shaderFilePaths m_shaderFilePaths;

	m_shaderFilePaths.m_VSPath = "GL4.0//preTAAPassVertex.sf";
	m_shaderFilePaths.m_FSPath = "GL4.0//preTAAPassFragment.sf";

	auto l_GLSPC = addShaderProgramComponent(m_shaderFilePaths);

	GLFinalRenderPassSingletonComponent::getInstance().m_preTAAPass_uni_lightPassRT0 = getUniformLocation(
		l_GLSPC->m_program,
		"uni_lightPassRT0");
	updateUniform(
		GLFinalRenderPassSingletonComponent::getInstance().m_preTAAPass_uni_lightPassRT0,
		0);
	GLFinalRenderPassSingletonComponent::getInstance().m_preTAAPass_uni_skyPassRT0 = getUniformLocation(
		l_GLSPC->m_program,
		"uni_skyPassRT0");
	updateUniform(
		GLFinalRenderPassSingletonComponent::getInstance().m_preTAAPass_uni_skyPassRT0,
		1);
	GLFinalRenderPassSingletonComponent::getInstance().m_preTAAPass_uni_terrainPassRT0 = getUniformLocation(
		l_GLSPC->m_program,
		"uni_terrainPassRT0");
	updateUniform(
		GLFinalRenderPassSingletonComponent::getInstance().m_preTAAPass_uni_terrainPassRT0,
		2);
	GLFinalRenderPassSingletonComponent::getInstance().m_preTAAPassSPC = l_GLSPC;

	m_shaderFilePaths.m_VSPath = "GL4.0//TAAPassVertex.sf";
	m_shaderFilePaths.m_FSPath = "GL4.0//TAAPassFragment.sf";

	l_GLSPC = addShaderProgramComponent(m_shaderFilePaths);


	GLFinalRenderPassSingletonComponent::getInstance().m_TAAPass_uni_preTAAPassRT0 = getUniformLocation(
		l_GLSPC->m_program,
		"uni_preTAAPassRT0");
	updateUniform(
		GLFinalRenderPassSingletonComponent::getInstance().m_TAAPass_uni_preTAAPassRT0,
		0);
	GLFinalRenderPassSingletonComponent::getInstance().m_TAAPass_uni_lastFrameTAAPassRT0 = getUniformLocation(
		l_GLSPC->m_program,
		"uni_lastFrameTAAPassRT0");
	updateUniform(
		GLFinalRenderPassSingletonComponent::getInstance().m_TAAPass_uni_lastFrameTAAPassRT0,
		1);
	GLFinalRenderPassSingletonComponent::getInstance().m_TAAPass_uni_motionVectorTexture = getUniformLocation(
		l_GLSPC->m_program,
		"uni_motionVectorTexture");
	updateUniform(
		GLFinalRenderPassSingletonComponent::getInstance().m_TAAPass_uni_motionVectorTexture,
		2);
	GLFinalRenderPassSingletonComponent::getInstance().m_TAAPass_uni_renderTargetSize = getUniformLocation(
		l_GLSPC->m_program,
		"uni_renderTargetSize");

	GLFinalRenderPassSingletonComponent::getInstance().m_TAAPassSPC = l_GLSPC;

	m_shaderFilePaths.m_VSPath = "GL4.0//TAASharpenPassVertex.sf";
	m_shaderFilePaths.m_FSPath = "GL4.0//TAASharpenPassFragment.sf";

	l_GLSPC = addShaderProgramComponent(m_shaderFilePaths);

	GLFinalRenderPassSingletonComponent::getInstance().m_TAASharpenPass_uni_lastTAAPassRT0 = getUniformLocation(
		l_GLSPC->m_program,
		"uni_lastTAAPassRT0");
	updateUniform(
		GLFinalRenderPassSingletonComponent::getInstance().m_TAASharpenPass_uni_lastTAAPassRT0,
		0);
	GLFinalRenderPassSingletonComponent::getInstance().m_TAASharpenPass_uni_renderTargetSize = getUniformLocation(
		l_GLSPC->m_program,
		"uni_renderTargetSize");

	GLFinalRenderPassSingletonComponent::getInstance().m_TAASharpenPassSPC = l_GLSPC;
}

void GLRenderingSystemNS::initializeBloomExtractPass()
{
	GLFinalRenderPassSingletonComponent::getInstance().m_bloomExtractPassGLRPC = addRenderPassComponent(1);

	// shader programs and shaders
	shaderFilePaths m_shaderFilePaths;

	m_shaderFilePaths.m_VSPath = "GL4.0//bloomExtractPassVertex.sf";
	m_shaderFilePaths.m_FSPath = "GL4.0//bloomExtractPassFragment.sf";

	auto l_GLSPC = addShaderProgramComponent(m_shaderFilePaths);

	GLFinalRenderPassSingletonComponent::getInstance().m_bloomExtractPass_uni_TAAPassRT0 = getUniformLocation(
		l_GLSPC->m_program,
		"uni_TAAPassRT0");
	updateUniform(
		GLFinalRenderPassSingletonComponent::getInstance().m_bloomExtractPass_uni_TAAPassRT0,
		0);

	GLFinalRenderPassSingletonComponent::getInstance().m_bloomExtractPassSPC = l_GLSPC;
}

void GLRenderingSystemNS::initializeBloomBlurPass()
{
	//Ping pass
	GLFinalRenderPassSingletonComponent::getInstance().m_bloomBlurPingPassGLRPC = addRenderPassComponent(1);

	//Pong pass
	GLFinalRenderPassSingletonComponent::getInstance().m_bloomBlurPongPassGLRPC = addRenderPassComponent(1);

	// shader programs and shaders
	shaderFilePaths m_shaderFilePaths;

	m_shaderFilePaths.m_VSPath = "GL4.0//bloomBlurPassVertex.sf";
	m_shaderFilePaths.m_FSPath = "GL4.0//bloomBlurPassFragment.sf";

	auto l_GLSPC = addShaderProgramComponent(m_shaderFilePaths);

	GLFinalRenderPassSingletonComponent::getInstance().m_bloomBlurPass_uni_bloomExtractPassRT0 = getUniformLocation(
		l_GLSPC->m_program,
		"uni_bloomExtractPassRT0");
	updateUniform(
		GLFinalRenderPassSingletonComponent::getInstance().m_bloomBlurPass_uni_bloomExtractPassRT0,
		0);
	GLFinalRenderPassSingletonComponent::getInstance().m_bloomBlurPass_uni_horizontal = getUniformLocation(
		l_GLSPC->m_program,
		"uni_horizontal");

	GLFinalRenderPassSingletonComponent::getInstance().m_bloomBlurPassSPC = l_GLSPC;
}

void GLRenderingSystemNS::initializeMotionBlurPass()
{
	GLFinalRenderPassSingletonComponent::getInstance().m_motionBlurPassGLRPC = addRenderPassComponent(1);

	// shader programs and shaders
	shaderFilePaths m_shaderFilePaths;

	m_shaderFilePaths.m_VSPath = "GL4.0//motionBlurPassVertex.sf";
	m_shaderFilePaths.m_FSPath = "GL4.0//motionBlurPassFragment.sf";

	auto l_GLSPC = addShaderProgramComponent(m_shaderFilePaths);

	GLFinalRenderPassSingletonComponent::getInstance().m_motionBlurPass_uni_motionVectorTexture = getUniformLocation(
		l_GLSPC->m_program,
		"uni_motionVectorTexture");
	updateUniform(
		GLFinalRenderPassSingletonComponent::getInstance().m_motionBlurPass_uni_motionVectorTexture,
		0);
	GLFinalRenderPassSingletonComponent::getInstance().m_motionBlurPass_uni_TAAPassRT0 = getUniformLocation(
		l_GLSPC->m_program,
		"uni_TAAPassRT0");
	updateUniform(
		GLFinalRenderPassSingletonComponent::getInstance().m_motionBlurPass_uni_TAAPassRT0,
		1);

	GLFinalRenderPassSingletonComponent::getInstance().m_motionBlurPassSPC = l_GLSPC;
}

void GLRenderingSystemNS::initializeBillboardPass()
{
	GLFinalRenderPassSingletonComponent::getInstance().m_billboardPassGLRPC = addRenderPassComponent(1);

	// shader programs and shaders
	shaderFilePaths m_shaderFilePaths;

	m_shaderFilePaths.m_VSPath = "GL4.0//billboardPassVertex.sf";
	m_shaderFilePaths.m_FSPath = "GL4.0//billboardPassFragment.sf";

	auto l_GLSPC = addShaderProgramComponent(m_shaderFilePaths);

	GLFinalRenderPassSingletonComponent::getInstance().m_billboardPass_uni_texture = getUniformLocation(
		l_GLSPC->m_program,
		"uni_texture");
	updateUniform(
		GLFinalRenderPassSingletonComponent::getInstance().m_billboardPass_uni_texture,
		0);
	GLFinalRenderPassSingletonComponent::getInstance().m_billboardPass_uni_p = getUniformLocation(
		l_GLSPC->m_program,
		"uni_p");
	GLFinalRenderPassSingletonComponent::getInstance().m_billboardPass_uni_r = getUniformLocation(
		l_GLSPC->m_program,
		"uni_r");
	GLFinalRenderPassSingletonComponent::getInstance().m_billboardPass_uni_t = getUniformLocation(
		l_GLSPC->m_program,
		"uni_t");
	GLFinalRenderPassSingletonComponent::getInstance().m_billboardPass_uni_pos = getUniformLocation(
		l_GLSPC->m_program,
		"uni_pos");
	GLFinalRenderPassSingletonComponent::getInstance().m_billboardPass_uni_albedo = getUniformLocation(
		l_GLSPC->m_program,
		"uni_albedo");
	GLFinalRenderPassSingletonComponent::getInstance().m_billboardPass_uni_size = getUniformLocation(
		l_GLSPC->m_program,
		"uni_size");

	GLFinalRenderPassSingletonComponent::getInstance().m_billboardPassSPC = l_GLSPC;
}

void GLRenderingSystemNS::initializeDebuggerPass()
{
	GLFinalRenderPassSingletonComponent::getInstance().m_debuggerPassGLRPC = addRenderPassComponent(1);

	// shader programs and shaders
	shaderFilePaths m_shaderFilePaths;

	//m_shaderFilePaths.m_VSPath = "GL4.0//debuggerPassVertex.sf";
	//m_shaderFilePaths.m_GSPath = "GL4.0//debuggerPassGeometry.sf";
	//m_shaderFilePaths.m_FSPath = "GL4.0//debuggerPassFragment.sf";

	m_shaderFilePaths.m_VSPath = "GL4.0//wireframeOverlayPassVertex.sf";
	m_shaderFilePaths.m_FSPath = "GL4.0//wireframeOverlayPassFragment.sf";

	auto l_GLSPC = addShaderProgramComponent(m_shaderFilePaths);

	//GLFinalRenderPassSingletonComponent::getInstance().m_debuggerPass_uni_normalTexture = getUniformLocation(
	//	l_GLSPC->m_program,
	//	"uni_normalTexture");
	//updateUniform(
	//	GLFinalRenderPassSingletonComponent::getInstance().m_debuggerPass_uni_normalTexture,
	//	0);
	GLFinalRenderPassSingletonComponent::getInstance().m_debuggerPass_uni_p = getUniformLocation(
		l_GLSPC->m_program,
		"uni_p");
	GLFinalRenderPassSingletonComponent::getInstance().m_debuggerPass_uni_r = getUniformLocation(
		l_GLSPC->m_program,
		"uni_r");
	GLFinalRenderPassSingletonComponent::getInstance().m_debuggerPass_uni_t = getUniformLocation(
		l_GLSPC->m_program,
		"uni_t");
	GLFinalRenderPassSingletonComponent::getInstance().m_debuggerPass_uni_m = getUniformLocation(
		l_GLSPC->m_program,
		"uni_m");

	GLFinalRenderPassSingletonComponent::getInstance().m_debuggerPassSPC = l_GLSPC;
}

void GLRenderingSystemNS::initializeFinalBlendPass()
{
	GLFinalRenderPassSingletonComponent::getInstance().m_finalBlendPassGLRPC = addRenderPassComponent(1);

	// shader programs and shaders
	shaderFilePaths m_shaderFilePaths;

	m_shaderFilePaths.m_VSPath = "GL4.0//finalBlendPassVertex.sf";
	m_shaderFilePaths.m_FSPath = "GL4.0//finalBlendPassFragment.sf";

	auto l_GLSPC = addShaderProgramComponent(m_shaderFilePaths);

	GLFinalRenderPassSingletonComponent::getInstance().m_uni_motionBlurPassRT0 = getUniformLocation(
		l_GLSPC->m_program,
		"uni_motionBlurPassRT0");
	updateUniform(
		GLFinalRenderPassSingletonComponent::getInstance().m_uni_motionBlurPassRT0,
		0);
	GLFinalRenderPassSingletonComponent::getInstance().m_uni_bloomPassRT0 = getUniformLocation(
		l_GLSPC->m_program,
		"uni_bloomPassRT0");
	updateUniform(
		GLFinalRenderPassSingletonComponent::getInstance().m_uni_bloomPassRT0,
		1);
	GLFinalRenderPassSingletonComponent::getInstance().m_uni_billboardPassRT0 = getUniformLocation(
		l_GLSPC->m_program,
		"uni_billboardPassRT0");
	updateUniform(
		GLFinalRenderPassSingletonComponent::getInstance().m_uni_billboardPassRT0,
		2);
	GLFinalRenderPassSingletonComponent::getInstance().m_uni_debuggerPassRT0 = getUniformLocation(
		l_GLSPC->m_program,
		"uni_debuggerPassRT0");
	updateUniform(
		GLFinalRenderPassSingletonComponent::getInstance().m_uni_debuggerPassRT0,
		3);
	GLFinalRenderPassSingletonComponent::getInstance().m_finalBlendPassSPC = l_GLSPC;
}

GLMeshDataComponent * GLRenderingSystemNS::addGLMeshDataComponent(EntityID rhs)
{
	GLMeshDataComponent* newMesh = g_pCoreSystem->getMemorySystem()->spawn<GLMeshDataComponent>();
	newMesh->m_parentEntity = rhs;
	auto l_meshMap = &GLRenderingSystemNS::g_GLRenderingSystemSingletonComponent->m_meshMap;
	l_meshMap->emplace(std::pair<EntityID, GLMeshDataComponent*>(rhs, newMesh));
	return newMesh;
}

GLTextureDataComponent * GLRenderingSystemNS::addGLTextureDataComponent(EntityID rhs)
{
	GLTextureDataComponent* newTexture = g_pCoreSystem->getMemorySystem()->spawn<GLTextureDataComponent>();
	newTexture->m_parentEntity = rhs;
	auto l_textureMap = &GLRenderingSystemNS::g_GLRenderingSystemSingletonComponent->m_textureMap;
	l_textureMap->emplace(std::pair<EntityID, GLTextureDataComponent*>(rhs, newTexture));
	return newTexture;
}

GLMeshDataComponent * GLRenderingSystemNS::getGLMeshDataComponent(EntityID rhs)
{
	auto result = GLRenderingSystemNS::g_GLRenderingSystemSingletonComponent->m_meshMap.find(rhs);
	if (result != GLRenderingSystemNS::g_GLRenderingSystemSingletonComponent->m_meshMap.end())
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
	auto result = GLRenderingSystemNS::g_GLRenderingSystemSingletonComponent->m_textureMap.find(rhs);
	if (result != GLRenderingSystemNS::g_GLRenderingSystemSingletonComponent->m_textureMap.end())
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

		glGenVertexArrays(1, &l_ptr->m_VAO);
		glGenBuffers(1, &l_ptr->m_VBO);
		glGenBuffers(1, &l_ptr->m_IBO);

		std::vector<float> l_verticesBuffer;
		auto& l_vertices = rhs->m_vertices;
		auto& l_indices = rhs->m_indices;
		auto l_containerSize = l_vertices.size() * 8;
		l_verticesBuffer.reserve(l_containerSize);

		std::for_each(l_vertices.begin(), l_vertices.end(), [&](Vertex val)
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

		glBindVertexArray(l_ptr->m_VAO);

		glBindBuffer(GL_ARRAY_BUFFER, l_ptr->m_VBO);
		glBufferData(GL_ARRAY_BUFFER, l_verticesBuffer.size() * sizeof(float), &l_verticesBuffer[0], GL_STATIC_DRAW);

		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, l_ptr->m_IBO);
		glBufferData(GL_ELEMENT_ARRAY_BUFFER, l_indices.size() * sizeof(unsigned int), &l_indices[0], GL_STATIC_DRAW);

		// position attribute, 1st attribution with 3 * sizeof(float) bits of data
		glEnableVertexAttribArray(0);
		glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)0);

		// texture attribute, 2nd attribution with 2 * sizeof(float) bits of data
		glEnableVertexAttribArray(1);
		glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(3 * sizeof(float)));

		// normal coord attribute, 3rd attribution with 3 * sizeof(float) bits of data
		glEnableVertexAttribArray(2);
		glVertexAttribPointer(2, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(5 * sizeof(float)));

		l_ptr->m_objectStatus = ObjectStatus::ALIVE;
		rhs->m_objectStatus = ObjectStatus::ALIVE;
		m_initializedMeshComponents.emplace(l_ptr->m_parentEntity, l_ptr);

		g_pCoreSystem->getLogSystem()->printLog(LogType::INNO_DEV_VERBOSE, "GLRenderingSystem: VAO " + std::to_string(l_ptr->m_VAO) + " is initialized.");
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

bool GLRenderingSystemNS::initializeGLTextureDataComponent(GLTextureDataComponent * rhs, TextureDataDesc textureDataDesc, const std::vector<void*>& textureData)
{
	//generate and bind texture object
	glGenTextures(1, &rhs->m_TAO);

	if (textureDataDesc.textureUsageType == TextureUsageType::ENVIRONMENT_CAPTURE || textureDataDesc.textureUsageType == TextureUsageType::ENVIRONMENT_CONVOLUTION || textureDataDesc.textureUsageType == TextureUsageType::ENVIRONMENT_PREFILTER)
	{
		glBindTexture(GL_TEXTURE_CUBE_MAP, rhs->m_TAO);
	}
	else
	{
		glBindTexture(GL_TEXTURE_2D, rhs->m_TAO);
	}

	// set the texture wrapping parameters
	GLenum l_textureWrapMethod;
	switch (textureDataDesc.textureWrapMethod)
	{
	case TextureWrapMethod::CLAMP_TO_EDGE: l_textureWrapMethod = GL_CLAMP_TO_EDGE; break;
	case TextureWrapMethod::REPEAT: l_textureWrapMethod = GL_REPEAT; break;
	case TextureWrapMethod::CLAMP_TO_BORDER: l_textureWrapMethod = GL_CLAMP_TO_BORDER; break;
	}
	if (textureDataDesc.textureUsageType == TextureUsageType::ENVIRONMENT_CAPTURE || textureDataDesc.textureUsageType == TextureUsageType::ENVIRONMENT_CONVOLUTION || textureDataDesc.textureUsageType == TextureUsageType::ENVIRONMENT_PREFILTER)
	{
		glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, l_textureWrapMethod);
		glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, l_textureWrapMethod);
		glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, l_textureWrapMethod);
	}
	else if (textureDataDesc.textureUsageType == TextureUsageType::SHADOWMAP)
	{
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, l_textureWrapMethod);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, l_textureWrapMethod);
		float borderColor[] = { 1.0f, 1.0f, 1.0f, 1.0f };
		glTexParameterfv(GL_TEXTURE_2D, GL_TEXTURE_BORDER_COLOR, borderColor);
	}
	else
	{
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, l_textureWrapMethod);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, l_textureWrapMethod);
	}

	// set texture filtering parameters
	GLenum l_minFilterParam;
	switch (textureDataDesc.textureMinFilterMethod)
	{
	case TextureFilterMethod::NEAREST: l_minFilterParam = GL_NEAREST; break;
	case TextureFilterMethod::LINEAR: l_minFilterParam = GL_LINEAR; break;
	case TextureFilterMethod::LINEAR_MIPMAP_LINEAR: l_minFilterParam = GL_LINEAR_MIPMAP_LINEAR; break;

	}
	GLenum l_magFilterParam;
	switch (textureDataDesc.textureMagFilterMethod)
	{
	case TextureFilterMethod::NEAREST: l_magFilterParam = GL_NEAREST; break;
	case TextureFilterMethod::LINEAR: l_magFilterParam = GL_LINEAR; break;
	case TextureFilterMethod::LINEAR_MIPMAP_LINEAR: l_magFilterParam = GL_LINEAR_MIPMAP_LINEAR; break;

	}
	if (textureDataDesc.textureUsageType == TextureUsageType::ENVIRONMENT_CAPTURE || textureDataDesc.textureUsageType == TextureUsageType::ENVIRONMENT_CONVOLUTION || textureDataDesc.textureUsageType == TextureUsageType::ENVIRONMENT_PREFILTER)
	{
		glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, l_minFilterParam);
		glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, l_magFilterParam);
	}
	else
	{
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, l_minFilterParam);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, l_magFilterParam);
	}

	// set texture formats
	GLenum l_internalFormat;
	GLenum l_dataFormat;
	GLenum l_type;
	if (textureDataDesc.textureUsageType == TextureUsageType::ALBEDO)
	{
		if (textureDataDesc.texturePixelDataFormat == TexturePixelDataFormat::RGB)
		{
			l_internalFormat = GL_SRGB;
		}
		else if (textureDataDesc.texturePixelDataFormat == TexturePixelDataFormat::RGBA)
		{
			l_internalFormat = GL_SRGB_ALPHA;
		}
	}
	else
	{
		switch (textureDataDesc.textureColorComponentsFormat)
		{
		case TextureColorComponentsFormat::RED: l_internalFormat = GL_RED; break;
		case TextureColorComponentsFormat::RG: l_internalFormat = GL_RG; break;
		case TextureColorComponentsFormat::RGB: l_internalFormat = GL_RGB; break;
		case TextureColorComponentsFormat::RGBA: l_internalFormat = GL_RGBA; break;
		case TextureColorComponentsFormat::R8: l_internalFormat = GL_R8; break;
		case TextureColorComponentsFormat::RG8: l_internalFormat = GL_RG8; break;
		case TextureColorComponentsFormat::RGB8: l_internalFormat = GL_RGB8; break;
		case TextureColorComponentsFormat::RGBA8: l_internalFormat = GL_RGBA8; break;
		case TextureColorComponentsFormat::R16: l_internalFormat = GL_R16; break;
		case TextureColorComponentsFormat::RG16: l_internalFormat = GL_RG16; break;
		case TextureColorComponentsFormat::RGB16: l_internalFormat = GL_RGB16; break;
		case TextureColorComponentsFormat::RGBA16: l_internalFormat = GL_RGBA16; break;
		case TextureColorComponentsFormat::R16F: l_internalFormat = GL_R16F; break;
		case TextureColorComponentsFormat::RG16F: l_internalFormat = GL_RG16F; break;
		case TextureColorComponentsFormat::RGB16F: l_internalFormat = GL_RGB16F; break;
		case TextureColorComponentsFormat::RGBA16F: l_internalFormat = GL_RGBA16F; break;
		case TextureColorComponentsFormat::R32F: l_internalFormat = GL_R32F; break;
		case TextureColorComponentsFormat::RG32F: l_internalFormat = GL_RG32F; break;
		case TextureColorComponentsFormat::RGB32F: l_internalFormat = GL_RGB32F; break;
		case TextureColorComponentsFormat::RGBA32F: l_internalFormat = GL_RGBA32F; break;
		case TextureColorComponentsFormat::SRGB: l_internalFormat = GL_SRGB; break;
		case TextureColorComponentsFormat::SRGBA: l_internalFormat = GL_SRGB_ALPHA; break;
		case TextureColorComponentsFormat::SRGB8: l_internalFormat = GL_SRGB8; break;
		case TextureColorComponentsFormat::SRGBA8: l_internalFormat = GL_SRGB8_ALPHA8; break;
		case TextureColorComponentsFormat::DEPTH_COMPONENT: l_internalFormat = GL_DEPTH_COMPONENT; break;
		}
	}
	switch (textureDataDesc.texturePixelDataFormat)
	{
	case TexturePixelDataFormat::RED:l_dataFormat = GL_RED; break;
	case TexturePixelDataFormat::RG:l_dataFormat = GL_RG; break;
	case TexturePixelDataFormat::RGB:l_dataFormat = GL_RGB; break;
	case TexturePixelDataFormat::RGBA:l_dataFormat = GL_RGBA; break;
	case TexturePixelDataFormat::DEPTH_COMPONENT:l_dataFormat = GL_DEPTH_COMPONENT; break;
	}
	switch (textureDataDesc.texturePixelDataType)
	{
	case TexturePixelDataType::UNSIGNED_BYTE:l_type = GL_UNSIGNED_BYTE; break;
	case TexturePixelDataType::BYTE:l_type = GL_BYTE; break;
	case TexturePixelDataType::UNSIGNED_SHORT:l_type = GL_UNSIGNED_SHORT; break;
	case TexturePixelDataType::SHORT:l_type = GL_SHORT; break;
	case TexturePixelDataType::UNSIGNED_INT:l_type = GL_UNSIGNED_INT; break;
	case TexturePixelDataType::INT:l_type = GL_INT; break;
	case TexturePixelDataType::FLOAT:l_type = GL_FLOAT; break;
	case TexturePixelDataType::DOUBLE:l_type = GL_FLOAT; break;
	}

	if (textureDataDesc.textureUsageType == TextureUsageType::ENVIRONMENT_CAPTURE || textureDataDesc.textureUsageType == TextureUsageType::ENVIRONMENT_CONVOLUTION || textureDataDesc.textureUsageType == TextureUsageType::ENVIRONMENT_PREFILTER)
	{
		glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X, 0, l_internalFormat, textureDataDesc.textureWidth, textureDataDesc.textureHeight, 0, l_dataFormat, l_type, textureData[0]);
		glTexImage2D(GL_TEXTURE_CUBE_MAP_NEGATIVE_X, 0, l_internalFormat, textureDataDesc.textureWidth, textureDataDesc.textureHeight, 0, l_dataFormat, l_type, textureData[1]);
		glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_Y, 0, l_internalFormat, textureDataDesc.textureWidth, textureDataDesc.textureHeight, 0, l_dataFormat, l_type, textureData[2]);
		glTexImage2D(GL_TEXTURE_CUBE_MAP_NEGATIVE_Y, 0, l_internalFormat, textureDataDesc.textureWidth, textureDataDesc.textureHeight, 0, l_dataFormat, l_type, textureData[3]);
		glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_Z, 0, l_internalFormat, textureDataDesc.textureWidth, textureDataDesc.textureHeight, 0, l_dataFormat, l_type, textureData[4]);
		glTexImage2D(GL_TEXTURE_CUBE_MAP_NEGATIVE_Z, 0, l_internalFormat, textureDataDesc.textureWidth, textureDataDesc.textureHeight, 0, l_dataFormat, l_type, textureData[5]);
	}
	else
	{
		glTexImage2D(GL_TEXTURE_2D, 0, l_internalFormat, textureDataDesc.textureWidth, textureDataDesc.textureHeight, 0, l_dataFormat, l_type, textureData[0]);
	}

	// should generate mipmap or not
	if (textureDataDesc.textureMinFilterMethod == TextureFilterMethod::LINEAR_MIPMAP_LINEAR)
	{
		// @TODO: generalization...
		if (textureDataDesc.textureUsageType == TextureUsageType::ENVIRONMENT_PREFILTER)
		{
			glGenerateMipmap(GL_TEXTURE_CUBE_MAP);
		}
		else if (textureDataDesc.textureUsageType != TextureUsageType::ENVIRONMENT_CAPTURE || textureDataDesc.textureUsageType != TextureUsageType::ENVIRONMENT_CONVOLUTION || textureDataDesc.textureUsageType != TextureUsageType::RENDER_BUFFER_SAMPLER)
		{
			glGenerateMipmap(GL_TEXTURE_2D);
		}
	}

	rhs->m_objectStatus = ObjectStatus::ALIVE;

	g_pCoreSystem->getLogSystem()->printLog(LogType::INNO_DEV_VERBOSE, "GLRenderingSystem: TAO " + std::to_string(rhs->m_TAO) + " is initialized.");

	return true;
}

bool GLRenderingSystem::update()
{
	if (AssetSystemSingletonComponent::getInstance().m_uninitializedMeshComponents.size() > 0)
	{
		MeshDataComponent* l_meshDataComponent;
		if (AssetSystemSingletonComponent::getInstance().m_uninitializedMeshComponents.tryPop(l_meshDataComponent))
		{
			GLRenderingSystemNS::generateGLMeshDataComponent(l_meshDataComponent);
		}
	}
	if (AssetSystemSingletonComponent::getInstance().m_uninitializedTextureComponents.size() > 0)
	{
		TextureDataComponent* l_textureDataComponent;
		if (AssetSystemSingletonComponent::getInstance().m_uninitializedTextureComponents.tryPop(l_textureDataComponent))
		{
			GLRenderingSystemNS::generateGLTextureDataComponent(l_textureDataComponent);
		}
	}

	GLRenderingSystemNS::prepareRenderingData();

	if (GLRenderingSystemNS::g_RenderingSystemSingletonComponent->m_shouldUpdateEnvironmentMap)
	{
		GLRenderingSystemNS::updateEnvironmentRenderPass();
	}
	GLRenderingSystemNS::updateShadowRenderPass();
	GLRenderingSystemNS::updateGeometryRenderPass();
	if (GLRenderingSystemNS::g_RenderingSystemSingletonComponent->m_drawTerrain)
	{
		GLRenderingSystemNS::updateTerrainRenderPass();
	}
	else
	{
		GLRenderingSystemNS::f_cleanFBC(GLTerrainRenderPassSingletonComponent::getInstance().m_GLRPC->m_GLFBC);
	}

	GLRenderingSystemNS::updateLightRenderPass();
	GLRenderingSystemNS::updateFinalRenderPass();

	return true;
}

void GLRenderingSystemNS::prepareRenderingData()
{
	// camera and light
	auto l_mainCamera = GameSystemSingletonComponent::getInstance().m_CameraComponents[0];
	auto l_mainCameraTransformComponent = g_pCoreSystem->getGameSystem()->get<TransformComponent>(l_mainCamera->m_parentEntity);
	auto l_directionalLight = GameSystemSingletonComponent::getInstance().m_LightComponents[0];
	auto l_directionalLightTransformComponent = g_pCoreSystem->getGameSystem()->get<TransformComponent>(l_directionalLight->m_parentEntity);

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

	if (g_RenderingSystemSingletonComponent->m_useTAA)
	{
		//TAA jitter for projection matrix
		auto& l_currentHaltonStep = g_RenderingSystemSingletonComponent->currentHaltonStep;
		if (l_currentHaltonStep >= 16)
		{
			l_currentHaltonStep = 0;
		}
		m_CamProjJittered.m02 = g_RenderingSystemSingletonComponent->HaltonSampler[l_currentHaltonStep].x / rtSizeX;
		m_CamProjJittered.m12 = g_RenderingSystemSingletonComponent->HaltonSampler[l_currentHaltonStep].y / rtSizeY;
		l_currentHaltonStep += 1;
	}

	m_CamRot = l_r;
	m_CamTrans = l_t;
	m_CamRot_prev = r_prev;
	m_CamTrans_prev = t_prev;

	m_LPassCBufferData.viewPos = l_mainCameraTransformComponent->m_globalTransformVector.m_pos;
	m_LPassCBufferData.lightDir = InnoMath::getDirection(direction::BACKWARD, l_directionalLightTransformComponent->m_globalTransformVector.m_rot);

	m_LPassCBufferData.color = l_directionalLight->m_color;

	for (auto& l_renderDataPack : RenderingSystemSingletonComponent::getInstance().m_renderDataPack)
	{
		auto l_GLMDC = GLRenderingSystemNS::m_initializedMeshComponents.find(l_renderDataPack.MDC->m_parentEntity);
		if (l_GLMDC != GLRenderingSystemNS::m_initializedMeshComponents.end())
		{
			GPassRenderingDataPack l_GLRenderDataPack;

			l_GLRenderDataPack.indiceSize = l_renderDataPack.MDC->m_indicesSize;
			l_GLRenderDataPack.m_meshDrawMethod = l_renderDataPack.MDC->m_meshDrawMethod;
			l_GLRenderDataPack.GPassCBuffer.m = l_renderDataPack.m;
			l_GLRenderDataPack.GPassCBuffer.m_prev = l_renderDataPack.m_prev;
			l_GLRenderDataPack.GPassCBuffer.m_normalMat = l_renderDataPack.normalMat;
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
				l_GLRenderDataPack.useNormalTexture = false;
			}
			// any albedo?
			l_TDC = l_material->m_texturePack.m_albedoTDC.second;
			if (l_TDC)
			{
				l_GLRenderDataPack.m_basicAlbedoGLTDC = getGLTextureDataComponent(l_TDC->m_parentEntity);
			}
			else
			{
				l_GLRenderDataPack.useAlbedoTexture = false;
			}
			// any metallic?
			l_TDC = l_material->m_texturePack.m_metallicTDC.second;
			if (l_TDC)
			{
				l_GLRenderDataPack.m_basicMetallicGLTDC = getGLTextureDataComponent(l_TDC->m_parentEntity);
			}
			else
			{
				l_GLRenderDataPack.useMetallicTexture = false;
			}
			// any roughness?
			l_TDC = l_material->m_texturePack.m_roughnessTDC.second;
			if (l_TDC)
			{
				l_GLRenderDataPack.m_basicRoughnessGLTDC = getGLTextureDataComponent(l_TDC->m_parentEntity);
			}
			else
			{
				l_GLRenderDataPack.useRoughnessTexture = false;
			}
			// any ao?
			l_TDC = l_material->m_texturePack.m_roughnessTDC.second;
			if (l_TDC)
			{
				l_GLRenderDataPack.m_basicAOGLTDC = getGLTextureDataComponent(l_TDC->m_parentEntity);
			}
			else
			{
				l_GLRenderDataPack.useAOTexture = false;
			}
			l_GLRenderDataPack.meshColor = l_material->m_meshColor;
			l_GLRenderDataPack.visiblilityType = l_renderDataPack.visiblilityType;
			m_GPassRenderingDataQueue.push(l_GLRenderDataPack);
		}
	}
}

void GLRenderingSystemNS::updateBRDFLUTRenderPass()
{
	glDisable(GL_DEPTH_TEST);
	glDisable(GL_CULL_FACE);

	// bind to framebuffer
	auto l_FBC = GLEnvironmentRenderPassSingletonComponent::getInstance().m_BRDFLUTPassFBC;
	glBindFramebuffer(GL_FRAMEBUFFER, l_FBC->m_FBO);
	glBindRenderbuffer(GL_RENDERBUFFER, l_FBC->m_RBO);
	glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT32, 512, 512);
	glViewport(0, 0, 512, 512);

	// draw split-Sum LUT
	activateShaderProgram(GLEnvironmentRenderPassSingletonComponent::getInstance().m_BRDFSplitSumLUTPassSPC);
	auto l_SSTDC = GLEnvironmentRenderPassSingletonComponent::getInstance().m_BRDFSplitSumLUTPassTDC;
	auto l_SSGLTDC = GLEnvironmentRenderPassSingletonComponent::getInstance().m_BRDFSplitSumLUTPassGLTDC;
	auto l_MDC = g_pCoreSystem->getAssetSystem()->getMeshDataComponent(MeshShapeType::QUAD);
	attachTextureToFramebuffer(l_SSTDC, l_SSGLTDC, l_FBC, 0, 0, 0);
	glClear(GL_COLOR_BUFFER_BIT);
	glClear(GL_DEPTH_BUFFER_BIT);
	drawMesh(l_MDC);

	// draw averange RsF1 LUT
	activateShaderProgram(GLEnvironmentRenderPassSingletonComponent::getInstance().m_BRDFMSAverageLUTPassSPC);
	auto l_MSTDC = GLEnvironmentRenderPassSingletonComponent::getInstance().m_BRDFMSAverageLUTPassTDC;
	auto l_MSGLTDC = GLEnvironmentRenderPassSingletonComponent::getInstance().m_BRDFMSAverageLUTPassGLTDC;
	attachTextureToFramebuffer(l_MSTDC, l_MSGLTDC, l_FBC, 0, 0, 0);
	glClear(GL_COLOR_BUFFER_BIT);
	glClear(GL_DEPTH_BUFFER_BIT);
	activate2DTexture(l_SSGLTDC, 0);
	drawMesh(l_MDC);

	RenderingSystemSingletonComponent::getInstance().m_shouldUpdateEnvironmentMap = false;
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

	// draw each lightComponent's shadowmap
	for (size_t i = 0; i < GLShadowRenderPassSingletonComponent::getInstance().m_FBCs.size(); i++)
	{
		auto l_FBC = GLShadowRenderPassSingletonComponent::getInstance().m_FBCs[i];
		// bind to framebuffer
		glBindFramebuffer(GL_FRAMEBUFFER, l_FBC->m_FBO);
		glBindRenderbuffer(GL_RENDERBUFFER, l_FBC->m_RBO);
		glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT32, 2048, 2048);
		glViewport(0, 0, 2048, 2048);

		glClear(GL_COLOR_BUFFER_BIT);
		glClear(GL_DEPTH_BUFFER_BIT);

		for (auto& l_lightComponent : GLRenderingSystemNS::g_GameSystemSingletonComponent->m_LightComponents)
		{
			if (l_lightComponent->m_lightType == LightType::DIRECTIONAL)
			{
				activateShaderProgram(GLShadowRenderPassSingletonComponent::getInstance().m_SPC);
				updateUniform(
					GLShadowRenderPassSingletonComponent::getInstance().m_shadowPass_uni_p,
					l_lightComponent->m_projectionMatrices[i]);
				updateUniform(
					GLShadowRenderPassSingletonComponent::getInstance().m_shadowPass_uni_v,
					g_pCoreSystem->getGameSystem()->get<TransformComponent>(l_lightComponent->m_parentEntity)->m_globalTransformMatrix.m_transformationMat.inverse());

				// draw each visibleComponent
				for (auto& l_visibleComponent : GLRenderingSystemNS::g_GameSystemSingletonComponent->m_VisibleComponents)
				{
					if (l_visibleComponent->m_visiblilityType == VisiblilityType::STATIC_MESH)
					{
						updateUniform(
							GLShadowRenderPassSingletonComponent::getInstance().m_shadowPass_uni_m,
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
		}
	}

	glDisable(GL_CULL_FACE);
	glDisable(GL_DEPTH_TEST);
}

void GLRenderingSystemNS::updateGeometryRenderPass()
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
	auto l_FBC = GLGeometryRenderPassSingletonComponent::getInstance().m_GLRPC->m_GLFBC;
	f_bindDeferredFBC(l_FBC);

	activateShaderProgram(GLGeometryRenderPassSingletonComponent::getInstance().m_GLSPC);

	if (GLRenderingSystemNS::g_GameSystemSingletonComponent->m_CameraComponents.size() > 0)
	{
		mat4 p_original = GLRenderingSystemNS::m_CamProjOriginal;
		mat4 p_jittered = GLRenderingSystemNS::m_CamProjJittered;

		mat4 r = GLRenderingSystemNS::m_CamRot;
		mat4 t = GLRenderingSystemNS::m_CamTrans;
		mat4 r_prev = GLRenderingSystemNS::m_CamRot_prev;
		mat4 t_prev = GLRenderingSystemNS::m_CamTrans_prev;

		updateUniform(
			GLGeometryRenderPassSingletonComponent::getInstance().m_geometryPass_uni_p_camera_original,
			p_original);
		updateUniform(
			GLGeometryRenderPassSingletonComponent::getInstance().m_geometryPass_uni_p_camera_jittered,
			p_jittered);
		updateUniform(
			GLGeometryRenderPassSingletonComponent::getInstance().m_geometryPass_uni_r_camera,
			r);
		updateUniform(
			GLGeometryRenderPassSingletonComponent::getInstance().m_geometryPass_uni_t_camera,
			t);
		updateUniform(
			GLGeometryRenderPassSingletonComponent::getInstance().m_geometryPass_uni_r_camera_prev,
			r_prev);
		updateUniform(
			GLGeometryRenderPassSingletonComponent::getInstance().m_geometryPass_uni_t_camera_prev,
			t_prev);

#ifdef CookTorrance
		//Cook-Torrance
		if (GLRenderingSystemNS::g_GameSystemSingletonComponent->m_LightComponents.size() > 0)
		{
			for (auto& l_lightComponent : GLRenderingSystemNS::g_GameSystemSingletonComponent->m_LightComponents)
			{
				// update light space transformation matrices
				if (l_lightComponent->m_lightType == LightType::DIRECTIONAL)
				{
					updateUniform(GLGeometryRenderPassSingletonComponent::getInstance().m_geometryPass_uni_p_light_0,
						l_lightComponent->m_projectionMatrices[0]);
					updateUniform(GLGeometryRenderPassSingletonComponent::getInstance().m_geometryPass_uni_p_light_1,
						l_lightComponent->m_projectionMatrices[1]);
					updateUniform(GLGeometryRenderPassSingletonComponent::getInstance().m_geometryPass_uni_p_light_2,
						l_lightComponent->m_projectionMatrices[2]);
					updateUniform(GLGeometryRenderPassSingletonComponent::getInstance().m_geometryPass_uni_p_light_3,
						l_lightComponent->m_projectionMatrices[3]);
					updateUniform(GLGeometryRenderPassSingletonComponent::getInstance().m_geometryPass_uni_v_light,
						InnoMath::getInvertRotationMatrix(
							g_pCoreSystem->getGameSystem()->get<TransformComponent>(l_lightComponent->m_parentEntity)->m_globalTransformVector.m_rot
						));
					while (GLRenderingSystemNS::m_GPassRenderingDataQueue.size() > 0)
					{
						auto l_renderPack = GLRenderingSystemNS::m_GPassRenderingDataQueue.front();
						if (l_renderPack.visiblilityType == VisiblilityType::STATIC_MESH)
						{
							glStencilFunc(GL_ALWAYS, 0x01, 0xFF);
							updateUniform(
								GLGeometryRenderPassSingletonComponent::getInstance().m_geometryPass_uni_m,
								l_renderPack.GPassCBuffer.m);
							updateUniform(
								GLGeometryRenderPassSingletonComponent::getInstance().m_geometryPass_uni_m_prev,
								l_renderPack.GPassCBuffer.m_prev);
							// any normal?
							if (l_renderPack.useNormalTexture)
							{
								activate2DTexture(l_renderPack.m_basicNormalGLTDC, 0);
								updateUniform(GLGeometryRenderPassSingletonComponent::getInstance().m_geometryPass_uni_useNormalTexture, true);
							}
							else
							{
								updateUniform(GLGeometryRenderPassSingletonComponent::getInstance().m_geometryPass_uni_useNormalTexture, false);
							}
							// any albedo?
							if (l_renderPack.useAlbedoTexture)
							{
								activate2DTexture(l_renderPack.m_basicAlbedoGLTDC, 1);
								updateUniform(GLGeometryRenderPassSingletonComponent::getInstance().m_geometryPass_uni_useAlbedoTexture, true);
							}
							else
							{
								updateUniform(GLGeometryRenderPassSingletonComponent::getInstance().m_geometryPass_uni_useAlbedoTexture, false);
							}
							// any metallic?
							if (l_renderPack.useMetallicTexture)
							{
								activate2DTexture(l_renderPack.m_basicMetallicGLTDC, 2);
								updateUniform(GLGeometryRenderPassSingletonComponent::getInstance().m_geometryPass_uni_useMetallicTexture, true);
							}
							else
							{
								updateUniform(GLGeometryRenderPassSingletonComponent::getInstance().m_geometryPass_uni_useMetallicTexture, false);
							}
							// any roughness?
							if (l_renderPack.useRoughnessTexture)
							{
								activate2DTexture(l_renderPack.m_basicRoughnessGLTDC, 3);
								updateUniform(GLGeometryRenderPassSingletonComponent::getInstance().m_geometryPass_uni_useRoughnessTexture, true);
							}
							else
							{
								updateUniform(GLGeometryRenderPassSingletonComponent::getInstance().m_geometryPass_uni_useRoughnessTexture, false);
							}
							// any ao?
							if (l_renderPack.useAOTexture)
							{
								activate2DTexture(l_renderPack.m_basicAOGLTDC, 4);
								updateUniform(GLGeometryRenderPassSingletonComponent::getInstance().m_geometryPass_uni_useAOTexture, true);
							}
							else
							{
								updateUniform(GLGeometryRenderPassSingletonComponent::getInstance().m_geometryPass_uni_useAOTexture, false);
							}
							updateUniform(GLGeometryRenderPassSingletonComponent::getInstance().m_geometryPass_uni_albedo, l_renderPack.meshColor.albedo_r, l_renderPack.meshColor.albedo_g, l_renderPack.meshColor.albedo_b);
							updateUniform(GLGeometryRenderPassSingletonComponent::getInstance().m_geometryPass_uni_MRA, l_renderPack.meshColor.metallic, l_renderPack.meshColor.roughness, l_renderPack.meshColor.ao);

							drawMesh(l_renderPack.indiceSize, l_renderPack.m_meshDrawMethod, l_renderPack.GLMDC);
						}
						else if (l_renderPack.visiblilityType == VisiblilityType::EMISSIVE)
						{
							glStencilFunc(GL_ALWAYS, 0x02, 0xFF);

							updateUniform(
								GLGeometryRenderPassSingletonComponent::getInstance().m_geometryPass_uni_m,
								l_renderPack.GPassCBuffer.m);
							updateUniform(
								GLGeometryRenderPassSingletonComponent::getInstance().m_geometryPass_uni_m_prev,
								l_renderPack.GPassCBuffer.m_prev);
							updateUniform(GLGeometryRenderPassSingletonComponent::getInstance().m_geometryPass_uni_albedo, l_renderPack.meshColor.albedo_r, l_renderPack.meshColor.albedo_g, l_renderPack.meshColor.albedo_b);
							updateUniform(GLGeometryRenderPassSingletonComponent::getInstance().m_geometryPass_uni_useAlbedoTexture, false);

							drawMesh(l_renderPack.indiceSize, l_renderPack.m_meshDrawMethod, l_renderPack.GLMDC);
						}
						else
						{
							glStencilFunc(GL_ALWAYS, 0x00, 0xFF);
						}
						GLRenderingSystemNS::m_GPassRenderingDataQueue.pop();
					}
				}
			}

			//glDisable(GL_CULL_FACE);
			glDisable(GL_STENCIL_TEST);
			glDisable(GL_DEPTH_CLAMP);
			glDisable(GL_DEPTH_TEST);
			glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
#elif BlinnPhong
		// draw each visibleComponent
		for (auto& l_visibleComponent : visibleComponents)
		{
			if (l_visibleComponent->m_visiblilityType == VisiblilityType::STATIC_MESH)
			{
				updateUniform(
					GLGeometryRenderPassSingletonComponent::getInstance().m_geometryPass_uni_m,
					g_pCoreSystem->getGameSystem()->get<TransformComponent>(l_visibleComponent->m_parentEntity)->m_transform.caclGlobalTransformationMatrix());

				// draw each graphic data of visibleComponent
				for (auto& l_graphicData : l_visibleComponent->m_modelMap)
				{
					//active and bind textures
					// is there any texture?
					auto l_textureMap = &l_graphicData.second;
					if (l_textureMap)
					{
						// any normal?
						auto l_normalTextureID = l_textureMap->find(TextureUsageType::NORMAL);
						if (l_normalTextureID != l_textureMap->end())
						{
							auto l_TDC = g_pCoreSystem->getAssetSystem()->getTextureDataComponent(l_normalTextureID->second);
							if (l_TDC)
							{
								if (l_TDC->m_objectStatus == ObjectStatus::ALIVE)
								{
									auto l_GLTDC = getGLTextureDataComponent(l_TDC->m_parentEntity);
									if (l_GLTDC)
									{
										if (l_GLTDC->m_objectStatus == ObjectStatus::ALIVE)
										{
											activateTexture(l_TDC, l_GLTDC, 0);
										}
									}
								}
							}
						}
						// any diffuse?
						auto l_diffuseTextureID = l_textureMap->find(TextureUsageType::ALBEDO);
						if (l_diffuseTextureID != l_textureMap->end())
						{
							auto l_TDC = g_pCoreSystem->getAssetSystem()->getTextureDataComponent(l_diffuseTextureID->second);
							if (l_TDC)
							{
								if (l_TDC->m_objectStatus == ObjectStatus::ALIVE)
								{
									auto l_GLTDC = getGLTextureDataComponent(l_TDC->m_parentEntity);
									if (l_GLTDC)
									{
										if (l_GLTDC->m_objectStatus == ObjectStatus::ALIVE)
										{
											activateTexture(l_TDC, l_GLTDC, 1);
										}
									}
								}
							}
						}
						// any specular?
						auto l_specularTextureID = l_textureMap->find(TextureUsageType::METALLIC);
						if (l_specularTextureID != l_textureMap->end())
						{
							auto l_TDC = g_pCoreSystem->getAssetSystem()->getTextureDataComponent(l_specularTextureID->second);
							if (l_TDC)
							{
								auto l_GLTDC = getGLTextureDataComponent(l_TDC->m_parentEntity);
								if (l_GLTDC)
								{
									if (l_GLTDC->m_objectStatus == ObjectStatus::ALIVE)
									{
										activateTexture(l_TDC, l_GLTDC, 2);
									}
								}
							}
						}
					}
					// draw meshes
					drawMesh(l_graphicData.first);
				}
			}
		}
		glDisable(GL_DEPTH_TEST);
		glDisable(GL_DEPTH_CLAMP);
#endif
		}
	}
}

void GLRenderingSystemNS::updateTerrainRenderPass()
{
	glEnable(GL_DEPTH_TEST);

	// bind to framebuffer
	auto l_FBC = GLTerrainRenderPassSingletonComponent::getInstance().m_GLRPC->m_GLFBC;
	f_bindDeferredFBC(l_FBC);

	f_copyDepthBuffer(GLGeometryRenderPassSingletonComponent::getInstance().m_GLRPC->m_GLFBC, l_FBC);

	activateShaderProgram(GLTerrainRenderPassSingletonComponent::getInstance().m_GLSPC);

	mat4 p_original = GLRenderingSystemNS::m_CamProjOriginal;

	mat4 r = GLRenderingSystemNS::m_CamRot;
	mat4 t = GLRenderingSystemNS::m_CamTrans;
	mat4 m = InnoMath::generateIdentityMatrix<float>();
	updateUniform(
		GLTerrainRenderPassSingletonComponent::getInstance().m_terrainPass_uni_p_camera,
		p_original);
	updateUniform(
		GLTerrainRenderPassSingletonComponent::getInstance().m_terrainPass_uni_r_camera,
		r);
	updateUniform(
		GLTerrainRenderPassSingletonComponent::getInstance().m_terrainPass_uni_t_camera,
		t);
	updateUniform(
		GLTerrainRenderPassSingletonComponent::getInstance().m_terrainPass_uni_m,
		m);

	auto l_MDC = g_pCoreSystem->getAssetSystem()->getMeshDataComponent(MeshShapeType::TERRAIN);
	activate2DTexture(m_basicAlbedoTemplate, 0);

	drawMesh(l_MDC);

	glDisable(GL_DEPTH_TEST);
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
	auto l_FBC = GLLightRenderPassSingletonComponent::getInstance().m_GLRPC->m_GLFBC;
	f_bindDeferredFBC(l_FBC);

	// 1. opaque objects
	// copy stencil buffer of opaque objects from G-Pass
	f_copyStencilBuffer(GLGeometryRenderPassSingletonComponent::getInstance().m_GLRPC->m_GLFBC, l_FBC);

	activateShaderProgram(GLLightRenderPassSingletonComponent::getInstance().m_GLSPC);

#ifdef CookTorrance
	// Cook-Torrance
	// world space position + metallic
	activate2DTexture(
		GLGeometryRenderPassSingletonComponent::getInstance().m_GLRPC->m_GLTDCs[0],
		0);
	// normal + roughness
	activate2DTexture(
		GLGeometryRenderPassSingletonComponent::getInstance().m_GLRPC->m_GLTDCs[1],
		1);
	// albedo + ambient occlusion
	activate2DTexture(
		GLGeometryRenderPassSingletonComponent::getInstance().m_GLRPC->m_GLTDCs[2],
		2);
	// motion vector + transparency
	activate2DTexture(
		GLGeometryRenderPassSingletonComponent::getInstance().m_GLRPC->m_GLTDCs[3],
		3);
	// light space position 0
	activate2DTexture(
		GLGeometryRenderPassSingletonComponent::getInstance().m_GLRPC->m_GLTDCs[4],
		4);
	// light space position 1
	activate2DTexture(
		GLGeometryRenderPassSingletonComponent::getInstance().m_GLRPC->m_GLTDCs[5],
		5);
	// light space position 2
	activate2DTexture(
		GLGeometryRenderPassSingletonComponent::getInstance().m_GLRPC->m_GLTDCs[6],
		6);
	// light space position 3
	activate2DTexture(
		GLGeometryRenderPassSingletonComponent::getInstance().m_GLRPC->m_GLTDCs[7],
		7);
	// shadow map 0
	activate2DTexture(
		GLShadowRenderPassSingletonComponent::getInstance().m_GLTDCs[0],
		8);
	// shadow map 1
	activate2DTexture(
		GLShadowRenderPassSingletonComponent::getInstance().m_GLTDCs[1],
		9);
	// shadow map 2
	activate2DTexture(
		GLShadowRenderPassSingletonComponent::getInstance().m_GLTDCs[2],
		10);
	// shadow map 3
	activate2DTexture(
		GLShadowRenderPassSingletonComponent::getInstance().m_GLTDCs[3],
		11);
	// BRDF look-up table 1
	activate2DTexture(
		GLEnvironmentRenderPassSingletonComponent::getInstance().m_BRDFSplitSumLUTPassGLTDC,
		12);
	// BRDF look-up table 2
	activate2DTexture(
		GLEnvironmentRenderPassSingletonComponent::getInstance().m_BRDFMSAverageLUTPassGLTDC,
		13);
#endif

	updateUniform(
		GLLightRenderPassSingletonComponent::getInstance().m_uni_isEmissive,
		false);

	if (GLRenderingSystemNS::g_GameSystemSingletonComponent->m_LightComponents.size() > 0)
	{
		int l_pointLightIndexOffset = 0;
		auto l_viewPos = g_pCoreSystem->getGameSystem()->get<TransformComponent>(GLRenderingSystemNS::g_GameSystemSingletonComponent->m_CameraComponents[0]->m_parentEntity)->m_globalTransformVector.m_pos;
		for (auto i = (unsigned int)0; i < GLRenderingSystemNS::g_GameSystemSingletonComponent->m_LightComponents.size(); i++)
		{
			auto l_lightComponent = GLRenderingSystemNS::g_GameSystemSingletonComponent->m_LightComponents[i];
			auto l_lightPos = g_pCoreSystem->getGameSystem()->get<TransformComponent>(l_lightComponent->m_parentEntity)->m_globalTransformVector.m_pos;

			auto l_dirLightDirection =
				InnoMath::getDirection(direction::BACKWARD, g_pCoreSystem->getGameSystem()->get<TransformComponent>(l_lightComponent->m_parentEntity)->m_globalTransformVector.m_rot);

			auto l_lightColor = l_lightComponent->m_color;

			updateUniform(
				GLLightRenderPassSingletonComponent::getInstance().m_uni_viewPos,
				l_viewPos.x, l_viewPos.y, l_viewPos.z);

			if (l_lightComponent->m_lightType == LightType::DIRECTIONAL)
			{
				l_pointLightIndexOffset -= 1;
				updateUniform(
					GLLightRenderPassSingletonComponent::getInstance().m_uni_dirLight_direction,
					l_dirLightDirection.x, l_dirLightDirection.y, l_dirLightDirection.z);
				updateUniform(
					GLLightRenderPassSingletonComponent::getInstance().m_uni_dirLight_color,
					l_lightColor.x, l_lightColor.y, l_lightColor.z);

				for (size_t j = 0; j < 4; j++)
				{
					auto l_shadowSplitCorner = vec4(
						l_lightComponent->m_AABBs[j].m_boundMin.x,
						l_lightComponent->m_AABBs[j].m_boundMin.z,
						l_lightComponent->m_AABBs[j].m_boundMax.x,
						l_lightComponent->m_AABBs[j].m_boundMax.z
					);
					updateUniform(
						GLLightRenderPassSingletonComponent::getInstance().m_uni_shadowSplitAreas[j],
						l_shadowSplitCorner.x, l_shadowSplitCorner.y, l_shadowSplitCorner.z, l_shadowSplitCorner.w);
				}
			}
			else if (l_lightComponent->m_lightType == LightType::POINT)
			{
				updateUniform(
					GLLightRenderPassSingletonComponent::getInstance().m_uni_pointLights_position[i + l_pointLightIndexOffset],
					l_lightPos.x, l_lightPos.y, l_lightPos.z);
				updateUniform(
					GLLightRenderPassSingletonComponent::getInstance().m_uni_pointLights_radius[i + l_pointLightIndexOffset],
					l_lightComponent->m_radius);
				updateUniform(
					GLLightRenderPassSingletonComponent::getInstance().m_uni_pointLights_color[i + l_pointLightIndexOffset],
					l_lightColor.x, l_lightColor.y, l_lightColor.z);
			}
		}
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
	f_copyStencilBuffer(GLGeometryRenderPassSingletonComponent::getInstance().m_GLRPC->m_GLFBC, l_FBC);

	updateUniform(
		GLLightRenderPassSingletonComponent::getInstance().m_uni_isEmissive,
		true);

	// draw light pass rectangle
	l_MDC = g_pCoreSystem->getAssetSystem()->getMeshDataComponent(MeshShapeType::QUAD);
	drawMesh(l_MDC);

	glDisable(GL_STENCIL_TEST);
}

GLTextureDataComponent* GLRenderingSystemNS::updateSkyPass()
{
	glEnable(GL_DEPTH_TEST);
	glDepthFunc(GL_LEQUAL);

	//glEnable(GL_CULL_FACE);
	//glFrontFace(GL_CW);
	//glCullFace(GL_FRONT);

	// bind to framebuffer
	auto l_FBC = GLFinalRenderPassSingletonComponent::getInstance().m_skyPassGLRPC->m_GLFBC;
	f_bindDeferredFBC(l_FBC);

	activateShaderProgram(GLFinalRenderPassSingletonComponent::getInstance().m_skyPassSPC);

	auto l_MDC = g_pCoreSystem->getAssetSystem()->getMeshDataComponent(MeshShapeType::CUBE);

	if (g_GameSystemSingletonComponent->m_CameraComponents.size() > 0)
	{
		auto l_mainCamera = g_GameSystemSingletonComponent->m_CameraComponents[0];
		mat4 p = l_mainCamera->m_projectionMatrix;
		mat4 r =
			InnoMath::getInvertRotationMatrix(
				g_pCoreSystem->getGameSystem()->get<TransformComponent>(l_mainCamera->m_parentEntity)->m_globalTransformVector.m_rot
			);

		updateUniform(
			GLFinalRenderPassSingletonComponent::getInstance().m_skyPass_uni_p,
			p);
		updateUniform(
			GLFinalRenderPassSingletonComponent::getInstance().m_skyPass_uni_r,
			r);
		updateUniform(
			GLFinalRenderPassSingletonComponent::getInstance().m_skyPass_uni_viewportSize,
			(float)rtSizeX, (float)rtSizeY);

		auto l_eyePos = g_pCoreSystem->getGameSystem()->get<TransformComponent>(l_mainCamera->m_parentEntity)->m_globalTransformVector.m_pos;
		updateUniform(
			GLFinalRenderPassSingletonComponent::getInstance().m_skyPass_uni_eyePos,
			l_eyePos.x, l_eyePos.y, l_eyePos.z);

		auto l_lightDir = InnoMath::getDirection(
			direction::BACKWARD,
			g_pCoreSystem->getGameSystem()->get<TransformComponent>(g_GameSystemSingletonComponent->m_LightComponents[0]->m_parentEntity)->m_globalTransformVector.m_rot
		);
		updateUniform(
			GLFinalRenderPassSingletonComponent::getInstance().m_skyPass_uni_lightDir,
			l_lightDir.x, l_lightDir.y, l_lightDir.z);

		drawMesh(l_MDC);
	}

	glDisable(GL_CULL_FACE);
	glDisable(GL_DEPTH_TEST);

	return GLFinalRenderPassSingletonComponent::getInstance().m_skyPassGLRPC->m_GLTDCs[0];
}

GLTextureDataComponent* GLRenderingSystemNS::updatePreTAAPass()
{
	auto l_FBC = GLFinalRenderPassSingletonComponent::getInstance().m_preTAAPassGLRPC->m_GLFBC;
	f_bindDeferredFBC(l_FBC);

	activateShaderProgram(GLFinalRenderPassSingletonComponent::getInstance().m_preTAAPassSPC);

	auto l_MDC = g_pCoreSystem->getAssetSystem()->getMeshDataComponent(MeshShapeType::QUAD);

	activate2DTexture(
		GLLightRenderPassSingletonComponent::getInstance().m_GLRPC->m_GLTDCs[0],
		0);
	activate2DTexture(
		GLFinalRenderPassSingletonComponent::getInstance().m_skyPassGLRPC->m_GLTDCs[0],
		1);
	activate2DTexture(
		GLTerrainRenderPassSingletonComponent::getInstance().m_GLRPC->m_GLTDCs[0],
		2);
	drawMesh(l_MDC);

	return GLFinalRenderPassSingletonComponent::getInstance().m_preTAAPassGLRPC->m_GLTDCs[0];
}

GLTextureDataComponent* GLRenderingSystemNS::updateTAAPass(GLTextureDataComponent* inputGLTDC)
{
	GLTextureDataComponent* l_currentFrameTAAGLTDC = GLFinalRenderPassSingletonComponent::getInstance().m_TAAPingPassGLRPC->m_GLTDCs[0];
	GLTextureDataComponent* l_lastFrameTAAGLTDC = GLFinalRenderPassSingletonComponent::getInstance().m_TAAPongPassGLRPC->m_GLTDCs[0];

	activateShaderProgram(GLFinalRenderPassSingletonComponent::getInstance().m_TAAPassSPC);

	GLFrameBufferComponent* l_FBC;

	if (g_RenderingSystemSingletonComponent->m_isTAAPingPass)
	{
		l_currentFrameTAAGLTDC = GLFinalRenderPassSingletonComponent::getInstance().m_TAAPingPassGLRPC->m_GLTDCs[0];
		l_lastFrameTAAGLTDC = GLFinalRenderPassSingletonComponent::getInstance().m_TAAPongPassGLRPC->m_GLTDCs[0];

		l_FBC = GLFinalRenderPassSingletonComponent::getInstance().m_TAAPingPassGLRPC->m_GLFBC;

		g_RenderingSystemSingletonComponent->m_isTAAPingPass = false;
	}
	else
	{
		l_currentFrameTAAGLTDC = GLFinalRenderPassSingletonComponent::getInstance().m_TAAPongPassGLRPC->m_GLTDCs[0];
		l_lastFrameTAAGLTDC = GLFinalRenderPassSingletonComponent::getInstance().m_TAAPingPassGLRPC->m_GLTDCs[0];

		l_FBC = GLFinalRenderPassSingletonComponent::getInstance().m_TAAPongPassGLRPC->m_GLFBC;

		g_RenderingSystemSingletonComponent->m_isTAAPingPass = true;
	}

	f_bindDeferredFBC(l_FBC);

	activate2DTexture(inputGLTDC,
		0);
	activate2DTexture(
		l_lastFrameTAAGLTDC,
		1);
	activate2DTexture(
		GLGeometryRenderPassSingletonComponent::getInstance().m_GLRPC->m_GLTDCs[3],
		2);

	updateUniform(
		GLFinalRenderPassSingletonComponent::getInstance().m_TAAPass_uni_renderTargetSize,
		(float)rtSizeX, (float)rtSizeY);

	auto l_MDC = g_pCoreSystem->getAssetSystem()->getMeshDataComponent(MeshShapeType::QUAD);
	drawMesh(l_MDC);

	return l_currentFrameTAAGLTDC;
}

GLTextureDataComponent* GLRenderingSystemNS::updateTAASharpenPass(GLTextureDataComponent * inputGLTDC)
{
	auto l_FBC = GLFinalRenderPassSingletonComponent::getInstance().m_TAASharpenPassGLRPC->m_GLFBC;
	f_bindDeferredFBC(l_FBC);
	activateShaderProgram(GLFinalRenderPassSingletonComponent::getInstance().m_TAASharpenPassSPC);

	activate2DTexture(
		inputGLTDC,
		0);

	updateUniform(
		GLFinalRenderPassSingletonComponent::getInstance().m_TAASharpenPass_uni_renderTargetSize,
		(float)rtSizeX, (float)rtSizeY);

	auto l_MDC = g_pCoreSystem->getAssetSystem()->getMeshDataComponent(MeshShapeType::QUAD);
	drawMesh(l_MDC);

	return GLFinalRenderPassSingletonComponent::getInstance().m_TAASharpenPassGLRPC->m_GLTDCs[0];
}

GLTextureDataComponent* GLRenderingSystemNS::updateBloomExtractPass(GLTextureDataComponent * inputGLTDC)
{
	auto l_FBC = GLFinalRenderPassSingletonComponent::getInstance().m_bloomExtractPassGLRPC->m_GLFBC;
	f_bindDeferredFBC(l_FBC);

	activateShaderProgram(GLFinalRenderPassSingletonComponent::getInstance().m_bloomExtractPassSPC);

	activate2DTexture(inputGLTDC, 0);

	auto l_MDC = g_pCoreSystem->getAssetSystem()->getMeshDataComponent(MeshShapeType::QUAD);
	drawMesh(l_MDC);

	return GLFinalRenderPassSingletonComponent::getInstance().m_bloomExtractPassGLRPC->m_GLTDCs[0];
}

GLTextureDataComponent* GLRenderingSystemNS::updateBloomBlurPass(GLTextureDataComponent * inputGLTDC)
{
	GLTextureDataComponent* l_currentFrameBloomBlurGLTDC = GLFinalRenderPassSingletonComponent::getInstance().m_bloomBlurPingPassGLRPC->m_GLTDCs[0];
	GLTextureDataComponent* l_lastFrameBloomBlurGLTDC = GLFinalRenderPassSingletonComponent::getInstance().m_bloomBlurPongPassGLRPC->m_GLTDCs[0];

	activateShaderProgram(GLFinalRenderPassSingletonComponent::getInstance().m_bloomBlurPassSPC);

	bool l_isPing = true;
	bool l_isFirstIteration = true;

	auto l_MDC = g_pCoreSystem->getAssetSystem()->getMeshDataComponent(MeshShapeType::QUAD);

	for (size_t i = 0; i < 5; i++)
	{
		if (l_isPing)
		{
			l_currentFrameBloomBlurGLTDC = GLFinalRenderPassSingletonComponent::getInstance().m_bloomBlurPingPassGLRPC->m_GLTDCs[0];
			l_lastFrameBloomBlurGLTDC = GLFinalRenderPassSingletonComponent::getInstance().m_bloomBlurPongPassGLRPC->m_GLTDCs[0];

			auto l_FBC = GLFinalRenderPassSingletonComponent::getInstance().m_bloomBlurPingPassGLRPC->m_GLFBC;
			f_bindDeferredFBC(l_FBC);

			updateUniform(
				GLFinalRenderPassSingletonComponent::getInstance().m_bloomBlurPass_uni_horizontal,
				true);

			if (l_isFirstIteration)
			{
				activate2DTexture(
					inputGLTDC,
					0);
				l_isFirstIteration = false;
			}
			else
			{
				activate2DTexture(
					l_lastFrameBloomBlurGLTDC,
					0);
			}

			drawMesh(l_MDC);

			l_isPing = false;
		}
		else
		{
			l_currentFrameBloomBlurGLTDC = GLFinalRenderPassSingletonComponent::getInstance().m_bloomBlurPongPassGLRPC->m_GLTDCs[0];
			l_lastFrameBloomBlurGLTDC = GLFinalRenderPassSingletonComponent::getInstance().m_bloomBlurPingPassGLRPC->m_GLTDCs[0];

			auto l_FBC = GLFinalRenderPassSingletonComponent::getInstance().m_bloomBlurPongPassGLRPC->m_GLFBC;
			f_bindDeferredFBC(l_FBC);

			updateUniform(
				GLFinalRenderPassSingletonComponent::getInstance().m_bloomBlurPass_uni_horizontal,
				false);

			activate2DTexture(
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
	auto l_FBC = GLFinalRenderPassSingletonComponent::getInstance().m_motionBlurPassGLRPC->m_GLFBC;
	f_bindDeferredFBC(l_FBC);

	activateShaderProgram(GLFinalRenderPassSingletonComponent::getInstance().m_motionBlurPassSPC);

	activate2DTexture(
		GLGeometryRenderPassSingletonComponent::getInstance().m_GLRPC->m_GLTDCs[3],
		0);
	activate2DTexture(inputGLTDC, 1);

	auto l_MDC = g_pCoreSystem->getAssetSystem()->getMeshDataComponent(MeshShapeType::QUAD);
	drawMesh(l_MDC);

	return GLFinalRenderPassSingletonComponent::getInstance().m_motionBlurPassGLRPC->m_GLTDCs[0];
}

GLTextureDataComponent* GLRenderingSystemNS::updateBillboardPass()
{
	glEnable(GL_DEPTH_TEST);
	glDepthFunc(GL_LESS);

	auto l_FBC = GLFinalRenderPassSingletonComponent::getInstance().m_billboardPassGLRPC->m_GLFBC;
	f_bindDeferredFBC(l_FBC);

	// copy depth buffer from G-Pass
	f_copyDepthBuffer(GLGeometryRenderPassSingletonComponent::getInstance().m_GLRPC->m_GLFBC, l_FBC);

	activateShaderProgram(GLFinalRenderPassSingletonComponent::getInstance().m_billboardPassSPC);

	if (g_GameSystemSingletonComponent->m_CameraComponents.size() > 0)
	{
		mat4 p = g_GameSystemSingletonComponent->m_CameraComponents[0]->m_projectionMatrix;
		mat4 r =
			InnoMath::getInvertRotationMatrix(
				g_pCoreSystem->getGameSystem()->get<TransformComponent>(g_GameSystemSingletonComponent->m_CameraComponents[0]->m_parentEntity)->m_globalTransformVector.m_rot
			);
		mat4 t =
			InnoMath::getInvertTranslationMatrix(
				g_pCoreSystem->getGameSystem()->get<TransformComponent>(g_GameSystemSingletonComponent->m_CameraComponents[0]->m_parentEntity)->m_globalTransformVector.m_pos
			);

		updateUniform(
			GLFinalRenderPassSingletonComponent::getInstance().m_billboardPass_uni_p,
			p);
		updateUniform(
			GLFinalRenderPassSingletonComponent::getInstance().m_billboardPass_uni_r,
			r);
		updateUniform(
			GLFinalRenderPassSingletonComponent::getInstance().m_billboardPass_uni_t,
			t);
	}
	if (g_GameSystemSingletonComponent->m_VisibleComponents.size() > 0)
	{
		// draw each visibleComponent
		for (auto& l_visibleComponent : g_GameSystemSingletonComponent->m_VisibleComponents)
		{
			if (l_visibleComponent->m_visiblilityType == VisiblilityType::BILLBOARD)
			{
				auto l_GlobalPos = g_pCoreSystem->getGameSystem()->get<TransformComponent>(l_visibleComponent->m_parentEntity)->m_globalTransformVector.m_pos;
				auto l_GlobalCameraPos = g_pCoreSystem->getGameSystem()->get<TransformComponent>(g_GameSystemSingletonComponent->m_CameraComponents[0]->m_parentEntity)->m_globalTransformVector.m_pos;

				updateUniform(
					GLFinalRenderPassSingletonComponent::getInstance().m_billboardPass_uni_pos,
					l_GlobalPos.x, l_GlobalPos.y, l_GlobalPos.z);
				//updateUniform(
				//	GLFinalRenderPassSingletonComponent::getInstance().m_billboardPass_uni_albedo,
				//	l_visibleComponent->m_albedo.x, l_visibleComponent->m_albedo.y, l_visibleComponent->m_albedo.z);
				auto l_distanceToCamera = (l_GlobalCameraPos - l_GlobalPos).length();
				if (l_distanceToCamera > 1.0f)
				{
					updateUniform(
						GLFinalRenderPassSingletonComponent::getInstance().m_billboardPass_uni_size,
						(1.0f / l_distanceToCamera) * (9.0f / 16.0f), (1.0f / l_distanceToCamera));
				}
				else
				{
					updateUniform(
						GLFinalRenderPassSingletonComponent::getInstance().m_billboardPass_uni_size,
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
							activate2DTexture(m_basicNormalTemplate, 0);
						}
						drawMesh(l_MDC);
					}
				}
			}
		}
	}

	glDisable(GL_DEPTH_TEST);

	return GLFinalRenderPassSingletonComponent::getInstance().m_billboardPassGLRPC->m_GLTDCs[0];
}

GLTextureDataComponent* GLRenderingSystemNS::updateDebuggerPass()
{
	glEnable(GL_DEPTH_TEST);
	glDepthFunc(GL_LESS);

	auto l_FBC = GLFinalRenderPassSingletonComponent::getInstance().m_debuggerPassGLRPC->m_GLFBC;
	f_bindDeferredFBC(l_FBC);

	// copy depth buffer from G-Pass
	f_copyDepthBuffer(GLGeometryRenderPassSingletonComponent::getInstance().m_GLRPC->m_GLFBC, l_FBC);

	activateShaderProgram(GLFinalRenderPassSingletonComponent::getInstance().m_debuggerPassSPC);

	glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
	if (g_GameSystemSingletonComponent->m_CameraComponents.size() > 0)
	{
		updateUniform(
			GLFinalRenderPassSingletonComponent::getInstance().m_debuggerPass_uni_p,
			m_CamProjOriginal);
		updateUniform(
			GLFinalRenderPassSingletonComponent::getInstance().m_debuggerPass_uni_r,
			m_CamRot);
		updateUniform(
			GLFinalRenderPassSingletonComponent::getInstance().m_debuggerPass_uni_t,
			m_CamTrans);

		//	for (auto& l_cameraComponent : g_GameSystemSingletonComponent->m_CameraComponents)
		//	{
		//		// draw frustum for cameraComponent
		//		if (l_cameraComponent->m_drawFrustum)
		//		{
		//			auto l_cameraLocalMat = InnoMath::generateIdentityMatrix<float>();
		//			updateUniform(
		//				GLFinalRenderPassSingletonComponent::getInstance().m_debuggerPass_uni_m,
		//				l_cameraLocalMat);
		//			auto l_mesh = g_pCoreSystem->getAssetSystem()->getMeshDataComponent(l_cameraComponent->m_FrustumMeshID);
		//			drawMesh(l_mesh);
		//		}
		//		// draw AABB of frustum for cameraComponent
		//		if (l_cameraComponent->m_drawAABB)
		//		{
		//			auto l_cameraLocalMat = InnoMath::generateIdentityMatrix<float>();
		//			updateUniform(
		//				GLFinalRenderPassSingletonComponent::getInstance().m_debuggerPass_uni_m,
		//				l_cameraLocalMat);
		//			auto l_mesh = g_pCoreSystem->getAssetSystem()->getMeshDataComponent(l_cameraComponent->m_AABBMeshID);
		//			drawMesh(l_mesh);
		//		}
		//	}
		//}
		//if (g_GameSystemSingletonComponent->m_LightComponents.size() > 0)
		//{
		//	// draw AABB for lightComponent
		//	for (auto& l_lightComponent : g_GameSystemSingletonComponent->m_LightComponents)
		//	{
		//		if (l_lightComponent->m_drawAABB)
		//		{
		//			updateUniform(
		//				GLFinalRenderPassSingletonComponent::getInstance().m_debuggerPass_uni_m,
		//				g_pCoreSystem->getGameSystem()->get<TransformComponent>(l_lightComponent->m_parentEntity)->m_globalTransformMatrix.m_transformationMat);
		//			for (auto l_AABBMeshID : l_lightComponent->m_AABBMeshIDs)
		//			{
		//				auto l_mesh = g_pCoreSystem->getAssetSystem()->getMeshDataComponent(l_AABBMeshID);
		//				drawMesh(l_mesh);
		//			}
		//		}
		//	}
		//}
		
		for (auto& AABBWireframeDataPack : PhysicsSystemSingletonComponent::getInstance().m_AABBWireframeDataPack)
		{
			updateUniform(
				GLFinalRenderPassSingletonComponent::getInstance().m_debuggerPass_uni_m,
				AABBWireframeDataPack.m);
			drawMesh(AABBWireframeDataPack.MDC);
		}
	}
	glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
	glDisable(GL_DEPTH_TEST);

	return GLFinalRenderPassSingletonComponent::getInstance().m_debuggerPassGLRPC->m_GLTDCs[0];
}

GLTextureDataComponent* GLRenderingSystemNS::updateFinalBlendPass()
{
	auto l_FBC = GLFinalRenderPassSingletonComponent::getInstance().m_finalBlendPassGLRPC->m_GLFBC;
	f_bindDeferredFBC(l_FBC);

	activateShaderProgram(GLFinalRenderPassSingletonComponent::getInstance().m_finalBlendPassSPC);

	// motion blur pass rendering target
	activate2DTexture(
		GLFinalRenderPassSingletonComponent::getInstance().m_motionBlurPassGLRPC->m_GLTDCs[0],
		0);
	// bloom pass rendering target
	activate2DTexture(
		GLFinalRenderPassSingletonComponent::getInstance().m_bloomBlurPongPassGLRPC->m_GLTDCs[0],
		1);
	// billboard pass rendering target
	activate2DTexture(
		GLFinalRenderPassSingletonComponent::getInstance().m_billboardPassGLRPC->m_GLTDCs[0],
		2);
	// debugger pass rendering target
	activate2DTexture(
		GLFinalRenderPassSingletonComponent::getInstance().m_debuggerPassGLRPC->m_GLTDCs[0],
		3);
	// draw final pass rectangle
	auto l_MDC = g_pCoreSystem->getAssetSystem()->getMeshDataComponent(MeshShapeType::QUAD);
	drawMesh(l_MDC);

	// draw again for game build
	glBindFramebuffer(GL_FRAMEBUFFER, 0);
	glBindRenderbuffer(GL_RENDERBUFFER, 0);

	drawMesh(l_MDC);

	return GLFinalRenderPassSingletonComponent::getInstance().m_finalBlendPassGLRPC->m_GLTDCs[0];
}

void GLRenderingSystemNS::updateFinalRenderPass()
{
	auto skyPassResult = updateSkyPass();

	auto preTAAPassResult = updatePreTAAPass();

	GLTextureDataComponent* bloomInputGLTDC;

	if (g_RenderingSystemSingletonComponent->m_useTAA)
	{
		auto TAAPassResult = updateTAAPass(preTAAPassResult);

		auto TAASharpenPassResult = updateTAASharpenPass(TAAPassResult);

		bloomInputGLTDC = TAASharpenPassResult;
	}
	else
	{
		bloomInputGLTDC = preTAAPassResult;
	}

	if (g_RenderingSystemSingletonComponent->m_useBloom)
	{
		auto bloomExtractPassResult = updateBloomExtractPass(bloomInputGLTDC);

		auto bloomBlurPassResult = updateBloomBlurPass(bloomExtractPassResult);
	}
	else
	{
		f_cleanFBC(GLFinalRenderPassSingletonComponent::getInstance().m_bloomExtractPassGLRPC->m_GLFBC);
		f_cleanFBC(GLFinalRenderPassSingletonComponent::getInstance().m_bloomBlurPingPassGLRPC->m_GLFBC);
		f_cleanFBC(GLFinalRenderPassSingletonComponent::getInstance().m_bloomBlurPongPassGLRPC->m_GLFBC);
	}

	updateMotionBlurPass(bloomInputGLTDC);

	updateBillboardPass();

	if (g_RenderingSystemSingletonComponent->m_drawOverlapWireframe)
	{
		updateDebuggerPass();
	}
	else
	{
		f_cleanFBC(GLFinalRenderPassSingletonComponent::getInstance().m_debuggerPassGLRPC->m_GLFBC);
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
	GLRenderingSystemNS::rtSizeX = (GLsizei)WindowSystemSingletonComponent::getInstance().m_windowResolution.x;
	GLRenderingSystemNS::rtSizeY = (GLsizei)WindowSystemSingletonComponent::getInstance().m_windowResolution.y;

	GLRenderingSystemNS::resizeGLRenderPassComponent(GLGeometryRenderPassSingletonComponent::getInstance().m_GLRPC);
	GLRenderingSystemNS::resizeGLRenderPassComponent(GLTerrainRenderPassSingletonComponent::getInstance().m_GLRPC);
	GLRenderingSystemNS::resizeGLRenderPassComponent(GLLightRenderPassSingletonComponent::getInstance().m_GLRPC);
	GLRenderingSystemNS::resizeGLRenderPassComponent(GLFinalRenderPassSingletonComponent::getInstance().m_skyPassGLRPC);
	GLRenderingSystemNS::resizeGLRenderPassComponent(GLFinalRenderPassSingletonComponent::getInstance().m_preTAAPassGLRPC);
	GLRenderingSystemNS::resizeGLRenderPassComponent(GLFinalRenderPassSingletonComponent::getInstance().m_TAAPingPassGLRPC);
	GLRenderingSystemNS::resizeGLRenderPassComponent(GLFinalRenderPassSingletonComponent::getInstance().m_TAAPongPassGLRPC);
	GLRenderingSystemNS::resizeGLRenderPassComponent(GLFinalRenderPassSingletonComponent::getInstance().m_TAASharpenPassGLRPC);
	GLRenderingSystemNS::resizeGLRenderPassComponent(GLFinalRenderPassSingletonComponent::getInstance().m_bloomExtractPassGLRPC);
	GLRenderingSystemNS::resizeGLRenderPassComponent(GLFinalRenderPassSingletonComponent::getInstance().m_bloomBlurPingPassGLRPC);
	GLRenderingSystemNS::resizeGLRenderPassComponent(GLFinalRenderPassSingletonComponent::getInstance().m_bloomBlurPongPassGLRPC);
	GLRenderingSystemNS::resizeGLRenderPassComponent(GLFinalRenderPassSingletonComponent::getInstance().m_motionBlurPassGLRPC);
	GLRenderingSystemNS::resizeGLRenderPassComponent(GLFinalRenderPassSingletonComponent::getInstance().m_billboardPassGLRPC);
	GLRenderingSystemNS::resizeGLRenderPassComponent(GLFinalRenderPassSingletonComponent::getInstance().m_debuggerPassGLRPC);
	GLRenderingSystemNS::resizeGLRenderPassComponent(GLFinalRenderPassSingletonComponent::getInstance().m_finalBlendPassGLRPC);

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
	if (TDC->m_textureDataDesc.textureUsageType == TextureUsageType::ENVIRONMENT_CAPTURE || TDC->m_textureDataDesc.textureUsageType == TextureUsageType::ENVIRONMENT_CONVOLUTION || TDC->m_textureDataDesc.textureUsageType == TextureUsageType::ENVIRONMENT_PREFILTER)
	{
		glBindTexture(GL_TEXTURE_CUBE_MAP, GLTDC->m_TAO);
		glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0 + colorAttachmentIndex, GL_TEXTURE_CUBE_MAP_POSITIVE_X + textureIndex, GLTDC->m_TAO, mipLevel);
	}
	else if (TDC->m_textureDataDesc.textureUsageType == TextureUsageType::SHADOWMAP)
	{
		glBindTexture(GL_TEXTURE_2D, GLTDC->m_TAO);
		glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, GLTDC->m_TAO, mipLevel);
	}
	else
	{
		glBindTexture(GL_TEXTURE_2D, GLTDC->m_TAO);
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
	if (l_GLTDC)
	{
		if (TDC->m_textureDataDesc.textureUsageType == TextureUsageType::ENVIRONMENT_CAPTURE || TDC->m_textureDataDesc.textureUsageType == TextureUsageType::ENVIRONMENT_CONVOLUTION || TDC->m_textureDataDesc.textureUsageType == TextureUsageType::ENVIRONMENT_PREFILTER)
		{
			activateCubemapTexture(l_GLTDC, activateIndex);
		}
		else
		{
			activate2DTexture(l_GLTDC, activateIndex);
		}
	}
}

void GLRenderingSystemNS::activate2DTexture(GLTextureDataComponent * GLTDC, int activateIndex)
{
	glActiveTexture(GL_TEXTURE0 + activateIndex);
	glBindTexture(GL_TEXTURE_2D, GLTDC->m_TAO);
}

void GLRenderingSystemNS::activateCubemapTexture(GLTextureDataComponent * GLTDC, int activateIndex)
{
	glActiveTexture(GL_TEXTURE0 + activateIndex);
	glBindTexture(GL_TEXTURE_CUBE_MAP, GLTDC->m_TAO);
}