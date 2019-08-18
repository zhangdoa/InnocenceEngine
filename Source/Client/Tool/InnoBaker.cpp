#include "InnoBaker.h"
#include "../DefaultGPUBuffers/DefaultGPUBuffers.h"
#include "../../Engine/Common/CommonMacro.inl"
#include "../../Engine/ComponentManager/ITransformComponentManager.h"
#include "../../Engine/ComponentManager/IVisibleComponentManager.h"

#include "../../Engine/Common/InnoMathHelper.h"

#include "../../Engine/ModuleManager/IModuleManager.h"
INNO_ENGINE_API extern IModuleManager* g_pModuleManager;

#include "../../Engine/Core/IOService.h"

struct BrickCache
{
	vec4 pos;
	std::vector<Surfel> surfelCaches;
};

struct BrickCacheSummary
{
	vec4 pos;
	size_t fileIndex;
	size_t fileSize;
};

namespace InnoBakerNS
{
	std::string parseFileName(const std::string & fileName);

	bool gatherStaticMeshData();
	bool generateProbeCaches(std::vector<Probe>& probesForSurfelCaches, std::vector<Probe>& probesForRuntime);
	vec4 generateProbes(std::vector<Probe>& probes, const std::vector<vec4>& heightMap, unsigned int probeMapSamplingInterval);
	bool serializeProbeInfos(vec4 probeCounts);

	bool captureSurfels(std::vector<Probe>& probesForSurfelCaches, std::vector<Probe>& probesForRuntime);
	bool drawOpaquePass(Probe& probe, const mat4& p, const std::vector<mat4>& v);
	bool readBackSurfelCaches(std::vector<Surfel>& surfelCaches);
	bool eliminateDuplicatedSurfels(std::vector<Surfel>& surfelCaches);
	bool drawSkyVisibilityPass(Probe& probe, const mat4& p, const std::vector<mat4>& v);

	bool serializeSurfelCaches(const std::vector<Surfel>& surfelCaches);

	bool generateBrickCaches(std::vector<Surfel>& surfelCaches);
	bool serializeBrickCaches(const std::vector<BrickCache>& brickCaches);

	bool deserializeBrickCaches(const std::vector<BrickCacheSummary>& brickCacheSummaries, std::vector<BrickCache>& brickCaches);
	bool generateBricks(const std::vector<BrickCache>& brickCaches);
	bool serializeSurfels(const std::vector<Surfel>& surfels);
	bool serializeBricks(const std::vector<Brick>& bricks);

	bool assignBrickFactorToProbesByGPU(const std::vector<Brick>& bricks, std::vector<Probe>& probes);
	bool drawBricks(vec4 pos, unsigned int bricksCount, const mat4 & p, const std::vector<mat4>& v);
	bool readBackBrickFactors(Probe& probe, std::vector<BrickFactor>& brickFactors);

	bool serializeBrickFactors(const std::vector<BrickFactor>& brickFactors);
	bool serializeProbes(const std::vector<Probe>& probes);

	std::string m_exportFileName;

	unsigned int m_staticMeshDrawCallCount = 0;
	std::vector<OpaquePassDrawCallData> m_staticMeshDrawCallData;
	std::vector<MeshGPUData> m_staticMeshMeshGPUData;
	std::vector<MaterialGPUData> m_staticMeshMaterialGPUData;

	const unsigned int m_probeMapResolution = 1024;
	const float m_probeHeightOffset = 4.0f;
	const unsigned int m_probeCacheInterval = 128;
	const unsigned int m_probeInterval = 32;
	const unsigned int m_captureResolution = 64;
	const unsigned int m_sampleCountPerFace = m_captureResolution * m_captureResolution;
	const vec4 m_brickSize = vec4(8.0f, 8.0f, 8.0f, 0.0f);

	RenderPassDataComponent* m_RPDC_Probe;
	ShaderProgramComponent* m_SPC_Probe;

	RenderPassDataComponent* m_RPDC_Surfel;
	ShaderProgramComponent* m_SPC_Surfel;
	SamplerDataComponent* m_SDC_Surfel;

	RenderPassDataComponent* m_RPDC_BrickFactor;
	ShaderProgramComponent* m_SPC_BrickFactor;
}

using namespace InnoBakerNS;
using namespace DefaultGPUBuffers;

std::string InnoBakerNS::parseFileName(const std::string & fileName)
{
	auto l_startOffset = fileName.find_last_of("/");
	auto l_endOffset = fileName.find_last_of(".");
	auto l_result = fileName.substr(l_startOffset + 1, l_endOffset - l_startOffset - 1);

	return l_result;
}

bool InnoBakerNS::gatherStaticMeshData()
{
	g_pModuleManager->getLogSystem()->Log(LogLevel::Success, "InnoBakerNS: Gathering static meshes...");

	unsigned int l_index = 0;

	auto l_visibleComponents = GetComponentManager(VisibleComponent)->GetAllComponents();
	for (auto visibleComponent : l_visibleComponents)
	{
		if (visibleComponent->m_visiblilityType == VisiblilityType::Opaque
			&& visibleComponent->m_objectStatus == ObjectStatus::Activated
			&& visibleComponent->m_meshUsageType == MeshUsageType::Static
			)
		{
			auto l_transformComponent = GetComponent(TransformComponent, visibleComponent->m_parentEntity);
			auto l_globalTm = l_transformComponent->m_globalTransformMatrix.m_transformationMat;

			for (auto& l_modelPair : visibleComponent->m_modelMap)
			{
				OpaquePassDrawCallData l_staticMeshGPUData;

				l_staticMeshGPUData.mesh = l_modelPair.first;
				l_staticMeshGPUData.material = l_modelPair.second;

				MeshGPUData l_meshGPUData;

				l_meshGPUData.m = l_transformComponent->m_globalTransformMatrix.m_transformationMat;
				l_meshGPUData.m_prev = l_transformComponent->m_globalTransformMatrix_prev.m_transformationMat;
				l_meshGPUData.normalMat = l_transformComponent->m_globalTransformMatrix.m_rotationMat;
				l_meshGPUData.UUID = (float)visibleComponent->m_UUID;

				MaterialGPUData l_materialGPUData;

				l_materialGPUData.useNormalTexture = !(l_staticMeshGPUData.material->m_normalTexture == nullptr);
				l_materialGPUData.useAlbedoTexture = !(l_staticMeshGPUData.material->m_albedoTexture == nullptr);
				l_materialGPUData.useMetallicTexture = !(l_staticMeshGPUData.material->m_metallicTexture == nullptr);
				l_materialGPUData.useRoughnessTexture = !(l_staticMeshGPUData.material->m_roughnessTexture == nullptr);
				l_materialGPUData.useAOTexture = !(l_staticMeshGPUData.material->m_aoTexture == nullptr);

				l_materialGPUData.customMaterial = l_modelPair.second->m_meshCustomMaterial;

				m_staticMeshDrawCallData[l_index] = l_staticMeshGPUData;
				m_staticMeshMeshGPUData[l_index] = l_meshGPUData;
				m_staticMeshMaterialGPUData[l_index] = l_materialGPUData;
				l_index++;
			}
		}
	}

	m_staticMeshDrawCallCount = l_index;

	g_pModuleManager->getLogSystem()->Log(LogLevel::Success, "InnoBakerNS: There are ", m_staticMeshDrawCallCount, " static meshes in current scene.");

	auto l_MeshGBDC = GetGPUBufferDataComponent(GPUBufferUsageType::Mesh);
	auto l_MaterialGBDC = GetGPUBufferDataComponent(GPUBufferUsageType::Material);

	g_pModuleManager->getRenderingServer()->UploadGPUBufferDataComponent(l_MeshGBDC, m_staticMeshMeshGPUData, 0, m_staticMeshMeshGPUData.size());
	g_pModuleManager->getRenderingServer()->UploadGPUBufferDataComponent(l_MaterialGBDC, m_staticMeshMaterialGPUData, 0, m_staticMeshMaterialGPUData.size());

	return true;
}

