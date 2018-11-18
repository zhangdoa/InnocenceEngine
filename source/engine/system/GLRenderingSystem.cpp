#include "GLRenderingSystem.h"

#include <sstream>

#include "../component/EnvironmentRenderPassSingletonComponent.h"
#include "../component/ShadowRenderPassSingletonComponent.h"
#include "../component/GeometryRenderPassSingletonComponent.h"
#include "../component/LightRenderPassSingletonComponent.h"
#include "../component/GLFinalRenderPassSingletonComponent.h"

#include "../component/RenderingSystemSingletonComponent.h"
#include "../component/AssetSystemSingletonComponent.h"
#include "../component/GameSystemSingletonComponent.h"
#include "../component/GLRenderingSystemSingletonComponent.h"

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
	void initializeEnvironmentRenderPass();
	void initializeShadowRenderPass();
	void initializeGeometryRenderPass();
	void initializeLightRenderPass();
	void initializeFinalRenderPass();

	void initializeSkyPass();
	void initializeTAAPass();
	void initializeBloomExtractPass();
	void initializeBloomBlurPass();
	void initializeMotionBlurPass();
	void initializeBillboardPass();
	void initializeDebuggerPass();
	void initializeFinalBlendPass();

	GLMeshDataComponent* initializeMeshDataComponent(MeshDataComponent* rhs);
	GLTextureDataComponent* initializeTextureDataComponent(TextureDataComponent* rhs);
	void initializeShader(GLuint& shaderProgram, GLuint& shaderID, GLuint shaderType, const std::string& shaderFilePath);

	GLMeshDataComponent* addGLMeshDataComponent(EntityID rhs);
	GLTextureDataComponent* addGLTextureDataComponent(EntityID rhs);

	GLMeshDataComponent* getGLMeshDataComponent(EntityID rhs);
	GLTextureDataComponent* getGLTextureDataComponent(EntityID rhs);

	void updateEnvironmentRenderPass();
	void updateShadowRenderPass();
	void updateGeometryRenderPass();
	void updateLightRenderPass();
	void updateFinalRenderPass();

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
	void activateTexture(TextureDataComponent* TDC, int activateIndex);
	void activate2DTexture(GLTextureDataComponent* GLTDC, int activateIndex);
	void activateCubemapTexture(GLTextureDataComponent* GLTDC, int activateIndex);

	static GameSystemSingletonComponent* g_GameSystemSingletonComponent;
	static RenderingSystemSingletonComponent* g_RenderingSystemSingletonComponent;
	static GLRenderingSystemSingletonComponent* g_GLRenderingSystemSingletonComponent;

	objectStatus m_objectStatus = objectStatus::SHUTDOWN;

	GLMeshDataComponent* m_UnitLineTemplate;
	GLMeshDataComponent* m_UnitQuadTemplate;
	GLMeshDataComponent* m_UnitCubeTemplate;
	GLMeshDataComponent* m_UnitSphereTemplate;

	GLTextureDataComponent* m_basicNormalTemplate;
	GLTextureDataComponent* m_basicAlbedoTemplate;
	GLTextureDataComponent* m_basicMetallicTemplate;
	GLTextureDataComponent* m_basicRoughnessTemplate;
	GLTextureDataComponent* m_basicAOTemplate;
}

bool GLRenderingSystem::setup()
{
	GLRenderingSystemNS::g_GameSystemSingletonComponent = &GameSystemSingletonComponent::getInstance();
	GLRenderingSystemNS::g_RenderingSystemSingletonComponent = &RenderingSystemSingletonComponent::getInstance();
	GLRenderingSystemNS::g_GLRenderingSystemSingletonComponent = &GLRenderingSystemSingletonComponent::getInstance();

	if (GLRenderingSystemNS::g_RenderingSystemSingletonComponent->m_MSAAdepth)
	{
		// 16x antialiasing
		glfwWindowHint(GLFW_SAMPLES, 16);
		// MSAA
		glEnable(GL_MULTISAMPLE);
	}
	// enable seamless cubemap sampling for lower mip levels in the pre-filter map.
	glEnable(GL_TEXTURE_CUBE_MAP_SEAMLESS);
	glEnable(GL_PROGRAM_POINT_SIZE);
	glEnable(GL_TEXTURE_2D);

	return true;
}

bool GLRenderingSystem::initialize()
{
	GLRenderingSystemNS::initializeDefaultAssets();
	GLRenderingSystemNS::initializeHaltonSampler();
	GLRenderingSystemNS::initializeEnvironmentRenderPass();
	GLRenderingSystemNS::initializeShadowRenderPass();
	GLRenderingSystemNS::initializeGeometryRenderPass();
	GLRenderingSystemNS::initializeLightRenderPass();
	GLRenderingSystemNS::initializeFinalRenderPass();

	return true;
}

void  GLRenderingSystemNS::initializeDefaultAssets()
{
	m_UnitLineTemplate = initializeMeshDataComponent(g_pCoreSystem->getAssetSystem()->getMeshDataComponent(meshShapeType::LINE));
	m_UnitQuadTemplate = initializeMeshDataComponent(g_pCoreSystem->getAssetSystem()->getMeshDataComponent(meshShapeType::QUAD));
	m_UnitCubeTemplate = initializeMeshDataComponent(g_pCoreSystem->getAssetSystem()->getMeshDataComponent(meshShapeType::CUBE));
	m_UnitSphereTemplate = initializeMeshDataComponent(g_pCoreSystem->getAssetSystem()->getMeshDataComponent(meshShapeType::SPHERE));

	m_basicNormalTemplate = initializeTextureDataComponent(g_pCoreSystem->getAssetSystem()->getTextureDataComponent(textureType::NORMAL));
	m_basicAlbedoTemplate = initializeTextureDataComponent(g_pCoreSystem->getAssetSystem()->getTextureDataComponent(textureType::ALBEDO));
	m_basicMetallicTemplate = initializeTextureDataComponent(g_pCoreSystem->getAssetSystem()->getTextureDataComponent(textureType::METALLIC));
	m_basicRoughnessTemplate = initializeTextureDataComponent(g_pCoreSystem->getAssetSystem()->getTextureDataComponent(textureType::ROUGHNESS));
	m_basicAOTemplate = initializeTextureDataComponent(g_pCoreSystem->getAssetSystem()->getTextureDataComponent(textureType::AMBIENT_OCCLUSION));
}

float GLRenderingSystemNS::RadicalInverse(unsigned int n, unsigned int base) {

	float val = 0.0f;
	float invBase = 1.0f / base, invBi = invBase;
	while (n > 0)
	{
		auto d_i = (n % base);
		val += d_i * invBi;
		n *= invBase;
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

void GLRenderingSystemNS::initializeEnvironmentRenderPass()
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

	EnvironmentRenderPassSingletonComponent::getInstance().m_FBC = l_FBC;

	// generate and bind texture
	auto l_TDC = g_pCoreSystem->getMemorySystem()->spawn<TextureDataComponent>();

	l_TDC->m_textureType = textureType::ENVIRONMENT_CAPTURE;
	l_TDC->m_textureColorComponentsFormat = textureColorComponentsFormat::RGB16F;
	l_TDC->m_texturePixelDataFormat = texturePixelDataFormat::RGB;
	l_TDC->m_textureMinFilterMethod = textureFilterMethod::LINEAR_MIPMAP_LINEAR;
	l_TDC->m_textureMagFilterMethod = textureFilterMethod::LINEAR;
	l_TDC->m_textureWrapMethod = textureWrapMethod::REPEAT;
	l_TDC->m_textureWidth = 2048;
	l_TDC->m_textureHeight = 2048;
	l_TDC->m_texturePixelDataType = texturePixelDataType::FLOAT;
	l_TDC->m_textureData = { nullptr, nullptr, nullptr, nullptr, nullptr, nullptr };

	EnvironmentRenderPassSingletonComponent::getInstance().m_capturePassTDC = l_TDC;

	auto l_GLTDC = initializeTextureDataComponent(l_TDC);

	EnvironmentRenderPassSingletonComponent::getInstance().m_capturePassGLTDC = l_GLTDC;

	////
	l_TDC = g_pCoreSystem->getMemorySystem()->spawn<TextureDataComponent>();

	l_TDC->m_textureType = textureType::ENVIRONMENT_CONVOLUTION;
	l_TDC->m_textureColorComponentsFormat = textureColorComponentsFormat::RGB16F;
	l_TDC->m_texturePixelDataFormat = texturePixelDataFormat::RGB;
	l_TDC->m_textureMinFilterMethod = textureFilterMethod::LINEAR;
	l_TDC->m_textureMagFilterMethod = textureFilterMethod::LINEAR;
	l_TDC->m_textureWrapMethod = textureWrapMethod::REPEAT;
	l_TDC->m_textureWidth = 128;
	l_TDC->m_textureHeight = 128;
	l_TDC->m_texturePixelDataType = texturePixelDataType::FLOAT;
	l_TDC->m_textureData = { nullptr, nullptr, nullptr, nullptr, nullptr, nullptr };

	EnvironmentRenderPassSingletonComponent::getInstance().m_convolutionPassTDC = l_TDC;

	l_GLTDC = initializeTextureDataComponent(l_TDC);

	EnvironmentRenderPassSingletonComponent::getInstance().m_convolutionPassGLTDC = l_GLTDC;

	////
	l_TDC = g_pCoreSystem->getMemorySystem()->spawn<TextureDataComponent>();

	l_TDC->m_textureType = textureType::ENVIRONMENT_PREFILTER;
	l_TDC->m_textureColorComponentsFormat = textureColorComponentsFormat::RGB16F;
	l_TDC->m_texturePixelDataFormat = texturePixelDataFormat::RGB;
	l_TDC->m_textureMinFilterMethod = textureFilterMethod::LINEAR_MIPMAP_LINEAR;
	l_TDC->m_textureMagFilterMethod = textureFilterMethod::LINEAR;
	l_TDC->m_textureWrapMethod = textureWrapMethod::REPEAT;
	l_TDC->m_textureWidth = 128;
	l_TDC->m_textureHeight = 128;
	l_TDC->m_texturePixelDataType = texturePixelDataType::FLOAT;
	l_TDC->m_textureData = { nullptr, nullptr, nullptr, nullptr, nullptr, nullptr };

	EnvironmentRenderPassSingletonComponent::getInstance().m_preFilterPassTDC = l_TDC;

	l_GLTDC = initializeTextureDataComponent(l_TDC);

	EnvironmentRenderPassSingletonComponent::getInstance().m_preFilterPassGLTDC = l_GLTDC;

	////
	l_TDC = g_pCoreSystem->getMemorySystem()->spawn<TextureDataComponent>();

	l_TDC->m_textureType = textureType::RENDER_BUFFER_SAMPLER;
	l_TDC->m_textureColorComponentsFormat = textureColorComponentsFormat::RG16F;
	l_TDC->m_texturePixelDataFormat = texturePixelDataFormat::RG;
	l_TDC->m_textureMinFilterMethod = textureFilterMethod::LINEAR;
	l_TDC->m_textureMagFilterMethod = textureFilterMethod::LINEAR;
	l_TDC->m_textureWrapMethod = textureWrapMethod::CLAMP_TO_EDGE;
	l_TDC->m_textureWidth = 512;
	l_TDC->m_textureHeight = 512;
	l_TDC->m_texturePixelDataType = texturePixelDataType::FLOAT;
	l_TDC->m_textureData = { nullptr, nullptr, nullptr, nullptr, nullptr, nullptr };

	EnvironmentRenderPassSingletonComponent::getInstance().m_BRDFLUTTDC = l_TDC;

	l_GLTDC = initializeTextureDataComponent(l_TDC);

	EnvironmentRenderPassSingletonComponent::getInstance().m_BRDFLUTGLTDC = l_GLTDC;

	// finally check if framebuffer is complete
	if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
	{
		g_pCoreSystem->getLogSystem()->printLog("GLFrameBuffer: EnvironmentRenderPass Framebuffer is not completed!");
	}

	glBindRenderbuffer(GL_RENDERBUFFER, 0);
	glBindFramebuffer(GL_FRAMEBUFFER, 0);

	// shader programs and shaders
	auto l_shaderProgram = g_pCoreSystem->getMemorySystem()->spawn<GLShaderProgramComponent>();

	l_shaderProgram->m_program = glCreateProgram();

	initializeShader(
		l_shaderProgram->m_program,
		EnvironmentRenderPassSingletonComponent::getInstance().m_capturePassVertexShaderID,
		GL_VERTEX_SHADER,
		"GL3.3//environmentCapturePassPBSVertex.sf");
	initializeShader(
		l_shaderProgram->m_program,
		EnvironmentRenderPassSingletonComponent::getInstance().m_capturePassFragmentShaderID,
		GL_FRAGMENT_SHADER,
		"GL3.3//environmentCapturePassPBSFragment.sf");

	EnvironmentRenderPassSingletonComponent::getInstance().m_capturePass_uni_equirectangularMap = getUniformLocation(
		l_shaderProgram->m_program,
		"uni_equirectangularMap");
	updateUniform(
		EnvironmentRenderPassSingletonComponent::getInstance().m_capturePass_uni_equirectangularMap,
		0);
	EnvironmentRenderPassSingletonComponent::getInstance().m_capturePass_uni_p = getUniformLocation(
		l_shaderProgram->m_program,
		"uni_p");
	EnvironmentRenderPassSingletonComponent::getInstance().m_capturePass_uni_r = getUniformLocation(
		l_shaderProgram->m_program,
		"uni_r");

	EnvironmentRenderPassSingletonComponent::getInstance().m_capturePassSPC = l_shaderProgram;

	////
	l_shaderProgram = g_pCoreSystem->getMemorySystem()->spawn<GLShaderProgramComponent>();

	l_shaderProgram->m_program = glCreateProgram();

	initializeShader(
		l_shaderProgram->m_program,
		EnvironmentRenderPassSingletonComponent::getInstance().m_convolutionPassVertexShaderID,
		GL_VERTEX_SHADER,
		"GL3.3//environmentConvolutionPassPBSVertex.sf");
	initializeShader(
		l_shaderProgram->m_program,
		EnvironmentRenderPassSingletonComponent::getInstance().m_convolutionPassFragmentShaderID,
		GL_FRAGMENT_SHADER,
		"GL3.3//environmentConvolutionPassPBSFragment.sf");
	EnvironmentRenderPassSingletonComponent::getInstance().m_convolutionPass_uni_capturedCubeMap = getUniformLocation(
		l_shaderProgram->m_program,
		"uni_capturedCubeMap");
	updateUniform(
		EnvironmentRenderPassSingletonComponent::getInstance().m_convolutionPass_uni_capturedCubeMap,
		0);
	EnvironmentRenderPassSingletonComponent::getInstance().m_convolutionPass_uni_p = getUniformLocation(
		l_shaderProgram->m_program,
		"uni_p");
	EnvironmentRenderPassSingletonComponent::getInstance().m_convolutionPass_uni_r = getUniformLocation(
		l_shaderProgram->m_program,
		"uni_r");

	EnvironmentRenderPassSingletonComponent::getInstance().m_convolutionPassSPC = l_shaderProgram;

	////
	l_shaderProgram = g_pCoreSystem->getMemorySystem()->spawn<GLShaderProgramComponent>();

	l_shaderProgram->m_program = glCreateProgram();

	initializeShader(
		l_shaderProgram->m_program,
		EnvironmentRenderPassSingletonComponent::getInstance().m_preFilterPassVertexShaderID,
		GL_VERTEX_SHADER,
		"GL3.3//environmentPreFilterPassPBSVertex.sf");
	initializeShader(
		l_shaderProgram->m_program,
		EnvironmentRenderPassSingletonComponent::getInstance().m_preFilterPassFragmentShaderID,
		GL_FRAGMENT_SHADER,
		"GL3.3//environmentPreFilterPassPBSFragment.sf");
	EnvironmentRenderPassSingletonComponent::getInstance().m_preFilterPass_uni_capturedCubeMap = getUniformLocation(
		l_shaderProgram->m_program,
		"uni_capturedCubeMap");
	updateUniform(
		EnvironmentRenderPassSingletonComponent::getInstance().m_preFilterPass_uni_capturedCubeMap,
		0);
	EnvironmentRenderPassSingletonComponent::getInstance().m_preFilterPass_uni_roughness = getUniformLocation(
		l_shaderProgram->m_program,
		"uni_roughness");
	EnvironmentRenderPassSingletonComponent::getInstance().m_preFilterPass_uni_p = getUniformLocation(
		l_shaderProgram->m_program,
		"uni_p");
	EnvironmentRenderPassSingletonComponent::getInstance().m_preFilterPass_uni_r = getUniformLocation(
		l_shaderProgram->m_program,
		"uni_r");

	EnvironmentRenderPassSingletonComponent::getInstance().m_preFilterPassSPC = l_shaderProgram;

	////
	l_shaderProgram = g_pCoreSystem->getMemorySystem()->spawn<GLShaderProgramComponent>();

	l_shaderProgram->m_program = glCreateProgram();

	initializeShader(
		l_shaderProgram->m_program,
		EnvironmentRenderPassSingletonComponent::getInstance().m_BRDFLUTPassVertexShaderID,
		GL_VERTEX_SHADER,
		"GL3.3//environmentBRDFLUTPassPBSVertex.sf");
	initializeShader(
		l_shaderProgram->m_program,
		EnvironmentRenderPassSingletonComponent::getInstance().m_BRDFLUTPassFragmentShaderID,
		GL_FRAGMENT_SHADER,
		"GL3.3//environmentBRDFLUTPassPBSFragment.sf");

	EnvironmentRenderPassSingletonComponent::getInstance().m_BRDFLUTPassSPC = l_shaderProgram;
}

void GLRenderingSystemNS::initializeShadowRenderPass()
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

		ShadowRenderPassSingletonComponent::getInstance().m_FBCs.emplace_back(l_FBC);

		// generate and bind texture
		auto l_TDC = g_pCoreSystem->getMemorySystem()->spawn<TextureDataComponent>();

		l_TDC->m_textureType = textureType::SHADOWMAP;
		l_TDC->m_textureColorComponentsFormat = textureColorComponentsFormat::DEPTH_COMPONENT;
		l_TDC->m_texturePixelDataFormat = texturePixelDataFormat::DEPTH_COMPONENT;
		l_TDC->m_textureMinFilterMethod = textureFilterMethod::NEAREST;
		l_TDC->m_textureMagFilterMethod = textureFilterMethod::NEAREST;
		l_TDC->m_textureWrapMethod = textureWrapMethod::CLAMP_TO_BORDER;
		l_TDC->m_textureWidth = 2048;
		l_TDC->m_textureHeight = 2048;
		l_TDC->m_texturePixelDataType = texturePixelDataType::FLOAT;
		l_TDC->m_textureData = { nullptr };

		ShadowRenderPassSingletonComponent::getInstance().m_TDCs.emplace_back(l_TDC);

		auto l_GLTDC = initializeTextureDataComponent(l_TDC);

		attachTextureToFramebuffer(
			l_TDC,
			l_GLTDC,
			l_FBC,
			0, 0, 0
		);

		ShadowRenderPassSingletonComponent::getInstance().m_GLTDCs.emplace_back(l_GLTDC);

		glDrawBuffer(GL_NONE);
		glReadBuffer(GL_NONE);

		// finally check if framebuffer is complete
		if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
		{
			std::stringstream ss;
			ss << i;
			g_pCoreSystem->getLogSystem()->printLog("GLFrameBuffer: ShadowRenderPass level " + ss.str() + " Framebuffer is not completed!");
		}
	}

	glBindRenderbuffer(GL_RENDERBUFFER, 0);
	glBindFramebuffer(GL_FRAMEBUFFER, 0);

	// shader programs and shaders
	auto l_GLSPC = g_pCoreSystem->getMemorySystem()->spawn<GLShaderProgramComponent>();

	l_GLSPC->m_program = glCreateProgram();

	initializeShader(
		l_GLSPC->m_program,
		ShadowRenderPassSingletonComponent::getInstance().m_shadowPassVertexShaderID,
		GL_VERTEX_SHADER,
		"GL3.3//shadowPassVertex.sf");
	initializeShader(
		l_GLSPC->m_program,
		ShadowRenderPassSingletonComponent::getInstance().m_shadowPassFragmentShaderID,
		GL_FRAGMENT_SHADER,
		"GL3.3//shadowPassFragment.sf");

	ShadowRenderPassSingletonComponent::getInstance().m_shadowPass_uni_p = getUniformLocation(
		l_GLSPC->m_program,
		"uni_p");
	ShadowRenderPassSingletonComponent::getInstance().m_shadowPass_uni_v = getUniformLocation(
		l_GLSPC->m_program,
		"uni_v");
	ShadowRenderPassSingletonComponent::getInstance().m_shadowPass_uni_m = getUniformLocation(
		l_GLSPC->m_program,
		"uni_m");

	ShadowRenderPassSingletonComponent::getInstance().m_SPC = l_GLSPC;
}

