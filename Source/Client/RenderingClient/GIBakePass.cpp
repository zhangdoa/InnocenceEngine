#include "GIBakePass.h"
#include "DefaultGPUBuffers.h"

#include "../../Engine/ModuleManager/IModuleManager.h"
INNO_ENGINE_API extern IModuleManager* g_pModuleManager;

#include "../../Engine/Core/IOService.h"

struct ProbeCache
{
	vec4 pos;
	std::vector<Surfel> surfelCaches;
};

using namespace DefaultGPUBuffers;

namespace GIBakePass
{
	bool loadGIData();

	bool generateProbes();
	bool generateBricks();

	bool capture();
	bool drawCubemaps(unsigned int probeIndex, const mat4& p, const std::vector<mat4>& v);
	bool drawOpaquePass(unsigned int probeIndex, const mat4& p, const std::vector<mat4>& v);
	bool readBackSurfelCaches(unsigned int probeIndex);
	bool drawSkyVisibilityPass(const mat4& p, const std::vector<mat4>& v);

	bool eliminateDuplicatedSurfels();
	bool eliminateEmptyBricks();
	bool isBrickEmpty(const std::vector<Surfel>& surfels, const Brick& brick);
	bool assignSurfelRangeToBricks();
	bool findSurfelRangeForBrick(Brick& brick);
	bool assignBrickFactorToProbes();

	bool serializeProbes();
	bool serializeSurfels();
	bool serializeBricks();
	bool serializeBrickFactors();

	const unsigned int m_captureResolution = 128;
	const unsigned int m_sampleCountPerFace = m_captureResolution * m_captureResolution;
	const unsigned int m_subDivideDimension = 2;
	const unsigned int m_totalCaptureProbes = m_subDivideDimension * m_subDivideDimension * m_subDivideDimension;

	TextureDataComponent* m_testSampleCubemap;
	TextureDataComponent* m_testSample3DTexture;

	RenderPassDataComponent* m_RPDC_Bake;
	ShaderProgramComponent* m_SPC_Bake;
	SamplerDataComponent* m_SDC_Bake;
	TextureDataComponent* m_testSurfelTexture;

	GPUBufferDataComponent* m_GICameraGBDC;

	std::vector<ProbeCache> m_probeCaches;
	std::vector<Probe> m_probes;
	std::vector<BrickFactor> m_brickFactors;
	std::vector<Brick> m_brickCaches;
	std::vector<Brick> m_bricks;
	std::vector<Surfel> m_surfels;

	RenderPassDataComponent* m_RPDC_Relight;
	ShaderProgramComponent* m_SPC_Relight;
	SamplerDataComponent* m_SDC_Relight;

	std::function<void()> f_sceneLoadingFinishCallback;
}

bool GIBakePass::loadGIData()
{
	auto l_filePath = g_pModuleManager->getFileSystem()->getWorkingDirectory();
	auto l_currentSceneName = g_pModuleManager->getFileSystem()->getCurrentSceneName();

	std::ifstream l_probeFile;
	l_probeFile.open(l_filePath + "//Res//Scenes//" + l_currentSceneName + ".InnoProbe", std::ios::binary);

	if (l_probeFile.is_open())
	{
		IOService::deserializeVector(l_probeFile, m_probes);

		std::ifstream l_surfelFile;
		l_surfelFile.open(l_filePath + "//Res//Scenes//" + l_currentSceneName + ".InnoSurfel", std::ios::binary);
		IOService::deserializeVector(l_surfelFile, m_surfels);

		std::ifstream l_brickFile;
		l_brickFile.open(l_filePath + "//Res//Scenes//" + l_currentSceneName + ".InnoBrick", std::ios::binary);
		IOService::deserializeVector(l_brickFile, m_bricks);

		std::ifstream l_brickFactorFile;
		l_brickFactorFile.open(l_filePath + "//Res//Scenes//" + l_currentSceneName + ".InnoBrickFactor", std::ios::binary);
		IOService::deserializeVector(l_brickFactorFile, m_brickFactors);

		return true;
	}
	else
	{
		return false;
	}
}