bool InnoBakerNS::generateProbeCaches(std::vector<Probe>& probesForSurfelCaches, std::vector<Probe>& probesForRuntime)
{
	g_pModuleManager->getLogSystem()->Log(LogLevel::Success, "InnoBakerNS: Generate probe caches...");

	auto l_sceneAABB = g_pModuleManager->getPhysicsSystem()->getStaticSceneAABB();

	auto l_startPos = l_sceneAABB.m_boundMin;
	auto l_sceneCenter = l_sceneAABB.m_center;
	auto l_extendedAxisSize = l_sceneAABB.m_extend;

	auto l_p = InnoMath::generateOrthographicMatrix(-l_extendedAxisSize.x / 2.0f, l_extendedAxisSize.x / 2.0f, -l_extendedAxisSize.z / 2.0f, l_extendedAxisSize.z / 2.0f, -l_extendedAxisSize.y / 2.0f, l_extendedAxisSize.y / 2.0f);

	std::vector<mat4> l_GICameraGPUData(8);
	l_GICameraGPUData[0] = l_p;
	l_GICameraGPUData[1] = InnoMath::lookAt(vec4(0.0f, 0.0f, 0.0f, 1.0f), vec4(0.0f, -1.0f, 0.0f, 1.0f), vec4(0.0f, 0.0f, 1.0f, 0.0f));
	l_GICameraGPUData[7] = InnoMath::generateIdentityMatrix<float>();

	g_pModuleManager->getRenderingServer()->UploadGPUBufferDataComponent(GetGPUBufferDataComponent(GPUBufferUsageType::GICamera), l_GICameraGPUData);

	g_pModuleManager->getLogSystem()->Log(LogLevel::Success, "InnoBakerNS: Start to draw probe height map...");

	auto l_MeshGBDC = GetGPUBufferDataComponent(GPUBufferUsageType::Mesh);

	g_pModuleManager->getRenderingServer()->CommandListBegin(m_RPDC_Probe, 0);
	g_pModuleManager->getRenderingServer()->BindRenderPassDataComponent(m_RPDC_Probe);
	g_pModuleManager->getRenderingServer()->CleanRenderTargets(m_RPDC_Probe);
	g_pModuleManager->getRenderingServer()->ActivateResourceBinder(m_RPDC_Probe, ShaderStage::Vertex, GetGPUBufferDataComponent(GPUBufferUsageType::GICamera)->m_ResourceBinder, 0, 10, Accessibility::ReadOnly);

	unsigned int l_offset = 0;

	for (unsigned int i = 0; i < m_staticMeshDrawCallCount; i++)
	{
		auto l_staticMeshGPUData = m_staticMeshDrawCallData[i];

		if (l_staticMeshGPUData.mesh->m_objectStatus == ObjectStatus::Activated)
		{
			g_pModuleManager->getRenderingServer()->ActivateResourceBinder(m_RPDC_Probe, ShaderStage::Vertex, l_MeshGBDC->m_ResourceBinder, 1, 1, Accessibility::ReadOnly, l_offset, 1);

			g_pModuleManager->getRenderingServer()->DispatchDrawCall(m_RPDC_Probe, l_staticMeshGPUData.mesh);
		}

		l_offset++;
	}

	g_pModuleManager->getRenderingServer()->CommandListEnd(m_RPDC_Probe);

	g_pModuleManager->getRenderingServer()->ExecuteCommandList(m_RPDC_Probe);

	g_pModuleManager->getRenderingServer()->WaitForFrame(m_RPDC_Probe);

	g_pModuleManager->getLogSystem()->Log(LogLevel::Success, "InnoBakerNS: Start to generate probe location...");

	// Read back results and generate probe caches and real probes
	auto l_probePosTextureResults = g_pModuleManager->getRenderingServer()->ReadTextureBackToCPU(m_RPDC_Probe, m_RPDC_Probe->m_RenderTargets[0]);

	generateProbes(probesForSurfelCaches, l_probePosTextureResults, m_probeCacheInterval);

	auto l_probesCount = generateProbes(probesForRuntime, l_probePosTextureResults, m_probeInterval);

	serializeProbeInfos(l_probesCount);

	g_pModuleManager->getLogSystem()->Log(LogLevel::Success, "InnoBakerNS: ", probesForSurfelCaches.size(), " probes for surfel cache location generated.");
	g_pModuleManager->getLogSystem()->Log(LogLevel::Success, "InnoBakerNS: ", probesForRuntime.size(), " probes for runtime generated.");

	return true;
}

vec4 InnoBakerNS::generateProbes(std::vector<Probe>& probes, const std::vector<vec4>& heightMap, unsigned int probeMapSamplingInterval)
{
	auto l_totalTextureSize = heightMap.size();

	probes.reserve(l_totalTextureSize);

	auto l_probesCountPerLine = m_probeMapResolution / probeMapSamplingInterval;

	for (size_t i = 0; i < l_probesCountPerLine; i++)
	{
		for (size_t j = 0; j < l_probesCountPerLine; j++)
		{
			auto l_currentIndex = i * probeMapSamplingInterval * m_probeMapResolution + j * probeMapSamplingInterval;
			auto l_textureResult = heightMap[l_currentIndex];

			Probe l_Probe;
			l_Probe.pos = l_textureResult;
			l_Probe.pos.y = l_textureResult.y + m_probeHeightOffset;

			probes.emplace_back(l_Probe);
		}
	}
	auto l_probesCount = probes.size();

	g_pModuleManager->getLogSystem()->Log(LogLevel::Success, "InnoBakerNS: ", l_probesCount, " probe location generated over the surface.");

	// Generate probes along the wall
	std::vector<Probe> l_wallProbes;
	l_wallProbes.reserve(l_totalTextureSize);

	auto l_posIntervalX = std::abs((heightMap[0] - heightMap[probeMapSamplingInterval - 1]).x);
	auto l_posIntervalZ = std::abs((heightMap[0] - heightMap[m_probeMapResolution * probeMapSamplingInterval - 1]).z);

	unsigned int l_maxVerticalProbesCount = 1;

	for (size_t i = 0; i < l_probesCount; i++)
	{
		// Not the last one in all, not any one in last column, and not the last one each row
		if ((i + 1 < l_probesCount) && ((i + l_probesCountPerLine) < l_probesCount) && ((i + 1) % l_probesCountPerLine))
		{
			auto l_currentProbe = probes[i];
			auto l_nextRowProbe = probes[i + 1];
			auto l_nextColumnProbe = probes[i + l_probesCountPerLine];

			// Eliminate sampling error
			auto epsX = std::abs(l_currentProbe.pos.x - l_nextColumnProbe.pos.x);
			auto epsZ = std::abs(l_currentProbe.pos.z - l_nextRowProbe.pos.z);

			if (epsX)
			{
				l_nextColumnProbe.pos.x = l_currentProbe.pos.x;
			}
			if (epsZ)
			{
				l_nextRowProbe.pos.z = l_currentProbe.pos.z;
			}

			// Texture space partial derivatives
			auto ddx = l_currentProbe.pos.y - l_nextRowProbe.pos.y;
			auto ddy = l_currentProbe.pos.y - l_nextColumnProbe.pos.y;

			if (ddx > l_posIntervalX)
			{
				auto l_verticalProbesCount = std::floor(ddx / l_posIntervalX);

				l_maxVerticalProbesCount = std::max((unsigned int)l_verticalProbesCount + 2, l_maxVerticalProbesCount);

				for (size_t k = 0; k < l_verticalProbesCount; k++)
				{
					Probe l_verticalProbe;
					l_verticalProbe.pos = l_nextRowProbe.pos;
					l_verticalProbe.pos.y += l_posIntervalX * (k + 1);

					l_wallProbes.emplace_back(l_verticalProbe);
				}
			}
			if (ddx < -l_posIntervalX)
			{
				auto l_verticalProbesCount = std::floor(std::abs(ddx) / l_posIntervalX);

				l_maxVerticalProbesCount = std::max((unsigned int)l_verticalProbesCount + 2, l_maxVerticalProbesCount);

				for (size_t k = 0; k < l_verticalProbesCount; k++)
				{
					Probe l_verticalProbe;
					l_verticalProbe.pos = l_currentProbe.pos;
					l_verticalProbe.pos.y += l_posIntervalX * (k + 1);

					l_wallProbes.emplace_back(l_verticalProbe);
				}
			}
			if (ddy > l_posIntervalZ)
			{
				auto l_verticalProbesCount = std::floor(ddy / l_posIntervalZ);

				l_maxVerticalProbesCount = std::max((unsigned int)l_verticalProbesCount + 2, l_maxVerticalProbesCount);

				for (size_t k = 0; k < l_verticalProbesCount; k++)
				{
					Probe l_verticalProbe;
					l_verticalProbe.pos = l_nextColumnProbe.pos;
					l_verticalProbe.pos.y += l_posIntervalZ * (k + 1);

					l_wallProbes.emplace_back(l_verticalProbe);
				}
			}
			if (ddy < -l_posIntervalZ)
			{
				auto l_verticalProbesCount = std::floor(std::abs(ddy) / l_posIntervalZ);

				l_maxVerticalProbesCount = std::max((unsigned int)l_verticalProbesCount + 2, l_maxVerticalProbesCount);

				for (size_t k = 0; k < l_verticalProbesCount; k++)
				{
					Probe l_verticalProbe;
					l_verticalProbe.pos = l_currentProbe.pos;
					l_verticalProbe.pos.y += l_posIntervalZ * (k + 1);

					l_wallProbes.emplace_back(l_verticalProbe);
				}
			}
		}
	}

	l_wallProbes.shrink_to_fit();
	probes.insert(probes.end(), l_wallProbes.begin(), l_wallProbes.end());
	probes.shrink_to_fit();

	g_pModuleManager->getLogSystem()->Log(LogLevel::Success, "InnoBakerNS: ", probes.size() - l_probesCount, " probe location generated along the wall.");

	return vec4((float)l_probesCountPerLine, (float)l_maxVerticalProbesCount, (float)l_probesCountPerLine, 1.0f);
}