void GLRenderingSystemNS::initializeGeometryRenderPass()
{
	// generate and bind framebuffer
	auto l_FBC = g_pCoreSystem->getMemorySystem()->spawn<GLFrameBufferComponent>();

	glGenFramebuffers(1, &l_FBC->m_FBO);
	glBindFramebuffer(GL_FRAMEBUFFER, l_FBC->m_FBO);

	// generate and bind renderbuffer
	glGenRenderbuffers(1, &l_FBC->m_RBO);
	glBindRenderbuffer(GL_RENDERBUFFER, l_FBC->m_RBO);

	glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT, GL_RENDERBUFFER, l_FBC->m_RBO);
	glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH24_STENCIL8, (GLsizei)GLRenderingSystemNS::g_RenderingSystemSingletonComponent->m_renderTargetSize.x, (GLsizei)GLRenderingSystemNS::g_RenderingSystemSingletonComponent->m_renderTargetSize.y);

	GeometryRenderPassSingletonComponent::getInstance().m_FBC = l_FBC;

	// generate and bind texture
	for (size_t i = 0; i < 8; i++)
	{
		auto l_TDC = g_pCoreSystem->getMemorySystem()->spawn<TextureDataComponent>();

		l_TDC->m_textureType = textureType::RENDER_BUFFER_SAMPLER;
		l_TDC->m_textureColorComponentsFormat = textureColorComponentsFormat::RGBA16F;
		l_TDC->m_texturePixelDataFormat = texturePixelDataFormat::RGBA;
		l_TDC->m_textureMinFilterMethod = textureFilterMethod::NEAREST;
		l_TDC->m_textureMagFilterMethod = textureFilterMethod::NEAREST;
		l_TDC->m_textureWrapMethod = textureWrapMethod::CLAMP_TO_EDGE;
		l_TDC->m_textureWidth = (GLsizei)GLRenderingSystemNS::g_RenderingSystemSingletonComponent->m_renderTargetSize.x;
		l_TDC->m_textureHeight = (GLsizei)GLRenderingSystemNS::g_RenderingSystemSingletonComponent->m_renderTargetSize.y;
		l_TDC->m_texturePixelDataType = texturePixelDataType::FLOAT;
		l_TDC->m_textureData = { nullptr };

		GeometryRenderPassSingletonComponent::getInstance().m_TDCs.emplace_back(l_TDC);

		auto l_GLTDC = initializeTextureDataComponent(l_TDC);

		attachTextureToFramebuffer(
			l_TDC,
			l_GLTDC,
			l_FBC,
			(int)i, 0, 0
		);

		GeometryRenderPassSingletonComponent::getInstance().m_GLTDCs.emplace_back(l_GLTDC);
	}

	std::vector<unsigned int> l_colorAttachments;
	for (unsigned int i = 0; i < 8; ++i)
	{
		l_colorAttachments.emplace_back(GL_COLOR_ATTACHMENT0 + i);
	}
	glDrawBuffers((GLsizei)l_colorAttachments.size(), &l_colorAttachments[0]);

	// finally check if framebuffer is complete
	if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
	{
		g_pCoreSystem->getLogSystem()->printLog("GLFrameBuffer: GeometryRenderPass Framebuffer is not completed!");
	}

	glBindRenderbuffer(GL_RENDERBUFFER, 0);
	glBindFramebuffer(GL_FRAMEBUFFER, 0);

	// shader programs and shaders
	auto l_GLSPC = g_pCoreSystem->getMemorySystem()->spawn<GLShaderProgramComponent>();

	l_GLSPC->m_program = glCreateProgram();

#ifdef CookTorrance
	initializeShader(
		l_GLSPC->m_program,
		GeometryRenderPassSingletonComponent::getInstance().m_geometryPassVertexShaderID,
		GL_VERTEX_SHADER,
		"GL3.3//geometryPassCookTorranceVertex.sf");
	initializeShader(
		l_GLSPC->m_program,
		GeometryRenderPassSingletonComponent::getInstance().m_geometryPassFragmentShaderID,
		GL_FRAGMENT_SHADER,
		"GL3.3//geometryPassCookTorranceFragment.sf");
#elif BlinnPhong
	initializeShader(
		l_shaderProgram->m_program,
		GeometryRenderPassSingletonComponent::getInstance().m_geometryPassVertexShaderID,
		GL_VERTEX_SHADER,
		"GL3.3//geometryPassBlinnPhongVertex.sf");
	initializeShader(
		l_shaderProgram->m_program,
		GeometryRenderPassSingletonComponent::getInstance().m_geometryPassFragmentShaderID,
		GL_FRAGMENT_SHADER,
		"GL3.3//geometryPassBlinnPhongFragment.sf");
#endif
	GeometryRenderPassSingletonComponent::getInstance().m_geometryPass_uni_p_camera_original = getUniformLocation(
		l_GLSPC->m_program,
		"uni_p_camera_original");
	GeometryRenderPassSingletonComponent::getInstance().m_geometryPass_uni_p_camera_jittered = getUniformLocation(
		l_GLSPC->m_program,
		"uni_p_camera_jittered");
	GeometryRenderPassSingletonComponent::getInstance().m_geometryPass_uni_r_camera = getUniformLocation(
		l_GLSPC->m_program,
		"uni_r_camera");
	GeometryRenderPassSingletonComponent::getInstance().m_geometryPass_uni_t_camera = getUniformLocation(
		l_GLSPC->m_program,
		"uni_t_camera");
	GeometryRenderPassSingletonComponent::getInstance().m_geometryPass_uni_m = getUniformLocation(
		l_GLSPC->m_program,
		"uni_m");
#ifdef CookTorrance
	GeometryRenderPassSingletonComponent::getInstance().m_geometryPass_uni_r_camera_prev = getUniformLocation(
		l_GLSPC->m_program,
		"uni_r_camera_prev");
	GeometryRenderPassSingletonComponent::getInstance().m_geometryPass_uni_t_camera_prev = getUniformLocation(
		l_GLSPC->m_program,
		"uni_t_camera_prev");
	GeometryRenderPassSingletonComponent::getInstance().m_geometryPass_uni_m_prev = getUniformLocation(
		l_GLSPC->m_program,
		"uni_m_prev");
	GeometryRenderPassSingletonComponent::getInstance().m_geometryPass_uni_p_light_0 = getUniformLocation(
		l_GLSPC->m_program,
		"uni_p_light_0");
	GeometryRenderPassSingletonComponent::getInstance().m_geometryPass_uni_p_light_1 = getUniformLocation(
		l_GLSPC->m_program,
		"uni_p_light_1");
	GeometryRenderPassSingletonComponent::getInstance().m_geometryPass_uni_p_light_2 = getUniformLocation(
		l_GLSPC->m_program,
		"uni_p_light_2");
	GeometryRenderPassSingletonComponent::getInstance().m_geometryPass_uni_p_light_3 = getUniformLocation(
		l_GLSPC->m_program,
		"uni_p_light_3");
	GeometryRenderPassSingletonComponent::getInstance().m_geometryPass_uni_v_light = getUniformLocation(
		l_GLSPC->m_program,
		"uni_v_light");
	GeometryRenderPassSingletonComponent::getInstance().m_geometryPass_uni_normalTexture = getUniformLocation(
		l_GLSPC->m_program,
		"uni_normalTexture");
	updateUniform(
		GeometryRenderPassSingletonComponent::getInstance().m_geometryPass_uni_normalTexture,
		0);
	GeometryRenderPassSingletonComponent::getInstance().m_geometryPass_uni_albedoTexture = getUniformLocation(
		l_GLSPC->m_program,
		"uni_albedoTexture");
	updateUniform(
		GeometryRenderPassSingletonComponent::getInstance().m_geometryPass_uni_albedoTexture,
		1);
	GeometryRenderPassSingletonComponent::getInstance().m_geometryPass_uni_metallicTexture = getUniformLocation(
		l_GLSPC->m_program,
		"uni_metallicTexture");
	updateUniform(
		GeometryRenderPassSingletonComponent::getInstance().m_geometryPass_uni_metallicTexture,
		2);
	GeometryRenderPassSingletonComponent::getInstance().m_geometryPass_uni_roughnessTexture = getUniformLocation(
		l_GLSPC->m_program,
		"uni_roughnessTexture");
	updateUniform(
		GeometryRenderPassSingletonComponent::getInstance().m_geometryPass_uni_roughnessTexture,
		3);
	GeometryRenderPassSingletonComponent::getInstance().m_geometryPass_uni_aoTexture = getUniformLocation(
		l_GLSPC->m_program,
		"uni_aoTexture");
	updateUniform(
		GeometryRenderPassSingletonComponent::getInstance().m_geometryPass_uni_aoTexture,
		4);
	GeometryRenderPassSingletonComponent::getInstance().m_geometryPass_uni_useNormalTexture = getUniformLocation(
		l_GLSPC->m_program,
		"uni_useNormalTexture");
	GeometryRenderPassSingletonComponent::getInstance().m_geometryPass_uni_useAlbedoTexture = getUniformLocation(
		l_GLSPC->m_program,
		"uni_useAlbedoTexture");
	GeometryRenderPassSingletonComponent::getInstance().m_geometryPass_uni_useMetallicTexture = getUniformLocation(
		l_GLSPC->m_program,
		"uni_useMetallicTexture");
	GeometryRenderPassSingletonComponent::getInstance().m_geometryPass_uni_useRoughnessTexture = getUniformLocation(
		l_GLSPC->m_program,
		"uni_useRoughnessTexture");
	GeometryRenderPassSingletonComponent::getInstance().m_geometryPass_uni_useAOTexture = getUniformLocation(
		l_GLSPC->m_program,
		"uni_useAOTexture");
	GeometryRenderPassSingletonComponent::getInstance().m_geometryPass_uni_albedo = getUniformLocation(
		l_GLSPC->m_program,
		"uni_albedo");
	GeometryRenderPassSingletonComponent::getInstance().m_geometryPass_uni_MRA = getUniformLocation(
		l_GLSPC->m_program,
		"uni_MRA");
#elif BlinnPhong
	// @TODO: texture uniforms
#endif

	GeometryRenderPassSingletonComponent::getInstance().m_GLSPC = l_GLSPC;
}

void GLRenderingSystemNS::initializeLightRenderPass()
{
	// generate and bind framebuffer
	auto l_FBC = g_pCoreSystem->getMemorySystem()->spawn<GLFrameBufferComponent>();

	glGenFramebuffers(1, &l_FBC->m_FBO);
	glBindFramebuffer(GL_FRAMEBUFFER, l_FBC->m_FBO);

	// generate and bind renderbuffer
	glGenRenderbuffers(1, &l_FBC->m_RBO);
	glBindRenderbuffer(GL_RENDERBUFFER, l_FBC->m_RBO);

	glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT, GL_RENDERBUFFER, l_FBC->m_RBO);
	glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH24_STENCIL8, (GLsizei)GLRenderingSystemNS::g_RenderingSystemSingletonComponent->m_renderTargetSize.x, (GLsizei)GLRenderingSystemNS::g_RenderingSystemSingletonComponent->m_renderTargetSize.y);

	LightRenderPassSingletonComponent::getInstance().m_FBC = l_FBC;

	// generate and bind texture
	auto l_TDC = g_pCoreSystem->getMemorySystem()->spawn<TextureDataComponent>();

	l_TDC->m_textureType = textureType::RENDER_BUFFER_SAMPLER;
	l_TDC->m_textureColorComponentsFormat = textureColorComponentsFormat::RGBA16F;
	l_TDC->m_texturePixelDataFormat = texturePixelDataFormat::RGBA;
	l_TDC->m_textureMinFilterMethod = textureFilterMethod::NEAREST;
	l_TDC->m_textureMagFilterMethod = textureFilterMethod::NEAREST;
	l_TDC->m_textureWrapMethod = textureWrapMethod::CLAMP_TO_EDGE;
	l_TDC->m_textureWidth = (GLsizei)GLRenderingSystemNS::g_RenderingSystemSingletonComponent->m_renderTargetSize.x;
	l_TDC->m_textureHeight = (GLsizei)GLRenderingSystemNS::g_RenderingSystemSingletonComponent->m_renderTargetSize.y;
	l_TDC->m_texturePixelDataType = texturePixelDataType::FLOAT;
	l_TDC->m_textureData = { nullptr };

	LightRenderPassSingletonComponent::getInstance().m_TDC = l_TDC;

	auto l_GLTDC = initializeTextureDataComponent(l_TDC);

	attachTextureToFramebuffer(
		l_TDC,
		l_GLTDC,
		l_FBC,
		0, 0, 0
	);

	LightRenderPassSingletonComponent::getInstance().m_GLTDC = l_GLTDC;

	std::vector<unsigned int> l_colorAttachments;
	l_colorAttachments.emplace_back(GL_COLOR_ATTACHMENT0);
	glDrawBuffers((GLsizei)l_colorAttachments.size(), &l_colorAttachments[0]);

	// finally check if framebuffer is complete
	if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
	{
		g_pCoreSystem->getLogSystem()->printLog("GLFrameBuffer: LightRenderPass Framebuffer is not completed!");
	}

	glBindRenderbuffer(GL_RENDERBUFFER, 0);
	glBindFramebuffer(GL_FRAMEBUFFER, 0);

	// shader programs and shaders
	auto l_GLSPC = g_pCoreSystem->getMemorySystem()->spawn<GLShaderProgramComponent>();

	l_GLSPC->m_program = glCreateProgram();

#ifdef CookTorrance
	initializeShader(
		l_GLSPC->m_program,
		LightRenderPassSingletonComponent::getInstance().m_lightPassVertexShaderID,
		GL_VERTEX_SHADER,
		"GL3.3//lightPassCookTorranceVertex.sf");
	initializeShader(
		l_GLSPC->m_program,
		LightRenderPassSingletonComponent::getInstance().m_lightPassFragmentShaderID,
		GL_FRAGMENT_SHADER,
		"GL3.3//lightPassCookTorranceFragment.sf");
#elif BlinnPhong
	initializeShader(
		l_GLSPC->m_program,
		LightRenderPassSingletonComponent::getInstance().m_lightPassVertexShaderID,
		GL_VERTEX_SHADER,
		"GL3.3//lightPassBlinnPhongVertex.sf");
	initializeShader(
		l_GLSPC->m_program,
		LightRenderPassSingletonComponent::getInstance().m_lightPassFragmentShaderID,
		GL_FRAGMENT_SHADER,
		"GL3.3//lightPassBlinnPhongFragment.sf");
#endif
	LightRenderPassSingletonComponent::getInstance().m_uni_geometryPassRT0 = getUniformLocation(
		l_GLSPC->m_program,
		"uni_geometryPassRT0");
	updateUniform(
		LightRenderPassSingletonComponent::getInstance().m_uni_geometryPassRT0,
		0);
	LightRenderPassSingletonComponent::getInstance().m_uni_geometryPassRT1 = getUniformLocation(
		l_GLSPC->m_program,
		"uni_geometryPassRT1");
	updateUniform(
		LightRenderPassSingletonComponent::getInstance().m_uni_geometryPassRT1,
		1);
	LightRenderPassSingletonComponent::getInstance().m_uni_geometryPassRT2 = getUniformLocation(
		l_GLSPC->m_program,
		"uni_geometryPassRT2");
	updateUniform(
		LightRenderPassSingletonComponent::getInstance().m_uni_geometryPassRT2,
		2);
	LightRenderPassSingletonComponent::getInstance().m_uni_geometryPassRT4 = getUniformLocation(
		l_GLSPC->m_program,
		"uni_geometryPassRT4");
	updateUniform(
		LightRenderPassSingletonComponent::getInstance().m_uni_geometryPassRT4,
		3);
	LightRenderPassSingletonComponent::getInstance().m_uni_geometryPassRT5 = getUniformLocation(
		l_GLSPC->m_program,
		"uni_geometryPassRT5");
	updateUniform(
		LightRenderPassSingletonComponent::getInstance().m_uni_geometryPassRT5,
		4);
	LightRenderPassSingletonComponent::getInstance().m_uni_geometryPassRT6 = getUniformLocation(
		l_GLSPC->m_program,
		"uni_geometryPassRT6");
	updateUniform(
		LightRenderPassSingletonComponent::getInstance().m_uni_geometryPassRT6,
		5);
	LightRenderPassSingletonComponent::getInstance().m_uni_geometryPassRT7 = getUniformLocation(
		l_GLSPC->m_program,
		"uni_geometryPassRT7");
	updateUniform(
		LightRenderPassSingletonComponent::getInstance().m_uni_geometryPassRT7,
		6);
	LightRenderPassSingletonComponent::getInstance().m_uni_shadowMap_0 = getUniformLocation(
		l_GLSPC->m_program,
		"uni_shadowMap_0");
	updateUniform(
		LightRenderPassSingletonComponent::getInstance().m_uni_shadowMap_0,
		7);
	LightRenderPassSingletonComponent::getInstance().m_uni_shadowMap_1 = getUniformLocation(
		l_GLSPC->m_program,
		"uni_shadowMap_1");
	updateUniform(
		LightRenderPassSingletonComponent::getInstance().m_uni_shadowMap_1,
		8);
	LightRenderPassSingletonComponent::getInstance().m_uni_shadowMap_2 = getUniformLocation(
		l_GLSPC->m_program,
		"uni_shadowMap_2");
	updateUniform(
		LightRenderPassSingletonComponent::getInstance().m_uni_shadowMap_2,
		9);
	LightRenderPassSingletonComponent::getInstance().m_uni_shadowMap_3 = getUniformLocation(
		l_GLSPC->m_program,
		"uni_shadowMap_3");
	updateUniform(
		LightRenderPassSingletonComponent::getInstance().m_uni_shadowMap_3,
		10);
	for (size_t i = 0; i < 4; i++)
	{
		std::stringstream ss;
		ss << i;
		LightRenderPassSingletonComponent::getInstance().m_uni_shadowSplitAreas.emplace_back(
			getUniformLocation(l_GLSPC->m_program, "uni_shadowSplitAreas[" + ss.str() + "]")
		);
	}
	LightRenderPassSingletonComponent::getInstance().m_uni_irradianceMap = getUniformLocation(
		l_GLSPC->m_program,
		"uni_irradianceMap");
	updateUniform(
		LightRenderPassSingletonComponent::getInstance().m_uni_irradianceMap,
		11);
	LightRenderPassSingletonComponent::getInstance().m_uni_preFiltedMap = getUniformLocation(
		l_GLSPC->m_program,
		"uni_preFiltedMap");
	updateUniform(
		LightRenderPassSingletonComponent::getInstance().m_uni_preFiltedMap,
		12);
	LightRenderPassSingletonComponent::getInstance().m_uni_brdfLUT = getUniformLocation(
		l_GLSPC->m_program,
		"uni_brdfLUT");
	updateUniform(
		LightRenderPassSingletonComponent::getInstance().m_uni_brdfLUT,
		13);
	LightRenderPassSingletonComponent::getInstance().m_uni_viewPos = getUniformLocation(
		l_GLSPC->m_program,
		"uni_viewPos");
	LightRenderPassSingletonComponent::getInstance().m_uni_dirLight_position = getUniformLocation(
		l_GLSPC->m_program,
		"uni_dirLight.position");
	LightRenderPassSingletonComponent::getInstance().m_uni_dirLight_direction = getUniformLocation(
		l_GLSPC->m_program,
		"uni_dirLight.direction");
	LightRenderPassSingletonComponent::getInstance().m_uni_dirLight_color = getUniformLocation(
		l_GLSPC->m_program,
		"uni_dirLight.color");
	int l_pointLightIndexOffset = 0;
	for (auto i = (unsigned int)0; i < GLRenderingSystemNS::g_GameSystemSingletonComponent->m_LightComponents.size(); i++)
	{
		if (GLRenderingSystemNS::g_GameSystemSingletonComponent->m_LightComponents[i]->m_lightType == lightType::DIRECTIONAL)
		{
			l_pointLightIndexOffset -= 1;
		}
		if (GLRenderingSystemNS::g_GameSystemSingletonComponent->m_LightComponents[i]->m_lightType == lightType::POINT)
		{
			std::stringstream ss;
			ss << i + l_pointLightIndexOffset;
			LightRenderPassSingletonComponent::getInstance().m_uni_pointLights_position.emplace_back(
				getUniformLocation(l_GLSPC->m_program, "uni_pointLights[" + ss.str() + "].position")
			);
			LightRenderPassSingletonComponent::getInstance().m_uni_pointLights_radius.emplace_back(
				getUniformLocation(l_GLSPC->m_program, "uni_pointLights[" + ss.str() + "].radius")
			);
			LightRenderPassSingletonComponent::getInstance().m_uni_pointLights_color.emplace_back(
				getUniformLocation(l_GLSPC->m_program, "uni_pointLights[" + ss.str() + "].color")
			);
		}
	}
	LightRenderPassSingletonComponent::getInstance().m_uni_isEmissive = getUniformLocation(
		l_GLSPC->m_program,
		"uni_isEmissive");

	LightRenderPassSingletonComponent::getInstance().m_GLSPC = l_GLSPC;
}

