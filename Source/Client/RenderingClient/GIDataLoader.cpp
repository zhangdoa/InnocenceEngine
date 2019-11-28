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

	TextureDataComponent* m_testSampleCubemap;
	TextureDataComponent* m_testSample3DTexture;

	std::vector<Probe> m_probes;
	std::vector<Surfel> m_surfels;
	std::vector<Brick> m_bricks;
	std::vector<BrickFactor> m_brickFactors;
	ProbeInfo m_probeInfo;

	bool m_IsSurfelLoaded = false;
	bool m_IsBrickLoaded = false;
	bool m_IsBrickFactorLoaded = false;
	bool m_IsProbeLoaded = false;

	std::function<void()> f_sceneLoadingFinishCallback;

	auto m_testCubemapResolution = 128;
	auto m_sampleCountPerFace = m_testCubemapResolution * m_testCubemapResolution;

	std::vector<Vec4> m_cubemapTextureSamples(m_sampleCountPerFace * 6);
	std::vector<Vec4> m_3DTextureSamples(m_testCubemapResolution * m_testCubemapResolution * m_testCubemapResolution);
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
		IOService::deserialize(l_probeInfoFile, &m_probeInfo);
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

	std::vector<Vec4> l_faceColors = {
	Vec4(1.0f, 0.0f, 0.0f, 1.0f),
	Vec4(1.0f, 1.0f, 0.0f, 1.0f),
	Vec4(0.0f, 1.0f, 0.0f, 1.0f),
	Vec4(0.0f, 1.0f, 1.0f, 1.0f),
	Vec4(0.0f, 0.0f, 1.0f, 1.0f),
	Vec4(1.0f, 0.0f, 1.0f, 1.0f),
	};
	for (size_t i = 0; i < 6; i++)
	{
		for (size_t j = 0; j < m_sampleCountPerFace; j++)
		{
			auto l_color = l_faceColors[i] * 2.0f * (float)j / (float)m_sampleCountPerFace;
			l_color.w = 1.0f;
			m_cubemapTextureSamples[i * m_sampleCountPerFace + j] = l_color;
		}
	}

	auto l_RenderPassDesc = g_pModuleManager->getRenderingFrontend()->getDefaultRenderPassDesc();

	m_testSampleCubemap->m_textureDesc = l_RenderPassDesc.m_RenderTargetDesc;
	m_testSampleCubemap->m_textureDesc.SamplerType = TextureSamplerType::SamplerCubemap;
	m_testSampleCubemap->m_textureDesc.UsageType = TextureUsageType::Sample;
	m_testSampleCubemap->m_textureDesc.PixelDataFormat = TexturePixelDataFormat::RGBA;
	m_testSampleCubemap->m_textureDesc.Width = m_testCubemapResolution;
	m_testSampleCubemap->m_textureDesc.Height = m_testCubemapResolution;
	m_testSampleCubemap->m_textureDesc.PixelDataType = TexturePixelDataType::FLOAT32;
	m_testSampleCubemap->m_textureData = &m_cubemapTextureSamples[0];

	////
	size_t l_pixelIndex = 0;
	for (size_t i = 0; i < m_testCubemapResolution; i++)
	{
		for (size_t j = 0; j < m_testCubemapResolution; j++)
		{
			for (size_t k = 0; k < m_testCubemapResolution; k++)
			{
				m_3DTextureSamples[l_pixelIndex] = Vec4((float)i / (float)m_testCubemapResolution, (float)j / (float)m_testCubemapResolution, (float)k / (float)m_testCubemapResolution, 1.0f);
				l_pixelIndex++;
			}
		}
	}

	m_testSample3DTexture = g_pModuleManager->getRenderingServer()->AddTextureDataComponent("TestSample3D/");

	m_testSample3DTexture->m_textureDesc = l_RenderPassDesc.m_RenderTargetDesc;
	m_testSample3DTexture->m_textureDesc.SamplerType = TextureSamplerType::Sampler3D;
	m_testSample3DTexture->m_textureDesc.UsageType = TextureUsageType::Sample;
	m_testSample3DTexture->m_textureDesc.PixelDataFormat = TexturePixelDataFormat::RGBA;
	m_testSample3DTexture->m_textureDesc.Width = m_testCubemapResolution;
	m_testSample3DTexture->m_textureDesc.Height = m_testCubemapResolution;
	m_testSample3DTexture->m_textureDesc.DepthOrArraySize = m_testCubemapResolution;
	m_testSample3DTexture->m_textureDesc.PixelDataType = TexturePixelDataType::FLOAT32;
	m_testSample3DTexture->m_textureData = &m_3DTextureSamples[0];

	return true;
}

bool GIDataLoader::Initialize()
{
	g_pModuleManager->getRenderingServer()->InitializeTextureDataComponent(m_testSampleCubemap);
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

const ProbeInfo& GIDataLoader::GetProbeInfo()
{
	return m_probeInfo;
}