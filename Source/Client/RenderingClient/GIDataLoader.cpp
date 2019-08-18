#include "GIDataLoader.h"
#include "../DefaultGPUBuffers/DefaultGPUBuffers.h"

#include "../../Engine/Common/InnoMathHelper.h"

#include "../../Engine/ModuleManager/IModuleManager.h"
INNO_ENGINE_API extern IModuleManager* g_pModuleManager;

#include "../../Engine/Core/IOService.h"

using namespace DefaultGPUBuffers;

namespace GIDataLoader
{
	bool loadGIData();

	vec4 m_irradianceVolumeRange;

	TextureDataComponent* m_testSampleCubemap;
	TextureDataComponent* m_testSample3DTexture;

	std::vector<Probe> m_probes;
	std::vector<Surfel> m_surfels;
	std::vector<Brick> m_bricks;
	std::vector<BrickFactor> m_brickFactors;
	vec4 m_probeCount;

	bool m_IsSurfelLoaded = false;
	bool m_IsBrickLoaded = false;
	bool m_IsBrickFactorLoaded = false;
	bool m_IsProbeLoaded = false;

	std::function<void()> f_sceneLoadingFinishCallback;
}

bool GIDataLoader::loadGIData()
{
	auto l_filePath = g_pModuleManager->getFileSystem()->getWorkingDirectory();
	auto l_currentSceneName = g_pModuleManager->getFileSystem()->getCurrentSceneName();

	std::ifstream l_surfelFile;
	l_surfelFile.open(l_filePath + "//Res//Scenes//" + l_currentSceneName + ".InnoSurfel", std::ios::binary);

	if (l_surfelFile.is_open())
	{
		IOService::deserializeVector(l_surfelFile, m_surfels);
		m_IsSurfelLoaded = true;
	}

	std::ifstream l_brickFile;
	l_brickFile.open(l_filePath + "//Res//Scenes//" + l_currentSceneName + ".InnoBrick", std::ios::binary);

	if (l_brickFile.is_open())
	{
		IOService::deserializeVector(l_brickFile, m_bricks);

		auto l_bricksCount = m_bricks.size();

		auto l_min = InnoMath::maxVec4<float>;
		auto l_max = InnoMath::minVec4<float>;

		for (size_t i = 0; i < l_bricksCount; i++)
		{
			l_min = InnoMath::elementWiseMin(l_min, m_bricks[i].boundBox.m_boundMin);
			l_max = InnoMath::elementWiseMax(l_max, m_bricks[i].boundBox.m_boundMax);
		}
		m_irradianceVolumeRange = l_max - l_min;

		m_IsBrickLoaded = true;
	}

	std::ifstream l_brickFactorFile;
	l_brickFactorFile.open(l_filePath + "//Res//Scenes//" + l_currentSceneName + ".InnoBrickFactor", std::ios::binary);

	if (l_brickFactorFile.is_open())
	{
		IOService::deserializeVector(l_brickFactorFile, m_brickFactors);
		m_IsBrickFactorLoaded = true;
	}

	std::ifstream l_probeFile;
	l_probeFile.open(l_filePath + "//Res//Scenes//" + l_currentSceneName + ".InnoProbe", std::ios::binary);

	if (l_probeFile.is_open())
	{
		IOService::deserializeVector(l_probeFile, m_probes);

		m_IsProbeLoaded = true;
	}

	std::ifstream l_probeInfoFile;
	l_probeInfoFile.open(l_filePath + "//Res//Scenes//" + l_currentSceneName + ".InnoProbeInfo", std::ios::binary);

	if (l_probeInfoFile.is_open())
	{
		IOService::deserialize(l_probeInfoFile, &m_probeCount);
	}
	else
	{
		m_IsProbeLoaded = false;
	}

	return m_IsProbeLoaded && m_IsBrickFactorLoaded && m_IsBrickLoaded && m_IsSurfelLoaded;
}

