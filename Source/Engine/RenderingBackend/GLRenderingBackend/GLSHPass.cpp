#include "GLSHPass.h"

#include "GLRenderingBackendUtilities.h"
#include "../../Component/GLRenderingBackendComponent.h"

using namespace GLRenderingBackendNS;

#include "../../ModuleManager/IModuleManager.h"

extern IModuleManager* g_pModuleManager;

struct SH9
{
	vec4 L00;
	vec4 L11;
	vec4 L10;
	vec4 L1_1;
	vec4 L21;
	vec4 L2_1;
	vec4 L2_2;
	vec4 L20;
	vec4 L22;

	SH9 operator+= (const SH9& rhs)
	{
		L00 = L00 + rhs.L00;
		L11 = L11 + rhs.L11;
		L10 = L10 + rhs.L10;
		L1_1 = L1_1 + rhs.L1_1;
		L21 = L21 + rhs.L21;
		L2_1 = L2_1 + rhs.L2_1;
		L2_2 = L2_2 + rhs.L2_2;
		L20 = L20 + rhs.L20;
		L22 = L22 + rhs.L22;

		return *this;
	}

	SH9 operator/= (float rhs)
	{
		L00 = L00 / rhs;
		L11 = L11 / rhs;
		L10 = L10 / rhs;
		L1_1 = L1_1 / rhs;
		L21 = L21 / rhs;
		L2_1 = L2_1 / rhs;
		L2_2 = L2_2 / rhs;
		L20 = L20 / rhs;
		L22 = L22 / rhs;

		return *this;
	}
};

INNO_PRIVATE_SCOPE GLSHPass
{
	void initializeShaders();
	SH9 samplesToSH(const std::vector<vec4>& samples);
	std::vector<vec4> readbackCubemapSamples(GLRenderPassComponent* GLRPC, GLTextureDataComponent* GLTDC);

	EntityID m_entityID;

	GLRenderPassComponent* m_GLRPC;

	GLShaderProgramComponent* m_GLSPC;
	GLTextureDataComponent* m_testSampleCubemap;

	ShaderFilePaths m_shaderFilePaths = { "SHPass.vert/", "", "", "", "SHPass.frag/" };

	const unsigned int m_captureResolution = 256;
	const unsigned int m_sampleCountPerFace = m_captureResolution * m_captureResolution;
}

bool GLSHPass::initialize()
{
	m_entityID = InnoMath::createEntityID();

	auto l_renderPassDesc = GLRenderingBackendComponent::get().m_deferredRenderPassDesc;
	l_renderPassDesc.RTNumber = 0;
	m_GLRPC = addGLRenderPassComponent(m_entityID, "SphericalHarmonicPassGLRPC/");
	m_GLRPC->m_renderPassDesc = l_renderPassDesc;
	m_GLRPC->m_drawColorBuffers = false;

	initializeGLRenderPassComponent(m_GLRPC);

	//initializeShaders();

	m_testSampleCubemap = addGLTextureDataComponent();
	m_testSampleCubemap->m_textureDataDesc = GLRenderingBackendComponent::get().m_deferredRenderPassDesc.RTDesc;
	m_testSampleCubemap->m_textureDataDesc.samplerType = TextureSamplerType::SAMPLER_CUBEMAP;
	m_testSampleCubemap->m_textureDataDesc.usageType = TextureUsageType::NORMAL;
	m_testSampleCubemap->m_textureDataDesc.pixelDataFormat = TexturePixelDataFormat::RGBA;
	m_testSampleCubemap->m_textureDataDesc.minFilterMethod = TextureFilterMethod::LINEAR;
	m_testSampleCubemap->m_textureDataDesc.magFilterMethod = TextureFilterMethod::LINEAR;
	m_testSampleCubemap->m_textureDataDesc.wrapMethod = TextureWrapMethod::REPEAT;
	m_testSampleCubemap->m_textureDataDesc.width = m_captureResolution;
	m_testSampleCubemap->m_textureDataDesc.height = m_captureResolution;
	m_testSampleCubemap->m_textureDataDesc.pixelDataType = TexturePixelDataType::FLOAT32;

	std::vector<vec4> l_textureSamples(m_captureResolution * m_captureResolution * 6);
	std::vector<vec4> l_faceColors = {
	vec4(1.0f, 0.0f, 0.0f, 1.0f),
	vec4(1.0f, 1.0f, 0.0f, 1.0f),
	vec4(0.0f, 1.0f, 0.0f, 1.0f),
	vec4(0.0f, 1.0f, 1.0f, 1.0f),
	vec4(0.0f, 0.0f, 1.0f, 1.0f),
	vec4(1.0f, 0.0f, 1.0f, 1.0f),
	};
	for (size_t i = 0; i < 6; i++)
	{
		for (size_t j = 0; j < m_sampleCountPerFace; j++)
		{
			auto l_color = l_faceColors[i] * (float)j / (float)m_sampleCountPerFace;
			l_color.w = 1.0f;
			l_textureSamples[i * m_sampleCountPerFace + j] = l_color;
		}
	}
	m_testSampleCubemap->m_textureData = &l_textureSamples[0];
	initializeGLTextureDataComponent(m_testSampleCubemap);

	return true;
}