void GLRenderingSystemNS::initializeFinalRenderPass()
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
	// generate and bind framebuffer
	auto l_FBC = g_pCoreSystem->getMemorySystem()->spawn<GLFrameBufferComponent>();

	glGenFramebuffers(1, &l_FBC->m_FBO);
	glBindFramebuffer(GL_FRAMEBUFFER, l_FBC->m_FBO);

	// generate and bind renderbuffer
	glGenRenderbuffers(1, &l_FBC->m_RBO);
	glBindRenderbuffer(GL_RENDERBUFFER, l_FBC->m_RBO);

	glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT, GL_RENDERBUFFER, l_FBC->m_RBO);
	glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH24_STENCIL8, (GLsizei)GLRenderingSystemNS::g_RenderingSystemSingletonComponent->m_renderTargetSize.x, (GLsizei)GLRenderingSystemNS::g_RenderingSystemSingletonComponent->m_renderTargetSize.y);

	GLFinalRenderPassSingletonComponent::getInstance().m_skyPassFBC = l_FBC;

	// generate and bind texture
	auto l_TDC = g_pCoreSystem->getMemorySystem()->spawn<TextureDataComponent>();

	l_TDC->m_textureType = textureType::RENDER_BUFFER_SAMPLER;
	l_TDC->m_textureColorComponentsFormat = textureColorComponentsFormat::RGBA16F;
	l_TDC->m_texturePixelDataFormat = texturePixelDataFormat::RGBA;
	l_TDC->m_textureMinFilterMethod = textureFilterMethod::NEAREST;
	l_TDC->m_textureMagFilterMethod = textureFilterMethod::NEAREST;
	l_TDC->m_textureWrapMethod = textureWrapMethod::CLAMP_TO_EDGE;
	l_TDC->m_textureWidth = (GLsizei)GLRenderingSystemNS::g_RenderingSystemSingletonComponent->m_renderTargetSize.x;
	l_TDC->m_textureHeight = (GLsizei)GLRenderingSystemNS::g_RenderingSystemSingletonComponent->m_renderTargetSize.y;
	l_TDC->m_texturePixelDataType = texturePixelDataType::FLOAT;
	l_TDC->m_textureData = { nullptr };

	GLFinalRenderPassSingletonComponent::getInstance().m_skyPassTDC = l_TDC;

	auto l_GLTDC = initializeTextureDataComponent(l_TDC);

	attachTextureToFramebuffer(
		l_TDC,
		l_GLTDC,
		l_FBC,
		0, 0, 0
	);

	GLFinalRenderPassSingletonComponent::getInstance().m_skyPassGLTDC = l_GLTDC;

	std::vector<unsigned int> l_colorAttachments;
	l_colorAttachments.emplace_back(GL_COLOR_ATTACHMENT0);
	glDrawBuffers((GLsizei)l_colorAttachments.size(), &l_colorAttachments[0]);

	// finally check if framebuffer is complete
	if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
	{
		g_pCoreSystem->getLogSystem()->printLog("GLFrameBuffer: SkyRenderPass Framebuffer is not completed!");
	}

	glBindRenderbuffer(GL_RENDERBUFFER, 0);
	glBindFramebuffer(GL_FRAMEBUFFER, 0);

	// shader programs and shaders
	auto l_GLSPC = g_pCoreSystem->getMemorySystem()->spawn<GLShaderProgramComponent>();

	l_GLSPC->m_program = glCreateProgram();

	initializeShader(
		l_GLSPC->m_program,
		GLFinalRenderPassSingletonComponent::getInstance().m_skyPassVertexShaderID,
		GL_VERTEX_SHADER,
		"GL3.3//skyPassVertex.sf");
	initializeShader(
		l_GLSPC->m_program,
		GLFinalRenderPassSingletonComponent::getInstance().m_skyPassFragmentShaderID,
		GL_FRAGMENT_SHADER,
		"GL3.3//skyPassFragment.sf");
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
	//Ping pass
	// generate and bind framebuffer
	auto l_FBC = g_pCoreSystem->getMemorySystem()->spawn<GLFrameBufferComponent>();

	glGenFramebuffers(1, &l_FBC->m_FBO);
	glBindFramebuffer(GL_FRAMEBUFFER, l_FBC->m_FBO);

	// generate and bind renderbuffer
	glGenRenderbuffers(1, &l_FBC->m_RBO);
	glBindRenderbuffer(GL_RENDERBUFFER, l_FBC->m_RBO);

	glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT, GL_RENDERBUFFER, l_FBC->m_RBO);
	glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH24_STENCIL8, (GLsizei)GLRenderingSystemNS::g_RenderingSystemSingletonComponent->m_renderTargetSize.x, (GLsizei)GLRenderingSystemNS::g_RenderingSystemSingletonComponent->m_renderTargetSize.y);

	GLFinalRenderPassSingletonComponent::getInstance().m_TAAPingPassFBC = l_FBC;

	// generate and bind texture
	auto l_TDC = g_pCoreSystem->getMemorySystem()->spawn<TextureDataComponent>();

	l_TDC->m_textureType = textureType::RENDER_BUFFER_SAMPLER;
	l_TDC->m_textureColorComponentsFormat = textureColorComponentsFormat::RGBA16F;
	l_TDC->m_texturePixelDataFormat = texturePixelDataFormat::RGBA;
	l_TDC->m_textureMinFilterMethod = textureFilterMethod::NEAREST;
	l_TDC->m_textureMagFilterMethod = textureFilterMethod::NEAREST;
	l_TDC->m_textureWrapMethod = textureWrapMethod::CLAMP_TO_EDGE;
	l_TDC->m_textureWidth = (GLsizei)GLRenderingSystemNS::g_RenderingSystemSingletonComponent->m_renderTargetSize.x;
	l_TDC->m_textureHeight = (GLsizei)GLRenderingSystemNS::g_RenderingSystemSingletonComponent->m_renderTargetSize.y;
	l_TDC->m_texturePixelDataType = texturePixelDataType::FLOAT;
	l_TDC->m_textureData = { nullptr };

	GLFinalRenderPassSingletonComponent::getInstance().m_TAAPingPassTDC = l_TDC;

	auto l_GLTDC = initializeTextureDataComponent(l_TDC);

	attachTextureToFramebuffer(
		l_TDC,
		l_GLTDC,
		l_FBC,
		0, 0, 0
	);

	GLFinalRenderPassSingletonComponent::getInstance().m_TAAPingPassGLTDC = l_GLTDC;

	std::vector<unsigned int> l_pingPassColorAttachments;
	l_pingPassColorAttachments.emplace_back(GL_COLOR_ATTACHMENT0);
	glDrawBuffers((GLsizei)l_pingPassColorAttachments.size(), &l_pingPassColorAttachments[0]);

	// finally check if framebuffer is complete
	if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
	{
		g_pCoreSystem->getLogSystem()->printLog("GLFrameBuffer: TAAPingRenderPass Framebuffer is not completed!");
	}

	//Pong pass
	// generate and bind framebuffer
	l_FBC = g_pCoreSystem->getMemorySystem()->spawn<GLFrameBufferComponent>();

	glGenFramebuffers(1, &l_FBC->m_FBO);
	glBindFramebuffer(GL_FRAMEBUFFER, l_FBC->m_FBO);

	// generate and bind renderbuffer
	glGenRenderbuffers(1, &l_FBC->m_RBO);
	glBindRenderbuffer(GL_RENDERBUFFER, l_FBC->m_RBO);

	glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT, GL_RENDERBUFFER, l_FBC->m_RBO);
	glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH24_STENCIL8, (GLsizei)GLRenderingSystemNS::g_RenderingSystemSingletonComponent->m_renderTargetSize.x, (GLsizei)GLRenderingSystemNS::g_RenderingSystemSingletonComponent->m_renderTargetSize.y);

	GLFinalRenderPassSingletonComponent::getInstance().m_TAAPongPassFBC = l_FBC;

	// generate and bind texture
	l_TDC = g_pCoreSystem->getMemorySystem()->spawn<TextureDataComponent>();

	l_TDC->m_textureType = textureType::RENDER_BUFFER_SAMPLER;
	l_TDC->m_textureColorComponentsFormat = textureColorComponentsFormat::RGBA16F;
	l_TDC->m_texturePixelDataFormat = texturePixelDataFormat::RGBA;
	l_TDC->m_textureMinFilterMethod = textureFilterMethod::NEAREST;
	l_TDC->m_textureMagFilterMethod = textureFilterMethod::NEAREST;
	l_TDC->m_textureWrapMethod = textureWrapMethod::CLAMP_TO_EDGE;
	l_TDC->m_textureWidth = (GLsizei)GLRenderingSystemNS::g_RenderingSystemSingletonComponent->m_renderTargetSize.x;
	l_TDC->m_textureHeight = (GLsizei)GLRenderingSystemNS::g_RenderingSystemSingletonComponent->m_renderTargetSize.y;
	l_TDC->m_texturePixelDataType = texturePixelDataType::FLOAT;
	l_TDC->m_textureData = { nullptr };

	GLFinalRenderPassSingletonComponent::getInstance().m_TAAPongPassTDC = l_TDC;

	l_GLTDC = initializeTextureDataComponent(l_TDC);

	attachTextureToFramebuffer(
		l_TDC,
		l_GLTDC,
		l_FBC,
		0, 0, 0
	);

	GLFinalRenderPassSingletonComponent::getInstance().m_TAAPongPassGLTDC = l_GLTDC;

	std::vector<unsigned int> l_pongPassColorAttachments;
	l_pongPassColorAttachments.emplace_back(GL_COLOR_ATTACHMENT0);
	glDrawBuffers((GLsizei)l_pongPassColorAttachments.size(), &l_pongPassColorAttachments[0]);

	// finally check if framebuffer is complete
	if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
	{
		g_pCoreSystem->getLogSystem()->printLog("GLFrameBuffer: TAAPongRenderPass Framebuffer is not completed!");
	}

	glBindRenderbuffer(GL_RENDERBUFFER, 0);
	glBindFramebuffer(GL_FRAMEBUFFER, 0);

	// shader programs and shaders
	auto l_GLSPC = g_pCoreSystem->getMemorySystem()->spawn<GLShaderProgramComponent>();

	l_GLSPC->m_program = glCreateProgram();

	initializeShader(
		l_GLSPC->m_program,
		GLFinalRenderPassSingletonComponent::getInstance().m_TAAPassVertexShaderID,
		GL_VERTEX_SHADER,
		"GL3.3//TAAPassVertex.sf");
	initializeShader(
		l_GLSPC->m_program,
		GLFinalRenderPassSingletonComponent::getInstance().m_TAAPassFragmentShaderID,
		GL_FRAGMENT_SHADER,
		"GL3.3//TAAPassFragment.sf");
	GLFinalRenderPassSingletonComponent::getInstance().m_TAAPass_uni_lightPassRT0 = getUniformLocation(
		l_GLSPC->m_program,
		"uni_lightPassRT0");
	updateUniform(
		GLFinalRenderPassSingletonComponent::getInstance().m_TAAPass_uni_lightPassRT0,
		0);
	GLFinalRenderPassSingletonComponent::getInstance().m_TAAPass_uni_lastTAAPassRT0 = getUniformLocation(
		l_GLSPC->m_program,
		"uni_lastTAAPassRT0");
	updateUniform(
		GLFinalRenderPassSingletonComponent::getInstance().m_TAAPass_uni_lastTAAPassRT0,
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
}