bool GIDataLoader::Setup()
{
	f_sceneLoadingFinishCallback = []()
	{
		m_IsSurfelLoaded = false;
		m_IsBrickLoaded = false;
		m_IsBrickFactorLoaded = false;
		m_IsProbeLoaded = false;
		loadGIData();
	};

	g_pModuleManager->getFileSystem()->addSceneLoadingFinishCallback(&f_sceneLoadingFinishCallback);

	////
	m_testSampleCubemap = g_pModuleManager->getRenderingServer()->AddTextureDataComponent("TestSampleCubemap/");

	auto l_testCubemapResolution = 128;
	auto l_sampleCountPerFace = l_testCubemapResolution * l_testCubemapResolution;

	std::vector<vec4> l_cubemapTextureSamples(l_sampleCountPerFace * 6);
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
		for (size_t j = 0; j < l_sampleCountPerFace; j++)
		{
			auto l_color = l_faceColors[i] * 2.0f * (float)j / (float)l_sampleCountPerFace;
			l_color.w = 1.0f;
			l_cubemapTextureSamples[i * l_sampleCountPerFace + j] = l_color;
		}
	}

	auto l_RenderPassDesc = g_pModuleManager->getRenderingFrontend()->getDefaultRenderPassDesc();

	m_testSampleCubemap->m_textureDataDesc = l_RenderPassDesc.m_RenderTargetDesc;
	m_testSampleCubemap->m_textureDataDesc.SamplerType = TextureSamplerType::SamplerCubemap;
	m_testSampleCubemap->m_textureDataDesc.UsageType = TextureUsageType::Normal;
	m_testSampleCubemap->m_textureDataDesc.PixelDataFormat = TexturePixelDataFormat::RGBA;
	m_testSampleCubemap->m_textureDataDesc.MinFilterMethod = TextureFilterMethod::Linear;
	m_testSampleCubemap->m_textureDataDesc.MagFilterMethod = TextureFilterMethod::Linear;
	m_testSampleCubemap->m_textureDataDesc.WrapMethod = TextureWrapMethod::Repeat;
	m_testSampleCubemap->m_textureDataDesc.Width = l_testCubemapResolution;
	m_testSampleCubemap->m_textureDataDesc.Height = l_testCubemapResolution;
	m_testSampleCubemap->m_textureDataDesc.PixelDataType = TexturePixelDataType::FLOAT32;
	m_testSampleCubemap->m_textureData = &l_cubemapTextureSamples[0];

	g_pModuleManager->getRenderingServer()->InitializeTextureDataComponent(m_testSampleCubemap);

	////
	std::vector<vec4> l_3DTextureSamples(l_testCubemapResolution * l_testCubemapResolution * l_testCubemapResolution);
	size_t l_pixelIndex = 0;
	for (size_t i = 0; i < l_testCubemapResolution; i++)
	{
		for (size_t j = 0; j < l_testCubemapResolution; j++)
		{
			for (size_t k = 0; k < l_testCubemapResolution; k++)
			{
				l_3DTextureSamples[l_pixelIndex] = vec4((float)i / (float)l_testCubemapResolution, (float)j / (float)l_testCubemapResolution, (float)k / (float)l_testCubemapResolution, 1.0f);
				l_pixelIndex++;
			}
		}
	}

	m_testSample3DTexture = g_pModuleManager->getRenderingServer()->AddTextureDataComponent("TestSample3D/");

	m_testSample3DTexture->m_textureDataDesc = l_RenderPassDesc.m_RenderTargetDesc;
	m_testSample3DTexture->m_textureDataDesc.SamplerType = TextureSamplerType::Sampler3D;
	m_testSample3DTexture->m_textureDataDesc.UsageType = TextureUsageType::Normal;
	m_testSample3DTexture->m_textureDataDesc.PixelDataFormat = TexturePixelDataFormat::RGBA;
	m_testSample3DTexture->m_textureDataDesc.MinFilterMethod = TextureFilterMethod::Linear;
	m_testSample3DTexture->m_textureDataDesc.MagFilterMethod = TextureFilterMethod::Linear;
	m_testSample3DTexture->m_textureDataDesc.WrapMethod = TextureWrapMethod::Repeat;
	m_testSample3DTexture->m_textureDataDesc.Width = l_testCubemapResolution;
	m_testSample3DTexture->m_textureDataDesc.Height = l_testCubemapResolution;
	m_testSample3DTexture->m_textureDataDesc.DepthOrArraySize = l_testCubemapResolution;
	m_testSample3DTexture->m_textureDataDesc.PixelDataType = TexturePixelDataType::FLOAT32;
	m_testSample3DTexture->m_textureData = &l_3DTextureSamples[0];

	g_pModuleManager->getRenderingServer()->InitializeTextureDataComponent(m_testSample3DTexture);

	return true;
}

bool GIDataLoader::ReloadGIData()
{
	m_probes.clear();
	m_surfels.clear();
	m_bricks.clear();
	m_brickFactors.clear();

	loadGIData();

	return true;
}

bool GIDataLoader::Terminate()
{
	return true;
}

const std::vector<Surfel>& GIDataLoader::GetSurfels()
{
	return m_surfels;
}

const std::vector<Brick>& GIDataLoader::GetBricks()
{
	return m_bricks;
}

const std::vector<BrickFactor>& GIDataLoader::GetBrickFactors()
{
	return m_brickFactors;
}

const std::vector<Probe>& GIDataLoader::GetProbes()
{
	return m_probes;
}

vec4 GIDataLoader::GetIrradianceVolumeRange()
{
	return m_irradianceVolumeRange;
}

vec4 GIDataLoader::GetProbeMaxCount()
{
	return m_probeCount;
}