#include "GLSSAONoisePass.h"
#include "GLOpaquePass.h"

#include "GLRenderingSystemUtilities.h"
#include "../../component/GLRenderingSystemComponent.h"

using namespace GLRenderingSystemNS;

#include "../ICoreSystem.h"

extern ICoreSystem* g_pCoreSystem;

INNO_PRIVATE_SCOPE GLSSAONoisePass
{
	void initializeShaders();
	void bindUniformLocations(GLShaderProgramComponent* rhs);

	void generateSSAONoiseTexture();

	EntityID m_entityID;

	GLRenderPassComponent* m_GLRPC;

	GLShaderProgramComponent* m_GLSPC;

	ShaderFilePaths m_shaderFilePaths = { "GL//SSAONoisePassVertex.sf" , "", "GL//SSAONoisePassFragment.sf" };

	GLuint m_uni_p;
	GLuint m_uni_r;
	GLuint m_uni_t;

	std::vector<GLuint> m_uni_samples;

	std::vector<std::string> m_uniformNames =
	{
		"uni_Position",
		"uni_Normal",
		"uni_texNoise",
	};

	std::vector<vec4> m_SSAOKernel;
	std::vector<vec4> m_SSAONoise;

	TextureDataComponent* m_SSAONoiseTDC;
	GLTextureDataComponent* m_SSAONoiseGLTDC;

	CameraDataPack m_cameraDataPack;
}

bool GLSSAONoisePass::initialize()
{
	m_entityID = InnoMath::createEntityID();

	m_GLRPC = addGLRenderPassComponent(1, GLRenderingSystemComponent::get().deferredPassFBDesc, GLRenderingSystemComponent::get().deferredPassTextureDesc);

	initializeShaders();

	generateSSAONoiseTexture();

	return true;
}

void GLSSAONoisePass::initializeShaders()
{
	// shader programs and shaders
	auto rhs = addGLShaderProgramComponent(m_entityID);

	initializeGLShaderProgramComponent(rhs, m_shaderFilePaths);

	bindUniformLocations(rhs);

	m_GLSPC = rhs;
}

void GLSSAONoisePass::bindUniformLocations(GLShaderProgramComponent * rhs)
{
	updateTextureUniformLocations(rhs->m_program, m_uniformNames);

	m_uni_p = getUniformLocation(
		rhs->m_program,
		"uni_p");
	m_uni_r = getUniformLocation(
		rhs->m_program,
		"uni_r");
	m_uni_t = getUniformLocation(
		rhs->m_program,
		"uni_t");

	for (size_t i = 0; i < 64; i++)
	{
		m_uni_samples.emplace_back(
			getUniformLocation(rhs->m_program, "uni_samples[" + std::to_string(i) + "]")
		);
	}
}

