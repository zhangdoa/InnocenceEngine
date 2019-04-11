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

	static float perlinNoiseFade(float t)
	{
		return t * t * t * (t * (t * 6.0f - 15.0f) + 10.0f);
	}

	static float perlinNoiseLerp(float t, float a, float b)
	{
		return a + t * (b - a);
	}

	static float perlinNoiseGrad(std::int32_t hash, float x, float y, float z)
	{
		std::int32_t h = hash & 15;
		float u = h < 8 ? x : y;
		float v = h < 4 ? y : h == 12 || h == 14 ? x : z;
		return ((h & 1) == 0 ? u : -u) + ((h & 2) == 0 ? v : -v);
	}

	static float noise(float x, float y, float z, const std::vector<float>& seeds, unsigned int halfSize)
	{
		std::int32_t X = static_cast<std::int32_t>(std::floor(x)) & (halfSize - 1);
		std::int32_t Y = static_cast<std::int32_t>(std::floor(y)) & (halfSize - 1);
		std::int32_t Z = static_cast<std::int32_t>(std::floor(z)) & (halfSize - 1);

		x -= std::floor(x);
		y -= std::floor(y);
		z -= std::floor(z);

		float u = perlinNoiseFade(x);
		float v = perlinNoiseFade(y);
		float w = perlinNoiseFade(z);

		auto A = (std::int32_t)seeds[X] + Y;
		auto AA = (std::int32_t)seeds[A] + Z;
		auto AB = (std::int32_t)seeds[A + 1] + Z;
		auto B = (std::int32_t)seeds[X + 1] + Y;
		auto BA = (std::int32_t)seeds[B] + Z;
		auto BB = (std::int32_t)seeds[B + 1] + Z;

		auto gradAA = perlinNoiseGrad((std::int32_t)seeds[AA], x, y, z);
		auto gradBA = perlinNoiseGrad((std::int32_t)seeds[BA], x - 1, y, z);
		auto gradAB = perlinNoiseGrad((std::int32_t)seeds[AB], x, y - 1, z);
		auto gradBB = perlinNoiseGrad((std::int32_t)seeds[BB], x - 1, y - 1, z);

		auto gradAAPlus1 = perlinNoiseGrad((std::int32_t)seeds[AA + 1], x, y, z - 1);
		auto gradBAPlus1 = perlinNoiseGrad((std::int32_t)seeds[BA + 1], x - 1, y, z - 1);
		auto gradABPlus1 = perlinNoiseGrad((std::int32_t)seeds[AB + 1], x, y - 1, z - 1);
		auto gradBBPlus1 = perlinNoiseGrad((std::int32_t)seeds[BB + 1], x - 1, y - 1, z - 1);

		return perlinNoiseLerp(w,
			perlinNoiseLerp(v,
				perlinNoiseLerp(u, gradAA,
			gradBA),
			perlinNoiseLerp(u, gradAB,
				gradBB)
			),
			perlinNoiseLerp(v,
				perlinNoiseLerp(u,
					gradAAPlus1, gradBAPlus1
				),
				perlinNoiseLerp(u, gradABPlus1,
					gradBBPlus1)
			)
		);
	}

	float generateOctaveNoise(float x, float y, float z, std::int32_t octaves, const std::vector<float>& seeds, unsigned int halfSize)
	{
		float result = 0.0;
		float amp = 1.0;

		for (std::int32_t i = 0; i < octaves; ++i)
		{
			result += noise(x, y, z, seeds, halfSize) * amp;
			x *= 2.0;
			y *= 2.0;
			z *= 2.0;
			amp *= 0.5;
		}

		return result;
	}

	std::vector<vec4> generatePerlinNoise(unsigned int size, float frequency, unsigned int octaves)
	{
		auto l_halfSize = size / 2;
		std::vector<float> l_seeds(size);

		for (size_t i = 0; i < l_halfSize; i++)
		{
			l_seeds[i] = (float)i;
		}

		std::shuffle(std::begin(l_seeds), std::begin(l_seeds) + l_halfSize, std::default_random_engine(std::default_random_engine::default_seed));

		for (size_t i = 0; i < l_halfSize; i++)
		{
			l_seeds[l_halfSize + i] = (float)i;
		}

		const float fx = (float)size / frequency;
		const float fy = (float)size / frequency;

		std::vector<vec4> l_result;
		l_result.reserve(size * size);

		for (unsigned int y = 0; y < size; ++y)
		{
			for (unsigned int x = 0; x < size; ++x)
			{
				auto l_singleChannel = generateOctaveNoise(x / fx, y / fy, 0, octaves, l_seeds, l_halfSize);
				l_singleChannel = l_singleChannel * 0.5f + 0.5f;
				auto noiseColor = vec4(l_singleChannel, l_singleChannel, l_singleChannel, 1.0f);
				l_result.emplace_back(noiseColor);
			}
		}

		return l_result;
	}

	std::vector<vec4> m_terrainNoise;

	TextureDataComponent* m_terrainNoiseTDC;
	GLTextureDataComponent* m_terrainNoiseGLTDC;

	CameraDataPack m_cameraDataPack;
}

bool GLTerrainPass::initialize()
{
	m_entityID = InnoMath::createEntityID();

	m_GLRPC = addGLRenderPassComponent(1, GLRenderingSystemComponent::get().deferredPassFBDesc, GLRenderingSystemComponent::get().deferredPassTextureDesc);

	initializeShaders();

	auto l_textureSize = 256;

	m_terrainNoise = generatePerlinNoise(l_textureSize, 6.0, 8);

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

	// copy camera data pack for local scope
	auto l_cameraDataPack = g_pCoreSystem->getVisionSystem()->getRenderingFrontend()->getCameraDataPack();
	if (l_cameraDataPack.has_value())
	{
		m_cameraDataPack = l_cameraDataPack.value();
	}

	if (l_renderingConfig.drawTerrain)
	{
		glEnable(GL_DEPTH_TEST);

		activateRenderPass(m_GLRPC);

		copyDepthBuffer(GLOpaquePass::getGLRPC(), m_GLRPC);

		activateShaderProgram(m_GLSPC);

		mat4 m = InnoMath::generateIdentityMatrix<float>();

		updateUniform(
			m_uni_p_camera,
			m_cameraDataPack.p_original);
		updateUniform(
			m_uni_r_camera,
			m_cameraDataPack.r);
		updateUniform(
			m_uni_t_camera,
			m_cameraDataPack.t);
		updateUniform(
			m_uni_m,
			m);

		auto l_MDC = g_pCoreSystem->getAssetSystem()->getMeshDataComponent(MeshShapeType::TERRAIN);
		activateTexture(GLRenderingSystemComponent::get().m_basicAlbedoGLTDC, 0);
		activateTexture(m_terrainNoiseGLTDC, 1);

		//glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
		drawMesh(l_MDC);
		//glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);

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