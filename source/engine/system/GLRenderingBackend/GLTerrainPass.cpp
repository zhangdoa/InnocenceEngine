#include "GLTerrainPass.h"
#include "GLOpaquePass.h"

#include "GLRenderingSystemUtilities.h"
#include "../../component/GLRenderingSystemComponent.h"
#include "../../component/RenderingFrontendSystemComponent.h"

using namespace GLRenderingSystemNS;

#include "../ICoreSystem.h"

extern ICoreSystem* g_pCoreSystem;

INNO_PRIVATE_SCOPE GLTerrainPass
{
	void initializeShaders();

	EntityID m_entityID;

	GLRenderPassComponent* m_GLRPC;

	GLShaderProgramComponent* m_GLSPC;

	ShaderFilePaths m_shaderFilePaths = { "GL//terrainPass.vert" , "", "GL//terrainPass.frag" };

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

	GLTextureDataComponent* m_terrainNoiseGLTDC;
}

bool GLTerrainPass::initialize()
{
	m_entityID = InnoMath::createEntityID();

	m_GLRPC = addGLRenderPassComponent(1, GLRenderingSystemComponent::get().deferredPassFBDesc, GLRenderingSystemComponent::get().deferredPassTextureDesc);

	initializeShaders();

	auto l_textureSize = 256;

	m_terrainNoise = generatePerlinNoise(l_textureSize, 6.0, 8);

	m_terrainNoiseGLTDC = addGLTextureDataComponent();

	m_terrainNoiseGLTDC->m_textureDataDesc.samplerType = TextureSamplerType::SAMPLER_2D;
	m_terrainNoiseGLTDC->m_textureDataDesc.usageType = TextureUsageType::RENDER_TARGET;
	m_terrainNoiseGLTDC->m_textureDataDesc.colorComponentsFormat = TextureColorComponentsFormat::RGB32F;
	m_terrainNoiseGLTDC->m_textureDataDesc.pixelDataFormat = TexturePixelDataFormat::RGBA;
	m_terrainNoiseGLTDC->m_textureDataDesc.minFilterMethod = TextureFilterMethod::NEAREST;
	m_terrainNoiseGLTDC->m_textureDataDesc.magFilterMethod = TextureFilterMethod::NEAREST;
	m_terrainNoiseGLTDC->m_textureDataDesc.wrapMethod = TextureWrapMethod::REPEAT;
	m_terrainNoiseGLTDC->m_textureDataDesc.width = l_textureSize;
	m_terrainNoiseGLTDC->m_textureDataDesc.height = l_textureSize;
	m_terrainNoiseGLTDC->m_textureDataDesc.pixelDataType = TexturePixelDataType::FLOAT;

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

	m_terrainNoiseGLTDC->m_textureData.emplace_back(&l_pixelBuffer[0]);

	initializeGLTextureDataComponent(m_terrainNoiseGLTDC);

	return true;
}

void GLTerrainPass::initializeShaders()
{
	// shader programs and shaders
	auto rhs = addGLShaderProgramComponent(m_entityID);

	initializeGLShaderProgramComponent(rhs, m_shaderFilePaths);

	m_GLSPC = rhs;
}

bool GLTerrainPass::update()
{
	auto l_renderingConfig = g_pCoreSystem->getVisionSystem()->getRenderingFrontend()->getRenderingConfig();

	if (l_renderingConfig.drawTerrain)
	{
		glEnable(GL_DEPTH_TEST);

		activateRenderPass(m_GLRPC);

		copyDepthBuffer(GLOpaquePass::getGLRPC(), m_GLRPC);

		activateShaderProgram(m_GLSPC);

		mat4 m = InnoMath::generateIdentityMatrix<float>();

		updateUniform(
			0,
			RenderingFrontendSystemComponent::get().m_cameraGPUData.p_original);
		updateUniform(
			1,
			RenderingFrontendSystemComponent::get().m_cameraGPUData.r);
		updateUniform(
			2,
			RenderingFrontendSystemComponent::get().m_cameraGPUData.t);
		updateUniform(
			3,
			m);

		auto l_MDC = getGLMeshDataComponent(MeshShapeType::TERRAIN);

		activateTexture(m_terrainNoiseGLTDC, 0);
		activateTexture(getGLTextureDataComponent(TextureUsageType::ALBEDO), 1);

		//glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
		drawMesh(l_MDC);
		//glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);

		glDisable(GL_DEPTH_TEST);
	}
	else
	{
		cleanRenderBuffers(m_GLRPC);
	}
	return true;
}

bool GLTerrainPass::resize(unsigned int newSizeX, unsigned int newSizeY)
{
	resizeGLRenderPassComponent(m_GLRPC, newSizeX, newSizeY);

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