void GLSHPass::initializeShaders()
{
	// shader programs and shaders
	auto rhs = addGLShaderProgramComponent(m_entityID);

	initializeGLShaderProgramComponent(rhs, m_shaderFilePaths);

	m_GLSPC = rhs;
}

bool GLSHPass::update()
{
	auto l_textureSamples = readbackCubemapSamples(m_GLRPC, m_testSampleCubemap);
	auto l_SH9 = samplesToSH(l_textureSamples);
	return true;
}

bool GLSHPass::resize(unsigned int newSizeX, unsigned int newSizeY)
{
	return true;
}

bool GLSHPass::reloadShader()
{
	deleteShaderProgram(m_GLSPC);

	initializeGLShaderProgramComponent(m_GLSPC, m_shaderFilePaths);

	return true;
}

GLRenderPassComponent * GLSHPass::getGLRPC()
{
	return nullptr;
}

std::vector<vec4> GLSHPass::readbackCubemapSamples(GLRenderPassComponent* GLRPC, GLTextureDataComponent* GLTDC)
{
	///////////////
	auto l_resolution = GLTDC->m_GLTextureDataDesc.width;
	auto l_pixelDataFormat = GLTDC->m_GLTextureDataDesc.pixelDataFormat;
	auto l_pixelDataType = GLTDC->m_GLTextureDataDesc.pixelDataType;

	std::vector<vec4> l_textureSamples;
	auto l_sampleCountPerFace = l_resolution * l_resolution;
	l_textureSamples.resize(l_sampleCountPerFace * 6);

	glBindFramebuffer(GL_READ_FRAMEBUFFER, GLRPC->m_FBO);
	glReadBuffer(GL_COLOR_ATTACHMENT0);

	for (unsigned int i = 0; i < 6; i++)
	{
		bindCubemapTextureForWrite(GLTDC, GLRPC, 0, i, 0);

		glReadPixels(0, 0, l_resolution, l_resolution, l_pixelDataFormat, l_pixelDataType, &l_textureSamples[i * l_sampleCountPerFace]);

		unbindCubemapTextureForWrite(GLRPC, 0, i, 0);
	}

	glReadBuffer(GL_NONE);
	glBindFramebuffer(GL_READ_FRAMEBUFFER, 0);

	return l_textureSamples;
}

SH9 getSH9(vec4 normal, vec4 radiance)
{
	float Y00 = 0.282095f;
	float Y11 = 0.488603f * normal.x;
	float Y10 = 0.488603f * normal.z;
	float Y1_1 = 0.488603f * normal.y;
	float Y21 = 1.092548f * normal.x*normal.z;
	float Y2_1 = 1.092548f * normal.y*normal.z;
	float Y2_2 = 1.092548f * normal.y*normal.x;
	float Y20 = 0.946176f * normal.z * normal.z - 0.315392f;
	float Y22 = 0.546274f * (normal.x*normal.x - normal.y*normal.y);

	SH9 l_result;

	l_result.L00 = radiance * Y00;
	l_result.L11 = radiance * Y11;
	l_result.L10 = radiance * Y10;
	l_result.L1_1 = radiance * Y1_1;
	l_result.L21 = radiance * Y21;
	l_result.L2_1 = radiance * Y2_1;
	l_result.L2_2 = radiance * Y2_2;
	l_result.L20 = radiance * Y20;
	l_result.L22 = radiance * Y22;

	return l_result;
}