void GLSSAONoisePass::generateSSAONoiseTexture()
{
	std::uniform_real_distribution<GLfloat> randomFloats(0.0f, 1.0f);
	std::default_random_engine generator;
	for (unsigned int i = 0; i < 64; ++i)
	{
		auto sample = vec4(randomFloats(generator) * 2.0f - 1.0f, randomFloats(generator) * 2.0f - 1.0f, randomFloats(generator), 0.0f);
		sample = sample.normalize();
		sample = sample * randomFloats(generator);
		float scale = float(i) / 64.0f;

		// scale samples s.t. they're more aligned to center of kernel
		auto alpha = scale * scale;
		scale = 0.1f + 0.9f * alpha;
		sample = sample * scale;

		m_SSAOKernel.push_back(sample);
	}

	auto l_textureSize = 4;

	m_SSAONoise.reserve(l_textureSize * l_textureSize);

	for (size_t i = 0; i < m_SSAONoise.capacity(); i++)
	{
		// rotate around z-axis (in tangent space)
		auto noise = vec4(randomFloats(generator) * 2.0f - 1.0f, randomFloats(generator) * 2.0f - 1.0f, 0.0f, 0.0f);
		noise = noise.normalize();
		m_SSAONoise.push_back(noise);
	}

	m_SSAONoiseTDC = g_pCoreSystem->getAssetSystem()->addTextureDataComponent();

	m_SSAONoiseTDC->m_textureDataDesc.textureSamplerType = TextureSamplerType::SAMPLER_2D;
	m_SSAONoiseTDC->m_textureDataDesc.textureUsageType = TextureUsageType::RENDER_TARGET;
	m_SSAONoiseTDC->m_textureDataDesc.textureColorComponentsFormat = TextureColorComponentsFormat::RGB32F;
	m_SSAONoiseTDC->m_textureDataDesc.texturePixelDataFormat = TexturePixelDataFormat::RGBA;
	m_SSAONoiseTDC->m_textureDataDesc.textureMinFilterMethod = TextureFilterMethod::NEAREST;
	m_SSAONoiseTDC->m_textureDataDesc.textureMagFilterMethod = TextureFilterMethod::NEAREST;
	m_SSAONoiseTDC->m_textureDataDesc.textureWrapMethod = TextureWrapMethod::REPEAT;
	m_SSAONoiseTDC->m_textureDataDesc.textureWidth = l_textureSize;
	m_SSAONoiseTDC->m_textureDataDesc.textureHeight = l_textureSize;
	m_SSAONoiseTDC->m_textureDataDesc.texturePixelDataType = TexturePixelDataType::FLOAT;

	std::vector<float> l_pixelBuffer;
	auto l_containerSize = m_SSAONoise.size() * 4;
	l_pixelBuffer.reserve(l_containerSize);

	std::for_each(m_SSAONoise.begin(), m_SSAONoise.end(), [&](vec4 val)
	{
		l_pixelBuffer.emplace_back(val.x);
		l_pixelBuffer.emplace_back(val.y);
		l_pixelBuffer.emplace_back(val.z);
		l_pixelBuffer.emplace_back(val.w);
	});

	m_SSAONoiseTDC->m_textureData.emplace_back(&l_pixelBuffer[0]);

	m_SSAONoiseGLTDC = generateGLTextureDataComponent(m_SSAONoiseTDC);
}

bool GLSSAONoisePass::update()
{
	// copy camera data pack for local scope
	auto l_cameraDataPack = g_pCoreSystem->getVisionSystem()->getRenderingFrontend()->getCameraDataPack();
	if (l_cameraDataPack.has_value())
	{
		m_cameraDataPack = l_cameraDataPack.value();
	}

	activateRenderPass(m_GLRPC);

	activateShaderProgram(m_GLSPC);

	activateTexture(GLOpaquePass::getGLRPC()->m_GLTDCs[0], 0);
	activateTexture(GLOpaquePass::getGLRPC()->m_GLTDCs[1], 1);
	activateTexture(m_SSAONoiseGLTDC, 2);

	updateUniform(
		m_uni_p,
		m_cameraDataPack.p_jittered);
	updateUniform(
		m_uni_r,
		m_cameraDataPack.r);
	updateUniform(
		m_uni_t,
		m_cameraDataPack.t);

	for (size_t i = 0; i < m_uni_samples.size(); i++)
	{
		auto l_kernel = m_SSAOKernel[i];
		updateUniform(
			m_uni_samples[i],
			l_kernel.x, l_kernel.y, l_kernel.z, l_kernel.w);
	}

	auto l_MDC = g_pCoreSystem->getAssetSystem()->getMeshDataComponent(MeshShapeType::QUAD);
	drawMesh(l_MDC);

	return true;
}

bool GLSSAONoisePass::resize()
{
	resizeGLRenderPassComponent(m_GLRPC, GLRenderingSystemComponent::get().deferredPassFBDesc);

	return true;
}

bool GLSSAONoisePass::reloadShader()
{
	deleteShaderProgram(m_GLSPC);

	initializeGLShaderProgramComponent(m_GLSPC, m_shaderFilePaths);

	bindUniformLocations(m_GLSPC);

	return true;
}

GLRenderPassComponent * GLSSAONoisePass::getGLRPC()
{
	return m_GLRPC;
}