void GLRenderingSystemNS::initializeBloomExtractPass()
{
	// generate and bind framebuffer
	auto l_FBC = g_pCoreSystem->getMemorySystem()->spawn<GLFrameBufferComponent>();

	glGenFramebuffers(1, &l_FBC->m_FBO);
	glBindFramebuffer(GL_FRAMEBUFFER, l_FBC->m_FBO);

	// generate and bind renderbuffer
	glGenRenderbuffers(1, &l_FBC->m_RBO);
	glBindRenderbuffer(GL_RENDERBUFFER, l_FBC->m_RBO);

	glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT, GL_RENDERBUFFER, l_FBC->m_RBO);
	glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH24_STENCIL8, (GLsizei)GLRenderingSystemNS::g_RenderingSystemSingletonComponent->m_renderTargetSize.x, (GLsizei)GLRenderingSystemNS::g_RenderingSystemSingletonComponent->m_renderTargetSize.y);

	GLFinalRenderPassSingletonComponent::getInstance().m_bloomExtractPassFBC = l_FBC;

	// generate and bind texture
	auto l_TDC = g_pCoreSystem->getMemorySystem()->spawn<TextureDataComponent>();

	l_TDC->m_textureType = textureType::RENDER_BUFFER_SAMPLER;
	l_TDC->m_textureColorComponentsFormat = textureColorComponentsFormat::RGBA16F;
	l_TDC->m_texturePixelDataFormat = texturePixelDataFormat::RGBA;
	l_TDC->m_textureMinFilterMethod = textureFilterMethod::NEAREST;
	l_TDC->m_textureMagFilterMethod = textureFilterMethod::NEAREST;
	l_TDC->m_textureWrapMethod = textureWrapMethod::CLAMP_TO_EDGE;
	l_TDC->m_textureWidth = (GLsizei)GLRenderingSystemNS::g_RenderingSystemSingletonComponent->m_renderTargetSize.x;
	l_TDC->m_textureHeight = (GLsizei)GLRenderingSystemNS::g_RenderingSystemSingletonComponent->m_renderTargetSize.y;
	l_TDC->m_texturePixelDataType = texturePixelDataType::FLOAT;
	l_TDC->m_textureData = { nullptr };

	GLFinalRenderPassSingletonComponent::getInstance().m_bloomExtractPassTDC = l_TDC;

	auto l_GLTDC = initializeTextureDataComponent(l_TDC);

	attachTextureToFramebuffer(
		l_TDC,
		l_GLTDC,
		l_FBC,
		0, 0, 0
	);

	GLFinalRenderPassSingletonComponent::getInstance().m_bloomExtractPassGLTDC = l_GLTDC;

	std::vector<unsigned int> l_colorAttachments;
	l_colorAttachments.emplace_back(GL_COLOR_ATTACHMENT0);
	glDrawBuffers((GLsizei)l_colorAttachments.size(), &l_colorAttachments[0]);

	// finally check if framebuffer is complete
	if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
	{
		g_pCoreSystem->getLogSystem()->printLog("GLFrameBuffer: BloomExtractRenderPass Framebuffer is not completed!");
	}

	glBindRenderbuffer(GL_RENDERBUFFER, 0);
	glBindFramebuffer(GL_FRAMEBUFFER, 0);

	// shader programs and shaders
	auto l_GLSPC = g_pCoreSystem->getMemorySystem()->spawn<GLShaderProgramComponent>();

	l_GLSPC->m_program = glCreateProgram();

	initializeShader(
		l_GLSPC->m_program,
		GLFinalRenderPassSingletonComponent::getInstance().m_bloomExtractPassVertexShaderID,
		GL_VERTEX_SHADER,
		"GL3.3//bloomExtractPassVertex.sf");
	initializeShader(
		l_GLSPC->m_program,
		GLFinalRenderPassSingletonComponent::getInstance().m_bloomExtractPassFragmentShaderID,
		GL_FRAGMENT_SHADER,
		"GL3.3//bloomExtractPassFragment.sf");
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
	// generate and bind framebuffer
	auto l_FBC = g_pCoreSystem->getMemorySystem()->spawn<GLFrameBufferComponent>();

	glGenFramebuffers(1, &l_FBC->m_FBO);
	glBindFramebuffer(GL_FRAMEBUFFER, l_FBC->m_FBO);

	// generate and bind renderbuffer
	glGenRenderbuffers(1, &l_FBC->m_RBO);
	glBindRenderbuffer(GL_RENDERBUFFER, l_FBC->m_RBO);

	glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT, GL_RENDERBUFFER, l_FBC->m_RBO);
	glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH24_STENCIL8, (GLsizei)GLRenderingSystemNS::g_RenderingSystemSingletonComponent->m_renderTargetSize.x, (GLsizei)GLRenderingSystemNS::g_RenderingSystemSingletonComponent->m_renderTargetSize.y);

	GLFinalRenderPassSingletonComponent::getInstance().m_bloomBlurPingPassFBC = l_FBC;

	// generate and bind texture
	auto l_TDC = g_pCoreSystem->getMemorySystem()->spawn<TextureDataComponent>();

	l_TDC->m_textureType = textureType::RENDER_BUFFER_SAMPLER;
	l_TDC->m_textureColorComponentsFormat = textureColorComponentsFormat::RGBA16F;
	l_TDC->m_texturePixelDataFormat = texturePixelDataFormat::RGBA;
	l_TDC->m_textureMinFilterMethod = textureFilterMethod::NEAREST;
	l_TDC->m_textureMagFilterMethod = textureFilterMethod::NEAREST;
	l_TDC->m_textureWrapMethod = textureWrapMethod::CLAMP_TO_EDGE;
	l_TDC->m_textureWidth = (GLsizei)GLRenderingSystemNS::g_RenderingSystemSingletonComponent->m_renderTargetSize.x;
	l_TDC->m_textureHeight = (GLsizei)GLRenderingSystemNS::g_RenderingSystemSingletonComponent->m_renderTargetSize.y;
	l_TDC->m_texturePixelDataType = texturePixelDataType::FLOAT;
	l_TDC->m_textureData = { nullptr };

	GLFinalRenderPassSingletonComponent::getInstance().m_bloomBlurPingPassTDC = l_TDC;

	auto l_GLTDC = initializeTextureDataComponent(l_TDC);

	attachTextureToFramebuffer(
		l_TDC,
		l_GLTDC,
		l_FBC,
		0, 0, 0
	);

	GLFinalRenderPassSingletonComponent::getInstance().m_bloomBlurPingPassGLTDC = l_GLTDC;

	std::vector<unsigned int> l_pingPassColorAttachments;
	l_pingPassColorAttachments.emplace_back(GL_COLOR_ATTACHMENT0);
	glDrawBuffers((GLsizei)l_pingPassColorAttachments.size(), &l_pingPassColorAttachments[0]);

	// finally check if framebuffer is complete
	if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
	{
		g_pCoreSystem->getLogSystem()->printLog("GLFrameBuffer: BloomBlurPingRenderPass Framebuffer is not completed!");
	}

	//Pong pass
	// generate and bind framebuffer
	l_FBC = g_pCoreSystem->getMemorySystem()->spawn<GLFrameBufferComponent>();

	glGenFramebuffers(1, &l_FBC->m_FBO);
	glBindFramebuffer(GL_FRAMEBUFFER, l_FBC->m_FBO);

	// generate and bind renderbuffer
	glGenRenderbuffers(1, &l_FBC->m_RBO);
	glBindRenderbuffer(GL_RENDERBUFFER, l_FBC->m_RBO);

	glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT, GL_RENDERBUFFER, l_FBC->m_RBO);
	glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH24_STENCIL8, (GLsizei)GLRenderingSystemNS::g_RenderingSystemSingletonComponent->m_renderTargetSize.x, (GLsizei)GLRenderingSystemNS::g_RenderingSystemSingletonComponent->m_renderTargetSize.y);

	GLFinalRenderPassSingletonComponent::getInstance().m_bloomBlurPongPassFBC = l_FBC;

	// generate and bind texture
	l_TDC = g_pCoreSystem->getMemorySystem()->spawn<TextureDataComponent>();

	l_TDC->m_textureType = textureType::RENDER_BUFFER_SAMPLER;
	l_TDC->m_textureColorComponentsFormat = textureColorComponentsFormat::RGBA16F;
	l_TDC->m_texturePixelDataFormat = texturePixelDataFormat::RGBA;
	l_TDC->m_textureMinFilterMethod = textureFilterMethod::NEAREST;
	l_TDC->m_textureMagFilterMethod = textureFilterMethod::NEAREST;
	l_TDC->m_textureWrapMethod = textureWrapMethod::CLAMP_TO_EDGE;
	l_TDC->m_textureWidth = (GLsizei)GLRenderingSystemNS::g_RenderingSystemSingletonComponent->m_renderTargetSize.x;
	l_TDC->m_textureHeight = (GLsizei)GLRenderingSystemNS::g_RenderingSystemSingletonComponent->m_renderTargetSize.y;
	l_TDC->m_texturePixelDataType = texturePixelDataType::FLOAT;
	l_TDC->m_textureData = { nullptr };

	GLFinalRenderPassSingletonComponent::getInstance().m_bloomBlurPongPassTDC = l_TDC;

	l_GLTDC = initializeTextureDataComponent(l_TDC);

	attachTextureToFramebuffer(
		l_TDC,
		l_GLTDC,
		l_FBC,
		0, 0, 0
	);

	GLFinalRenderPassSingletonComponent::getInstance().m_bloomBlurPongPassGLTDC = l_GLTDC;

	std::vector<unsigned int> l_pongPassColorAttachments;
	l_pongPassColorAttachments.emplace_back(GL_COLOR_ATTACHMENT0);
	glDrawBuffers((GLsizei)l_pongPassColorAttachments.size(), &l_pongPassColorAttachments[0]);

	// finally check if framebuffer is complete
	if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
	{
		g_pCoreSystem->getLogSystem()->printLog("GLFrameBuffer: BloomBlurPongRenderPass Framebuffer is not completed!");
	}

	glBindRenderbuffer(GL_RENDERBUFFER, 0);
	glBindFramebuffer(GL_FRAMEBUFFER, 0);

	// shader programs and shaders
	auto l_GLSPC = g_pCoreSystem->getMemorySystem()->spawn<GLShaderProgramComponent>();

	l_GLSPC->m_program = glCreateProgram();

	initializeShader(
		l_GLSPC->m_program,
		GLFinalRenderPassSingletonComponent::getInstance().m_bloomBlurPassVertexShaderID,
		GL_VERTEX_SHADER,
		"GL3.3//bloomBlurPassVertex.sf");
	initializeShader(
		l_GLSPC->m_program,
		GLFinalRenderPassSingletonComponent::getInstance().m_bloomBlurPassFragmentShaderID,
		GL_FRAGMENT_SHADER,
		"GL3.3//bloomBlurPassFragment.sf");
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
	// generate and bind framebuffer
	auto l_FBC = g_pCoreSystem->getMemorySystem()->spawn<GLFrameBufferComponent>();

	glGenFramebuffers(1, &l_FBC->m_FBO);
	glBindFramebuffer(GL_FRAMEBUFFER, l_FBC->m_FBO);

	// generate and bind renderbuffer
	glGenRenderbuffers(1, &l_FBC->m_RBO);
	glBindRenderbuffer(GL_RENDERBUFFER, l_FBC->m_RBO);

	glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT, GL_RENDERBUFFER, l_FBC->m_RBO);
	glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH24_STENCIL8, (GLsizei)GLRenderingSystemNS::g_RenderingSystemSingletonComponent->m_renderTargetSize.x, (GLsizei)GLRenderingSystemNS::g_RenderingSystemSingletonComponent->m_renderTargetSize.y);

	GLFinalRenderPassSingletonComponent::getInstance().m_motionBlurPassFBC = l_FBC;

	// generate and bind texture
	auto l_TDC = g_pCoreSystem->getMemorySystem()->spawn<TextureDataComponent>();

	l_TDC->m_textureType = textureType::RENDER_BUFFER_SAMPLER;
	l_TDC->m_textureColorComponentsFormat = textureColorComponentsFormat::RGBA16F;
	l_TDC->m_texturePixelDataFormat = texturePixelDataFormat::RGBA;
	l_TDC->m_textureMinFilterMethod = textureFilterMethod::NEAREST;
	l_TDC->m_textureMagFilterMethod = textureFilterMethod::NEAREST;
	l_TDC->m_textureWrapMethod = textureWrapMethod::CLAMP_TO_EDGE;
	l_TDC->m_textureWidth = (GLsizei)GLRenderingSystemNS::g_RenderingSystemSingletonComponent->m_renderTargetSize.x;
	l_TDC->m_textureHeight = (GLsizei)GLRenderingSystemNS::g_RenderingSystemSingletonComponent->m_renderTargetSize.y;
	l_TDC->m_texturePixelDataType = texturePixelDataType::FLOAT;
	l_TDC->m_textureData = { nullptr };

	GLFinalRenderPassSingletonComponent::getInstance().m_motionBlurPassTDC = l_TDC;

	auto l_GLTDC = initializeTextureDataComponent(l_TDC);

	attachTextureToFramebuffer(
		l_TDC,
		l_GLTDC,
		l_FBC,
		0, 0, 0
	);

	GLFinalRenderPassSingletonComponent::getInstance().m_motionBlurPassGLTDC = l_GLTDC;

	std::vector<unsigned int> l_colorAttachments;
	l_colorAttachments.emplace_back(GL_COLOR_ATTACHMENT0);
	glDrawBuffers((GLsizei)l_colorAttachments.size(), &l_colorAttachments[0]);

	// finally check if framebuffer is complete
	if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
	{
		g_pCoreSystem->getLogSystem()->printLog("GLFrameBuffer: MotionBlurRenderPass Framebuffer is not completed!");
	}

	glBindRenderbuffer(GL_RENDERBUFFER, 0);
	glBindFramebuffer(GL_FRAMEBUFFER, 0);

	// shader programs and shaders
	auto l_GLSPC = g_pCoreSystem->getMemorySystem()->spawn<GLShaderProgramComponent>();

	l_GLSPC->m_program = glCreateProgram();

	initializeShader(
		l_GLSPC->m_program,
		GLFinalRenderPassSingletonComponent::getInstance().m_motionBlurPassVertexShaderID,
		GL_VERTEX_SHADER,
		"GL3.3//motionBlurPassVertex.sf");
	initializeShader(
		l_GLSPC->m_program,
		GLFinalRenderPassSingletonComponent::getInstance().m_motionBlurPassFragmentShaderID,
		GL_FRAGMENT_SHADER,
		"GL3.3//motionBlurPassFragment.sf");
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
	// generate and bind framebuffer
	auto l_FBC = g_pCoreSystem->getMemorySystem()->spawn<GLFrameBufferComponent>();

	glGenFramebuffers(1, &l_FBC->m_FBO);
	glBindFramebuffer(GL_FRAMEBUFFER, l_FBC->m_FBO);

	// generate and bind renderbuffer
	glGenRenderbuffers(1, &l_FBC->m_RBO);
	glBindRenderbuffer(GL_RENDERBUFFER, l_FBC->m_RBO);

	glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT, GL_RENDERBUFFER, l_FBC->m_RBO);
	glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH24_STENCIL8, (GLsizei)GLRenderingSystemNS::g_RenderingSystemSingletonComponent->m_renderTargetSize.x, (GLsizei)GLRenderingSystemNS::g_RenderingSystemSingletonComponent->m_renderTargetSize.y);

	GLFinalRenderPassSingletonComponent::getInstance().m_billboardPassFBC = l_FBC;

	// generate and bind texture
	auto l_TDC = g_pCoreSystem->getMemorySystem()->spawn<TextureDataComponent>();

	l_TDC->m_textureType = textureType::RENDER_BUFFER_SAMPLER;
	l_TDC->m_textureColorComponentsFormat = textureColorComponentsFormat::RGBA16F;
	l_TDC->m_texturePixelDataFormat = texturePixelDataFormat::RGBA;
	l_TDC->m_textureMinFilterMethod = textureFilterMethod::NEAREST;
	l_TDC->m_textureMagFilterMethod = textureFilterMethod::NEAREST;
	l_TDC->m_textureWrapMethod = textureWrapMethod::CLAMP_TO_EDGE;
	l_TDC->m_textureWidth = (GLsizei)GLRenderingSystemNS::g_RenderingSystemSingletonComponent->m_renderTargetSize.x;
	l_TDC->m_textureHeight = (GLsizei)GLRenderingSystemNS::g_RenderingSystemSingletonComponent->m_renderTargetSize.y;
	l_TDC->m_texturePixelDataType = texturePixelDataType::FLOAT;
	l_TDC->m_textureData = { nullptr };

	GLFinalRenderPassSingletonComponent::getInstance().m_billboardPassTDC = l_TDC;

	auto l_GLTDC = initializeTextureDataComponent(l_TDC);

	attachTextureToFramebuffer(
		l_TDC,
		l_GLTDC,
		l_FBC,
		0, 0, 0
	);

	GLFinalRenderPassSingletonComponent::getInstance().m_billboardPassGLTDC = l_GLTDC;

	std::vector<unsigned int> l_colorAttachments;
	l_colorAttachments.emplace_back(GL_COLOR_ATTACHMENT0);
	glDrawBuffers((GLsizei)l_colorAttachments.size(), &l_colorAttachments[0]);

	// finally check if framebuffer is complete
	if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
	{
		g_pCoreSystem->getLogSystem()->printLog("GLFrameBuffer: BillboardRenderPass Framebuffer is not completed!");
	}

	glBindRenderbuffer(GL_RENDERBUFFER, 0);
	glBindFramebuffer(GL_FRAMEBUFFER, 0);

	// shader programs and shaders
	auto l_GLSPC = g_pCoreSystem->getMemorySystem()->spawn<GLShaderProgramComponent>();

	l_GLSPC->m_program = glCreateProgram();

	initializeShader(
		l_GLSPC->m_program,
		GLFinalRenderPassSingletonComponent::getInstance().m_billboardPassVertexShaderID,
		GL_VERTEX_SHADER,
		"GL3.3//billboardPassVertex.sf");
	initializeShader(
		l_GLSPC->m_program,
		GLFinalRenderPassSingletonComponent::getInstance().m_billboardPassFragmentShaderID,
		GL_FRAGMENT_SHADER,
		"GL3.3//billboardPassFragment.sf");
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
	// generate and bind framebuffer
	auto l_FBC = g_pCoreSystem->getMemorySystem()->spawn<GLFrameBufferComponent>();

	glGenFramebuffers(1, &l_FBC->m_FBO);
	glBindFramebuffer(GL_FRAMEBUFFER, l_FBC->m_FBO);

	// generate and bind renderbuffer
	glGenRenderbuffers(1, &l_FBC->m_RBO);
	glBindRenderbuffer(GL_RENDERBUFFER, l_FBC->m_RBO);

	glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT, GL_RENDERBUFFER, l_FBC->m_RBO);
	glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH24_STENCIL8, (GLsizei)GLRenderingSystemNS::g_RenderingSystemSingletonComponent->m_renderTargetSize.x, (GLsizei)GLRenderingSystemNS::g_RenderingSystemSingletonComponent->m_renderTargetSize.y);

	GLFinalRenderPassSingletonComponent::getInstance().m_debuggerPassFBC = l_FBC;

	// generate and bind texture
	auto l_TDC = g_pCoreSystem->getMemorySystem()->spawn<TextureDataComponent>();

	l_TDC->m_textureType = textureType::RENDER_BUFFER_SAMPLER;
	l_TDC->m_textureColorComponentsFormat = textureColorComponentsFormat::RGBA16F;
	l_TDC->m_texturePixelDataFormat = texturePixelDataFormat::RGBA;
	l_TDC->m_textureMinFilterMethod = textureFilterMethod::NEAREST;
	l_TDC->m_textureMagFilterMethod = textureFilterMethod::NEAREST;
	l_TDC->m_textureWrapMethod = textureWrapMethod::CLAMP_TO_EDGE;
	l_TDC->m_textureWidth = (GLsizei)GLRenderingSystemNS::g_RenderingSystemSingletonComponent->m_renderTargetSize.x;
	l_TDC->m_textureHeight = (GLsizei)GLRenderingSystemNS::g_RenderingSystemSingletonComponent->m_renderTargetSize.y;
	l_TDC->m_texturePixelDataType = texturePixelDataType::FLOAT;
	l_TDC->m_textureData = { nullptr };

	GLFinalRenderPassSingletonComponent::getInstance().m_debuggerPassTDC = l_TDC;

	auto l_GLTDC = initializeTextureDataComponent(l_TDC);

	attachTextureToFramebuffer(
		l_TDC,
		l_GLTDC,
		l_FBC,
		0, 0, 0
	);

	GLFinalRenderPassSingletonComponent::getInstance().m_debuggerPassGLTDC = l_GLTDC;

	std::vector<unsigned int> l_colorAttachments;
	l_colorAttachments.emplace_back(GL_COLOR_ATTACHMENT0);
	glDrawBuffers((GLsizei)l_colorAttachments.size(), &l_colorAttachments[0]);

	// finally check if framebuffer is complete
	if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
	{
		g_pCoreSystem->getLogSystem()->printLog("GLFrameBuffer: DebuggerRenderPass Framebuffer is not completed!");
	}

	glBindRenderbuffer(GL_RENDERBUFFER, 0);
	glBindFramebuffer(GL_FRAMEBUFFER, 0);

	// shader programs and shaders
	auto l_GLSPC = g_pCoreSystem->getMemorySystem()->spawn<GLShaderProgramComponent>();

	l_GLSPC->m_program = glCreateProgram();

	initializeShader(
		l_GLSPC->m_program,
		GLFinalRenderPassSingletonComponent::getInstance().m_debuggerPassVertexShaderID,
		GL_VERTEX_SHADER,
		"GL3.3//debuggerPassVertex.sf");
	//initializeShader(
	//	l_GLSPC->m_program,
	//	GLFinalRenderPassSingletonComponent::getInstance().m_debuggerPassGeometryShaderID,
	//	GL_GEOMETRY_SHADER,
	//	"GL3.3//debuggerPassGeometry.sf");
	initializeShader(
		l_GLSPC->m_program,
		GLFinalRenderPassSingletonComponent::getInstance().m_debuggerPassFragmentShaderID,
		GL_FRAGMENT_SHADER,
		"GL3.3//debuggerPassFragment.sf");
	GLFinalRenderPassSingletonComponent::getInstance().m_debuggerPass_uni_normalTexture = getUniformLocation(
		l_GLSPC->m_program,
		"uni_normalTexture");
	updateUniform(
		GLFinalRenderPassSingletonComponent::getInstance().m_debuggerPass_uni_normalTexture,
		0);
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
	// generate and bind framebuffer
	auto l_FBC = g_pCoreSystem->getMemorySystem()->spawn<GLFrameBufferComponent>();

	glGenFramebuffers(1, &l_FBC->m_FBO);
	glBindFramebuffer(GL_FRAMEBUFFER, l_FBC->m_FBO);

	// generate and bind renderbuffer
	glGenRenderbuffers(1, &l_FBC->m_RBO);
	glBindRenderbuffer(GL_RENDERBUFFER, l_FBC->m_RBO);

	glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT, GL_RENDERBUFFER, l_FBC->m_RBO);
	glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH24_STENCIL8, (GLsizei)GLRenderingSystemNS::g_RenderingSystemSingletonComponent->m_renderTargetSize.x, (GLsizei)GLRenderingSystemNS::g_RenderingSystemSingletonComponent->m_renderTargetSize.y);

	GLFinalRenderPassSingletonComponent::getInstance().m_finalBlendPassFBC = l_FBC;

	// generate and bind texture
	auto l_TDC = g_pCoreSystem->getMemorySystem()->spawn<TextureDataComponent>();

	l_TDC->m_textureType = textureType::RENDER_BUFFER_SAMPLER;
	l_TDC->m_textureColorComponentsFormat = textureColorComponentsFormat::RGBA16F;
	l_TDC->m_texturePixelDataFormat = texturePixelDataFormat::RGBA;
	l_TDC->m_textureMinFilterMethod = textureFilterMethod::NEAREST;
	l_TDC->m_textureMagFilterMethod = textureFilterMethod::NEAREST;
	l_TDC->m_textureWrapMethod = textureWrapMethod::CLAMP_TO_EDGE;
	l_TDC->m_textureWidth = (GLsizei)GLRenderingSystemNS::g_RenderingSystemSingletonComponent->m_renderTargetSize.x;
	l_TDC->m_textureHeight = (GLsizei)GLRenderingSystemNS::g_RenderingSystemSingletonComponent->m_renderTargetSize.y;
	l_TDC->m_texturePixelDataType = texturePixelDataType::FLOAT;
	l_TDC->m_textureData = { nullptr };

	GLFinalRenderPassSingletonComponent::getInstance().m_finalBlendPassTDC = l_TDC;

	auto l_GLTDC = initializeTextureDataComponent(l_TDC);

	attachTextureToFramebuffer(
		l_TDC,
		l_GLTDC,
		l_FBC,
		0, 0, 0
	);

	GLFinalRenderPassSingletonComponent::getInstance().m_finalBlendPassGLTDC = l_GLTDC;

	std::vector<unsigned int> l_colorAttachments;
	l_colorAttachments.emplace_back(GL_COLOR_ATTACHMENT0);
	glDrawBuffers((GLsizei)l_colorAttachments.size(), &l_colorAttachments[0]);

	// finally check if framebuffer is complete
	if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
	{
		g_pCoreSystem->getLogSystem()->printLog("GLFrameBuffer: FinalBlendRenderPass Framebuffer is not completed!");
	}

	glBindRenderbuffer(GL_RENDERBUFFER, 0);
	glBindFramebuffer(GL_FRAMEBUFFER, 0);

	// shader programs and shaders
	auto l_GLSPC = g_pCoreSystem->getMemorySystem()->spawn<GLShaderProgramComponent>();

	l_GLSPC->m_program = glCreateProgram();

	initializeShader(
		l_GLSPC->m_program,
		GLFinalRenderPassSingletonComponent::getInstance().m_finalBlendPassVertexShaderID,
		GL_VERTEX_SHADER,
		"GL3.3//finalBlendPassVertex.sf");
	initializeShader(
		l_GLSPC->m_program,
		GLFinalRenderPassSingletonComponent::getInstance().m_finalBlendPassFragmentShaderID,
		GL_FRAGMENT_SHADER,
		"GL3.3//finalBlendPassFragment.sf");
	GLFinalRenderPassSingletonComponent::getInstance().m_uni_motionBlurPassRT0 = getUniformLocation(
		l_GLSPC->m_program,
		"uni_motionBlurPassRT0");
	updateUniform(
		GLFinalRenderPassSingletonComponent::getInstance().m_uni_motionBlurPassRT0,
		0);
	GLFinalRenderPassSingletonComponent::getInstance().m_uni_skyPassRT0 = getUniformLocation(
		l_GLSPC->m_program,
		"uni_skyPassRT0");
	updateUniform(
		GLFinalRenderPassSingletonComponent::getInstance().m_uni_skyPassRT0,
		1);
	GLFinalRenderPassSingletonComponent::getInstance().m_uni_bloomPassRT0 = getUniformLocation(
		l_GLSPC->m_program,
		"uni_bloomPassRT0");
	updateUniform(
		GLFinalRenderPassSingletonComponent::getInstance().m_uni_bloomPassRT0,
		2);
	GLFinalRenderPassSingletonComponent::getInstance().m_uni_billboardPassRT0 = getUniformLocation(
		l_GLSPC->m_program,
		"uni_billboardPassRT0");
	updateUniform(
		GLFinalRenderPassSingletonComponent::getInstance().m_uni_billboardPassRT0,
		3);
	GLFinalRenderPassSingletonComponent::getInstance().m_uni_debuggerPassRT0 = getUniformLocation(
		l_GLSPC->m_program,
		"uni_debuggerPassRT0");
	updateUniform(
		GLFinalRenderPassSingletonComponent::getInstance().m_uni_debuggerPassRT0,
		4);

	GLFinalRenderPassSingletonComponent::getInstance().m_finalBlendPassSPC = l_GLSPC;
}