bool GIBakePass::generateProbes()
{
	auto l_sceneAABB = g_pModuleManager->getPhysicsSystem()->getTotalSceneAABB();

	auto l_sceneCenter = l_sceneAABB.m_center;
	auto l_extendedAxisSize = l_sceneAABB.m_extend;
	l_extendedAxisSize = l_extendedAxisSize - vec4(32.0, 2.0f, 32.0f, 0.0f);
	l_extendedAxisSize.w = 0.0f;
	auto l_probeDistance = l_extendedAxisSize / (float)(m_subDivideDimension + 1);
	auto l_startPos = l_sceneAABB.m_center - (l_extendedAxisSize / 2.0f);
	l_startPos.x += l_probeDistance.x;
	l_startPos.z += l_probeDistance.z;
	auto l_currentPos = l_startPos;

	unsigned int l_probeIndex = 0;
	for (size_t i = 0; i < m_subDivideDimension; i++)
	{
		l_currentPos.y = l_startPos.y;
		for (size_t j = 0; j < m_subDivideDimension; j++)
		{
			l_currentPos.z = l_startPos.z;
			for (size_t k = 0; k < m_subDivideDimension; k++)
			{
				Probe l_probe;
				l_probe.pos = l_currentPos;
				m_probes.emplace_back(l_probe);

				ProbeCache l_probeCache;
				l_probeCache.pos = l_currentPos;
				m_probeCaches.emplace_back(l_probeCache);

				l_currentPos.z += l_probeDistance.z;
				l_probeIndex++;
			}
			l_currentPos.y += l_probeDistance.y;
		}
		l_currentPos.x += l_probeDistance.x;

		g_pModuleManager->getLogSystem()->Log(LogLevel::Verbose, "GIBakePass: Generating probes: ", (float)l_probeIndex * 100.0f / m_totalCaptureProbes, "%");
	}

	return true;
}

bool GIBakePass::generateBricks()
{
	auto l_sceneAABB = g_pModuleManager->getPhysicsSystem()->getTotalSceneAABB();

	auto l_sceneCenter = l_sceneAABB.m_center;
	auto l_extendedAxisSize = l_sceneAABB.m_extend;
	l_extendedAxisSize.w = 0.0f;
	auto l_startPos = l_sceneAABB.m_center - (l_extendedAxisSize / 2.0f);
	auto l_currentPos = l_startPos;

	auto l_brickSize = 16.0f;
	auto l_maxBrickCountX = (unsigned int)std::ceil(l_extendedAxisSize.x / l_brickSize);
	auto l_maxBrickCountY = (unsigned int)std::ceil(l_extendedAxisSize.y / l_brickSize);
	auto l_maxBrickCountZ = (unsigned int)std::ceil(l_extendedAxisSize.z / l_brickSize);

	auto l_totalBricks = l_maxBrickCountX * l_maxBrickCountY * l_maxBrickCountZ;

	unsigned int l_brickIndex = 0;
	for (size_t i = 0; i < l_maxBrickCountX; i++)
	{
		l_currentPos.y = l_startPos.y;
		for (size_t j = 0; j < l_maxBrickCountY; j++)
		{
			l_currentPos.z = l_startPos.z;
			for (size_t k = 0; k < l_maxBrickCountZ; k++)
			{
				AABB l_brickAABB;
				l_brickAABB.m_boundMin = l_currentPos;
				l_brickAABB.m_extend = vec4(l_brickSize, l_brickSize, l_brickSize, 0.0f);
				l_brickAABB.m_boundMax = l_currentPos + l_brickAABB.m_extend;
				l_brickAABB.m_center = l_currentPos + (l_brickAABB.m_extend / 2.0f);

				Brick l_brick;
				l_brick.boundBox = l_brickAABB;
				m_brickCaches.emplace_back(l_brick);

				l_currentPos.z += l_brickSize;
				l_brickIndex++;
			}
			l_currentPos.y += l_brickSize;
		}
		l_currentPos.x += l_brickSize;
		g_pModuleManager->getLogSystem()->Log(LogLevel::Verbose, "GIBakePass: Generating brick: ", (float)l_brickIndex * 100.0f / (float)l_totalBricks, "%");
	}

	return true;
}

bool GIBakePass::capture()
{
	auto l_cameraGPUData = g_pModuleManager->getRenderingFrontend()->getCameraGPUData();

	auto l_p = InnoMath::generatePerspectiveMatrix((90.0f / 180.0f) * PI<float>, 1.0f, l_cameraGPUData.zNear, l_cameraGPUData.zFar);

	auto l_rPX = InnoMath::lookAt(vec4(0.0f, 0.0f, 0.0f, 1.0f), vec4(1.0f, 0.0f, 0.0f, 1.0f), vec4(0.0f, -1.0f, 0.0f, 0.0f));
	auto l_rNX = InnoMath::lookAt(vec4(0.0f, 0.0f, 0.0f, 1.0f), vec4(-1.0f, 0.0f, 0.0f, 1.0f), vec4(0.0f, -1.0f, 0.0f, 0.0f));
	auto l_rPY = InnoMath::lookAt(vec4(0.0f, 0.0f, 0.0f, 1.0f), vec4(0.0f, 1.0f, 0.0f, 1.0f), vec4(0.0f, 0.0f, 1.0f, 0.0f));
	auto l_rNY = InnoMath::lookAt(vec4(0.0f, 0.0f, 0.0f, 1.0f), vec4(0.0f, -1.0f, 0.0f, 1.0f), vec4(0.0f, 0.0f, 1.0f, 0.0f));
	auto l_rPZ = InnoMath::lookAt(vec4(0.0f, 0.0f, 0.0f, 1.0f), vec4(0.0f, 0.0f, 1.0f, 1.0f), vec4(0.0f, -1.0f, 0.0f, 0.0f));
	auto l_rNZ = InnoMath::lookAt(vec4(0.0f, 0.0f, 0.0f, 1.0f), vec4(0.0f, 0.0f, -1.0f, 1.0f), vec4(0.0f, -1.0f, 0.0f, 0.0f));

	std::vector<mat4> l_v =
	{
		l_rPX, l_rNX, l_rPY, l_rNY, l_rPZ, l_rNZ
	};

	for (unsigned int i = 0; i < m_totalCaptureProbes; i++)
	{
		drawCubemaps(i, l_p, l_v);
		readBackSurfelCaches(i);
	}

	return true;
}