bool InnoBakerNS::serializeProbeInfos(vec4 probeCounts)
{
	auto l_filePath = g_pModuleManager->getFileSystem()->getWorkingDirectory();

	std::ofstream l_file;
	l_file.open(l_filePath + "Res//Scenes//" + m_exportFileName + ".InnoProbeInfo", std::ios::binary);
	l_file.write((char*)&probeCounts, sizeof(probeCounts));
	l_file.close();

	return true;
}

bool InnoBakerNS::captureSurfels(std::vector<Probe>& probesForSurfelCaches, std::vector<Probe>& probesForRuntime)
{
	g_pModuleManager->getLogSystem()->Log(LogLevel::Success, "InnoBakerNS: Start to capture surfels...");

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

	auto l_probeForSurfelCachesCount = probesForSurfelCaches.size();

	std::vector<Surfel> l_surfelCaches;
	l_surfelCaches.reserve(l_probeForSurfelCachesCount * m_captureResolution * m_captureResolution * 6);

	for (unsigned int i = 0; i < l_probeForSurfelCachesCount; i++)
	{
		drawOpaquePass(probesForSurfelCaches[i], l_p, l_v);

		readBackSurfelCaches(l_surfelCaches);

		g_pModuleManager->getLogSystem()->Log(LogLevel::Verbose, "InnoBakerNS: Progress: ", (float)i * 100.0f / (float)l_probeForSurfelCachesCount, "%...");
	}

	g_pModuleManager->getLogSystem()->Log(LogLevel::Success, "InnoBakerNS: ", l_surfelCaches.size(), " surfel caches captured.");

	eliminateDuplicatedSurfels(l_surfelCaches);

	serializeSurfelCaches(l_surfelCaches);

	// Sky visibility
	auto l_probeForRuntimeCount = probesForRuntime.size();

	for (unsigned int i = 0; i < l_probeForRuntimeCount; i++)
	{
		drawOpaquePass(probesForRuntime[i], l_p, l_v);
		drawSkyVisibilityPass(probesForRuntime[i], l_p, l_v);

		g_pModuleManager->getLogSystem()->Log(LogLevel::Verbose, "InnoBakerNS: Progress: ", (float)i * 100.0f / (float)l_probeForRuntimeCount, "%...");
	}

	serializeProbes(probesForRuntime);

	g_pModuleManager->getLogSystem()->Log(LogLevel::Success, "InnoBakerNS: sky visibility generated.");

	return true;
}

bool InnoBakerNS::drawOpaquePass(Probe& probeCache, const mat4& p, const std::vector<mat4>& v)
{
	auto l_t = InnoMath::getInvertTranslationMatrix(probeCache.pos);

	std::vector<mat4> l_GICameraGPUData(8);
	l_GICameraGPUData[0] = p;
	for (size_t i = 0; i < 6; i++)
	{
		l_GICameraGPUData[i + 1] = v[i];
	}
	l_GICameraGPUData[7] = l_t;

	g_pModuleManager->getRenderingServer()->UploadGPUBufferDataComponent(GetGPUBufferDataComponent(GPUBufferUsageType::GICamera), l_GICameraGPUData);

	auto l_MeshGBDC = GetGPUBufferDataComponent(GPUBufferUsageType::Mesh);
	auto l_MaterialGBDC = GetGPUBufferDataComponent(GPUBufferUsageType::Material);

	g_pModuleManager->getRenderingServer()->CommandListBegin(m_RPDC_Surfel, 0);
	g_pModuleManager->getRenderingServer()->BindRenderPassDataComponent(m_RPDC_Surfel);
	g_pModuleManager->getRenderingServer()->CleanRenderTargets(m_RPDC_Surfel);
	g_pModuleManager->getRenderingServer()->ActivateResourceBinder(m_RPDC_Surfel, ShaderStage::Pixel, m_SDC_Surfel->m_ResourceBinder, 8, 0);
	g_pModuleManager->getRenderingServer()->ActivateResourceBinder(m_RPDC_Surfel, ShaderStage::Geometry, GetGPUBufferDataComponent(GPUBufferUsageType::GICamera)->m_ResourceBinder, 0, 10, Accessibility::ReadOnly);

	unsigned int l_offset = 0;

	for (unsigned int i = 0; i < m_staticMeshDrawCallCount; i++)
	{
		auto l_staticMeshGPUData = m_staticMeshDrawCallData[i];

		if (l_staticMeshGPUData.mesh->m_objectStatus == ObjectStatus::Activated)
		{
			g_pModuleManager->getRenderingServer()->ActivateResourceBinder(m_RPDC_Surfel, ShaderStage::Vertex, l_MeshGBDC->m_ResourceBinder, 1, 1, Accessibility::ReadOnly, l_offset, 1);
			g_pModuleManager->getRenderingServer()->ActivateResourceBinder(m_RPDC_Surfel, ShaderStage::Pixel, l_MaterialGBDC->m_ResourceBinder, 2, 2, Accessibility::ReadOnly, l_offset, 1);

			if (l_staticMeshGPUData.material->m_objectStatus == ObjectStatus::Activated)
			{
				g_pModuleManager->getRenderingServer()->ActivateResourceBinder(m_RPDC_Surfel, ShaderStage::Pixel, l_staticMeshGPUData.material->m_ResourceBinders[0], 3, 0);
				g_pModuleManager->getRenderingServer()->ActivateResourceBinder(m_RPDC_Surfel, ShaderStage::Pixel, l_staticMeshGPUData.material->m_ResourceBinders[1], 4, 1);
				g_pModuleManager->getRenderingServer()->ActivateResourceBinder(m_RPDC_Surfel, ShaderStage::Pixel, l_staticMeshGPUData.material->m_ResourceBinders[2], 5, 2);
				g_pModuleManager->getRenderingServer()->ActivateResourceBinder(m_RPDC_Surfel, ShaderStage::Pixel, l_staticMeshGPUData.material->m_ResourceBinders[3], 6, 3);
				g_pModuleManager->getRenderingServer()->ActivateResourceBinder(m_RPDC_Surfel, ShaderStage::Pixel, l_staticMeshGPUData.material->m_ResourceBinders[4], 7, 4);
			}

			g_pModuleManager->getRenderingServer()->DispatchDrawCall(m_RPDC_Surfel, l_staticMeshGPUData.mesh);

			if (l_staticMeshGPUData.material->m_objectStatus == ObjectStatus::Activated)
			{
				g_pModuleManager->getRenderingServer()->DeactivateResourceBinder(m_RPDC_Surfel, ShaderStage::Pixel, l_staticMeshGPUData.material->m_ResourceBinders[0], 3, 0);
				g_pModuleManager->getRenderingServer()->DeactivateResourceBinder(m_RPDC_Surfel, ShaderStage::Pixel, l_staticMeshGPUData.material->m_ResourceBinders[1], 4, 1);
				g_pModuleManager->getRenderingServer()->DeactivateResourceBinder(m_RPDC_Surfel, ShaderStage::Pixel, l_staticMeshGPUData.material->m_ResourceBinders[2], 5, 2);
				g_pModuleManager->getRenderingServer()->DeactivateResourceBinder(m_RPDC_Surfel, ShaderStage::Pixel, l_staticMeshGPUData.material->m_ResourceBinders[3], 6, 3);
				g_pModuleManager->getRenderingServer()->DeactivateResourceBinder(m_RPDC_Surfel, ShaderStage::Pixel, l_staticMeshGPUData.material->m_ResourceBinders[4], 7, 4);
			}
		}

		l_offset++;
	}

	g_pModuleManager->getRenderingServer()->CommandListEnd(m_RPDC_Surfel);

	g_pModuleManager->getRenderingServer()->ExecuteCommandList(m_RPDC_Surfel);

	g_pModuleManager->getRenderingServer()->WaitForFrame(m_RPDC_Surfel);

	return true;
}

bool InnoBakerNS::drawSkyVisibilityPass(Probe& probeCache, const mat4& p, const std::vector<mat4>& v)
{
	auto l_depthStencilRT = g_pModuleManager->getRenderingServer()->ReadTextureBackToCPU(m_RPDC_Surfel, m_RPDC_Surfel->m_DepthStencilRenderTarget);

	auto l_depthStencilRTSize = l_depthStencilRT.size();

	l_depthStencilRTSize /= 6;

	for (size_t i = 0; i < 6; i++)
	{
		unsigned int l_stencil = 0;
		for (size_t j = 0; j < l_depthStencilRTSize; j++)
		{
			auto& l_depthStencil = l_depthStencilRT[i * l_depthStencilRTSize + j];

			if (l_depthStencil.y == 1.0f)
			{
				l_stencil++;
			}
		}

		probeCache.skyVisibility[i] = 1.0f - ((float)l_stencil / (float)l_depthStencilRTSize);
	}

	return true;
}