void GLRenderingSystemNS::initializeShader(GLuint& shaderProgram, GLuint& shaderID, GLuint shaderType, const std::string & shaderFilePath)
{
	shaderID = glCreateShader(shaderType);

	if (shaderID == 0) {
		g_pCoreSystem->getLogSystem()->printLog("Error: Shader creation failed: memory location invaild when adding shader!");
	}

	auto l_shaderCodeContent = g_pCoreSystem->getAssetSystem()->loadShader(shaderFilePath);
	const char* l_sourcePointer = l_shaderCodeContent.c_str();

	if (l_sourcePointer == nullptr)
	{
		g_pCoreSystem->getLogSystem()->printLog("Error: Shader loading failed!");
	}

	glShaderSource(shaderID, 1, &l_sourcePointer, NULL);
	glCompileShader(shaderID);

	GLint l_compileResult = GL_FALSE;
	GLint l_infoLogLength = 0;
	GLint l_shaderFileLength = 0;
	glGetShaderiv(shaderID, GL_COMPILE_STATUS, &l_compileResult);

	if (!l_compileResult)
	{
		g_pCoreSystem->getLogSystem()->printLog("GLRenderingSystem: innoShader: " + shaderFilePath + " compile failed!");
		glGetShaderiv(shaderID, GL_SHADER_SOURCE_LENGTH, &l_shaderFileLength);
		g_pCoreSystem->getLogSystem()->printLog("GLRenderingSystem: innoShader: " + shaderFilePath + " file length is: " + std::to_string(l_shaderFileLength));
		glGetShaderiv(shaderID, GL_INFO_LOG_LENGTH, &l_infoLogLength);

		if (l_infoLogLength > 0) {
			std::vector<char> l_shaderErrorMessage(l_infoLogLength + 1);
			glGetShaderInfoLog(shaderID, l_infoLogLength, NULL, &l_shaderErrorMessage[0]);
			g_pCoreSystem->getLogSystem()->printLog("GLRenderingSystem: innoShader: " + shaderFilePath + " compile error: " + &l_shaderErrorMessage[0] + "\n -- --------------------------------------------------- -- ");
			return;
		}
	}

	g_pCoreSystem->getLogSystem()->printLog("GLRenderingSystem: innoShader: " + shaderFilePath + " Shader has been compiled.");

	glAttachShader(shaderProgram, shaderID);
	glLinkProgram(shaderProgram);
	glValidateProgram(shaderProgram);

	g_pCoreSystem->getLogSystem()->printLog("GLRenderingSystem: innoShader: " + shaderFilePath + " is linking ...");

	glGetShaderiv(shaderID, GL_COMPILE_STATUS, &l_compileResult);
	if (!l_compileResult)
	{
		g_pCoreSystem->getLogSystem()->printLog("GLRenderingSystem: innoShader: " + shaderFilePath + " link failed!");
		glGetShaderiv(shaderID, GL_SHADER_SOURCE_LENGTH, &l_shaderFileLength);
		g_pCoreSystem->getLogSystem()->printLog("GLRenderingSystem: innoShader: " + shaderFilePath + " file length is: " + std::to_string(l_shaderFileLength));
		glGetShaderiv(shaderID, GL_INFO_LOG_LENGTH, &l_infoLogLength);

		if (l_infoLogLength > 0) {
			std::vector<char> l_shaderErrorMessage(l_infoLogLength + 1);
			glGetShaderInfoLog(shaderID, l_infoLogLength, NULL, &l_shaderErrorMessage[0]);
			g_pCoreSystem->getLogSystem()->printLog("GLRenderingSystem: innoShader: " + shaderFilePath + " link error: " + &l_shaderErrorMessage[0] + "\n -- --------------------------------------------------- -- ");
		}
	}

	g_pCoreSystem->getLogSystem()->printLog("GLRenderingSystem: innoShader: " + shaderFilePath + " Shader has been linked.");
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

GLMeshDataComponent* GLRenderingSystemNS::initializeMeshDataComponent(MeshDataComponent* rhs)
{
	if (rhs->m_objectStatus == objectStatus::ALIVE)
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

		l_ptr->m_objectStatus = objectStatus::ALIVE;
		rhs->m_objectStatus = objectStatus::ALIVE;

		g_pCoreSystem->getAssetSystem()->releaseRawDataForMeshDataComponent(rhs->m_parentEntity);
		return l_ptr;
	}
}

GLTextureDataComponent* GLRenderingSystemNS::initializeTextureDataComponent(TextureDataComponent * rhs)
{
	if (rhs->m_objectStatus == objectStatus::ALIVE)
	{
		return getGLTextureDataComponent(rhs->m_parentEntity);
	}
	else
	{
		if (rhs->m_textureType == textureType::INVISIBLE)
		{
			return nullptr;
		}
		else
		{
			auto l_ptr = addGLTextureDataComponent(rhs->m_parentEntity);

			//generate and bind texture object
			glGenTextures(1, &l_ptr->m_TAO);

			if (rhs->m_textureType == textureType::ENVIRONMENT_CAPTURE || rhs->m_textureType == textureType::ENVIRONMENT_CONVOLUTION || rhs->m_textureType == textureType::ENVIRONMENT_PREFILTER)
			{
				glBindTexture(GL_TEXTURE_CUBE_MAP, l_ptr->m_TAO);
			}
			else
			{
				glBindTexture(GL_TEXTURE_2D, l_ptr->m_TAO);
			}

			// set the texture wrapping parameters
			GLenum l_textureWrapMethod;
			switch (rhs->m_textureWrapMethod)
			{
			case textureWrapMethod::CLAMP_TO_EDGE: l_textureWrapMethod = GL_CLAMP_TO_EDGE; break;
			case textureWrapMethod::REPEAT: l_textureWrapMethod = GL_REPEAT; break;
			case textureWrapMethod::CLAMP_TO_BORDER: l_textureWrapMethod = GL_CLAMP_TO_BORDER; break;
			}
			if (rhs->m_textureType == textureType::ENVIRONMENT_CAPTURE || rhs->m_textureType == textureType::ENVIRONMENT_CONVOLUTION || rhs->m_textureType == textureType::ENVIRONMENT_PREFILTER)
			{
				glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, l_textureWrapMethod);
				glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, l_textureWrapMethod);
				glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, l_textureWrapMethod);
			}
			else if (rhs->m_textureType == textureType::SHADOWMAP)
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
			switch (rhs->m_textureMinFilterMethod)
			{
			case textureFilterMethod::NEAREST: l_minFilterParam = GL_NEAREST; break;
			case textureFilterMethod::LINEAR: l_minFilterParam = GL_LINEAR; break;
			case textureFilterMethod::LINEAR_MIPMAP_LINEAR: l_minFilterParam = GL_LINEAR_MIPMAP_LINEAR; break;

			}
			GLenum l_magFilterParam;
			switch (rhs->m_textureMagFilterMethod)
			{
			case textureFilterMethod::NEAREST: l_magFilterParam = GL_NEAREST; break;
			case textureFilterMethod::LINEAR: l_magFilterParam = GL_LINEAR; break;
			case textureFilterMethod::LINEAR_MIPMAP_LINEAR: l_magFilterParam = GL_LINEAR_MIPMAP_LINEAR; break;

			}
			if (rhs->m_textureType == textureType::ENVIRONMENT_CAPTURE || rhs->m_textureType == textureType::ENVIRONMENT_CONVOLUTION || rhs->m_textureType == textureType::ENVIRONMENT_PREFILTER)
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
			if (rhs->m_textureType == textureType::ALBEDO)
			{
				if (rhs->m_texturePixelDataFormat == texturePixelDataFormat::RGB)
				{
					l_internalFormat = GL_SRGB;
				}
				else if (rhs->m_texturePixelDataFormat == texturePixelDataFormat::RGBA)
				{
					l_internalFormat = GL_SRGB_ALPHA;
				}
			}
			else
			{
				switch (rhs->m_textureColorComponentsFormat)
				{
				case textureColorComponentsFormat::RED: l_internalFormat = GL_RED; break;
				case textureColorComponentsFormat::RG: l_internalFormat = GL_RG; break;
				case textureColorComponentsFormat::RGB: l_internalFormat = GL_RGB; break;
				case textureColorComponentsFormat::RGBA: l_internalFormat = GL_RGBA; break;
				case textureColorComponentsFormat::R8: l_internalFormat = GL_R8; break;
				case textureColorComponentsFormat::RG8: l_internalFormat = GL_RG8; break;
				case textureColorComponentsFormat::RGB8: l_internalFormat = GL_RGB8; break;
				case textureColorComponentsFormat::RGBA8: l_internalFormat = GL_RGBA8; break;
				case textureColorComponentsFormat::R16: l_internalFormat = GL_R16; break;
				case textureColorComponentsFormat::RG16: l_internalFormat = GL_RG16; break;
				case textureColorComponentsFormat::RGB16: l_internalFormat = GL_RGB16; break;
				case textureColorComponentsFormat::RGBA16: l_internalFormat = GL_RGBA16; break;
				case textureColorComponentsFormat::R16F: l_internalFormat = GL_R16F; break;
				case textureColorComponentsFormat::RG16F: l_internalFormat = GL_RG16F; break;
				case textureColorComponentsFormat::RGB16F: l_internalFormat = GL_RGB16F; break;
				case textureColorComponentsFormat::RGBA16F: l_internalFormat = GL_RGBA16F; break;
				case textureColorComponentsFormat::R32F: l_internalFormat = GL_R32F; break;
				case textureColorComponentsFormat::RG32F: l_internalFormat = GL_RG32F; break;
				case textureColorComponentsFormat::RGB32F: l_internalFormat = GL_RGB32F; break;
				case textureColorComponentsFormat::RGBA32F: l_internalFormat = GL_RGBA32F; break;
				case textureColorComponentsFormat::SRGB: l_internalFormat = GL_SRGB; break;
				case textureColorComponentsFormat::SRGBA: l_internalFormat = GL_SRGB_ALPHA; break;
				case textureColorComponentsFormat::SRGB8: l_internalFormat = GL_SRGB8; break;
				case textureColorComponentsFormat::SRGBA8: l_internalFormat = GL_SRGB8_ALPHA8; break;
				case textureColorComponentsFormat::DEPTH_COMPONENT: l_internalFormat = GL_DEPTH_COMPONENT; break;
				}
			}
			switch (rhs->m_texturePixelDataFormat)
			{
			case texturePixelDataFormat::RED:l_dataFormat = GL_RED; break;
			case texturePixelDataFormat::RG:l_dataFormat = GL_RG; break;
			case texturePixelDataFormat::RGB:l_dataFormat = GL_RGB; break;
			case texturePixelDataFormat::RGBA:l_dataFormat = GL_RGBA; break;
			case texturePixelDataFormat::DEPTH_COMPONENT:l_dataFormat = GL_DEPTH_COMPONENT; break;
			}
			switch (rhs->m_texturePixelDataType)
			{
			case texturePixelDataType::UNSIGNED_BYTE:l_type = GL_UNSIGNED_BYTE; break;
			case texturePixelDataType::BYTE:l_type = GL_BYTE; break;
			case texturePixelDataType::UNSIGNED_SHORT:l_type = GL_UNSIGNED_SHORT; break;
			case texturePixelDataType::SHORT:l_type = GL_SHORT; break;
			case texturePixelDataType::UNSIGNED_INT:l_type = GL_UNSIGNED_INT; break;
			case texturePixelDataType::INT:l_type = GL_INT; break;
			case texturePixelDataType::FLOAT:l_type = GL_FLOAT; break;
			}

			if (rhs->m_textureType == textureType::ENVIRONMENT_CAPTURE || rhs->m_textureType == textureType::ENVIRONMENT_CONVOLUTION || rhs->m_textureType == textureType::ENVIRONMENT_PREFILTER)
			{
				glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X, 0, l_internalFormat, rhs->m_textureWidth, rhs->m_textureHeight, 0, l_dataFormat, l_type, rhs->m_textureData[0]);
				glTexImage2D(GL_TEXTURE_CUBE_MAP_NEGATIVE_X, 0, l_internalFormat, rhs->m_textureWidth, rhs->m_textureHeight, 0, l_dataFormat, l_type, rhs->m_textureData[1]);
				glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_Y, 0, l_internalFormat, rhs->m_textureWidth, rhs->m_textureHeight, 0, l_dataFormat, l_type, rhs->m_textureData[2]);
				glTexImage2D(GL_TEXTURE_CUBE_MAP_NEGATIVE_Y, 0, l_internalFormat, rhs->m_textureWidth, rhs->m_textureHeight, 0, l_dataFormat, l_type, rhs->m_textureData[3]);
				glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_Z, 0, l_internalFormat, rhs->m_textureWidth, rhs->m_textureHeight, 0, l_dataFormat, l_type, rhs->m_textureData[4]);
				glTexImage2D(GL_TEXTURE_CUBE_MAP_NEGATIVE_Z, 0, l_internalFormat, rhs->m_textureWidth, rhs->m_textureHeight, 0, l_dataFormat, l_type, rhs->m_textureData[5]);
			}
			else
			{
				glTexImage2D(GL_TEXTURE_2D, 0, l_internalFormat, rhs->m_textureWidth, rhs->m_textureHeight, 0, l_dataFormat, l_type, rhs->m_textureData[0]);
			}

			// should generate mipmap or not
			if (rhs->m_textureMinFilterMethod == textureFilterMethod::LINEAR_MIPMAP_LINEAR)
			{
				// @TODO: generalization...
				if (rhs->m_textureType == textureType::ENVIRONMENT_PREFILTER)
				{
					glGenerateMipmap(GL_TEXTURE_CUBE_MAP);
				}
				else if (rhs->m_textureType != textureType::ENVIRONMENT_CAPTURE || rhs->m_textureType != textureType::ENVIRONMENT_CONVOLUTION || rhs->m_textureType != textureType::RENDER_BUFFER_SAMPLER)
				{
					glGenerateMipmap(GL_TEXTURE_2D);
				}
			}

			l_ptr->m_objectStatus = objectStatus::ALIVE;
			rhs->m_objectStatus = objectStatus::ALIVE;

			return l_ptr;
		}
	}
}

bool GLRenderingSystem::update()
{
	if (AssetSystemSingletonComponent::getInstance().m_uninitializedMeshComponents.size() > 0)
	{
		MeshDataComponent* l_meshDataComponent;
		if (AssetSystemSingletonComponent::getInstance().m_uninitializedMeshComponents.tryPop(l_meshDataComponent))
		{
			GLRenderingSystemNS::initializeMeshDataComponent(l_meshDataComponent);
		}
	}
	if (AssetSystemSingletonComponent::getInstance().m_uninitializedTextureComponents.size() > 0)
	{
		TextureDataComponent* l_textureDataComponent;
		if (AssetSystemSingletonComponent::getInstance().m_uninitializedTextureComponents.tryPop(l_textureDataComponent))
		{
			GLRenderingSystemNS::initializeTextureDataComponent(l_textureDataComponent);
		}
	}
	if (GLRenderingSystemNS::g_RenderingSystemSingletonComponent->m_shouldUpdateEnvironmentMap)
	{
		GLRenderingSystemNS::updateEnvironmentRenderPass();
	}
	GLRenderingSystemNS::updateShadowRenderPass();
	GLRenderingSystemNS::updateGeometryRenderPass();
	GLRenderingSystemNS::updateLightRenderPass();
	GLRenderingSystemNS::updateFinalRenderPass();

	return true;
}