bool GIBakePass::drawCubemaps(unsigned int probeIndex, const mat4& p, const std::vector<mat4>& v)
{
	auto l_renderingConfig = g_pModuleManager->getRenderingFrontend()->getRenderingConfig();

	drawOpaquePass(probeIndex, p, v);

	drawSkyVisibilityPass(p, v);

	return true;
}

bool GIBakePass::drawOpaquePass(unsigned int probeIndex, const mat4& p, const std::vector<mat4>& v)
{
	auto l_t = InnoMath::getInvertTranslationMatrix(m_probes[probeIndex].pos);

	std::vector<mat4> l_GICameraGPUData(8);
	l_GICameraGPUData[0] = p;
	for (size_t i = 0; i < 6; i++)
	{
		l_GICameraGPUData[i + 1] = v[i];
	}
	l_GICameraGPUData[7] = l_t;

	g_pModuleManager->getRenderingServer()->UploadGPUBufferDataComponent(m_GICameraGBDC, l_GICameraGPUData);

	auto l_MeshGBDC = GetGPUBufferDataComponent(GPUBufferUsageType::Mesh);
	auto l_MaterialGBDC = GetGPUBufferDataComponent(GPUBufferUsageType::Material);

	g_pModuleManager->getRenderingServer()->CommandListBegin(m_RPDC_Bake, 0);
	g_pModuleManager->getRenderingServer()->BindRenderPassDataComponent(m_RPDC_Bake);
	g_pModuleManager->getRenderingServer()->CleanRenderTargets(m_RPDC_Bake);
	g_pModuleManager->getRenderingServer()->ActivateResourceBinder(m_RPDC_Bake, ShaderStage::Pixel, m_SDC_Bake->m_ResourceBinder, 8, 0);
	g_pModuleManager->getRenderingServer()->ActivateResourceBinder(m_RPDC_Bake, ShaderStage::Geometry, m_GICameraGBDC->m_ResourceBinder, 0, 11, Accessibility::ReadOnly);

	unsigned int l_offset = 0;

	auto l_totalDrawCallCount = g_pModuleManager->getRenderingFrontend()->getGIPassDrawCallCount();
	for (unsigned int i = 0; i < l_totalDrawCallCount; i++)
	{
		auto l_GIPassGPUData = g_pModuleManager->getRenderingFrontend()->getGIPassGPUData()[i];

		if (l_GIPassGPUData.mesh->m_objectStatus == ObjectStatus::Activated)
		{
			g_pModuleManager->getRenderingServer()->ActivateResourceBinder(m_RPDC_Bake, ShaderStage::Vertex, l_MeshGBDC->m_ResourceBinder, 1, 1, Accessibility::ReadOnly, l_offset, 1);
			g_pModuleManager->getRenderingServer()->ActivateResourceBinder(m_RPDC_Bake, ShaderStage::Pixel, l_MaterialGBDC->m_ResourceBinder, 2, 2, Accessibility::ReadOnly, l_offset, 1);

			if (l_GIPassGPUData.material->m_objectStatus == ObjectStatus::Activated)
			{
				g_pModuleManager->getRenderingServer()->ActivateResourceBinder(m_RPDC_Bake, ShaderStage::Pixel, l_GIPassGPUData.material->m_ResourceBinders[0], 3, 0);
				g_pModuleManager->getRenderingServer()->ActivateResourceBinder(m_RPDC_Bake, ShaderStage::Pixel, l_GIPassGPUData.material->m_ResourceBinders[1], 4, 1);
				g_pModuleManager->getRenderingServer()->ActivateResourceBinder(m_RPDC_Bake, ShaderStage::Pixel, l_GIPassGPUData.material->m_ResourceBinders[2], 5, 2);
				g_pModuleManager->getRenderingServer()->ActivateResourceBinder(m_RPDC_Bake, ShaderStage::Pixel, l_GIPassGPUData.material->m_ResourceBinders[3], 6, 3);
				g_pModuleManager->getRenderingServer()->ActivateResourceBinder(m_RPDC_Bake, ShaderStage::Pixel, l_GIPassGPUData.material->m_ResourceBinders[4], 7, 4);
			}

			g_pModuleManager->getRenderingServer()->DispatchDrawCall(m_RPDC_Bake, l_GIPassGPUData.mesh);

			if (l_GIPassGPUData.material->m_objectStatus == ObjectStatus::Activated)
			{
				g_pModuleManager->getRenderingServer()->DeactivateResourceBinder(m_RPDC_Bake, ShaderStage::Pixel, l_GIPassGPUData.material->m_ResourceBinders[0], 3, 0);
				g_pModuleManager->getRenderingServer()->DeactivateResourceBinder(m_RPDC_Bake, ShaderStage::Pixel, l_GIPassGPUData.material->m_ResourceBinders[1], 4, 1);
				g_pModuleManager->getRenderingServer()->DeactivateResourceBinder(m_RPDC_Bake, ShaderStage::Pixel, l_GIPassGPUData.material->m_ResourceBinders[2], 5, 2);
				g_pModuleManager->getRenderingServer()->DeactivateResourceBinder(m_RPDC_Bake, ShaderStage::Pixel, l_GIPassGPUData.material->m_ResourceBinders[3], 6, 3);
				g_pModuleManager->getRenderingServer()->DeactivateResourceBinder(m_RPDC_Bake, ShaderStage::Pixel, l_GIPassGPUData.material->m_ResourceBinders[4], 7, 4);
			}
		}

		l_offset++;
	}

	g_pModuleManager->getRenderingServer()->CommandListEnd(m_RPDC_Bake);

	g_pModuleManager->getRenderingServer()->ExecuteCommandList(m_RPDC_Bake);

	g_pModuleManager->getRenderingServer()->WaitForFrame(m_RPDC_Bake);

	return true;
}