SH9 GLSHPass::samplesToSH(const std::vector<vec4>& samples)
{
	auto l_totalSampleCount = samples.size();
	auto l_sampleCountPerFace = l_totalSampleCount / 6;
	auto l_resolution = (unsigned int)std::sqrt(l_sampleCountPerFace);
	auto l_texelSize = 1.0f / (float)l_resolution;

	SH9 l_result;

	////// +X
	for (size_t i = 0; i < l_sampleCountPerFace; i++)
	{
		auto l_x = 1.0f;
		auto l_y = (float)(i % l_resolution) * l_texelSize;
		l_y = l_y * 2.0f - 1.0f;
		auto l_z = (float)((i / l_resolution) % l_resolution) * l_texelSize;
		l_z = l_z * 2.0f - 1.0f;

		auto l_normal = vec4(l_x, l_y, l_z, 0.0f);
		l_normal = l_normal.normalize();

		l_result += getSH9(l_normal, samples[i]);
	}

	////// -X
	for (size_t i = 0; i < l_sampleCountPerFace; i++)
	{
		auto l_x = -1.0f;
		auto l_y = (float)(i % l_resolution) * l_texelSize;
		l_y = l_y * 2.0f - 1.0f;
		auto l_z = (float)((i / l_resolution) % l_resolution) * l_texelSize;
		l_z = l_z * 2.0f - 1.0f;

		auto l_normal = vec4(l_x, l_y, l_z, 0.0f);
		l_normal = l_normal.normalize();

		l_result += getSH9(l_normal, samples[i + l_sampleCountPerFace * 1]);
	}

	////// +Y
	for (size_t i = 0; i < l_sampleCountPerFace; i++)
	{
		auto l_x = (float)(i % l_resolution) * l_texelSize;
		l_x = l_x * 2.0f - 1.0f;
		auto l_y = 1.0f;
		auto l_z = (float)((i / l_resolution) % l_resolution) * l_texelSize;
		l_z = l_z * 2.0f - 1.0f;

		auto l_normal = vec4(l_x, l_y, l_z, 0.0f);
		l_normal = l_normal.normalize();

		l_result += getSH9(l_normal, samples[i + l_sampleCountPerFace * 2]);
	}

	////// -Y
	for (size_t i = 0; i < l_sampleCountPerFace; i++)
	{
		auto l_x = (float)(i % l_resolution) * l_texelSize;
		l_x = l_x * 2.0f - 1.0f;
		auto l_y = -1.0f;
		auto l_z = (float)((i / l_resolution) % l_resolution) * l_texelSize;
		l_z = l_z * 2.0f - 1.0f;

		auto l_normal = vec4(l_x, l_y, l_z, 0.0f);
		l_normal = l_normal.normalize();

		l_result += getSH9(l_normal, samples[i + l_sampleCountPerFace * 3]);
	}

	////// +Z
	for (size_t i = 0; i < l_sampleCountPerFace; i++)
	{
		auto l_x = (float)(i % l_resolution) * l_texelSize;
		l_x = l_x * 2.0f - 1.0f;
		auto l_y = (float)((i / l_resolution) % l_resolution) * l_texelSize;
		l_y = l_y * 2.0f - 1.0f;
		auto l_z = 1.0f;

		auto l_normal = vec4(l_x, l_y, l_z, 0.0f);
		l_normal = l_normal.normalize();

		l_result += getSH9(l_normal, samples[i + l_sampleCountPerFace * 4]);
	}

	////// -Z
	for (size_t i = 0; i < l_sampleCountPerFace; i++)
	{
		auto l_x = (float)(i % l_resolution) * l_texelSize;
		l_x = l_x * 2.0f - 1.0f;
		auto l_y = (float)((i / l_resolution) % l_resolution) * l_texelSize;
		l_y = l_y * 2.0f - 1.0f;
		auto l_z = -1.0f;

		auto l_normal = vec4(l_x, l_y, l_z, 0.0f);
		l_normal = l_normal.normalize();

		l_result += getSH9(l_normal, samples[i + l_sampleCountPerFace * 5]);
	}

	l_result /= (float)l_totalSampleCount;

	return l_result;
}