bool InnoBakerNS::readBackSurfelCaches(std::vector<Surfel>& surfelCaches)
{
	auto l_posWSMetallic = g_pModuleManager->getRenderingServer()->ReadTextureBackToCPU(m_RPDC_Surfel, m_RPDC_Surfel->m_RenderTargets[0]);
	auto l_normalRoughness = g_pModuleManager->getRenderingServer()->ReadTextureBackToCPU(m_RPDC_Surfel, m_RPDC_Surfel->m_RenderTargets[1]);
	auto l_albedoAO = g_pModuleManager->getRenderingServer()->ReadTextureBackToCPU(m_RPDC_Surfel, m_RPDC_Surfel->m_RenderTargets[2]);

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

	surfelCaches.insert(surfelCaches.end(), l_surfels.begin(), l_surfels.end());

	return true;
}

bool InnoBakerNS::eliminateDuplicatedSurfels(std::vector<Surfel>& surfelCaches)
{
	g_pModuleManager->getLogSystem()->Log(LogLevel::Success, "InnoBakerNS: Start to eliminate duplicated surfels...");

	std::sort(surfelCaches.begin(), surfelCaches.end(), [&](Surfel A, Surfel B)
	{
		if (A.pos.x != B.pos.x) {
			return A.pos.x < B.pos.x;
		}
		if (A.pos.y != B.pos.y) {
			return A.pos.y < B.pos.y;
		}
		return A.pos.z < B.pos.z;
	});

	surfelCaches.erase(std::unique(surfelCaches.begin(), surfelCaches.end()), surfelCaches.end());
	surfelCaches.shrink_to_fit();

	g_pModuleManager->getLogSystem()->Log(LogLevel::Success, "InnoBakerNS: Duplicated surfels have been removed, there are ", surfelCaches.size(), " surfels now.");

	return true;
}

bool InnoBakerNS::serializeSurfelCaches(const std::vector<Surfel>& surfelCaches)
{
	auto l_filePath = g_pModuleManager->getFileSystem()->getWorkingDirectory();

	std::ofstream l_file;
	l_file.open(l_filePath + "Res//Intermediate//" + m_exportFileName + ".InnoSurfelCache", std::ios::binary);
	IOService::serializeVector(l_file, surfelCaches);
	l_file.close();

	g_pModuleManager->getLogSystem()->Log(LogLevel::Success, "InnoBakerNS: ", l_filePath.c_str(), "Res//Intermediate//", m_exportFileName.c_str(), ".InnoSurfelCache has been saved.");

	return true;
}

bool InnoBakerNS::generateBrickCaches(std::vector<Surfel>& surfelCaches)
{
	g_pModuleManager->getLogSystem()->Log(LogLevel::Success, "InnoBakerNS: Start to generate brick caches...");

	// Find bound corner position
	auto l_surfelsCount = surfelCaches.size();

	auto l_startPos = InnoMath::maxVec4<float>;
	l_startPos.w = 1.0f;

	auto l_endPos = InnoMath::minVec4<float>;
	l_endPos.w = 1.0f;

	for (size_t i = 0; i < l_surfelsCount; i++)
	{
		l_startPos = InnoMath::elementWiseMin(surfelCaches[i].pos, l_startPos);
		l_endPos = InnoMath::elementWiseMax(surfelCaches[i].pos, l_endPos);
	}

	// Fit the end corner to contain at least one brick in each axis
	auto l_adjustedEndPos = l_endPos;
	l_adjustedEndPos.x = (std::trunc(l_endPos.x / m_brickSize.x) + 1) * m_brickSize.x;
	l_adjustedEndPos.y = (std::trunc(l_endPos.y / m_brickSize.y) + 1) * m_brickSize.y;
	l_adjustedEndPos.z = (std::trunc(l_endPos.z / m_brickSize.z) + 1) * m_brickSize.z;
	l_endPos = l_adjustedEndPos;

	auto l_extends = l_endPos - l_startPos;
	auto l_bricksCountX = std::trunc(l_extends.x / m_brickSize.x);
	auto l_bricksCountY = std::trunc(l_extends.y / m_brickSize.y);
	auto l_bricksCountZ = std::trunc(l_extends.z / m_brickSize.z);

	// generate all possible brick position
	auto l_totalBricksWorkCount = (int)(l_bricksCountX * l_bricksCountY * l_bricksCountZ);

	std::vector<vec4> l_brickPos;
	l_brickPos.reserve(l_totalBricksWorkCount);

	auto l_currentMaxPos = l_startPos + m_brickSize;
	auto l_currentMinPos = l_startPos;

	while (l_currentMaxPos.x <= l_endPos.x)
	{
		l_currentMaxPos.y = l_startPos.y + m_brickSize.y;
		l_currentMinPos.y = l_startPos.y;

		while (l_currentMaxPos.y <= l_endPos.y)
		{
			l_currentMaxPos.z = l_startPos.z + m_brickSize.z;
			l_currentMinPos.z = l_startPos.z;

			while (l_currentMaxPos.z <= l_endPos.z)
			{
				l_brickPos.emplace_back(l_currentMinPos + m_brickSize / 2.0f);

				l_currentMaxPos.z += m_brickSize.z;
				l_currentMinPos.z += m_brickSize.z;
			}

			l_currentMaxPos.y += m_brickSize.y;
			l_currentMinPos.y += m_brickSize.y;
		}

		l_currentMaxPos.x += m_brickSize.x;
		l_currentMinPos.x += m_brickSize.x;
	}

	// Assign surfels to brick cache
	ThreadSafeVector<BrickCache> l_brickCaches;
	l_brickCaches.reserve(l_totalBricksWorkCount);

	std::atomic_int l_currentWorkloadIndex = 0;

	std::vector<std::shared_ptr<IInnoTask>> l_tasks;
	l_tasks.reserve(l_totalBricksWorkCount);

	while (l_currentWorkloadIndex < l_totalBricksWorkCount)
	{
		auto l_task = g_pModuleManager->getTaskSystem()->submit("InnoBakerBrickCacheTask", -1, nullptr,
			[&]() {
			l_currentWorkloadIndex++;

			if (l_currentWorkloadIndex < l_totalBricksWorkCount)
			{
				size_t l_currentBrickIndex = l_currentWorkloadIndex;

				BrickCache l_BrickCache;
				l_BrickCache.pos = l_brickPos[l_currentBrickIndex];
				l_BrickCache.surfelCaches.reserve(l_surfelsCount);

				auto l_currentMaxPos = l_BrickCache.pos + m_brickSize / 2.0f;
				auto l_currentMinPos = l_BrickCache.pos - m_brickSize / 2.0f;

				for (size_t j = 0; j < l_surfelsCount; j++)
				{
					if (
						InnoMath::isALessEqualThanBVec3(surfelCaches[j].pos, l_currentMaxPos)
						&& InnoMath::isAGreaterEqualThanBVec3(surfelCaches[j].pos, l_currentMinPos)
						)
					{
						l_BrickCache.surfelCaches.emplace_back(surfelCaches[j]);
					}
				}

				if (l_BrickCache.surfelCaches.size() > 0)
				{
					l_brickCaches.emplace_back(std::move(l_BrickCache));
				}

				g_pModuleManager->getLogSystem()->Log(LogLevel::Verbose, "InnoBakerNS: Progress: ", (float)l_currentBrickIndex * 100.0f / (float)l_totalBricksWorkCount, "%...");
			}
		});

		l_tasks.emplace_back(l_task);
	}

	for (auto i : l_tasks)
	{
		i->Wait();
	}

	g_pModuleManager->getTaskSystem()->waitAllTasksToFinish();

	l_brickCaches.shrink_to_fit();

	g_pModuleManager->getLogSystem()->Log(LogLevel::Success, "InnoBakerNS: ", l_brickCaches.size(), " brick caches have been generated.");

	serializeBrickCaches(l_brickCaches.getRawData());

	return true;
}