bool GIBakePass::readBackSurfelCaches(unsigned int probeIndex)
{
	auto l_posWSMetallic = g_pModuleManager->getRenderingServer()->ReadTextureBackToCPU(m_RPDC_Bake, m_RPDC_Bake->m_RenderTargets[0]);
	auto l_normalRoughness = g_pModuleManager->getRenderingServer()->ReadTextureBackToCPU(m_RPDC_Bake, m_RPDC_Bake->m_RenderTargets[1]);
	auto l_albedoAO = g_pModuleManager->getRenderingServer()->ReadTextureBackToCPU(m_RPDC_Bake, m_RPDC_Bake->m_RenderTargets[2]);

	auto l_surfelSize = l_posWSMetallic.size();

	std::vector<Surfel> l_surfels(l_surfelSize);
	for (size_t i = 0; i < l_surfelSize; i++)
	{
		l_surfels[i].pos = l_posWSMetallic[i];
		l_surfels[i].pos.w = 1.0f;
		l_surfels[i].normal = l_normalRoughness[i];
		l_surfels[i].normal.w = 0.0f;
		l_surfels[i].albedo = l_albedoAO[i];
		l_surfels[i].albedo.w = 1.0f;
		l_surfels[i].MRAT.x = l_posWSMetallic[i].w;
		l_surfels[i].MRAT.y = l_normalRoughness[i].w;
		l_surfels[i].MRAT.z = l_albedoAO[i].w;
		l_surfels[i].MRAT.w = 1.0f;
	}

	m_probeCaches[probeIndex].surfelCaches = l_surfels;

	m_surfels.insert(m_surfels.end(), l_surfels.begin(), l_surfels.end());

	return true;
}

bool GIBakePass::drawSkyVisibilityPass(const mat4& p, const std::vector<mat4>& v)
{
	auto l_MDC = g_pModuleManager->getRenderingFrontend()->getMeshDataComponent(MeshShapeType::Cube);

	auto l_capturePos = vec4(0.0f, 0.0f, 0.0f, 1.0f);
	auto l_t = InnoMath::getInvertTranslationMatrix(l_capturePos);

	return true;
}

bool GIBakePass::eliminateDuplicatedSurfels()
{
	m_surfels.erase(std::unique(m_surfels.begin(), m_surfels.end()), m_surfels.end());
	m_surfels.shrink_to_fit();

	std::sort(m_surfels.begin(), m_surfels.end(), [&](Surfel A, Surfel B)
	{
		if (A.pos.x != B.pos.x) {
			return A.pos.x < B.pos.x;
		}
		if (A.pos.y != B.pos.y) {
			return A.pos.y < B.pos.y;
		}
		return A.pos.z < B.pos.z;
	});

	return true;
}