void GLRenderingSystemNS::updateEnvironmentRenderPass()
{
	glDisable(GL_DEPTH_TEST);
	glDisable(GL_CULL_FACE);

	// bind to framebuffer
	auto l_FBC = EnvironmentRenderPassSingletonComponent::getInstance().m_FBC;
	glBindFramebuffer(GL_FRAMEBUFFER, l_FBC->m_FBO);
	glBindRenderbuffer(GL_RENDERBUFFER, l_FBC->m_RBO);

	glClear(GL_COLOR_BUFFER_BIT);
	glClear(GL_DEPTH_BUFFER_BIT);

	// draw environment capture texture
	glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT32, 2048, 2048);
	glViewport(0, 0, 2048, 2048);

	mat4 l_p;
	l_p.initializeToPerspectiveMatrix((90.0f / 180.0f) * PI<float>, 1.0f, 0.1f, 10.0f);
	std::vector<mat4> l_v =
	{
		mat4().lookAt(vec4(0.0f, 0.0f, 0.0f, 1.0f), vec4(1.0f,  0.0f,  0.0f, 1.0f), vec4(0.0f, -1.0f,  0.0f, 0.0f)),
		mat4().lookAt(vec4(0.0f, 0.0f, 0.0f, 1.0f), vec4(-1.0f,  0.0f,  0.0f, 1.0f), vec4(0.0f, -1.0f,  0.0f, 0.0f)),
		mat4().lookAt(vec4(0.0f, 0.0f, 0.0f, 1.0f), vec4(0.0f,  1.0f,  0.0f, 1.0f), vec4(0.0f,  0.0f,  1.0f, 0.0f)),
		mat4().lookAt(vec4(0.0f, 0.0f, 0.0f, 1.0f), vec4(0.0f, -1.0f,  0.0f, 1.0f), vec4(0.0f,  0.0f, -1.0f, 0.0f)),
		mat4().lookAt(vec4(0.0f, 0.0f, 0.0f, 1.0f), vec4(0.0f,  0.0f,  1.0f, 1.0f), vec4(0.0f, -1.0f,  0.0f, 0.0f)),
		mat4().lookAt(vec4(0.0f, 0.0f, 0.0f, 1.0f), vec4(0.0f,  0.0f, -1.0f, 1.0f), vec4(0.0f, -1.0f,  0.0f, 0.0f))
	};

	activateShaderProgram(EnvironmentRenderPassSingletonComponent::getInstance().m_capturePassSPC);
	updateUniform(EnvironmentRenderPassSingletonComponent::getInstance().m_capturePass_uni_p, l_p);

	auto l_MDC = g_pCoreSystem->getAssetSystem()->getMeshDataComponent(meshShapeType::CUBE);

	// activate equiretangular texture and remap equiretangular texture to cubemap
	auto l_capturePassTDC = EnvironmentRenderPassSingletonComponent::getInstance().m_capturePassTDC;
	auto l_capturePassGLTDC = EnvironmentRenderPassSingletonComponent::getInstance().m_capturePassGLTDC;

	auto l_equiretangularTDC = GLRenderingSystemNS::g_GameSystemSingletonComponent->m_EnvironmentCaptureComponents[0]->m_TDC;
	if (l_equiretangularTDC)
	{
		auto l_equiretangularGLTDC = getGLTextureDataComponent(l_equiretangularTDC->m_parentEntity);
		if (l_equiretangularGLTDC)
		{
			if (l_equiretangularTDC->m_objectStatus == objectStatus::ALIVE && l_equiretangularGLTDC->m_objectStatus == objectStatus::ALIVE)
			{
				GLRenderingSystemNS::g_RenderingSystemSingletonComponent->m_shouldUpdateEnvironmentMap = false;

				activateCubemapTexture(l_equiretangularGLTDC, 0);
				for (unsigned int i = 0; i < 6; ++i)
				{
					updateUniform(EnvironmentRenderPassSingletonComponent::getInstance().m_capturePass_uni_r, l_v[i]);
					attachTextureToFramebuffer(l_capturePassTDC, l_capturePassGLTDC, l_FBC, 0, i, 0);
					glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
					drawMesh(l_MDC);
				}
				glGenerateMipmap(GL_TEXTURE_CUBE_MAP);
			}
		}
	}

	// draw environment convolution texture
	glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT32, 128, 128);
	glViewport(0, 0, 128, 128);
	activateShaderProgram(EnvironmentRenderPassSingletonComponent::getInstance().m_convolutionPassSPC);
	updateUniform(EnvironmentRenderPassSingletonComponent::getInstance().m_convolutionPass_uni_p, l_p);

	auto l_convolutionPassTDC = EnvironmentRenderPassSingletonComponent::getInstance().m_convolutionPassTDC;
	auto l_convolutionPassGLTDC = EnvironmentRenderPassSingletonComponent::getInstance().m_convolutionPassGLTDC;

	activateCubemapTexture(l_capturePassGLTDC, 1);
	for (unsigned int i = 0; i < 6; ++i)
	{
		updateUniform(EnvironmentRenderPassSingletonComponent::getInstance().m_convolutionPass_uni_r, l_v[i]);
		attachTextureToFramebuffer(l_convolutionPassTDC, l_convolutionPassGLTDC, l_FBC, 0, i, 0);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
		drawMesh(l_MDC);
	}

	// draw environment pre-filter texture
	glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT32, 128, 128);
	glViewport(0, 0, 128, 128);
	activateShaderProgram(EnvironmentRenderPassSingletonComponent::getInstance().m_preFilterPassSPC);
	updateUniform(EnvironmentRenderPassSingletonComponent::getInstance().m_preFilterPass_uni_p, l_p);

	auto l_prefilterPassTDC = EnvironmentRenderPassSingletonComponent::getInstance().m_preFilterPassTDC;
	auto l_prefilterPassGLTDC = EnvironmentRenderPassSingletonComponent::getInstance().m_preFilterPassGLTDC;
	activateCubemapTexture(l_capturePassGLTDC, 2);

	auto l_maxMipLevels = EnvironmentRenderPassSingletonComponent::getInstance().m_maxMipLevels;
	for (unsigned int mip = 0; mip < l_maxMipLevels; ++mip)
	{
		// resize framebuffer according to mip-level size.
		unsigned int mipWidth = (int)(128 * std::pow(0.5, mip));
		unsigned int mipHeight = (int)(128 * std::pow(0.5, mip));

		glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT32, mipWidth, mipHeight);
		glViewport(0, 0, mipWidth, mipHeight);

		float roughness = (float)mip / (float)(l_maxMipLevels - 1);
		updateUniform(EnvironmentRenderPassSingletonComponent::getInstance().m_preFilterPass_uni_roughness, roughness);
		for (unsigned int i = 0; i < 6; ++i)
		{
			updateUniform(EnvironmentRenderPassSingletonComponent::getInstance().m_preFilterPass_uni_r, l_v[i]);
			attachTextureToFramebuffer(l_prefilterPassTDC, l_prefilterPassGLTDC, l_FBC, 0, i, mip);
			glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
			drawMesh(l_MDC);
		}
	}

	// draw environment BRDF look-up table texture
	glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT32, 512, 512);
	glViewport(0, 0, 512, 512);
	activateShaderProgram(EnvironmentRenderPassSingletonComponent::getInstance().m_BRDFLUTPassSPC);

	auto l_environmentBRDFLUTTDC = EnvironmentRenderPassSingletonComponent::getInstance().m_BRDFLUTTDC;
	auto l_environmentBRDFLUTGLTDC = EnvironmentRenderPassSingletonComponent::getInstance().m_BRDFLUTGLTDC;
	attachTextureToFramebuffer(l_environmentBRDFLUTTDC, l_environmentBRDFLUTGLTDC, l_FBC, 0, 0, 0);

	// draw environment map BRDF LUT rectangle
	l_MDC = g_pCoreSystem->getAssetSystem()->getMeshDataComponent(meshShapeType::QUAD);
	drawMesh(l_MDC);
}