bool InnoBakerNS::serializeBrickCaches(const std::vector<BrickCache>& brickCaches)
{
	auto l_brickCacheCount = brickCaches.size();

	// Serialize metadata
	auto l_filePath = g_pModuleManager->getFileSystem()->getWorkingDirectory();
	std::ofstream l_summaryFile;
	l_summaryFile.open(l_filePath + "Res//Intermediate//" + m_exportFileName + ".InnoBrickCacheSummary", std::ios::binary);

	std::vector<BrickCacheSummary> l_brickCacheSummaries;
	l_brickCacheSummaries.reserve(l_brickCacheCount);

	for (size_t i = 0; i < l_brickCacheCount; i++)
	{
		BrickCacheSummary l_brickCacheSummary;

		l_brickCacheSummary.pos = brickCaches[i].pos;
		l_brickCacheSummary.fileIndex = i;
		l_brickCacheSummary.fileSize = brickCaches[i].surfelCaches.size();

		l_brickCacheSummaries.emplace_back(l_brickCacheSummary);
	}

	IOService::serializeVector(l_summaryFile, l_brickCacheSummaries);

	l_summaryFile.close();

	// Serialize surfels cache for each brick
	std::ofstream l_surfelCacheFile;
	l_surfelCacheFile.open(l_filePath + "Res//Intermediate//" + m_exportFileName + ".InnoBrickCache", std::ios::binary);

	for (size_t i = 0; i < l_brickCacheCount; i++)
	{
		IOService::serializeVector(l_surfelCacheFile, brickCaches[i].surfelCaches);
	}
	l_surfelCacheFile.close();

	g_pModuleManager->getLogSystem()->Log(LogLevel::Success, "InnoBakerNS: ", l_filePath.c_str(), "Res//Intermediate//", m_exportFileName.c_str(), ".InnoBrickCacheSummary has been saved.");

	return true;
}

bool InnoBakerNS::deserializeBrickCaches(const std::vector<BrickCacheSummary>& brickCacheSummaries, std::vector<BrickCache>& brickCaches)
{
	auto l_brickCount = brickCacheSummaries.size();
	brickCaches.reserve(l_brickCount);

	auto l_filePath = g_pModuleManager->getFileSystem()->getWorkingDirectory();

	std::ifstream l_file;
	l_file.open(l_filePath + "Res//Intermediate//" + m_exportFileName + ".InnoBrickCache", std::ios::binary);

	size_t l_startOffset = 0;

	for (size_t i = 0; i < l_brickCount; i++)
	{
		BrickCache l_brickCache;
		l_brickCache.pos = brickCacheSummaries[i].pos;

		auto l_fileSize = brickCacheSummaries[i].fileSize;
		l_brickCache.surfelCaches.resize(l_fileSize);

		IOService::deserializeVector(l_file, l_startOffset * sizeof(Surfel), l_fileSize * sizeof(Surfel), l_brickCache.surfelCaches);

		l_startOffset += l_fileSize;

		brickCaches.emplace_back(std::move(l_brickCache));
	}

	return true;
}

bool InnoBakerNS::generateBricks(const std::vector<BrickCache>& brickCaches)
{
	g_pModuleManager->getLogSystem()->Log(LogLevel::Success, "InnoBakerNS: Start to generate bricks...");

	// Generate real bricks with surfel range
	auto l_bricksCount = brickCaches.size();
	std::vector<Brick> l_bricks;
	l_bricks.reserve(l_bricksCount);
	std::vector<Surfel> l_surfels;

	size_t l_surfelsCount = 0;
	for (size_t i = 0; i < l_bricksCount; i++)
	{
		l_surfelsCount += brickCaches[i].surfelCaches.size();
	}

	l_surfels.reserve(l_surfelsCount);

	size_t l_offset = 0;

	for (size_t i = 0; i < l_bricksCount; i++)
	{
		Brick l_brick;
		l_brick.boundBox = InnoMath::generateAABB(brickCaches[i].pos + m_brickSize / 2.0f, brickCaches[i].pos - m_brickSize / 2.0f);
		l_brick.surfelRangeBegin = (unsigned int)l_offset;
		l_brick.surfelRangeEnd = (unsigned int)(l_offset + brickCaches[i].surfelCaches.size() - 1);
		l_offset = brickCaches[i].surfelCaches.size();

		l_surfels.insert(l_surfels.end(), std::make_move_iterator(brickCaches[i].surfelCaches.begin()), std::make_move_iterator(brickCaches[i].surfelCaches.end()));

		l_bricks.emplace_back(l_brick);

		g_pModuleManager->getLogSystem()->Log(LogLevel::Verbose, "InnoBakerNS: Progress: ", (float)i * 100.0f / (float)l_bricksCount, "%...");
	}

	g_pModuleManager->getLogSystem()->Log(LogLevel::Success, "InnoBakerNS: ", l_bricksCount, " bricks have been generated.");

	serializeSurfels(l_surfels);
	serializeBricks(l_bricks);

	return true;
}

bool InnoBakerNS::serializeSurfels(const std::vector<Surfel>& surfels)
{
	auto l_filePath = g_pModuleManager->getFileSystem()->getWorkingDirectory();

	std::ofstream l_file;
	l_file.open(l_filePath + "Res//Scenes//" + m_exportFileName + ".InnoSurfel", std::ios::binary);
	IOService::serializeVector(l_file, surfels);
	l_file.close();

	g_pModuleManager->getLogSystem()->Log(LogLevel::Success, "InnoBakerNS: ", l_filePath.c_str(), "Res//Scenes//", m_exportFileName.c_str(), ".InnoSurfel has been saved.");

	return true;
}

bool InnoBakerNS::serializeBricks(const std::vector<Brick>& bricks)
{
	auto l_filePath = g_pModuleManager->getFileSystem()->getWorkingDirectory();

	std::ofstream l_file;
	l_file.open(l_filePath + "Res//Scenes//" + m_exportFileName + ".InnoBrick", std::ios::binary);
	IOService::serializeVector(l_file, bricks);
	l_file.close();

	g_pModuleManager->getLogSystem()->Log(LogLevel::Success, "InnoBakerNS: ", l_filePath.c_str(), "Res//Scenes//", m_exportFileName.c_str(), ".InnoBrick has been saved.");

	return true;
}

bool InnoBakerNS::assignBrickFactorToProbesByGPU(const std::vector<Brick>& bricks, std::vector<Probe>& probes)
{
	g_pModuleManager->getLogSystem()->Log(LogLevel::Success, "InnoBakerNS: Start to generate brick factor and assign to probes...");

	// Upload camera data and brick cubes data to GPU memory
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

	auto l_p = InnoMath::generatePerspectiveMatrix((90.0f / 180.0f) * PI<float>, 1.0f, 0.1f, 2000.0f);

	auto l_bricksCount = bricks.size();

	std::vector<MeshGPUData> l_bricksCubeMeshGPUData;
	l_bricksCubeMeshGPUData.resize(l_bricksCount);

	for (size_t i = 0; i < l_bricksCount; i++)
	{
		auto l_t = InnoMath::toTranslationMatrix(bricks[i].boundBox.m_center);
		auto l_s = InnoMath::toScaleMatrix(vec4(m_brickSize.x, m_brickSize.y, m_brickSize.z, 1.0f));

		l_bricksCubeMeshGPUData[i].m = l_t * l_s;

		// Index start from 1
		l_bricksCubeMeshGPUData[i].UUID = (float)i + 1.0f;
	}

	auto l_MeshGBDC = GetGPUBufferDataComponent(GPUBufferUsageType::Mesh);
	g_pModuleManager->getRenderingServer()->UploadGPUBufferDataComponent(l_MeshGBDC, l_bricksCubeMeshGPUData, 0, l_bricksCubeMeshGPUData.size());

	// assign bricks to probe by the depth test result
	auto l_probesCount = probes.size();

	std::vector<BrickFactor> l_brickFactors;
	l_brickFactors.reserve(l_probesCount * l_bricksCount);

	for (size_t i = 0; i < l_probesCount; i++)
	{
		drawBricks(probes[i].pos, (unsigned int)l_bricksCount, l_p, l_v);
		readBackBrickFactors(probes[i], l_brickFactors);

		g_pModuleManager->getLogSystem()->Log(LogLevel::Verbose, "InnoBakerNS: Progress: ", (float)i * 100.0f / (float)l_probesCount, "%...");
	}

	l_brickFactors.shrink_to_fit();

	g_pModuleManager->getLogSystem()->Log(LogLevel::Success, "InnoBakerNS: ", l_brickFactors.size(), " brick factors have been generated.");

	serializeBrickFactors(l_brickFactors);
	serializeProbes(probes);

	return true;
}