bool GIBakePass::eliminateEmptyBricks()
{
	for (size_t i = 0; i < m_brickCaches.size(); i++)
	{
		if (!isBrickEmpty(m_surfels, m_brickCaches[i]))
		{
			m_bricks.emplace_back(m_brickCaches[i]);
		}
	}

	m_bricks.shrink_to_fit();

	std::sort(m_bricks.begin(), m_bricks.end(), [&](Brick A, Brick B)
	{
		if (A.boundBox.m_center.x != B.boundBox.m_center.x) {
			return A.boundBox.m_center.x < B.boundBox.m_center.x;
		}
		if (A.boundBox.m_center.y != B.boundBox.m_center.y) {
			return A.boundBox.m_center.y < B.boundBox.m_center.y;
		}
		return A.boundBox.m_center.z < B.boundBox.m_center.z;
	});

	return true;
}

bool GIBakePass::isBrickEmpty(const std::vector<Surfel>& surfels, const Brick& brick)
{
	auto l_nearestSurfel = std::find_if(surfels.begin(), surfels.end(), [&](Surfel val) {
		return InnoMath::isAGreaterThanBVec3(val.pos, brick.boundBox.m_boundMin);
	});

	if (l_nearestSurfel != surfels.end())
	{
		auto l_farestSurfel = std::find_if(surfels.begin(), surfels.end(), [&](Surfel val) {
			return InnoMath::isALessThanBVec3(val.pos, brick.boundBox.m_boundMax);
		});

		if (l_farestSurfel != surfels.end())
		{
			return false;
		}
	}

	return true;
}

bool GIBakePass::assignSurfelRangeToBricks()
{
	for (size_t i = 0; i < m_bricks.size(); i++)
	{
		findSurfelRangeForBrick(m_bricks[i]);
	}

	return true;
}

bool GIBakePass::findSurfelRangeForBrick(Brick& brick)
{
	auto l_firstSurfel = std::find_if(m_surfels.begin(), m_surfels.end(), [&](Surfel val) {
		return InnoMath::isAGreaterThanBVec3(val.pos, brick.boundBox.m_boundMin);
	});

	auto l_firstSurfelIndex = std::distance(m_surfels.begin(), l_firstSurfel);
	brick.surfelRangeBegin = (unsigned int)l_firstSurfelIndex;

	auto l_lastSurfel = std::find_if(m_surfels.begin(), m_surfels.end(), [&](Surfel val) {
		return InnoMath::isAGreaterThanBVec3(val.pos, brick.boundBox.m_boundMax);
	});

	auto l_lastSurfelIndex = std::distance(m_surfels.begin(), l_lastSurfel);
	brick.surfelRangeEnd = (unsigned int)l_lastSurfelIndex;

	return true;
}

bool GIBakePass::assignBrickFactorToProbes()
{
	for (size_t i = 0; i < m_probes.size(); i++)
	{
		m_probes[i].brickFactorRangeBegin = (unsigned int)m_brickFactors.size();

		for (size_t j = 0; j < m_bricks.size(); j++)
		{
			if (!isBrickEmpty(m_probeCaches[i].surfelCaches, m_bricks[j]))
			{
				BrickFactor l_brickFactor;
				l_brickFactor.basisWeight = 1.0f;
				l_brickFactor.brickIndex = (unsigned int)j;

				m_brickFactors.emplace_back(l_brickFactor);
			}
		}

		m_probes[i].brickFactorRangeEnd = (unsigned int)m_brickFactors.size() - 1;
	}

	return true;
}

bool GIBakePass::serializeProbes()
{
	auto l_filePath = g_pModuleManager->getFileSystem()->getWorkingDirectory();
	auto l_currentSceneName = g_pModuleManager->getFileSystem()->getCurrentSceneName();

	std::ofstream l_file;
	l_file.open(l_filePath + "//Res//Scenes//" + l_currentSceneName + ".InnoProbe", std::ios::binary);
	IOService::serializeVector(l_file, m_probes);

	return true;
}

bool GIBakePass::serializeSurfels()
{
	auto l_filePath = g_pModuleManager->getFileSystem()->getWorkingDirectory();
	auto l_currentSceneName = g_pModuleManager->getFileSystem()->getCurrentSceneName();

	std::ofstream l_file;
	l_file.open(l_filePath + "//Res//Scenes//" + l_currentSceneName + ".InnoSurfel", std::ios::binary);
	IOService::serializeVector(l_file, m_surfels);

	return true;
}

bool GIBakePass::serializeBricks()
{
	auto l_filePath = g_pModuleManager->getFileSystem()->getWorkingDirectory();
	auto l_currentSceneName = g_pModuleManager->getFileSystem()->getCurrentSceneName();

	std::ofstream l_file;
	l_file.open(l_filePath + "//Res//Scenes//" + l_currentSceneName + ".InnoBrick", std::ios::binary);
	IOService::serializeVector(l_file, m_bricks);

	return true;
}

