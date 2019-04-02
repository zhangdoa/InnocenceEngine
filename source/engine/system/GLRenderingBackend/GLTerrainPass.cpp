#include "GLTerrainPass.h"
#include "GLOpaquePass.h"

#include "GLRenderingSystemUtilities.h"
#include "../../component/GLRenderingSystemComponent.h"

using namespace GLRenderingSystemNS;

#include "../ICoreSystem.h"

extern ICoreSystem* g_pCoreSystem;

INNO_PRIVATE_SCOPE GLTerrainPass
{
	void initializeShaders();
	void bindUniformLocations(GLShaderProgramComponent* rhs);

	EntityID m_entityID;

	GLRenderPassComponent* m_GLRPC;

	GLShaderProgramComponent* m_GLSPC;

	ShaderFilePaths m_shaderFilePaths = { "GL//terrainPassVertex.sf" , "", "GL//terrainPassFragment.sf" };

	GLuint m_uni_p_camera;
	GLuint m_uni_r_camera;
	GLuint m_uni_t_camera;
	GLuint m_uni_m;

	std::vector<std::string> m_TextureUniformNames =
	{
		"uni_albedoTexture",
		"uni_heightTexture",
	};

	std::vector<vec4> m_terrainNoise;

	TextureDataComponent* m_terrainNoiseTDC;
	GLTextureDataComponent* m_terrainNoiseGLTDC;
}

bool GLTerrainPass::initialize()
{
	m_entityID = InnoMath::createEntityID();

	m_GLRPC = addGLRenderPassComponent(1, GLRenderingSystemComponent::get().deferredPassFBDesc, GLRenderingSystemComponent::get().deferredPassTextureDesc);

	initializeShaders();

	auto l_textureSize = 256;

	std::uniform_real_distribution<GLfloat> randomFloats(0.0f, 1.0f);
	std::default_random_engine generator;

	m_terrainNoise.reserve(l_textureSize * l_textureSize);

	for (size_t i = 0; i < m_terrainNoise.capacity(); i++)
	{
		// random on z-axis
		auto noise = vec4(0.0f, 0.0f, randomFloats(generator) * 2.0f - 1.0f, 0.0f);
		m_terrainNoise.emplace_back(noise);
	}

	m_terrainNoiseTDC = g_pCoreSystem->getAssetSystem()->addTextureDataComponent();

	m_terrainNoiseTDC->m_textureDataDesc.textureSamplerType = TextureSamplerType::SAMPLER_2D;
	m_terrainNoiseTDC->m_textureDataDesc.textureUsageType = TextureUsageType::RENDER_TARGET;
	m_terrainNoiseTDC->m_textureDataDesc.textureColorComponentsFormat = TextureColorComponentsFormat::RGB32F;
	m_terrainNoiseTDC->m_textureDataDesc.texturePixelDataFormat = TexturePixelDataFormat::RGBA;
	m_terrainNoiseTDC->m_textureDataDesc.textureMinFilterMethod = TextureFilterMethod::NEAREST;
	m_terrainNoiseTDC->m_textureDataDesc.textureMagFilterMethod = TextureFilterMethod::NEAREST;
	m_terrainNoiseTDC->m_textureDataDesc.textureWrapMethod = TextureWrapMethod::REPEAT;
	m_terrainNoiseTDC->m_textureDataDesc.textureWidth = l_textureSize;
	m_terrainNoiseTDC->m_textureDataDesc.textureHeight = l_textureSize;
	m_terrainNoiseTDC->m_textureDataDesc.texturePixelDataType = TexturePixelDataType::FLOAT;

	std::vector<float> l_pixelBuffer;
	auto l_containerSize = m_terrainNoise.size() * 4;
	l_pixelBuffer.reserve(l_containerSize);

	std::for_each(m_terrainNoise.begin(), m_terrainNoise.end(), [&](vec4 val)
	{
		l_pixelBuffer.emplace_back(val.x);
		l_pixelBuffer.emplace_back(val.y);
		l_pixelBuffer.emplace_back(val.z);
		l_pixelBuffer.emplace_back(val.w);
	});

	m_terrainNoiseTDC->m_textureData.emplace_back(&l_pixelBuffer[0]);

	m_terrainNoiseGLTDC = generateGLTextureDataComponent(m_terrainNoiseTDC);

	return true;
}

void GLTerrainPass::initializeShaders()
{
	// shader programs and shaders
	auto rhs = addGLShaderProgramComponent(m_entityID);

	initializeGLShaderProgramComponent(rhs, m_shaderFilePaths);

	bindUniformLocations(rhs);

	m_GLSPC = rhs;
}

void GLTerrainPass::bindUniformLocations(GLShaderProgramComponent* rhs)
{
	m_uni_p_camera = getUniformLocation(
		rhs->m_program,
		"uni_p_camera");
	m_uni_r_camera = getUniformLocation(
		rhs->m_program,
		"uni_r_camera");
	m_uni_t_camera = getUniformLocation(
		rhs->m_program,
		"uni_t_camera");
	m_uni_m = getUniformLocation(
		rhs->m_program,
		"uni_m");

	updateTextureUniformLocations(rhs->m_program, m_TextureUniformNames);
}

bool GLTerrainPass::update()
{
	auto l_renderingConfig = g_pCoreSystem->getVisionSystem()->getRenderingFrontend()->getRenderingConfig();
	auto l_cameraDataPack = g_pCoreSystem->getVisionSystem()->getRenderingFrontend()->getCameraDataPack();

	if (l_renderingConfig.drawTerrain)
	{
		glEnable(GL_DEPTH_TEST);

		activateRenderPass(m_GLRPC);

		copyDepthBuffer(GLOpaquePass::getGLRPC(), m_GLRPC);

		activateShaderProgram(m_GLSPC);

		mat4 m = InnoMath::generateIdentityMatrix<float>();

		updateUniform(
			m_uni_p_camera,
			l_cameraDataPack.p_Original);
		updateUniform(
			m_uni_r_camera,
			l_cameraDataPack.r);
		updateUniform(
			m_uni_t_camera,
			l_cameraDataPack.t);
		updateUniform(
			m_uni_m,
			m);

		auto l_MDC = g_pCoreSystem->getAssetSystem()->getMeshDataComponent(MeshShapeType::TERRAIN);
		activateTexture(GLRenderingSystemComponent::get().m_basicAlbedoGLTDC, 0);
		activateTexture(m_terrainNoiseGLTDC, 1);

		drawMesh(l_MDC);

		glDisable(GL_DEPTH_TEST);
	}
	else
	{
		cleanFBC(m_GLRPC);
	}
	return true;
}

bool GLTerrainPass::resize()
{
	return true;
}

bool GLTerrainPass::reloadShader()
{
	return true;
}

GLRenderPassComponent * GLTerrainPass::getGLRPC()
{
	return m_GLRPC;
}