bool InnoBakerNS::drawBricks(vec4 pos, unsigned int bricksCount, const mat4 & p, const std::vector<mat4>& v)
{
	std::vector<mat4> l_GICameraGPUData(8);
	l_GICameraGPUData[0] = p;
	for (size_t i = 0; i < 6; i++)
	{
		l_GICameraGPUData[i + 1] = v[i];
	}
	l_GICameraGPUData[7] = InnoMath::getInvertTranslationMatrix(pos);

	g_pModuleManager->getRenderingServer()->UploadGPUBufferDataComponent(GetGPUBufferDataComponent(GPUBufferUsageType::GICamera), l_GICameraGPUData);

	auto l_MeshGBDC = GetGPUBufferDataComponent(GPUBufferUsageType::Mesh);

	auto l_mesh = g_pModuleManager->getRenderingFrontend()->getMeshDataComponent(MeshShapeType::Cube);

	unsigned int l_offset = 0;

	g_pModuleManager->getRenderingServer()->CommandListBegin(m_RPDC_BrickFactor, 0);
	g_pModuleManager->getRenderingServer()->BindRenderPassDataComponent(m_RPDC_BrickFactor);
	g_pModuleManager->getRenderingServer()->CleanRenderTargets(m_RPDC_BrickFactor);
	g_pModuleManager->getRenderingServer()->ActivateResourceBinder(m_RPDC_BrickFactor, ShaderStage::Geometry, GetGPUBufferDataComponent(GPUBufferUsageType::GICamera)->m_ResourceBinder, 0, 10, Accessibility::ReadOnly);

	for (unsigned int i = 0; i < bricksCount; i++)
	{
		g_pModuleManager->getRenderingServer()->ActivateResourceBinder(m_RPDC_BrickFactor, ShaderStage::Vertex, l_MeshGBDC->m_ResourceBinder, 1, 1, Accessibility::ReadOnly, l_offset, 1);

		g_pModuleManager->getRenderingServer()->DispatchDrawCall(m_RPDC_BrickFactor, l_mesh);

		l_offset++;
	}

	g_pModuleManager->getRenderingServer()->CommandListEnd(m_RPDC_BrickFactor);

	g_pModuleManager->getRenderingServer()->ExecuteCommandList(m_RPDC_BrickFactor);

	g_pModuleManager->getRenderingServer()->WaitForFrame(m_RPDC_BrickFactor);

	return true;
}

bool InnoBakerNS::readBackBrickFactors(Probe& probe, std::vector<BrickFactor>& brickFactors)
{
	auto l_brickIDResults = g_pModuleManager->getRenderingServer()->ReadTextureBackToCPU(m_RPDC_BrickFactor, m_RPDC_BrickFactor->m_RenderTargets[0]);

	auto l_brickIDResultSize = l_brickIDResults.size();

	l_brickIDResultSize /= 6;

	// 6 axis-aligned coefficients
	for (size_t i = 0; i < 6; i++)
	{
		std::vector<BrickFactor> l_brickFactors;
		l_brickFactors.reserve(l_brickIDResultSize);

		for (size_t j = 0; j < l_brickIDResultSize; j++)
		{
			auto& l_brickIDResult = l_brickIDResults[i * l_brickIDResultSize + j];
			if (l_brickIDResult.y != 0.0f)
			{
				BrickFactor l_BrickFactor;

				l_BrickFactor.basisWeight = std::abs(l_brickIDResult.x);

				// Index start from 1
				l_BrickFactor.brickIndex = (unsigned int)(std::round(l_brickIDResult.y) - 1.0f);
				l_brickFactors.emplace_back(l_BrickFactor);
			}
		}

		std::sort(l_brickFactors.begin(), l_brickFactors.end(), [&](BrickFactor A, BrickFactor B)
		{
			return A.brickIndex < B.brickIndex;
		});

		l_brickFactors.erase(std::unique(l_brickFactors.begin(), l_brickFactors.end()), l_brickFactors.end());
		l_brickFactors.shrink_to_fit();

		// Calculate brick weight
		auto l_brickFactorSize = l_brickFactors.size();

		if (l_brickFactorSize == 1)
		{
			l_brickFactors[0].basisWeight = 1.0f;
		}
		else
		{
			float denom = 0.0f;
			for (size_t i = 0; i < l_brickFactorSize; i++)
			{
				denom += l_brickFactors[i].basisWeight;
			}

			for (size_t i = 0; i < l_brickFactorSize; i++)
			{
				// Reverse view space Z axis
				l_brickFactors[i].basisWeight = (denom - l_brickFactors[i].basisWeight) / denom;
			}
		}

		// Assign brick factor range to probes
		auto l_brickFactorRangeBegin = brickFactors.size();
		auto l_brickFactorRangeEnd = l_brickFactorRangeBegin + l_brickFactors.size() - 1;

		probe.brickFactorRange[i * 2] = (unsigned int)l_brickFactorRangeBegin;
		probe.brickFactorRange[i * 2 + 1] = (unsigned int)l_brickFactorRangeEnd;

		brickFactors.insert(brickFactors.end(), std::make_move_iterator(l_brickFactors.begin()), std::make_move_iterator(l_brickFactors.end()));
	}

	return true;
}

bool InnoBakerNS::serializeBrickFactors(const std::vector<BrickFactor>& brickFactors)
{
	auto l_filePath = g_pModuleManager->getFileSystem()->getWorkingDirectory();

	std::ofstream l_file;
	l_file.open(l_filePath + "Res//Scenes//" + m_exportFileName + ".InnoBrickFactor", std::ios::binary);
	IOService::serializeVector(l_file, brickFactors);
	l_file.close();

	g_pModuleManager->getLogSystem()->Log(LogLevel::Success, "InnoBakerNS: ", l_filePath.c_str(), "Res//Scenes//", m_exportFileName.c_str(), ".InnoBrickFactor has been saved.");

	return true;
}

bool InnoBakerNS::serializeProbes(const std::vector<Probe>& probes)
{
	auto l_filePath = g_pModuleManager->getFileSystem()->getWorkingDirectory();

	std::ofstream l_file;
	l_file.open(l_filePath + "Res//Scenes//" + m_exportFileName + ".InnoProbe", std::ios::binary);
	IOService::serializeVector(l_file, probes);
	l_file.close();

	g_pModuleManager->getLogSystem()->Log(LogLevel::Success, "InnoBakerNS: ", l_filePath.c_str(), "Res//Scenes//", m_exportFileName.c_str(), ".InnoProbe has been saved.");

	return true;
}