void GLRenderingSystemNS::updateShadowRenderPass()
{
	glEnable(GL_DEPTH_TEST);
	glDepthFunc(GL_LESS);
	glEnable(GL_CULL_FACE);
	glFrontFace(GL_CW);
	glCullFace(GL_FRONT);

	// draw each lightComponent's shadowmap
	for (size_t i = 0; i < ShadowRenderPassSingletonComponent::getInstance().m_FBCs.size(); i++)
	{
		auto l_FBC = ShadowRenderPassSingletonComponent::getInstance().m_FBCs[i];
		// bind to framebuffer
		glBindFramebuffer(GL_FRAMEBUFFER, l_FBC->m_FBO);
		glBindRenderbuffer(GL_RENDERBUFFER, l_FBC->m_RBO);
		glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT32, 2048, 2048);
		glViewport(0, 0, 2048, 2048);

		glClear(GL_COLOR_BUFFER_BIT);
		glClear(GL_DEPTH_BUFFER_BIT);

		for (auto& l_lightComponent : GLRenderingSystemNS::g_GameSystemSingletonComponent->m_LightComponents)
		{
			if (l_lightComponent->m_lightType == lightType::DIRECTIONAL)
			{
				activateShaderProgram(ShadowRenderPassSingletonComponent::getInstance().m_SPC);
				updateUniform(
					ShadowRenderPassSingletonComponent::getInstance().m_shadowPass_uni_p,
					l_lightComponent->m_projectionMatrices[i]);
				updateUniform(
					ShadowRenderPassSingletonComponent::getInstance().m_shadowPass_uni_v,
					g_pCoreSystem->getGameSystem()->get<TransformComponent>(l_lightComponent->m_parentEntity)->m_globalTransformMatrix.m_transformationMat.inverse());

				// draw each visibleComponent
				for (auto& l_visibleComponent : GLRenderingSystemNS::g_GameSystemSingletonComponent->m_VisibleComponents)
				{
					if (l_visibleComponent->m_visiblilityType == visiblilityType::STATIC_MESH)
					{
						updateUniform(
							ShadowRenderPassSingletonComponent::getInstance().m_shadowPass_uni_m,
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
	auto l_FBC = GeometryRenderPassSingletonComponent::getInstance().m_FBC;
	glBindFramebuffer(GL_FRAMEBUFFER, l_FBC->m_FBO);
	glBindRenderbuffer(GL_RENDERBUFFER, l_FBC->m_RBO);
	auto l_RtSize = GLRenderingSystemNS::g_RenderingSystemSingletonComponent->m_renderTargetSize;
	glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH24_STENCIL8, (GLsizei)l_RtSize.x, (GLsizei)l_RtSize.y);
	glViewport(0, 0, (GLsizei)l_RtSize.x, (GLsizei)l_RtSize.y);

	glClearColor(0.0f, 0.0f, 0.0f, 0.0f);
	glClear(GL_COLOR_BUFFER_BIT);
	glClear(GL_DEPTH_BUFFER_BIT);
	glClear(GL_STENCIL_BUFFER_BIT);

	activateShaderProgram(GeometryRenderPassSingletonComponent::getInstance().m_GLSPC);

	if (GLRenderingSystemNS::g_GameSystemSingletonComponent->m_CameraComponents.size() > 0)
	{
		auto l_mainCamera = GLRenderingSystemNS::g_GameSystemSingletonComponent->m_CameraComponents[0];
		mat4 p_original = l_mainCamera->m_projectionMatrix;
		mat4 p_jittered = p_original;
		//TAA jitter for projection matrix
		auto& l_currentHaltonStep = GLRenderingSystemNS::g_RenderingSystemSingletonComponent->currentHaltonStep;
		if (l_currentHaltonStep >= 16)
		{
			l_currentHaltonStep = 0;
		}
		p_jittered.m02 = GLRenderingSystemNS::g_RenderingSystemSingletonComponent->HaltonSampler[l_currentHaltonStep].x / l_RtSize.x;
		p_jittered.m12 = GLRenderingSystemNS::g_RenderingSystemSingletonComponent->HaltonSampler[l_currentHaltonStep].y / l_RtSize.y;
		l_currentHaltonStep += 1;

		auto l_mainCameraTransformComponent = g_pCoreSystem->getGameSystem()->get<TransformComponent>(l_mainCamera->m_parentEntity);
		mat4 r =
			InnoMath::getInvertRotationMatrix(
				l_mainCameraTransformComponent->m_globalTransformVector.m_rot
			);
		mat4 t =
			InnoMath::getInvertTranslationMatrix(
				l_mainCameraTransformComponent->m_globalTransformVector.m_pos
			);
		mat4 r_prev = l_mainCameraTransformComponent->m_globalTransformMatrix_prev.m_rotationMat.inverse();
		mat4 t_prev = l_mainCameraTransformComponent->m_globalTransformMatrix_prev.m_translationMat.inverse();

		updateUniform(
			GeometryRenderPassSingletonComponent::getInstance().m_geometryPass_uni_p_camera_original,
			p_original);
		updateUniform(
			GeometryRenderPassSingletonComponent::getInstance().m_geometryPass_uni_p_camera_jittered,
			p_jittered);
		updateUniform(
			GeometryRenderPassSingletonComponent::getInstance().m_geometryPass_uni_r_camera,
			r);
		updateUniform(
			GeometryRenderPassSingletonComponent::getInstance().m_geometryPass_uni_t_camera,
			t);
		updateUniform(
			GeometryRenderPassSingletonComponent::getInstance().m_geometryPass_uni_r_camera_prev,
			r_prev);
		updateUniform(
			GeometryRenderPassSingletonComponent::getInstance().m_geometryPass_uni_t_camera_prev,
			t_prev);

#ifdef CookTorrance
		//Cook-Torrance
		if (GLRenderingSystemNS::g_GameSystemSingletonComponent->m_LightComponents.size() > 0)
		{
			for (auto& l_lightComponent : GLRenderingSystemNS::g_GameSystemSingletonComponent->m_LightComponents)
			{
				// update light space transformation matrices
				if (l_lightComponent->m_lightType == lightType::DIRECTIONAL)
				{
					updateUniform(GeometryRenderPassSingletonComponent::getInstance().m_geometryPass_uni_p_light_0,
						l_lightComponent->m_projectionMatrices[0]);
					updateUniform(GeometryRenderPassSingletonComponent::getInstance().m_geometryPass_uni_p_light_1,
						l_lightComponent->m_projectionMatrices[1]);
					updateUniform(GeometryRenderPassSingletonComponent::getInstance().m_geometryPass_uni_p_light_2,
						l_lightComponent->m_projectionMatrices[2]);
					updateUniform(GeometryRenderPassSingletonComponent::getInstance().m_geometryPass_uni_p_light_3,
						l_lightComponent->m_projectionMatrices[3]);
					updateUniform(GeometryRenderPassSingletonComponent::getInstance().m_geometryPass_uni_v_light,
						InnoMath::getInvertRotationMatrix(
							g_pCoreSystem->getGameSystem()->get<TransformComponent>(l_lightComponent->m_parentEntity)->m_globalTransformVector.m_rot
						));

					// draw each visibleComponent
					for (auto& l_visibleComponent : GLRenderingSystemNS::g_RenderingSystemSingletonComponent->m_inFrustumVisibleComponents)
					{
						if (l_visibleComponent->m_visiblilityType == visiblilityType::STATIC_MESH)
						{
							glStencilFunc(GL_ALWAYS, 0x01, 0xFF);

							updateUniform(
								GeometryRenderPassSingletonComponent::getInstance().m_geometryPass_uni_m,
								g_pCoreSystem->getGameSystem()->get<TransformComponent>(l_visibleComponent->m_parentEntity)->m_globalTransformMatrix.m_transformationMat);
							updateUniform(
								GeometryRenderPassSingletonComponent::getInstance().m_geometryPass_uni_m_prev,
								g_pCoreSystem->getGameSystem()->get<TransformComponent>(l_visibleComponent->m_parentEntity)->m_globalTransformMatrix_prev.m_transformationMat);

							// draw each graphic data of visibleComponent
							for (auto l_modelPair : l_visibleComponent->m_modelMap)
							{
								// draw meshes
								auto l_MDC = l_modelPair.first;
								if (l_MDC)
								{
									//active and bind textures
									// is there any texture?
									auto l_textureMap = l_modelPair.second;
									if (l_textureMap)
									{
										// any normal?
										auto l_TDC = l_textureMap->m_texturePack.m_normalTDC.second;
										if (l_TDC)
										{
											activateTexture(l_TDC, 0);
											updateUniform(GeometryRenderPassSingletonComponent::getInstance().m_geometryPass_uni_useNormalTexture, true);
										}
										else
										{
											updateUniform(GeometryRenderPassSingletonComponent::getInstance().m_geometryPass_uni_useNormalTexture, false);
										}
										// any albedo?
										l_TDC = l_textureMap->m_texturePack.m_albedoTDC.second;
										if (l_TDC)
										{
											activateTexture(l_TDC, 1);
											updateUniform(GeometryRenderPassSingletonComponent::getInstance().m_geometryPass_uni_useAlbedoTexture, true);
										}
										else
										{
											updateUniform(GeometryRenderPassSingletonComponent::getInstance().m_geometryPass_uni_useAlbedoTexture, false);
										}
										// any metallic?
										l_TDC = l_textureMap->m_texturePack.m_metallicTDC.second;
										if (l_TDC)
										{
											activateTexture(l_TDC, 2);
											updateUniform(GeometryRenderPassSingletonComponent::getInstance().m_geometryPass_uni_useMetallicTexture, true);
										}
										else
										{
											updateUniform(GeometryRenderPassSingletonComponent::getInstance().m_geometryPass_uni_useMetallicTexture, false);
										}
										// any roughness?
										l_TDC = l_textureMap->m_texturePack.m_roughnessTDC.second;
										if (l_TDC)
										{
											activateTexture(l_TDC, 3);
											updateUniform(GeometryRenderPassSingletonComponent::getInstance().m_geometryPass_uni_useRoughnessTexture, true);
										}
										else
										{
											updateUniform(GeometryRenderPassSingletonComponent::getInstance().m_geometryPass_uni_useRoughnessTexture, false);
										}
										// any ao?
										l_TDC = l_textureMap->m_texturePack.m_roughnessTDC.second;
										if (l_TDC)
										{
											activateTexture(l_TDC, 4);
											updateUniform(GeometryRenderPassSingletonComponent::getInstance().m_geometryPass_uni_useAOTexture, true);
										}
										else
										{
											updateUniform(GeometryRenderPassSingletonComponent::getInstance().m_geometryPass_uni_useAOTexture, false);
										}
										updateUniform(GeometryRenderPassSingletonComponent::getInstance().m_geometryPass_uni_albedo, l_textureMap->m_meshColor.albedo_r, l_textureMap->m_meshColor.albedo_g, l_textureMap->m_meshColor.albedo_b);
										updateUniform(GeometryRenderPassSingletonComponent::getInstance().m_geometryPass_uni_MRA, l_textureMap->m_meshColor.metallic, l_textureMap->m_meshColor.roughness, l_textureMap->m_meshColor.ao);
									}
									drawMesh(l_MDC);
								}
							}
						}
						else if (l_visibleComponent->m_visiblilityType == visiblilityType::EMISSIVE)
						{
							glStencilFunc(GL_ALWAYS, 0x02, 0xFF);

							updateUniform(
								GeometryRenderPassSingletonComponent::getInstance().m_geometryPass_uni_m,
								g_pCoreSystem->getGameSystem()->get<TransformComponent>(l_visibleComponent->m_parentEntity)->m_globalTransformMatrix.m_transformationMat);
							updateUniform(
								GeometryRenderPassSingletonComponent::getInstance().m_geometryPass_uni_m_prev,
								g_pCoreSystem->getGameSystem()->get<TransformComponent>(l_visibleComponent->m_parentEntity)->m_globalTransformMatrix_prev.m_transformationMat);

							// draw each graphic data of visibleComponent
							for (auto& l_modelPair : l_visibleComponent->m_modelMap)
							{
								// draw meshes
								auto l_MDC = l_modelPair.first;
								if (l_MDC)
								{
									auto l_textureMap = l_modelPair.second;
									if (l_textureMap)
									{
										updateUniform(GeometryRenderPassSingletonComponent::getInstance().m_geometryPass_uni_albedo, l_textureMap->m_meshColor.albedo_r, l_textureMap->m_meshColor.albedo_g, l_textureMap->m_meshColor.albedo_b);
										updateUniform(GeometryRenderPassSingletonComponent::getInstance().m_geometryPass_uni_useAlbedoTexture, false);
									}
									drawMesh(l_MDC);
								}
							}
						}
						else
						{
							glStencilFunc(GL_ALWAYS, 0x00, 0xFF);
						}
					}
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
			if (l_visibleComponent->m_visiblilityType == visiblilityType::STATIC_MESH)
			{
				updateUniform(
					GeometryRenderPassSingletonComponent::getInstance().m_geometryPass_uni_m,
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
						auto l_normalTextureID = l_textureMap->find(textureType::NORMAL);
						if (l_normalTextureID != l_textureMap->end())
						{
							auto l_TDC = g_pCoreSystem->getAssetSystem()->getTextureDataComponent(l_normalTextureID->second);
							if (l_TDC)
							{
								if (l_TDC->m_objectStatus == objectStatus::ALIVE)
								{
									auto l_GLTDC = getGLTextureDataComponent(l_TDC->m_parentEntity);
									if (l_GLTDC)
									{
										if (l_GLTDC->m_objectStatus == objectStatus::ALIVE)
										{
											activateTexture(l_TDC, l_GLTDC, 0);
										}
									}
								}
							}
						}
						// any diffuse?
						auto l_diffuseTextureID = l_textureMap->find(textureType::ALBEDO);
						if (l_diffuseTextureID != l_textureMap->end())
						{
							auto l_TDC = g_pCoreSystem->getAssetSystem()->getTextureDataComponent(l_diffuseTextureID->second);
							if (l_TDC)
							{
								if (l_TDC->m_objectStatus == objectStatus::ALIVE)
								{
									auto l_GLTDC = getGLTextureDataComponent(l_TDC->m_parentEntity);
									if (l_GLTDC)
									{
										if (l_GLTDC->m_objectStatus == objectStatus::ALIVE)
										{
											activateTexture(l_TDC, l_GLTDC, 1);
										}
									}
								}
							}
						}
						// any specular?
						auto l_specularTextureID = l_textureMap->find(textureType::METALLIC);
						if (l_specularTextureID != l_textureMap->end())
						{
							auto l_TDC = g_pCoreSystem->getAssetSystem()->getTextureDataComponent(l_specularTextureID->second);
							if (l_TDC)
							{
								auto l_GLTDC = getGLTextureDataComponent(l_TDC->m_parentEntity);
								if (l_GLTDC)
								{
									if (l_GLTDC->m_objectStatus == objectStatus::ALIVE)
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

void GLRenderingSystemNS::updateLightRenderPass()
{
	glDisable(GL_DEPTH_TEST);

	glEnable(GL_STENCIL_TEST);
	glStencilOp(GL_KEEP, GL_KEEP, GL_KEEP);
	glStencilFunc(GL_EQUAL, 0x01, 0xFF);
	glStencilMask(0x00);

	glDisable(GL_CULL_FACE);

	// bind to framebuffer
	auto l_FBC = LightRenderPassSingletonComponent::getInstance().m_FBC;
	glBindFramebuffer(GL_FRAMEBUFFER, l_FBC->m_FBO);
	glBindRenderbuffer(GL_RENDERBUFFER, l_FBC->m_RBO);
	glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH24_STENCIL8, (GLsizei)GLRenderingSystemNS::g_RenderingSystemSingletonComponent->m_renderTargetSize.x, (GLsizei)GLRenderingSystemNS::g_RenderingSystemSingletonComponent->m_renderTargetSize.y);
	glViewport(0, 0, (GLsizei)GLRenderingSystemNS::g_RenderingSystemSingletonComponent->m_renderTargetSize.x, (GLsizei)GLRenderingSystemNS::g_RenderingSystemSingletonComponent->m_renderTargetSize.y);

	glClearColor(0.0f, 0.0f, 0.0f, 0.0f);
	glClear(GL_COLOR_BUFFER_BIT);
	glClear(GL_DEPTH_BUFFER_BIT);
	glClear(GL_STENCIL_BUFFER_BIT);

	// 1. opaque objects
	// copy stencil buffer of opaque objects from G-Pass
	glBindFramebuffer(GL_READ_FRAMEBUFFER, GeometryRenderPassSingletonComponent::getInstance().m_FBC->m_FBO);
	glBindFramebuffer(GL_DRAW_FRAMEBUFFER, l_FBC->m_FBO);
	glBlitFramebuffer(0, 0, (GLsizei)GLRenderingSystemNS::g_RenderingSystemSingletonComponent->m_renderTargetSize.x, (GLsizei)GLRenderingSystemNS::g_RenderingSystemSingletonComponent->m_renderTargetSize.y, 0, 0, (GLsizei)GLRenderingSystemNS::g_RenderingSystemSingletonComponent->m_renderTargetSize.x, (GLsizei)GLRenderingSystemNS::g_RenderingSystemSingletonComponent->m_renderTargetSize.y, GL_STENCIL_BUFFER_BIT, GL_NEAREST);

	activateShaderProgram(LightRenderPassSingletonComponent::getInstance().m_GLSPC);

#ifdef CookTorrance
	// Cook-Torrance
	// world space position + metallic
	activate2DTexture(
		GeometryRenderPassSingletonComponent::getInstance().m_GLTDCs[0],
		0);
	// normal + roughness
	activate2DTexture(
		GeometryRenderPassSingletonComponent::getInstance().m_GLTDCs[1],
		1);
	// albedo + ambient occlusion
	activate2DTexture(
		GeometryRenderPassSingletonComponent::getInstance().m_GLTDCs[2],
		2);
	// light space position 0
	activate2DTexture(
		GeometryRenderPassSingletonComponent::getInstance().m_GLTDCs[4],
		3);
	// light space position 1
	activate2DTexture(
		GeometryRenderPassSingletonComponent::getInstance().m_GLTDCs[5],
		4);
	// light space position 2
	activate2DTexture(
		GeometryRenderPassSingletonComponent::getInstance().m_GLTDCs[6],
		5);
	// light space position 3
	activate2DTexture(
		GeometryRenderPassSingletonComponent::getInstance().m_GLTDCs[7],
		6);
	// shadow map 0
	activate2DTexture(
		ShadowRenderPassSingletonComponent::getInstance().m_GLTDCs[0],
		7);
	// shadow map 1
	activate2DTexture(
		ShadowRenderPassSingletonComponent::getInstance().m_GLTDCs[1],
		8);
	// shadow map 2
	activate2DTexture(
		ShadowRenderPassSingletonComponent::getInstance().m_GLTDCs[2],
		9);
	// shadow map 3
	activate2DTexture(
		ShadowRenderPassSingletonComponent::getInstance().m_GLTDCs[3],
		10);
	// irradiance environment map
	activateCubemapTexture(
		EnvironmentRenderPassSingletonComponent::getInstance().m_convolutionPassGLTDC,
		11);
	// pre-filter specular environment map
	activateCubemapTexture(
		EnvironmentRenderPassSingletonComponent::getInstance().m_preFilterPassGLTDC,
		12);
	// BRDF look-up table
	activateCubemapTexture(
		EnvironmentRenderPassSingletonComponent::getInstance().m_BRDFLUTGLTDC,
		13);
#endif

	updateUniform(
		LightRenderPassSingletonComponent::getInstance().m_uni_isEmissive,
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
				InnoMath::getDirection(direction::BACKWARD, g_pCoreSystem->getGameSystem()->get<TransformComponent>(l_lightComponent->m_parentEntity)->m_localTransformVector.m_rot);

			auto l_lightColor = l_lightComponent->m_color;

			updateUniform(
				LightRenderPassSingletonComponent::getInstance().m_uni_viewPos,
				l_viewPos.x, l_viewPos.y, l_viewPos.z);

			if (l_lightComponent->m_lightType == lightType::DIRECTIONAL)
			{
				l_pointLightIndexOffset -= 1;
				updateUniform(
					LightRenderPassSingletonComponent::getInstance().m_uni_dirLight_position,
					l_lightPos.x, l_lightPos.y, l_lightPos.z);
				updateUniform(
					LightRenderPassSingletonComponent::getInstance().m_uni_dirLight_direction,
					l_dirLightDirection.x, l_dirLightDirection.y, l_dirLightDirection.z);
				updateUniform(
					LightRenderPassSingletonComponent::getInstance().m_uni_dirLight_color,
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
						LightRenderPassSingletonComponent::getInstance().m_uni_shadowSplitAreas[j],
						l_shadowSplitCorner.x, l_shadowSplitCorner.y, l_shadowSplitCorner.z, l_shadowSplitCorner.w);
				}
			}
			else if (l_lightComponent->m_lightType == lightType::POINT)
			{
				updateUniform(
					LightRenderPassSingletonComponent::getInstance().m_uni_pointLights_position[i + l_pointLightIndexOffset],
					l_lightPos.x, l_lightPos.y, l_lightPos.z);
				updateUniform(
					LightRenderPassSingletonComponent::getInstance().m_uni_pointLights_radius[i + l_pointLightIndexOffset],
					l_lightComponent->m_radius);
				updateUniform(
					LightRenderPassSingletonComponent::getInstance().m_uni_pointLights_color[i + l_pointLightIndexOffset],
					l_lightColor.x, l_lightColor.y, l_lightColor.z);
			}
		}
	}
	// draw light pass rectangle
	auto l_MDC = g_pCoreSystem->getAssetSystem()->getMeshDataComponent(meshShapeType::QUAD);
	drawMesh(l_MDC);

	// 2. draw emissive objects
	glStencilOp(GL_KEEP, GL_KEEP, GL_KEEP);
	glStencilFunc(GL_EQUAL, 0x02, 0xFF);
	glStencilMask(0x00);

	glClear(GL_STENCIL_BUFFER_BIT);

	// copy stencil buffer of emmisive objects from G-Pass
	glBindFramebuffer(GL_READ_FRAMEBUFFER, GeometryRenderPassSingletonComponent::getInstance().m_FBC->m_FBO);
	glBindFramebuffer(GL_DRAW_FRAMEBUFFER, l_FBC->m_FBO);
	glBlitFramebuffer(0, 0, (GLsizei)GLRenderingSystemNS::g_RenderingSystemSingletonComponent->m_renderTargetSize.x, (GLsizei)GLRenderingSystemNS::g_RenderingSystemSingletonComponent->m_renderTargetSize.y, 0, 0, (GLsizei)GLRenderingSystemNS::g_RenderingSystemSingletonComponent->m_renderTargetSize.x, (GLsizei)GLRenderingSystemNS::g_RenderingSystemSingletonComponent->m_renderTargetSize.y, GL_STENCIL_BUFFER_BIT, GL_NEAREST);

	updateUniform(
		LightRenderPassSingletonComponent::getInstance().m_uni_isEmissive,
		true);

	// draw light pass rectangle
	l_MDC = g_pCoreSystem->getAssetSystem()->getMeshDataComponent(meshShapeType::QUAD);
	drawMesh(l_MDC);

	glDisable(GL_STENCIL_TEST);
}

void GLRenderingSystemNS::updateFinalRenderPass()
{
	// sky pass
	glEnable(GL_DEPTH_TEST);
	glDepthFunc(GL_LEQUAL);

	//glEnable(GL_CULL_FACE);
	//glFrontFace(GL_CW);
	//glCullFace(GL_FRONT);

	// bind to framebuffer
	auto l_FBC = GLFinalRenderPassSingletonComponent::getInstance().m_skyPassFBC;
	glBindFramebuffer(GL_FRAMEBUFFER, l_FBC->m_FBO);
	glBindRenderbuffer(GL_RENDERBUFFER, l_FBC->m_RBO);
	glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH24_STENCIL8, (GLsizei)GLRenderingSystemNS::g_RenderingSystemSingletonComponent->m_renderTargetSize.x, (GLsizei)GLRenderingSystemNS::g_RenderingSystemSingletonComponent->m_renderTargetSize.y);
	glViewport(0, 0, (GLsizei)GLRenderingSystemNS::g_RenderingSystemSingletonComponent->m_renderTargetSize.x, (GLsizei)GLRenderingSystemNS::g_RenderingSystemSingletonComponent->m_renderTargetSize.y);

	glClearColor(0.0f, 0.0f, 0.0f, 0.0f);
	glClear(GL_COLOR_BUFFER_BIT);
	glClear(GL_DEPTH_BUFFER_BIT);

	GLRenderingSystemNS::activateShaderProgram(GLFinalRenderPassSingletonComponent::getInstance().m_skyPassSPC);

	auto l_MDC = g_pCoreSystem->getAssetSystem()->getMeshDataComponent(meshShapeType::CUBE);
	auto l_GLMDC = GLRenderingSystemNS::getGLMeshDataComponent(l_MDC->m_parentEntity);

	if (GLRenderingSystemNS::g_GameSystemSingletonComponent->m_CameraComponents.size() > 0)
	{
		auto l_mainCamera = GLRenderingSystemNS::g_GameSystemSingletonComponent->m_CameraComponents[0];
		mat4 p = l_mainCamera->m_projectionMatrix;
		mat4 r =
			InnoMath::getInvertRotationMatrix(
				g_pCoreSystem->getGameSystem()->get<TransformComponent>(l_mainCamera->m_parentEntity)->m_globalTransformVector.m_rot
			);

		GLRenderingSystemNS::updateUniform(
			GLFinalRenderPassSingletonComponent::getInstance().m_skyPass_uni_p,
			p);
		GLRenderingSystemNS::updateUniform(
			GLFinalRenderPassSingletonComponent::getInstance().m_skyPass_uni_r,
			r);
		GLRenderingSystemNS::updateUniform(
			GLFinalRenderPassSingletonComponent::getInstance().m_skyPass_uni_viewportSize,
			GLRenderingSystemNS::g_RenderingSystemSingletonComponent->m_renderTargetSize.x, GLRenderingSystemNS::g_RenderingSystemSingletonComponent->m_renderTargetSize.y);

		auto l_eyePos = g_pCoreSystem->getGameSystem()->get<TransformComponent>(l_mainCamera->m_parentEntity)->m_globalTransformVector.m_pos;
		GLRenderingSystemNS::updateUniform(
			GLFinalRenderPassSingletonComponent::getInstance().m_skyPass_uni_eyePos,
			l_eyePos.x, l_eyePos.y, l_eyePos.z);

		auto l_lightDir = InnoMath::getDirection(
			direction::FORWARD,
			g_pCoreSystem->getGameSystem()->get<TransformComponent>(GLRenderingSystemNS::g_GameSystemSingletonComponent->m_LightComponents[0]->m_parentEntity)->m_localTransformVector.m_rot
		);
		GLRenderingSystemNS::updateUniform(
			GLFinalRenderPassSingletonComponent::getInstance().m_skyPass_uni_lightDir,
			l_lightDir.x, l_lightDir.y, l_lightDir.z);

		GLRenderingSystemNS::drawMesh(l_MDC);
	}

	glDisable(GL_CULL_FACE);
	glDisable(GL_DEPTH_TEST);

	// TAA pass
	TextureDataComponent* l_lastFrameTAATDC;
	GLTextureDataComponent* l_lastFrameTAAGLTDC;
	GLRenderingSystemNS::activateShaderProgram(GLFinalRenderPassSingletonComponent::getInstance().m_TAAPassSPC);

	if (GLRenderingSystemNS::g_RenderingSystemSingletonComponent->m_isTAAPingPass)
	{
		auto l_FBC = GLFinalRenderPassSingletonComponent::getInstance().m_TAAPingPassFBC;
		glBindFramebuffer(GL_FRAMEBUFFER, l_FBC->m_FBO);
		glBindRenderbuffer(GL_RENDERBUFFER, l_FBC->m_RBO);
		glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH24_STENCIL8, (GLsizei)GLRenderingSystemNS::g_RenderingSystemSingletonComponent->m_renderTargetSize.x, (GLsizei)GLRenderingSystemNS::g_RenderingSystemSingletonComponent->m_renderTargetSize.y);
		glViewport(0, 0, (GLsizei)GLRenderingSystemNS::g_RenderingSystemSingletonComponent->m_renderTargetSize.x, (GLsizei)GLRenderingSystemNS::g_RenderingSystemSingletonComponent->m_renderTargetSize.y);

		glClearColor(0.0f, 0.0f, 0.0f, 0.0f);
		glClear(GL_COLOR_BUFFER_BIT);
		glClear(GL_DEPTH_BUFFER_BIT);
		GLRenderingSystemNS::activate2DTexture(
			LightRenderPassSingletonComponent::getInstance().m_GLTDC,
			0);
		GLRenderingSystemNS::activate2DTexture(
			GLFinalRenderPassSingletonComponent::getInstance().m_TAAPongPassGLTDC,
			1);
		GLRenderingSystemNS::activate2DTexture(
			GeometryRenderPassSingletonComponent::getInstance().m_GLTDCs[3],
			2);
		GLRenderingSystemNS::g_RenderingSystemSingletonComponent->m_isTAAPingPass = false;
		l_lastFrameTAATDC = GLFinalRenderPassSingletonComponent::getInstance().m_TAAPingPassTDC;
		l_lastFrameTAAGLTDC = GLFinalRenderPassSingletonComponent::getInstance().m_TAAPingPassGLTDC;
	}
	else
	{
		auto l_FBC = GLFinalRenderPassSingletonComponent::getInstance().m_TAAPongPassFBC;
		glBindFramebuffer(GL_FRAMEBUFFER, l_FBC->m_FBO);
		glBindRenderbuffer(GL_RENDERBUFFER, l_FBC->m_RBO);
		glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH24_STENCIL8, (GLsizei)GLRenderingSystemNS::g_RenderingSystemSingletonComponent->m_renderTargetSize.x, (GLsizei)GLRenderingSystemNS::g_RenderingSystemSingletonComponent->m_renderTargetSize.y);
		glViewport(0, 0, (GLsizei)GLRenderingSystemNS::g_RenderingSystemSingletonComponent->m_renderTargetSize.x, (GLsizei)GLRenderingSystemNS::g_RenderingSystemSingletonComponent->m_renderTargetSize.y);

		glClearColor(0.0f, 0.0f, 0.0f, 0.0f);
		glClear(GL_COLOR_BUFFER_BIT);
		glClear(GL_DEPTH_BUFFER_BIT);
		GLRenderingSystemNS::activate2DTexture(
			LightRenderPassSingletonComponent::getInstance().m_GLTDC,
			0);
		GLRenderingSystemNS::activate2DTexture(
			GLFinalRenderPassSingletonComponent::getInstance().m_TAAPingPassGLTDC,
			1);
		GLRenderingSystemNS::activate2DTexture(
			GeometryRenderPassSingletonComponent::getInstance().m_GLTDCs[3],
			2);
		GLRenderingSystemNS::g_RenderingSystemSingletonComponent->m_isTAAPingPass = true;
		l_lastFrameTAATDC = GLFinalRenderPassSingletonComponent::getInstance().m_TAAPongPassTDC;
		l_lastFrameTAAGLTDC = GLFinalRenderPassSingletonComponent::getInstance().m_TAAPongPassGLTDC;
	}
	GLRenderingSystemNS::updateUniform(
		GLFinalRenderPassSingletonComponent::getInstance().m_TAAPass_uni_renderTargetSize,
		GLRenderingSystemNS::g_RenderingSystemSingletonComponent->m_renderTargetSize.x, GLRenderingSystemNS::g_RenderingSystemSingletonComponent->m_renderTargetSize.y);

	l_MDC = g_pCoreSystem->getAssetSystem()->getMeshDataComponent(meshShapeType::QUAD);
	l_GLMDC = GLRenderingSystemNS::getGLMeshDataComponent(l_MDC->m_parentEntity);

	GLRenderingSystemNS::drawMesh(l_MDC);

	// bloom extract pass
	l_FBC = GLFinalRenderPassSingletonComponent::getInstance().m_bloomExtractPassFBC;
	glBindFramebuffer(GL_FRAMEBUFFER, l_FBC->m_FBO);
	glBindRenderbuffer(GL_RENDERBUFFER, l_FBC->m_RBO);
	glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH24_STENCIL8, (GLsizei)GLRenderingSystemNS::g_RenderingSystemSingletonComponent->m_renderTargetSize.x, (GLsizei)GLRenderingSystemNS::g_RenderingSystemSingletonComponent->m_renderTargetSize.y);
	glViewport(0, 0, (GLsizei)GLRenderingSystemNS::g_RenderingSystemSingletonComponent->m_renderTargetSize.x, (GLsizei)GLRenderingSystemNS::g_RenderingSystemSingletonComponent->m_renderTargetSize.y);

	glClearColor(0.0f, 0.0f, 0.0f, 0.0f);
	glClear(GL_COLOR_BUFFER_BIT);
	glClear(GL_DEPTH_BUFFER_BIT);
	GLRenderingSystemNS::activateShaderProgram(GLFinalRenderPassSingletonComponent::getInstance().m_bloomExtractPassSPC);

	// 1. extract bright part from TAA pass
	GLRenderingSystemNS::activate2DTexture(l_lastFrameTAAGLTDC, 0);

	drawMesh(l_MDC);

	// bloom blur pass
	activateShaderProgram(GLFinalRenderPassSingletonComponent::getInstance().m_bloomBlurPassSPC);

	bool l_isPing = true;
	bool l_isFirstIteration = true;

	for (size_t i = 0; i < 5; i++)
	{
		if (l_isPing)
		{
			auto l_FBC = GLFinalRenderPassSingletonComponent::getInstance().m_bloomBlurPingPassFBC;
			glBindFramebuffer(GL_FRAMEBUFFER, l_FBC->m_FBO);
			glBindRenderbuffer(GL_RENDERBUFFER, l_FBC->m_RBO);
			glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH24_STENCIL8, (GLsizei)GLRenderingSystemNS::g_RenderingSystemSingletonComponent->m_renderTargetSize.x, (GLsizei)GLRenderingSystemNS::g_RenderingSystemSingletonComponent->m_renderTargetSize.y);
			glViewport(0, 0, (GLsizei)GLRenderingSystemNS::g_RenderingSystemSingletonComponent->m_renderTargetSize.x, (GLsizei)GLRenderingSystemNS::g_RenderingSystemSingletonComponent->m_renderTargetSize.y);

			glClearColor(0.0f, 0.0f, 0.0f, 0.0f);
			glClear(GL_COLOR_BUFFER_BIT);
			glClear(GL_DEPTH_BUFFER_BIT);

			GLRenderingSystemNS::updateUniform(
				GLFinalRenderPassSingletonComponent::getInstance().m_bloomBlurPass_uni_horizontal,
				true);

			if (l_isFirstIteration)
			{
				GLRenderingSystemNS::activate2DTexture(
					GLFinalRenderPassSingletonComponent::getInstance().m_bloomExtractPassGLTDC,
					0);
				l_isFirstIteration = false;
			}
			else
			{
				GLRenderingSystemNS::activate2DTexture(
					GLFinalRenderPassSingletonComponent::getInstance().m_bloomBlurPongPassGLTDC,
					0);
			}

			drawMesh(l_MDC);

			l_isPing = false;
		}
		else
		{
			auto l_FBC = GLFinalRenderPassSingletonComponent::getInstance().m_bloomBlurPongPassFBC;
			glBindFramebuffer(GL_FRAMEBUFFER, l_FBC->m_FBO);
			glBindRenderbuffer(GL_RENDERBUFFER, l_FBC->m_RBO);
			glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH24_STENCIL8, (GLsizei)GLRenderingSystemNS::g_RenderingSystemSingletonComponent->m_renderTargetSize.x, (GLsizei)GLRenderingSystemNS::g_RenderingSystemSingletonComponent->m_renderTargetSize.y);
			glViewport(0, 0, (GLsizei)GLRenderingSystemNS::g_RenderingSystemSingletonComponent->m_renderTargetSize.x, (GLsizei)GLRenderingSystemNS::g_RenderingSystemSingletonComponent->m_renderTargetSize.y);

			glClearColor(0.0f, 0.0f, 0.0f, 0.0f);
			glClear(GL_COLOR_BUFFER_BIT);
			glClear(GL_DEPTH_BUFFER_BIT);

			GLRenderingSystemNS::updateUniform(
				GLFinalRenderPassSingletonComponent::getInstance().m_bloomBlurPass_uni_horizontal,
				false);

			GLRenderingSystemNS::activate2DTexture(
				GLFinalRenderPassSingletonComponent::getInstance().m_bloomBlurPingPassGLTDC,
				0);

			drawMesh(l_MDC);

			l_isPing = true;
		}
	}

	// motion blur pass
	l_FBC = GLFinalRenderPassSingletonComponent::getInstance().m_motionBlurPassFBC;
	glBindFramebuffer(GL_FRAMEBUFFER, l_FBC->m_FBO);
	glBindRenderbuffer(GL_RENDERBUFFER, l_FBC->m_RBO);
	glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH24_STENCIL8, (GLsizei)GLRenderingSystemNS::g_RenderingSystemSingletonComponent->m_renderTargetSize.x, (GLsizei)GLRenderingSystemNS::g_RenderingSystemSingletonComponent->m_renderTargetSize.y);
	glViewport(0, 0, (GLsizei)GLRenderingSystemNS::g_RenderingSystemSingletonComponent->m_renderTargetSize.x, (GLsizei)GLRenderingSystemNS::g_RenderingSystemSingletonComponent->m_renderTargetSize.y);

	glClearColor(0.0f, 0.0f, 0.0f, 0.0f);
	glClear(GL_COLOR_BUFFER_BIT);
	glClear(GL_DEPTH_BUFFER_BIT);

	activateShaderProgram(GLFinalRenderPassSingletonComponent::getInstance().m_motionBlurPassSPC);

	GLRenderingSystemNS::activate2DTexture(
		GeometryRenderPassSingletonComponent::getInstance().m_GLTDCs[3],
		0);
	activate2DTexture(l_lastFrameTAAGLTDC, 1);

	drawMesh(l_MDC);

	// billboard pass
	glEnable(GL_DEPTH_TEST);
	glDepthFunc(GL_LESS);

	l_FBC = GLFinalRenderPassSingletonComponent::getInstance().m_billboardPassFBC;
	glBindFramebuffer(GL_FRAMEBUFFER, l_FBC->m_FBO);
	glBindRenderbuffer(GL_RENDERBUFFER, l_FBC->m_RBO);
	glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH24_STENCIL8, (GLsizei)GLRenderingSystemNS::g_RenderingSystemSingletonComponent->m_renderTargetSize.x, (GLsizei)GLRenderingSystemNS::g_RenderingSystemSingletonComponent->m_renderTargetSize.y);
	glViewport(0, 0, (GLsizei)GLRenderingSystemNS::g_RenderingSystemSingletonComponent->m_renderTargetSize.x, (GLsizei)GLRenderingSystemNS::g_RenderingSystemSingletonComponent->m_renderTargetSize.y);

	glClearColor(0.0f, 0.0f, 0.0f, 0.0f);
	glClear(GL_COLOR_BUFFER_BIT);
	glClear(GL_DEPTH_BUFFER_BIT);

	// copy depth buffer from G-Pass
	glBindFramebuffer(GL_READ_FRAMEBUFFER, GeometryRenderPassSingletonComponent::getInstance().m_FBC->m_FBO);
	glBindFramebuffer(GL_DRAW_FRAMEBUFFER, l_FBC->m_FBO);
	glBlitFramebuffer(0, 0, (GLsizei)GLRenderingSystemNS::g_RenderingSystemSingletonComponent->m_renderTargetSize.x, (GLsizei)GLRenderingSystemNS::g_RenderingSystemSingletonComponent->m_renderTargetSize.y, 0, 0, (GLsizei)GLRenderingSystemNS::g_RenderingSystemSingletonComponent->m_renderTargetSize.x, (GLsizei)GLRenderingSystemNS::g_RenderingSystemSingletonComponent->m_renderTargetSize.y, GL_DEPTH_BUFFER_BIT, GL_NEAREST);
	glBlitFramebuffer(0, 0, (GLsizei)GLRenderingSystemNS::g_RenderingSystemSingletonComponent->m_renderTargetSize.x, (GLsizei)GLRenderingSystemNS::g_RenderingSystemSingletonComponent->m_renderTargetSize.y, 0, 0, (GLsizei)GLRenderingSystemNS::g_RenderingSystemSingletonComponent->m_renderTargetSize.x, (GLsizei)GLRenderingSystemNS::g_RenderingSystemSingletonComponent->m_renderTargetSize.y, GL_STENCIL_BUFFER_BIT, GL_NEAREST);

	activateShaderProgram(GLFinalRenderPassSingletonComponent::getInstance().m_billboardPassSPC);

	if (GLRenderingSystemNS::g_GameSystemSingletonComponent->m_CameraComponents.size() > 0)
	{
		mat4 p = GLRenderingSystemNS::g_GameSystemSingletonComponent->m_CameraComponents[0]->m_projectionMatrix;
		mat4 r =
			InnoMath::getInvertRotationMatrix(
				g_pCoreSystem->getGameSystem()->get<TransformComponent>(GLRenderingSystemNS::g_GameSystemSingletonComponent->m_CameraComponents[0]->m_parentEntity)->m_globalTransformVector.m_rot
			);
		mat4 t =
			InnoMath::getInvertTranslationMatrix(
				g_pCoreSystem->getGameSystem()->get<TransformComponent>(GLRenderingSystemNS::g_GameSystemSingletonComponent->m_CameraComponents[0]->m_parentEntity)->m_globalTransformVector.m_pos
			);

		GLRenderingSystemNS::updateUniform(
			GLFinalRenderPassSingletonComponent::getInstance().m_billboardPass_uni_p,
			p);
		GLRenderingSystemNS::updateUniform(
			GLFinalRenderPassSingletonComponent::getInstance().m_billboardPass_uni_r,
			r);
		GLRenderingSystemNS::updateUniform(
			GLFinalRenderPassSingletonComponent::getInstance().m_billboardPass_uni_t,
			t);
	}
	if (GLRenderingSystemNS::g_GameSystemSingletonComponent->m_VisibleComponents.size() > 0)
	{
		// draw each visibleComponent
		for (auto& l_visibleComponent : GLRenderingSystemNS::g_GameSystemSingletonComponent->m_VisibleComponents)
		{
			if (l_visibleComponent->m_visiblilityType == visiblilityType::BILLBOARD)
			{
				auto l_GlobalPos = g_pCoreSystem->getGameSystem()->get<TransformComponent>(l_visibleComponent->m_parentEntity)->m_globalTransformVector.m_pos;
				auto l_GlobalCameraPos = g_pCoreSystem->getGameSystem()->get<TransformComponent>(GLRenderingSystemNS::g_GameSystemSingletonComponent->m_CameraComponents[0]->m_parentEntity)->m_globalTransformVector.m_pos;

				GLRenderingSystemNS::updateUniform(
					GLFinalRenderPassSingletonComponent::getInstance().m_billboardPass_uni_pos,
					l_GlobalPos.x, l_GlobalPos.y, l_GlobalPos.z);
				//GLRenderingSystemNS::updateUniform(
				//	GLFinalRenderPassSingletonComponent::getInstance().m_billboardPass_uni_albedo,
				//	l_visibleComponent->m_albedo.x, l_visibleComponent->m_albedo.y, l_visibleComponent->m_albedo.z);
				auto l_distanceToCamera = (l_GlobalCameraPos - l_GlobalPos).length();
				if (l_distanceToCamera > 1.0f)
				{
					GLRenderingSystemNS::updateUniform(
						GLFinalRenderPassSingletonComponent::getInstance().m_billboardPass_uni_size,
						(1.0f / l_distanceToCamera) * (9.0f / 16.0f), (1.0f / l_distanceToCamera));
				}
				else
				{
					GLRenderingSystemNS::updateUniform(
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

	// debugger pass
	glEnable(GL_DEPTH_TEST);
	glDepthFunc(GL_LESS);

	l_FBC = GLFinalRenderPassSingletonComponent::getInstance().m_debuggerPassFBC;
	glBindFramebuffer(GL_FRAMEBUFFER, l_FBC->m_FBO);
	glBindRenderbuffer(GL_RENDERBUFFER, l_FBC->m_RBO);
	glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH24_STENCIL8, (GLsizei)GLRenderingSystemNS::g_RenderingSystemSingletonComponent->m_renderTargetSize.x, (GLsizei)GLRenderingSystemNS::g_RenderingSystemSingletonComponent->m_renderTargetSize.y);
	glViewport(0, 0, (GLsizei)GLRenderingSystemNS::g_RenderingSystemSingletonComponent->m_renderTargetSize.x, (GLsizei)GLRenderingSystemNS::g_RenderingSystemSingletonComponent->m_renderTargetSize.y);

	// copy depth buffer from G-Pass
	glBindFramebuffer(GL_READ_FRAMEBUFFER, GeometryRenderPassSingletonComponent::getInstance().m_FBC->m_FBO);
	glBindFramebuffer(GL_DRAW_FRAMEBUFFER, l_FBC->m_FBO);
	glBlitFramebuffer(0, 0, (GLsizei)GLRenderingSystemNS::g_RenderingSystemSingletonComponent->m_renderTargetSize.x, (GLsizei)GLRenderingSystemNS::g_RenderingSystemSingletonComponent->m_renderTargetSize.y, 0, 0, (GLsizei)GLRenderingSystemNS::g_RenderingSystemSingletonComponent->m_renderTargetSize.x, (GLsizei)GLRenderingSystemNS::g_RenderingSystemSingletonComponent->m_renderTargetSize.y, GL_DEPTH_BUFFER_BIT, GL_NEAREST);
	glBlitFramebuffer(0, 0, (GLsizei)GLRenderingSystemNS::g_RenderingSystemSingletonComponent->m_renderTargetSize.x, (GLsizei)GLRenderingSystemNS::g_RenderingSystemSingletonComponent->m_renderTargetSize.y, 0, 0, (GLsizei)GLRenderingSystemNS::g_RenderingSystemSingletonComponent->m_renderTargetSize.x, (GLsizei)GLRenderingSystemNS::g_RenderingSystemSingletonComponent->m_renderTargetSize.y, GL_STENCIL_BUFFER_BIT, GL_NEAREST);

	glClear(GL_COLOR_BUFFER_BIT);
	glClear(GL_DEPTH_BUFFER_BIT);

	activateShaderProgram(GLFinalRenderPassSingletonComponent::getInstance().m_debuggerPassSPC);

	if (GLRenderingSystemNS::g_GameSystemSingletonComponent->m_CameraComponents.size() > 0)
	{
		auto l_mainCamera = GLRenderingSystemNS::g_GameSystemSingletonComponent->m_CameraComponents[0];
		auto l_mainCameraTransformComponent = g_pCoreSystem->getGameSystem()->get<TransformComponent>(l_mainCamera->m_parentEntity);

		mat4 p = l_mainCamera->m_projectionMatrix;
		mat4 r =
			InnoMath::getInvertRotationMatrix(
				l_mainCameraTransformComponent->m_globalTransformVector.m_rot
			);
		mat4 t =
			InnoMath::getInvertTranslationMatrix(
				l_mainCameraTransformComponent->m_globalTransformVector.m_pos
			);

		updateUniform(
			GLFinalRenderPassSingletonComponent::getInstance().m_debuggerPass_uni_p,
			p);
		updateUniform(
			GLFinalRenderPassSingletonComponent::getInstance().m_debuggerPass_uni_r,
			r);
		updateUniform(
			GLFinalRenderPassSingletonComponent::getInstance().m_debuggerPass_uni_t,
			t);

		for (auto& l_cameraComponent : GLRenderingSystemNS::g_GameSystemSingletonComponent->m_CameraComponents)
		{
			// draw frustum for cameraComponent
			if (l_cameraComponent->m_drawFrustum)
			{
				auto l_cameraLocalMat = mat4();
				l_cameraLocalMat.initializeToIdentityMatrix();
				updateUniform(
					GLFinalRenderPassSingletonComponent::getInstance().m_debuggerPass_uni_m,
					l_cameraLocalMat);
				auto l_mesh = g_pCoreSystem->getAssetSystem()->getMeshDataComponent(l_cameraComponent->m_FrustumMeshID);
				drawMesh(l_mesh);
			}
			// draw AABB of frustum for cameraComponent
			if (l_cameraComponent->m_drawAABB)
			{
				auto l_cameraLocalMat = mat4();
				l_cameraLocalMat.initializeToIdentityMatrix();
				updateUniform(
					GLFinalRenderPassSingletonComponent::getInstance().m_debuggerPass_uni_m,
					l_cameraLocalMat);
				auto l_mesh = g_pCoreSystem->getAssetSystem()->getMeshDataComponent(l_cameraComponent->m_AABBMeshID);
				drawMesh(l_mesh);
			}
		}
	}
	if (GLRenderingSystemNS::g_GameSystemSingletonComponent->m_LightComponents.size() > 0)
	{
		// draw AABB for lightComponent
		for (auto& l_lightComponent : GLRenderingSystemNS::g_GameSystemSingletonComponent->m_LightComponents)
		{
			if (l_lightComponent->m_drawAABB)
			{
				updateUniform(
					GLFinalRenderPassSingletonComponent::getInstance().m_debuggerPass_uni_m,
					g_pCoreSystem->getGameSystem()->get<TransformComponent>(l_lightComponent->m_parentEntity)->m_globalTransformMatrix.m_transformationMat);
				for (auto l_AABBMeshID : l_lightComponent->m_AABBMeshIDs)
				{
					auto l_mesh = g_pCoreSystem->getAssetSystem()->getMeshDataComponent(l_AABBMeshID);
					drawMesh(l_mesh);
				}
			}
		}
	}
	for (auto& l_visibleComponent : GLRenderingSystemNS::g_RenderingSystemSingletonComponent->m_inFrustumVisibleComponents)
	{
		if (l_visibleComponent->m_visiblilityType == visiblilityType::STATIC_MESH && l_visibleComponent->m_drawAABB)
		{
			updateUniform(
				GLFinalRenderPassSingletonComponent::getInstance().m_debuggerPass_uni_m,
				g_pCoreSystem->getGameSystem()->get<TransformComponent>(l_visibleComponent->m_parentEntity)->m_globalTransformMatrix.m_transformationMat);

			// draw each graphic data of visibleComponent
			// @TODO: WIP
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

	glDisable(GL_DEPTH_TEST);

	// final blend pass
	l_FBC = GLFinalRenderPassSingletonComponent::getInstance().m_finalBlendPassFBC;
	glBindFramebuffer(GL_FRAMEBUFFER, l_FBC->m_FBO);
	glBindRenderbuffer(GL_RENDERBUFFER, l_FBC->m_RBO);
	glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH24_STENCIL8, (GLsizei)GLRenderingSystemNS::g_RenderingSystemSingletonComponent->m_renderTargetSize.x, (GLsizei)GLRenderingSystemNS::g_RenderingSystemSingletonComponent->m_renderTargetSize.y);
	glViewport(0, 0, (GLsizei)GLRenderingSystemNS::g_RenderingSystemSingletonComponent->m_renderTargetSize.x, (GLsizei)GLRenderingSystemNS::g_RenderingSystemSingletonComponent->m_renderTargetSize.y);

	glClearColor(0.0f, 0.0f, 0.0f, 0.0f);
	glClear(GL_COLOR_BUFFER_BIT);
	glClear(GL_DEPTH_BUFFER_BIT);

	GLRenderingSystemNS::activateShaderProgram(GLFinalRenderPassSingletonComponent::getInstance().m_finalBlendPassSPC);

	// motion blur pass rendering target
	GLRenderingSystemNS::activate2DTexture(
		GLFinalRenderPassSingletonComponent::getInstance().m_motionBlurPassGLTDC,
		0);
	// sky pass rendering target
	GLRenderingSystemNS::activate2DTexture(
		GLFinalRenderPassSingletonComponent::getInstance().m_skyPassGLTDC,
		1);
	// bloom pass rendering target
	GLRenderingSystemNS::activate2DTexture(
		GLFinalRenderPassSingletonComponent::getInstance().m_bloomBlurPongPassGLTDC,
		2);
	// billboard pass rendering target
	GLRenderingSystemNS::activate2DTexture(
		GLFinalRenderPassSingletonComponent::getInstance().m_billboardPassGLTDC,
		3);
	// debugger pass rendering target
	GLRenderingSystemNS::activate2DTexture(
		GLFinalRenderPassSingletonComponent::getInstance().m_debuggerPassGLTDC,
		4);

	//// draw final pass rectangle
	GLRenderingSystemNS::drawMesh(l_MDC);

	//// draw again for game build
	glBindFramebuffer(GL_FRAMEBUFFER, 0);
	glBindRenderbuffer(GL_RENDERBUFFER, 0);

	GLRenderingSystemNS::drawMesh(l_MDC);
}

bool GLRenderingSystem::terminate()
{
	GLRenderingSystemNS::m_objectStatus = objectStatus::SHUTDOWN;
	g_pCoreSystem->getLogSystem()->printLog("GLRenderingSystem has been terminated.");
	return true;
}

objectStatus GLRenderingSystem::getStatus()
{
	return GLRenderingSystemNS::m_objectStatus;
}

GLuint GLRenderingSystemNS::getUniformLocation(GLuint shaderProgram, const std::string & uniformName)
{
	// @TODO: UBO
	glUseProgram(shaderProgram);
	int uniformLocation = glGetUniformLocation(shaderProgram, uniformName.c_str());
	if (uniformLocation == 0xFFFFFFFF)
	{
		g_pCoreSystem->getLogSystem()->printLog("GLRenderingSystem: innoShader: Error: Uniform lost: " + uniformName);
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
	if (TDC->m_textureType == textureType::ENVIRONMENT_CAPTURE || TDC->m_textureType == textureType::ENVIRONMENT_CONVOLUTION || TDC->m_textureType == textureType::ENVIRONMENT_PREFILTER)
	{
		glBindTexture(GL_TEXTURE_CUBE_MAP, GLTDC->m_TAO);
		glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0 + colorAttachmentIndex, GL_TEXTURE_CUBE_MAP_POSITIVE_X + textureIndex, GLTDC->m_TAO, mipLevel);
	}
	else if (TDC->m_textureType == textureType::SHADOWMAP)
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
		if (MDC->m_objectStatus == objectStatus::ALIVE && l_GLMDC->m_objectStatus == objectStatus::ALIVE)
		{
			glBindVertexArray(l_GLMDC->m_VAO);
			switch (MDC->m_meshDrawMethod)
			{
			case meshDrawMethod::TRIANGLE: glDrawElements(GL_TRIANGLES, (GLsizei)MDC->m_indicesSize, GL_UNSIGNED_INT, 0); break;
			case meshDrawMethod::TRIANGLE_STRIP: glDrawElements(GL_TRIANGLE_STRIP, (GLsizei)MDC->m_indicesSize, GL_UNSIGNED_INT, 0); break;
			default:
				break;
			}
		}
	}
}

void GLRenderingSystemNS::activateTexture(TextureDataComponent * TDC, int activateIndex)
{
	auto l_GLTDC = getGLTextureDataComponent(TDC->m_parentEntity);
	if (l_GLTDC)
	{
		if (TDC->m_textureType == textureType::ENVIRONMENT_CAPTURE || TDC->m_textureType == textureType::ENVIRONMENT_CONVOLUTION || TDC->m_textureType == textureType::ENVIRONMENT_PREFILTER)
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