bool GIBakePass::serializeBrickFactors()
{
	auto l_filePath = g_pModuleManager->getFileSystem()->getWorkingDirectory();
	auto l_currentSceneName = g_pModuleManager->getFileSystem()->getCurrentSceneName();

	std::ofstream l_file;
	l_file.open(l_filePath + "//Res//Scenes//" + l_currentSceneName + ".InnoBrickFactor", std::ios::binary);
	IOService::serializeVector(l_file, m_brickFactors);

	return true;
}

bool GIBakePass::Setup()
{
	f_sceneLoadingFinishCallback = []() { loadGIData(); };
	g_pModuleManager->getFileSystem()->addSceneLoadingFinishCallback(&f_sceneLoadingFinishCallback);

	////
	m_testSampleCubemap = g_pModuleManager->getRenderingServer()->AddTextureDataComponent("TestSampleCubemap/");

	std::vector<vec4> l_cubemapTextureSamples(m_captureResolution * m_captureResolution * 6);
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
			auto l_color = l_faceColors[i] * 2.0f * (float)j / (float)m_sampleCountPerFace;
			l_color.w = 1.0f;
			l_cubemapTextureSamples[i * m_sampleCountPerFace + j] = l_color;
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
	m_testSampleCubemap->m_textureDataDesc.Width = m_captureResolution;
	m_testSampleCubemap->m_textureDataDesc.Height = m_captureResolution;
	m_testSampleCubemap->m_textureDataDesc.PixelDataType = TexturePixelDataType::FLOAT32;
	m_testSampleCubemap->m_textureData = &l_cubemapTextureSamples[0];

	g_pModuleManager->getRenderingServer()->InitializeTextureDataComponent(m_testSampleCubemap);

	////
	std::vector<vec4> l_3DTextureSamples(m_captureResolution * m_captureResolution * m_captureResolution);
	size_t l_pixelIndex = 0;
	for (size_t i = 0; i < m_captureResolution; i++)
	{
		for (size_t j = 0; j < m_captureResolution; j++)
		{
			for (size_t k = 0; k < m_captureResolution; k++)
			{
				l_3DTextureSamples[l_pixelIndex] = vec4((float)i / (float)m_captureResolution, (float)j / (float)m_captureResolution, (float)k / (float)m_captureResolution, 1.0f);
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
	m_testSample3DTexture->m_textureDataDesc.Width = m_captureResolution;
	m_testSample3DTexture->m_textureDataDesc.Height = m_captureResolution;
	m_testSample3DTexture->m_textureDataDesc.DepthOrArraySize = m_captureResolution;
	m_testSample3DTexture->m_textureDataDesc.PixelDataType = TexturePixelDataType::FLOAT32;
	m_testSample3DTexture->m_textureData = &l_3DTextureSamples[0];

	g_pModuleManager->getRenderingServer()->InitializeTextureDataComponent(m_testSample3DTexture);

	////
	m_SPC_Bake = g_pModuleManager->getRenderingServer()->AddShaderProgramComponent("GIBakeOpaquePass/");

	m_SPC_Bake->m_ShaderFilePaths.m_VSPath = "GIOpaquePass.vert/";
	m_SPC_Bake->m_ShaderFilePaths.m_GSPath = "GIOpaquePass.geom/";
	m_SPC_Bake->m_ShaderFilePaths.m_PSPath = "GIOpaquePass.frag/";

	g_pModuleManager->getRenderingServer()->InitializeShaderProgramComponent(m_SPC_Bake);

	m_RPDC_Bake = g_pModuleManager->getRenderingServer()->AddRenderPassDataComponent("GIBakeOpaquePass/");

	m_RPDC_Bake->m_RenderPassDesc = l_RenderPassDesc;

	m_RPDC_Bake->m_RenderPassDesc.m_RenderTargetCount = 3;

	m_RPDC_Bake->m_RenderPassDesc.m_RenderTargetDesc.SamplerType = TextureSamplerType::SamplerCubemap;
	m_RPDC_Bake->m_RenderPassDesc.m_RenderTargetDesc.Width = m_captureResolution;
	m_RPDC_Bake->m_RenderPassDesc.m_RenderTargetDesc.Height = m_captureResolution;
	m_RPDC_Bake->m_RenderPassDesc.m_RenderTargetDesc.PixelDataType = TexturePixelDataType::FLOAT32;

	m_RPDC_Bake->m_RenderPassDesc.m_GraphicsPipelineDesc.m_DepthStencilDesc.m_UseDepthBuffer = true;
	m_RPDC_Bake->m_RenderPassDesc.m_GraphicsPipelineDesc.m_DepthStencilDesc.m_AllowDepthWrite = true;
	m_RPDC_Bake->m_RenderPassDesc.m_GraphicsPipelineDesc.m_DepthStencilDesc.m_DepthComparisionFunction = ComparisionFunction::LessEqual;

	m_RPDC_Bake->m_RenderPassDesc.m_GraphicsPipelineDesc.m_DepthStencilDesc.m_UseStencilBuffer = true;
	m_RPDC_Bake->m_RenderPassDesc.m_GraphicsPipelineDesc.m_DepthStencilDesc.m_AllowStencilWrite = true;
	m_RPDC_Bake->m_RenderPassDesc.m_GraphicsPipelineDesc.m_DepthStencilDesc.m_StencilReference = 0x01;
	m_RPDC_Bake->m_RenderPassDesc.m_GraphicsPipelineDesc.m_DepthStencilDesc.m_FrontFaceStencilPassOperation = StencilOperation::Replace;
	m_RPDC_Bake->m_RenderPassDesc.m_GraphicsPipelineDesc.m_DepthStencilDesc.m_FrontFaceStencilComparisionFunction = ComparisionFunction::Always;
	m_RPDC_Bake->m_RenderPassDesc.m_GraphicsPipelineDesc.m_DepthStencilDesc.m_BackFaceStencilPassOperation = StencilOperation::Replace;
	m_RPDC_Bake->m_RenderPassDesc.m_GraphicsPipelineDesc.m_DepthStencilDesc.m_BackFaceStencilComparisionFunction = ComparisionFunction::Always;

	m_RPDC_Bake->m_RenderPassDesc.m_GraphicsPipelineDesc.m_RasterizerDesc.m_UseCulling = true;
	m_RPDC_Bake->m_RenderPassDesc.m_GraphicsPipelineDesc.m_ViewportDesc.m_Width = m_captureResolution;
	m_RPDC_Bake->m_RenderPassDesc.m_GraphicsPipelineDesc.m_ViewportDesc.m_Height = m_captureResolution;

	m_RPDC_Bake->m_ResourceBinderLayoutDescs.resize(9);
	m_RPDC_Bake->m_ResourceBinderLayoutDescs[0].m_ResourceBinderType = ResourceBinderType::Buffer;
	m_RPDC_Bake->m_ResourceBinderLayoutDescs[0].m_GlobalSlot = 0;
	m_RPDC_Bake->m_ResourceBinderLayoutDescs[0].m_LocalSlot = 11;

	m_RPDC_Bake->m_ResourceBinderLayoutDescs[1].m_ResourceBinderType = ResourceBinderType::Buffer;
	m_RPDC_Bake->m_ResourceBinderLayoutDescs[1].m_GlobalSlot = 1;
	m_RPDC_Bake->m_ResourceBinderLayoutDescs[1].m_LocalSlot = 1;

	m_RPDC_Bake->m_ResourceBinderLayoutDescs[2].m_ResourceBinderType = ResourceBinderType::Buffer;
	m_RPDC_Bake->m_ResourceBinderLayoutDescs[2].m_GlobalSlot = 2;
	m_RPDC_Bake->m_ResourceBinderLayoutDescs[2].m_LocalSlot = 2;

	m_RPDC_Bake->m_ResourceBinderLayoutDescs[3].m_ResourceBinderType = ResourceBinderType::Image;
	m_RPDC_Bake->m_ResourceBinderLayoutDescs[3].m_GlobalSlot = 3;
	m_RPDC_Bake->m_ResourceBinderLayoutDescs[3].m_LocalSlot = 0;
	m_RPDC_Bake->m_ResourceBinderLayoutDescs[3].m_IsRanged = true;

	m_RPDC_Bake->m_ResourceBinderLayoutDescs[4].m_ResourceBinderType = ResourceBinderType::Image;
	m_RPDC_Bake->m_ResourceBinderLayoutDescs[4].m_GlobalSlot = 4;
	m_RPDC_Bake->m_ResourceBinderLayoutDescs[4].m_LocalSlot = 1;
	m_RPDC_Bake->m_ResourceBinderLayoutDescs[4].m_IsRanged = true;

	m_RPDC_Bake->m_ResourceBinderLayoutDescs[5].m_ResourceBinderType = ResourceBinderType::Image;
	m_RPDC_Bake->m_ResourceBinderLayoutDescs[5].m_GlobalSlot = 5;
	m_RPDC_Bake->m_ResourceBinderLayoutDescs[5].m_LocalSlot = 2;
	m_RPDC_Bake->m_ResourceBinderLayoutDescs[5].m_ResourceCount = 1;
	m_RPDC_Bake->m_ResourceBinderLayoutDescs[5].m_IsRanged = true;

	m_RPDC_Bake->m_ResourceBinderLayoutDescs[6].m_ResourceBinderType = ResourceBinderType::Image;
	m_RPDC_Bake->m_ResourceBinderLayoutDescs[6].m_GlobalSlot = 6;
	m_RPDC_Bake->m_ResourceBinderLayoutDescs[6].m_LocalSlot = 3;
	m_RPDC_Bake->m_ResourceBinderLayoutDescs[6].m_IsRanged = true;

	m_RPDC_Bake->m_ResourceBinderLayoutDescs[7].m_ResourceBinderType = ResourceBinderType::Image;
	m_RPDC_Bake->m_ResourceBinderLayoutDescs[7].m_GlobalSlot = 7;
	m_RPDC_Bake->m_ResourceBinderLayoutDescs[7].m_LocalSlot = 4;
	m_RPDC_Bake->m_ResourceBinderLayoutDescs[7].m_IsRanged = true;

	m_RPDC_Bake->m_ResourceBinderLayoutDescs[8].m_ResourceBinderType = ResourceBinderType::Sampler;
	m_RPDC_Bake->m_ResourceBinderLayoutDescs[8].m_GlobalSlot = 8;
	m_RPDC_Bake->m_ResourceBinderLayoutDescs[8].m_LocalSlot = 0;
	m_RPDC_Bake->m_ResourceBinderLayoutDescs[8].m_IsRanged = true;

	m_RPDC_Bake->m_ShaderProgram = m_SPC_Bake;

	g_pModuleManager->getRenderingServer()->InitializeRenderPassDataComponent(m_RPDC_Bake);

	m_SDC_Bake = g_pModuleManager->getRenderingServer()->AddSamplerDataComponent("GIBakeOpaquePass/");

	m_SDC_Bake->m_SamplerDesc.m_WrapMethodU = TextureWrapMethod::Repeat;
	m_SDC_Bake->m_SamplerDesc.m_WrapMethodV = TextureWrapMethod::Repeat;

	g_pModuleManager->getRenderingServer()->InitializeSamplerDataComponent(m_SDC_Bake);

	m_GICameraGBDC = g_pModuleManager->getRenderingServer()->AddGPUBufferDataComponent("GICameraGPUBuffer/");
	m_GICameraGBDC->m_ElementSize = sizeof(mat4) * 8;
	m_GICameraGBDC->m_ElementCount = 1;
	m_GICameraGBDC->m_BindingPoint = 11;

	g_pModuleManager->getRenderingServer()->InitializeGPUBufferDataComponent(m_GICameraGBDC);

	////
	m_testSurfelTexture = g_pModuleManager->getRenderingServer()->AddTextureDataComponent("TestSurfel/");
	m_testSurfelTexture->m_textureDataDesc = m_RPDC_Bake->m_RenderPassDesc.m_RenderTargetDesc;
	m_testSurfelTexture->m_textureDataDesc.UsageType = TextureUsageType::Normal;

	return true;
}

bool GIBakePass::Initialize()
{
	return true;
}

bool GIBakePass::Bake()
{
	auto l_MeshGBDC = GetGPUBufferDataComponent(GPUBufferUsageType::Mesh);
	auto l_MaterialGBDC = GetGPUBufferDataComponent(GPUBufferUsageType::Material);

	g_pModuleManager->getRenderingServer()->UploadGPUBufferDataComponent(l_MeshGBDC, g_pModuleManager->getRenderingFrontend()->getGIPassMeshGPUData());
	g_pModuleManager->getRenderingServer()->UploadGPUBufferDataComponent(l_MaterialGBDC, g_pModuleManager->getRenderingFrontend()->getGIPassMaterialGPUData());

	m_probeCaches.clear();
	m_probes.clear();
	m_surfels.clear();
	m_bricks.clear();
	m_brickCaches.clear();
	m_brickFactors.clear();

	generateProbes();
	generateBricks();

	capture();

	eliminateDuplicatedSurfels();
	eliminateEmptyBricks();

	assignSurfelRangeToBricks();
	assignBrickFactorToProbes();

	serializeProbes();
	serializeSurfels();
	serializeBricks();
	serializeBrickFactors();

	return true;
}

bool GIBakePass::PrepareCommandList()
{
	return true;
}

bool GIBakePass::ExecuteCommandList()
{
	return true;
}

bool GIBakePass::Terminate()
{
	g_pModuleManager->getRenderingServer()->DeleteRenderPassDataComponent(m_RPDC_Bake);

	return true;
}

RenderPassDataComponent * GIBakePass::GetRPDC()
{
	return m_RPDC_Bake;
}

ShaderProgramComponent * GIBakePass::GetSPC()
{
	return m_SPC_Bake;
}

const std::vector<Surfel>& GIBakePass::GetSurfels()
{
	return m_surfels;
}

const std::vector<Brick>& GIBakePass::GetBricks()
{
	return m_bricks;
}

const std::vector<BrickFactor>& GIBakePass::GetBrickFactors()
{
	return m_brickFactors;
}

const std::vector<Probe>& GIBakePass::GetProbes()
{
	return m_probes;
}

unsigned int GIBakePass::GetProbeDimension()
{
	return m_subDivideDimension;
}