void InnoBaker::Setup()
{
	////
	auto l_RenderingCapability = g_pModuleManager->getRenderingFrontend()->getRenderingCapability();

	m_staticMeshDrawCallData.resize(l_RenderingCapability.maxMeshes);
	m_staticMeshMeshGPUData.resize(l_RenderingCapability.maxMeshes);
	m_staticMeshMaterialGPUData.resize(l_RenderingCapability.maxMaterials);

	auto l_RenderPassDesc = g_pModuleManager->getRenderingFrontend()->getDefaultRenderPassDesc();

	////
	m_SPC_Probe = g_pModuleManager->getRenderingServer()->AddShaderProgramComponent("GIBakeProbePass/");

	m_SPC_Probe->m_ShaderFilePaths.m_VSPath = "GIBakeProbePass.vert/";
	m_SPC_Probe->m_ShaderFilePaths.m_PSPath = "GIBakeProbePass.frag/";

	g_pModuleManager->getRenderingServer()->InitializeShaderProgramComponent(m_SPC_Probe);

	m_RPDC_Probe = g_pModuleManager->getRenderingServer()->AddRenderPassDataComponent("GIBakeProbePass/");

	m_RPDC_Probe->m_RenderPassDesc = l_RenderPassDesc;
	m_RPDC_Probe->m_RenderPassDesc.m_RenderTargetDesc.SamplerType = TextureSamplerType::Sampler2D;
	m_RPDC_Probe->m_RenderPassDesc.m_RenderTargetDesc.Width = m_probeMapResolution;
	m_RPDC_Probe->m_RenderPassDesc.m_RenderTargetDesc.Height = m_probeMapResolution;
	m_RPDC_Probe->m_RenderPassDesc.m_RenderTargetDesc.PixelDataType = TexturePixelDataType::FLOAT32;

	m_RPDC_Probe->m_RenderPassDesc.m_GraphicsPipelineDesc.m_DepthStencilDesc.m_UseDepthBuffer = true;
	m_RPDC_Probe->m_RenderPassDesc.m_GraphicsPipelineDesc.m_DepthStencilDesc.m_AllowDepthWrite = true;
	m_RPDC_Probe->m_RenderPassDesc.m_GraphicsPipelineDesc.m_DepthStencilDesc.m_DepthComparisionFunction = ComparisionFunction::LessEqual;

	m_RPDC_Probe->m_RenderPassDesc.m_GraphicsPipelineDesc.m_RasterizerDesc.m_UseCulling = true;
	m_RPDC_Probe->m_RenderPassDesc.m_GraphicsPipelineDesc.m_ViewportDesc.m_Width = m_probeMapResolution;
	m_RPDC_Probe->m_RenderPassDesc.m_GraphicsPipelineDesc.m_ViewportDesc.m_Height = m_probeMapResolution;

	m_RPDC_Probe->m_ResourceBinderLayoutDescs.resize(2);
	m_RPDC_Probe->m_ResourceBinderLayoutDescs[0].m_ResourceBinderType = ResourceBinderType::Buffer;
	m_RPDC_Probe->m_ResourceBinderLayoutDescs[0].m_GlobalSlot = 0;
	m_RPDC_Probe->m_ResourceBinderLayoutDescs[0].m_LocalSlot = 10;

	m_RPDC_Probe->m_ResourceBinderLayoutDescs[1].m_ResourceBinderType = ResourceBinderType::Buffer;
	m_RPDC_Probe->m_ResourceBinderLayoutDescs[1].m_GlobalSlot = 1;
	m_RPDC_Probe->m_ResourceBinderLayoutDescs[1].m_LocalSlot = 1;

	m_RPDC_Probe->m_ShaderProgram = m_SPC_Probe;

	g_pModuleManager->getRenderingServer()->InitializeRenderPassDataComponent(m_RPDC_Probe);

	////
	m_SPC_Surfel = g_pModuleManager->getRenderingServer()->AddShaderProgramComponent("GIBakeSurfelPass/");

	m_SPC_Surfel->m_ShaderFilePaths.m_VSPath = "GIBakeSurfelPass.vert/";
	m_SPC_Surfel->m_ShaderFilePaths.m_GSPath = "GIBakeSurfelPass.geom/";
	m_SPC_Surfel->m_ShaderFilePaths.m_PSPath = "GIBakeSurfelPass.frag/";

	g_pModuleManager->getRenderingServer()->InitializeShaderProgramComponent(m_SPC_Surfel);

	m_RPDC_Surfel = g_pModuleManager->getRenderingServer()->AddRenderPassDataComponent("GIBakeSurfelPass/");

	m_RPDC_Surfel->m_RenderPassDesc = l_RenderPassDesc;

	m_RPDC_Surfel->m_RenderPassDesc.m_RenderTargetCount = 3;

	m_RPDC_Surfel->m_RenderPassDesc.m_RenderTargetDesc.SamplerType = TextureSamplerType::SamplerCubemap;
	m_RPDC_Surfel->m_RenderPassDesc.m_RenderTargetDesc.Width = m_captureResolution;
	m_RPDC_Surfel->m_RenderPassDesc.m_RenderTargetDesc.Height = m_captureResolution;
	m_RPDC_Surfel->m_RenderPassDesc.m_RenderTargetDesc.PixelDataType = TexturePixelDataType::FLOAT32;

	m_RPDC_Surfel->m_RenderPassDesc.m_GraphicsPipelineDesc.m_DepthStencilDesc.m_UseDepthBuffer = true;
	m_RPDC_Surfel->m_RenderPassDesc.m_GraphicsPipelineDesc.m_DepthStencilDesc.m_AllowDepthWrite = true;
	m_RPDC_Surfel->m_RenderPassDesc.m_GraphicsPipelineDesc.m_DepthStencilDesc.m_DepthComparisionFunction = ComparisionFunction::LessEqual;

	m_RPDC_Surfel->m_RenderPassDesc.m_GraphicsPipelineDesc.m_DepthStencilDesc.m_UseStencilBuffer = true;
	m_RPDC_Surfel->m_RenderPassDesc.m_GraphicsPipelineDesc.m_DepthStencilDesc.m_AllowStencilWrite = true;
	m_RPDC_Surfel->m_RenderPassDesc.m_GraphicsPipelineDesc.m_DepthStencilDesc.m_StencilReference = 0x01;
	m_RPDC_Surfel->m_RenderPassDesc.m_GraphicsPipelineDesc.m_DepthStencilDesc.m_FrontFaceStencilPassOperation = StencilOperation::Replace;
	m_RPDC_Surfel->m_RenderPassDesc.m_GraphicsPipelineDesc.m_DepthStencilDesc.m_FrontFaceStencilComparisionFunction = ComparisionFunction::Always;
	m_RPDC_Surfel->m_RenderPassDesc.m_GraphicsPipelineDesc.m_DepthStencilDesc.m_BackFaceStencilPassOperation = StencilOperation::Replace;
	m_RPDC_Surfel->m_RenderPassDesc.m_GraphicsPipelineDesc.m_DepthStencilDesc.m_BackFaceStencilComparisionFunction = ComparisionFunction::Always;

	m_RPDC_Surfel->m_RenderPassDesc.m_GraphicsPipelineDesc.m_RasterizerDesc.m_UseCulling = true;
	m_RPDC_Surfel->m_RenderPassDesc.m_GraphicsPipelineDesc.m_ViewportDesc.m_Width = m_captureResolution;
	m_RPDC_Surfel->m_RenderPassDesc.m_GraphicsPipelineDesc.m_ViewportDesc.m_Height = m_captureResolution;

	m_RPDC_Surfel->m_ResourceBinderLayoutDescs.resize(9);
	m_RPDC_Surfel->m_ResourceBinderLayoutDescs[0].m_ResourceBinderType = ResourceBinderType::Buffer;
	m_RPDC_Surfel->m_ResourceBinderLayoutDescs[0].m_GlobalSlot = 0;
	m_RPDC_Surfel->m_ResourceBinderLayoutDescs[0].m_LocalSlot = 10;

	m_RPDC_Surfel->m_ResourceBinderLayoutDescs[1].m_ResourceBinderType = ResourceBinderType::Buffer;
	m_RPDC_Surfel->m_ResourceBinderLayoutDescs[1].m_GlobalSlot = 1;
	m_RPDC_Surfel->m_ResourceBinderLayoutDescs[1].m_LocalSlot = 1;

	m_RPDC_Surfel->m_ResourceBinderLayoutDescs[2].m_ResourceBinderType = ResourceBinderType::Buffer;
	m_RPDC_Surfel->m_ResourceBinderLayoutDescs[2].m_GlobalSlot = 2;
	m_RPDC_Surfel->m_ResourceBinderLayoutDescs[2].m_LocalSlot = 2;

	m_RPDC_Surfel->m_ResourceBinderLayoutDescs[3].m_ResourceBinderType = ResourceBinderType::Image;
	m_RPDC_Surfel->m_ResourceBinderLayoutDescs[3].m_GlobalSlot = 3;
	m_RPDC_Surfel->m_ResourceBinderLayoutDescs[3].m_LocalSlot = 0;
	m_RPDC_Surfel->m_ResourceBinderLayoutDescs[3].m_IsRanged = true;

	m_RPDC_Surfel->m_ResourceBinderLayoutDescs[4].m_ResourceBinderType = ResourceBinderType::Image;
	m_RPDC_Surfel->m_ResourceBinderLayoutDescs[4].m_GlobalSlot = 4;
	m_RPDC_Surfel->m_ResourceBinderLayoutDescs[4].m_LocalSlot = 1;
	m_RPDC_Surfel->m_ResourceBinderLayoutDescs[4].m_IsRanged = true;

	m_RPDC_Surfel->m_ResourceBinderLayoutDescs[5].m_ResourceBinderType = ResourceBinderType::Image;
	m_RPDC_Surfel->m_ResourceBinderLayoutDescs[5].m_GlobalSlot = 5;
	m_RPDC_Surfel->m_ResourceBinderLayoutDescs[5].m_LocalSlot = 2;
	m_RPDC_Surfel->m_ResourceBinderLayoutDescs[5].m_ResourceCount = 1;
	m_RPDC_Surfel->m_ResourceBinderLayoutDescs[5].m_IsRanged = true;

	m_RPDC_Surfel->m_ResourceBinderLayoutDescs[6].m_ResourceBinderType = ResourceBinderType::Image;
	m_RPDC_Surfel->m_ResourceBinderLayoutDescs[6].m_GlobalSlot = 6;
	m_RPDC_Surfel->m_ResourceBinderLayoutDescs[6].m_LocalSlot = 3;
	m_RPDC_Surfel->m_ResourceBinderLayoutDescs[6].m_IsRanged = true;

	m_RPDC_Surfel->m_ResourceBinderLayoutDescs[7].m_ResourceBinderType = ResourceBinderType::Image;
	m_RPDC_Surfel->m_ResourceBinderLayoutDescs[7].m_GlobalSlot = 7;
	m_RPDC_Surfel->m_ResourceBinderLayoutDescs[7].m_LocalSlot = 4;
	m_RPDC_Surfel->m_ResourceBinderLayoutDescs[7].m_IsRanged = true;

	m_RPDC_Surfel->m_ResourceBinderLayoutDescs[8].m_ResourceBinderType = ResourceBinderType::Sampler;
	m_RPDC_Surfel->m_ResourceBinderLayoutDescs[8].m_GlobalSlot = 8;
	m_RPDC_Surfel->m_ResourceBinderLayoutDescs[8].m_LocalSlot = 0;
	m_RPDC_Surfel->m_ResourceBinderLayoutDescs[8].m_IsRanged = true;

	m_RPDC_Surfel->m_ShaderProgram = m_SPC_Surfel;

	g_pModuleManager->getRenderingServer()->InitializeRenderPassDataComponent(m_RPDC_Surfel);

	m_SDC_Surfel = g_pModuleManager->getRenderingServer()->AddSamplerDataComponent("GIBakeSurfelPass/");

	m_SDC_Surfel->m_SamplerDesc.m_WrapMethodU = TextureWrapMethod::Repeat;
	m_SDC_Surfel->m_SamplerDesc.m_WrapMethodV = TextureWrapMethod::Repeat;

	g_pModuleManager->getRenderingServer()->InitializeSamplerDataComponent(m_SDC_Surfel);

	////
	m_SPC_BrickFactor = g_pModuleManager->getRenderingServer()->AddShaderProgramComponent("GIBakeBrickFactorPass/");

	m_SPC_BrickFactor->m_ShaderFilePaths.m_VSPath = "GIBakeBrickFactorPass.vert/";
	m_SPC_BrickFactor->m_ShaderFilePaths.m_GSPath = "GIBakeBrickFactorPass.geom/";
	m_SPC_BrickFactor->m_ShaderFilePaths.m_PSPath = "GIBakeBrickFactorPass.frag/";

	g_pModuleManager->getRenderingServer()->InitializeShaderProgramComponent(m_SPC_BrickFactor);

	m_RPDC_BrickFactor = g_pModuleManager->getRenderingServer()->AddRenderPassDataComponent("GIBakeBrickFactorPass/");

	m_RPDC_BrickFactor->m_RenderPassDesc = l_RenderPassDesc;
	m_RPDC_BrickFactor->m_RenderPassDesc.m_RenderTargetDesc.SamplerType = TextureSamplerType::SamplerCubemap;
	m_RPDC_BrickFactor->m_RenderPassDesc.m_RenderTargetDesc.Width = m_probeCacheInterval;
	m_RPDC_BrickFactor->m_RenderPassDesc.m_RenderTargetDesc.Height = m_probeCacheInterval;
	m_RPDC_BrickFactor->m_RenderPassDesc.m_RenderTargetDesc.PixelDataType = TexturePixelDataType::FLOAT32;

	m_RPDC_BrickFactor->m_RenderPassDesc.m_GraphicsPipelineDesc.m_DepthStencilDesc.m_UseDepthBuffer = true;
	m_RPDC_BrickFactor->m_RenderPassDesc.m_GraphicsPipelineDesc.m_DepthStencilDesc.m_AllowDepthWrite = true;
	m_RPDC_BrickFactor->m_RenderPassDesc.m_GraphicsPipelineDesc.m_DepthStencilDesc.m_DepthComparisionFunction = ComparisionFunction::LessEqual;

	m_RPDC_BrickFactor->m_RenderPassDesc.m_GraphicsPipelineDesc.m_RasterizerDesc.m_UseCulling = true;
	m_RPDC_BrickFactor->m_RenderPassDesc.m_GraphicsPipelineDesc.m_ViewportDesc.m_Width = m_probeCacheInterval;
	m_RPDC_BrickFactor->m_RenderPassDesc.m_GraphicsPipelineDesc.m_ViewportDesc.m_Height = m_probeCacheInterval;

	m_RPDC_BrickFactor->m_ResourceBinderLayoutDescs.resize(2);
	m_RPDC_BrickFactor->m_ResourceBinderLayoutDescs[0].m_ResourceBinderType = ResourceBinderType::Buffer;
	m_RPDC_BrickFactor->m_ResourceBinderLayoutDescs[0].m_GlobalSlot = 0;
	m_RPDC_BrickFactor->m_ResourceBinderLayoutDescs[0].m_LocalSlot = 10;

	m_RPDC_BrickFactor->m_ResourceBinderLayoutDescs[1].m_ResourceBinderType = ResourceBinderType::Buffer;
	m_RPDC_BrickFactor->m_ResourceBinderLayoutDescs[1].m_GlobalSlot = 1;
	m_RPDC_BrickFactor->m_ResourceBinderLayoutDescs[1].m_LocalSlot = 1;

	m_RPDC_BrickFactor->m_ShaderProgram = m_SPC_BrickFactor;

	g_pModuleManager->getRenderingServer()->InitializeRenderPassDataComponent(m_RPDC_BrickFactor);
}

