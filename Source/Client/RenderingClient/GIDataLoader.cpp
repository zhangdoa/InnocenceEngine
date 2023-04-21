#include "GIDataLoader.h"
#include "../DefaultGPUBuffers/DefaultGPUBuffers.h"

#include "../../Engine/Common/MathHelper.h"

#include "../../Engine/Interface/IEngine.h"

using namespace Inno;
extern INNO_ENGINE_API IEngine* g_Engine;

#include "../../Engine/Core/IOService.h"

using namespace DefaultGPUBuffers;

namespace GIDataLoader
{
	bool loadGIData();

	TextureComponent* m_testSampleCubemap;
	TextureComponent* m_testSample3DTexture;

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
	std::vector<Vec4> m_3DTextureSamples(m_testCubemapResolution* m_testCubemapResolution* m_testCubemapResolution);
}

bool GIDataLoader::loadGIData()
{
	auto l_filePath = g_Engine->getFileSystem()->getWorkingDirectory();
	auto l_currentSceneName = g_Engine->getSceneSystem()->getCurrentSceneName();

	std::ifstream l_surfelFile;
	l_surfelFile.open(l_filePath + "..//Res//Scenes//" + l_currentSceneName + ".Surfel", std::ios::binary);

	if (l_surfelFile.is_open())
	{
		IOService::deserializeVector(l_surfelFile, m_surfels);
		m_IsSurfelLoaded = true;
	}

	std::ifstream l_brickFile;
	l_brickFile.open(l_filePath + "..//Res//Scenes//" + l_currentSceneName + ".Brick", std::ios::binary);

	if (l_brickFile.is_open())
	{
		IOService::deserializeVector(l_brickFile, m_bricks);
		m_IsBrickLoaded = true;
	}

	std::ifstream l_brickFactorFile;
	l_brickFactorFile.open(l_filePath + "..//Res//Scenes//" + l_currentSceneName + ".BrickFactor", std::ios::binary);

	if (l_brickFactorFile.is_open())
	{
		IOService::deserializeVector(l_brickFactorFile, m_brickFactors);
		m_IsBrickFactorLoaded = true;
	}

	std::ifstream l_probeFile;
	l_probeFile.open(l_filePath + "..//Res//Scenes//" + l_currentSceneName + ".Probe", std::ios::binary);

	if (l_probeFile.is_open())
	{
		IOService::deserializeVector(l_probeFile, m_probes);

		m_IsProbeLoaded = true;
	}

	std::ifstream l_probeInfoFile;
	l_probeInfoFile.open(l_filePath + "..//Res//Scenes//" + l_currentSceneName + ".ProbeInfo", std::ios::binary);

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

	g_Engine->getSceneSystem()->addSceneLoadingFinishCallback(&f_sceneLoadingFinishCallback);

	////
	m_testSampleCubemap = g_Engine->getRenderingServer()->AddTextureComponent("TestSampleCubemap/");

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

	auto l_RenderPassDesc = g_Engine->getRenderingFrontend()->getDefaultRenderPassDesc();

	m_testSampleCubemap->m_TextureDesc = l_RenderPassDesc.m_RenderTargetDesc;
	m_testSampleCubemap->m_TextureDesc.Sampler = TextureSampler::SamplerCubemap;
	m_testSampleCubemap->m_TextureDesc.Usage = TextureUsage::Sample;
	m_testSampleCubemap->m_TextureDesc.PixelDataFormat = TexturePixelDataFormat::RGBA;
	m_testSampleCubemap->m_TextureDesc.Width = m_testCubemapResolution;
	m_testSampleCubemap->m_TextureDesc.Height = m_testCubemapResolution;
	m_testSampleCubemap->m_TextureDesc.PixelDataType = TexturePixelDataType::Float32;
	m_testSampleCubemap->m_TextureData = &m_cubemapTextureSamples[0];

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

	m_testSample3DTexture = g_Engine->getRenderingServer()->AddTextureComponent("TestSample3D/");

	m_testSample3DTexture->m_TextureDesc = l_RenderPassDesc.m_RenderTargetDesc;
	m_testSample3DTexture->m_TextureDesc.Sampler = TextureSampler::Sampler3D;
	m_testSample3DTexture->m_TextureDesc.Usage = TextureUsage::Sample;
	m_testSample3DTexture->m_TextureDesc.PixelDataFormat = TexturePixelDataFormat::RGBA;
	m_testSample3DTexture->m_TextureDesc.Width = m_testCubemapResolution;
	m_testSample3DTexture->m_TextureDesc.Height = m_testCubemapResolution;
	m_testSample3DTexture->m_TextureDesc.DepthOrArraySize = m_testCubemapResolution;
	m_testSample3DTexture->m_TextureDesc.PixelDataType = TexturePixelDataType::Float32;
	m_testSample3DTexture->m_TextureData = &m_3DTextureSamples[0];

	return true;
}

bool GIDataLoader::Initialize()
{
	g_Engine->getRenderingServer()->InitializeTextureComponent(m_testSampleCubemap);
	g_Engine->getRenderingServer()->InitializeTextureComponent(m_testSample3DTexture);

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