void InnoBaker::BakeProbeCache(const std::string & sceneName)
{
	g_pModuleManager->getFileSystem()->loadScene(sceneName, false);
	g_pModuleManager->getPhysicsSystem()->updateCulling();
	g_pModuleManager->getRenderingFrontend()->update();
	m_exportFileName = g_pModuleManager->getFileSystem()->getCurrentSceneName();

	std::vector<Probe> l_probesForSurfelCaches;
	std::vector<Probe> l_probesForRuntime;

	auto l_InnoBakerProbeCacheTask = g_pModuleManager->getTaskSystem()->submit("InnoBakerProbeCacheTask", 2, nullptr,
		[&]() {
		gatherStaticMeshData();
		generateProbeCaches(l_probesForSurfelCaches, l_probesForRuntime);
		captureSurfels(l_probesForSurfelCaches, l_probesForRuntime);
	});

	l_InnoBakerProbeCacheTask->Wait();
}

void InnoBaker::BakeBrickCache(const std::string & surfelCacheFileName)
{
	auto l_filePath = g_pModuleManager->getFileSystem()->getWorkingDirectory();
	m_exportFileName = parseFileName(surfelCacheFileName);

	std::ifstream l_surfelCacheFile;

	l_surfelCacheFile.open(l_filePath + surfelCacheFileName, std::ios::binary);

	std::vector<Surfel> l_surfelCaches;

	if (l_surfelCacheFile.is_open())
	{
		IOService::deserializeVector(l_surfelCacheFile, l_surfelCaches);
		generateBrickCaches(l_surfelCaches);
	}
	else
	{
		g_pModuleManager->getLogSystem()->Log(LogLevel::Error, "InnoBakerNS: Surfel cache file not exists!");
	}
}

void InnoBaker::BakeBrick(const std::string & brickCacheFileName)
{
	auto l_filePath = g_pModuleManager->getFileSystem()->getWorkingDirectory();
	m_exportFileName = parseFileName(brickCacheFileName);

	std::ifstream l_brickCacheSummaryFile;

	l_brickCacheSummaryFile.open(l_filePath + brickCacheFileName, std::ios::binary);

	if (l_brickCacheSummaryFile.is_open())
	{
		std::vector<BrickCacheSummary> l_brickCacheSummaries;
		std::vector<BrickCache> l_brickCaches;

		IOService::deserializeVector(l_brickCacheSummaryFile, l_brickCacheSummaries);

		deserializeBrickCaches(l_brickCacheSummaries, l_brickCaches);
		generateBricks(l_brickCaches);
	}
	else
	{
		g_pModuleManager->getLogSystem()->Log(LogLevel::Error, "InnoBakerNS: Brick cache file not exists!");
	}
}

void InnoBaker::BakeBrickFactor(const std::string & brickFileName)
{
	auto l_filePath = g_pModuleManager->getFileSystem()->getWorkingDirectory();
	m_exportFileName = parseFileName(brickFileName);

	std::ifstream l_brickFile;

	l_brickFile.open(l_filePath + brickFileName, std::ios::binary);

	if (l_brickFile.is_open())
	{
		std::vector<Brick> l_bricks;

		IOService::deserializeVector(l_brickFile, l_bricks);

		std::ifstream l_probeFile;

		l_probeFile.open(l_filePath + "Res//Scenes//" + m_exportFileName + ".InnoProbe", std::ios::binary);

		if (l_probeFile.is_open())
		{
			std::vector<Probe> l_probes;

			IOService::deserializeVector(l_probeFile, l_probes);

			auto l_InnoBakerBrickFactorTask = g_pModuleManager->getTaskSystem()->submit("InnoBakerBrickFactorTask", 2, nullptr,
				[&]() {
				assignBrickFactorToProbesByGPU(l_bricks, l_probes);
			});

			l_InnoBakerBrickFactorTask->Wait();
		}
		else
		{
			g_pModuleManager->getLogSystem()->Log(LogLevel::Error, "InnoBakerNS: Probe cache file not exists!");
		}
	}
	else
	{
		g_pModuleManager->getLogSystem()->Log(LogLevel::Error, "InnoBakerNS: Brick file not exists!");
	}
}

bool InnoBakerRenderingClient::Setup()
{
	auto l_InnoBakerRenderingClientSetupTask = g_pModuleManager->getTaskSystem()->submit("InnoBakerRenderingClientSetupTask", 2, nullptr,
		[]() {
		DefaultGPUBuffers::Setup();
		InnoBaker::Setup();
	});
	l_InnoBakerRenderingClientSetupTask->Wait();

	return true;
}

bool InnoBakerRenderingClient::Initialize()
{
	auto l_InnoBakerRenderingClientInitializeTask = g_pModuleManager->getTaskSystem()->submit("InnoBakerRenderingClientInitializeTask", 2, nullptr,
		[]() {
		DefaultGPUBuffers::Initialize();
	});
	l_InnoBakerRenderingClientInitializeTask->Wait();

	return true;
}

bool InnoBakerRenderingClient::Render()
{
	return true;
}

bool InnoBakerRenderingClient::Terminate()
{
	DefaultGPUBuffers::Terminate();

	return true;
}