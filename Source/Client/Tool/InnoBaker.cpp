#include "InnoBaker.h"
#include "../DefaultGPUBuffers/DefaultGPUBuffers.h"
#include "../../Engine/Common/CommonMacro.inl"

#include "../../Engine/Common/InnoMathHelper.h"

#include "../../Engine/Interface/IModuleManager.h"
INNO_ENGINE_API extern IModuleManager* g_pModuleManager;

#include "../../Engine/Core/IOService.h"

struct BrickCache
{
	Vec4 pos;
	std::vector<Surfel> surfelCaches;
};

struct BrickCacheSummary
{
	Vec4 pos;
	size_t fileIndex;
	size_t fileSize;
};

namespace InnoBakerNS
{
	std::string parseFileName(const char* fileName);

	bool gatherStaticMeshData();
	bool generateProbeCaches(std::vector<Probe>& probes);
	ProbeInfo generateProbes(std::vector<Probe>& probes, const std::vector<Vec4>& heightMap, uint32_t probeMapSamplingInterval);
	bool generateProbesAlongTheSurface(std::vector<Probe>& probes, const std::vector<Vec4>& heightMap, uint32_t probeMapSamplingInterval);
	uint32_t generateProbesAlongTheWall(std::vector<Probe>& probes, const std::vector<Vec4>& heightMap, uint32_t probeMapSamplingInterval);
	bool serializeProbeInfos(const ProbeInfo& probeInfo);

	bool captureSurfels(std::vector<Probe>& probes);
	bool drawObjects(Probe& probe, const Mat4& p, const std::vector<Mat4>& v);
	bool readBackSurfelCaches(Probe& probe, std::vector<Surfel>& surfelCaches);
	bool eliminateDuplicatedSurfels(std::vector<Surfel>& surfelCaches);

	bool serializeSurfelCaches(const std::vector<Surfel>& surfelCaches);

	bool generateBrickCaches(std::vector<Surfel>& surfelCaches);
	bool serializeBrickCaches(const std::vector<BrickCache>& brickCaches);

	bool deserializeBrickCaches(const std::vector<BrickCacheSummary>& brickCacheSummaries, std::vector<BrickCache>& brickCaches);
	bool generateBricks(const std::vector<BrickCache>& brickCaches);
	bool serializeSurfels(const std::vector<Surfel>& surfels);
	bool serializeBricks(const std::vector<Brick>& bricks);

	bool assignBrickFactorToProbesByGPU(const std::vector<Brick>& bricks, std::vector<Probe>& probes);
	bool drawBricks(Vec4 pos, uint32_t bricksCount, const Mat4& p, const std::vector<Mat4>& v);
	bool readBackBrickFactors(Probe& probe, std::vector<BrickFactor>& brickFactors, const std::vector<Brick>& bricks);

	bool serializeBrickFactors(const std::vector<BrickFactor>& brickFactors);
	bool serializeProbes(const std::vector<Probe>& probes);

	std::string m_exportFileName;

	uint32_t m_staticMeshDrawCallCount = 0;
	std::vector<DrawCallInfo> m_staticMeshDrawCallInfo;
	std::vector<PerObjectConstantBuffer> m_staticMeshPerObjectConstantBuffer;
	std::vector<MaterialConstantBuffer> m_staticMeshMaterialConstantBuffer;

	const uint32_t m_probeMapResolution = 1024;
	const float m_probeHeightOffset = 6.0f;
	const uint32_t m_probeInterval = 64;
	const uint32_t m_captureResolution = 32;
	const uint32_t m_surfelSampleCountPerFace = 32;
	const TVec4<double> m_brickSize = TVec4<double>(4.0, 4.0, 4.0, 0.0);
	const TVec4<double> m_halfBrickSize = m_brickSize / 2.0;

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

std::string InnoBakerNS::parseFileName(const char* fileName)
{
	auto l_fileName = std::string(fileName);
	auto l_startOffset = l_fileName.find_last_of("/");
	auto l_endOffset = l_fileName.find_last_of(".");
	auto l_result = l_fileName.substr(l_startOffset + 1, l_endOffset - l_startOffset - 1);

	return l_result;
}

bool InnoBakerNS::gatherStaticMeshData()
{
	g_pModuleManager->getLogSystem()->Log(LogLevel::Success, "InnoBakerNS: Gathering static meshes...");

	uint32_t l_index = 0;

	auto l_visibleComponents = g_pModuleManager->getComponentManager()->GetAll<VisibleComponent>();
	for (auto visibleComponent : l_visibleComponents)
	{
		if (visibleComponent->m_ObjectStatus == ObjectStatus::Activated && visibleComponent->m_meshUsage == MeshUsage::Static)
		{
			auto l_transformComponent = g_pModuleManager->getComponentManager()->Find<TransformComponent>(visibleComponent->m_Owner);
			auto l_globalTm = l_transformComponent->m_globalTransformMatrix.m_transformationMat;

			for (uint64_t j = 0; j < visibleComponent->m_model->meshMaterialPairs.m_count; j++)
			{
				auto l_meshMaterialPair = g_pModuleManager->getAssetSystem()->getMeshMaterialPair(visibleComponent->m_model->meshMaterialPairs.m_startOffset + j);

				if (l_meshMaterialPair->material->m_ShaderModel == ShaderModel::Opaque)
				{
					DrawCallInfo l_staticPerObjectConstantBuffer;

					l_staticPerObjectConstantBuffer.mesh = l_meshMaterialPair->mesh;
					l_staticPerObjectConstantBuffer.material = l_meshMaterialPair->material;

					PerObjectConstantBuffer l_meshConstantBuffer;

					l_meshConstantBuffer.m = l_transformComponent->m_globalTransformMatrix.m_transformationMat;
					l_meshConstantBuffer.m_prev = l_transformComponent->m_globalTransformMatrix_prev.m_transformationMat;
					l_meshConstantBuffer.normalMat = l_transformComponent->m_globalTransformMatrix.m_rotationMat;
					l_meshConstantBuffer.UUID = (float)visibleComponent->m_UUID;

					MaterialConstantBuffer l_materialConstantBuffer;

					for (size_t i = 0; i < 8; i++)
					{
						uint32_t l_writeMask = l_meshMaterialPair->material->m_TextureSlots[i].m_Activate ? 0x00000001 : 0x00000000;
						l_writeMask = l_writeMask << i;
						l_materialConstantBuffer.textureSlotMask |= l_writeMask;
					}

					l_materialConstantBuffer.materialAttributes = l_meshMaterialPair->material->m_materialAttributes;

					m_staticMeshDrawCallInfo.emplace_back(l_staticPerObjectConstantBuffer);
					m_staticMeshPerObjectConstantBuffer.emplace_back(l_meshConstantBuffer);
					m_staticMeshMaterialConstantBuffer.emplace_back(l_materialConstantBuffer);
					l_index++;
				}
			}
		}
	}

	m_staticMeshDrawCallCount = l_index;

	g_pModuleManager->getLogSystem()->Log(LogLevel::Success, "InnoBakerNS: There are ", m_staticMeshDrawCallCount, " static meshes in current scene.");

	auto l_MeshGBDC = GetGPUBufferDataComponent(GPUBufferUsageType::Mesh);
	auto l_MaterialGBDC = GetGPUBufferDataComponent(GPUBufferUsageType::Material);

	g_pModuleManager->getRenderingServer()->UploadGPUBufferDataComponent(l_MeshGBDC, m_staticMeshPerObjectConstantBuffer, 0, m_staticMeshPerObjectConstantBuffer.size());
	g_pModuleManager->getRenderingServer()->UploadGPUBufferDataComponent(l_MaterialGBDC, m_staticMeshMaterialConstantBuffer, 0, m_staticMeshMaterialConstantBuffer.size());

	return true;
}

bool InnoBakerNS::generateProbeCaches(std::vector<Probe>& probes)
{
	g_pModuleManager->getLogSystem()->Log(LogLevel::Success, "InnoBakerNS: Generate probe caches...");

	auto l_sceneAABB = g_pModuleManager->getPhysicsSystem()->getStaticSceneAABB();

	auto l_eyePos = l_sceneAABB.m_center;
	auto l_extendedAxisSize = l_sceneAABB.m_extend;
	l_eyePos.y += l_extendedAxisSize.y / 2.0f;

	auto l_p = InnoMath::generateOrthographicMatrix(-l_extendedAxisSize.x / 2.0f, l_extendedAxisSize.x / 2.0f, -l_extendedAxisSize.z / 2.0f, l_extendedAxisSize.z / 2.0f, -l_extendedAxisSize.y / 2.0f, l_extendedAxisSize.y / 2.0f);

	std::vector<Mat4> l_GICameraConstantBuffer(8);
	l_GICameraConstantBuffer[0] = l_p;
	l_GICameraConstantBuffer[1] = InnoMath::lookAt(Vec4(0.0f, 0.0f, 0.0f, 1.0f), Vec4(0.0f, -1.0f, 0.0f, 1.0f), Vec4(0.0f, 0.0f, 1.0f, 0.0f));
	l_GICameraConstantBuffer[7] = InnoMath::getInvertTranslationMatrix(l_eyePos);

	g_pModuleManager->getRenderingServer()->UploadGPUBufferDataComponent(GetGPUBufferDataComponent(GPUBufferUsageType::GI), l_GICameraConstantBuffer);

	g_pModuleManager->getLogSystem()->Log(LogLevel::Success, "InnoBakerNS: Start to draw probe height map...");

	auto l_MeshGBDC = GetGPUBufferDataComponent(GPUBufferUsageType::Mesh);

	g_pModuleManager->getRenderingServer()->CommandListBegin(m_RPDC_Probe, 0);
	g_pModuleManager->getRenderingServer()->BindRenderPassDataComponent(m_RPDC_Probe);
	g_pModuleManager->getRenderingServer()->CleanRenderTargets(m_RPDC_Probe);
	g_pModuleManager->getRenderingServer()->ActivateResourceBinder(m_RPDC_Probe, ShaderStage::Vertex, GetGPUBufferDataComponent(GPUBufferUsageType::GI)->m_ResourceBinder, 0, 8, Accessibility::ReadOnly);

	uint32_t l_offset = 0;

	for (uint32_t i = 0; i < m_staticMeshDrawCallCount; i++)
	{
		auto l_staticPerObjectConstantBuffer = m_staticMeshDrawCallInfo[i];

		if (l_staticPerObjectConstantBuffer.mesh->m_ObjectStatus == ObjectStatus::Activated)
		{
			g_pModuleManager->getRenderingServer()->ActivateResourceBinder(m_RPDC_Probe, ShaderStage::Vertex, l_MeshGBDC->m_ResourceBinder, 1, 1, Accessibility::ReadOnly, l_offset, 1);

			g_pModuleManager->getRenderingServer()->DispatchDrawCall(m_RPDC_Probe, l_staticPerObjectConstantBuffer.mesh);
		}

		l_offset++;
	}

	g_pModuleManager->getRenderingServer()->CommandListEnd(m_RPDC_Probe);

	g_pModuleManager->getRenderingServer()->ExecuteCommandList(m_RPDC_Probe);

	g_pModuleManager->getRenderingServer()->WaitForFrame(m_RPDC_Probe);

	g_pModuleManager->getLogSystem()->Log(LogLevel::Success, "InnoBakerNS: Start to generate probe location...");

	// Read back results and generate probes
	auto l_probePosTextureResults = g_pModuleManager->getRenderingServer()->ReadTextureBackToCPU(m_RPDC_Probe, m_RPDC_Probe->m_RenderTargets[0]);

	//#ifdef DEBUG_
	auto l_TDC = g_pModuleManager->getRenderingServer()->AddTextureDataComponent();
	l_TDC->m_TextureDesc = m_RPDC_Probe->m_RenderTargets[0]->m_TextureDesc;
	l_TDC->m_TextureData = l_probePosTextureResults.data();
	g_pModuleManager->getAssetSystem()->saveTexture("..//Res//Intermediate//ProbePosTexture", l_TDC);
	//#endif // DEBUG_

	auto l_probeInfos = generateProbes(probes, l_probePosTextureResults, m_probeInterval);

	serializeProbeInfos(l_probeInfos);

	g_pModuleManager->getLogSystem()->Log(LogLevel::Success, "InnoBakerNS: ", probes.size(), " probes generated.");

	return true;
}

ProbeInfo InnoBakerNS::generateProbes(std::vector<Probe>& probes, const std::vector<Vec4>& heightMap, uint32_t probeMapSamplingInterval)
{
	probes.reserve(heightMap.size());

	generateProbesAlongTheSurface(probes, heightMap, probeMapSamplingInterval);
	auto l_maxVerticalProbesCount = generateProbesAlongTheWall(probes, heightMap, probeMapSamplingInterval);

	ProbeInfo l_result;

	auto l_minProbePos = InnoMath::maxVec4<float>;
	auto l_maxProbePos = InnoMath::minVec4<float>;

	auto l_probesCount = probes.size();

	for (size_t i = 0; i < l_probesCount; i++)
	{
		if (probes[i].pos.x < l_minProbePos.x)
		{
			l_minProbePos.x = probes[i].pos.x;
		}
		if (probes[i].pos.x > l_maxProbePos.x)
		{
			l_maxProbePos.x = probes[i].pos.x;
		}
		if (probes[i].pos.y < l_minProbePos.y)
		{
			l_minProbePos.y = probes[i].pos.y;
		}
		if (probes[i].pos.y > l_maxProbePos.y)
		{
			l_maxProbePos.y = probes[i].pos.y;
		}
		if (probes[i].pos.z < l_minProbePos.z)
		{
			l_minProbePos.z = probes[i].pos.z;
		}
		if (probes[i].pos.z > l_maxProbePos.z)
		{
			l_maxProbePos.z = probes[i].pos.z;
		}
	}

	l_result.probeRange.x = l_maxProbePos.x - l_minProbePos.x;
	l_result.probeRange.y = l_maxProbePos.y - l_minProbePos.y;
	l_result.probeRange.z = l_maxProbePos.z - l_minProbePos.z;
	l_result.probeRange.w = 1.0f;

	auto l_probesCountPerLine = m_probeMapResolution / probeMapSamplingInterval;

	l_result.probeCount.x = (float)l_probesCountPerLine;
	l_result.probeCount.y = (float)l_maxVerticalProbesCount;
	l_result.probeCount.z = (float)l_probesCountPerLine;
	l_result.probeCount.w = 1.0f;

	return l_result;
}

bool InnoBakerNS::generateProbesAlongTheSurface(std::vector<Probe>& probes, const std::vector<Vec4>& heightMap, uint32_t probeMapSamplingInterval)
{
	auto l_probesCountPerLine = m_probeMapResolution / probeMapSamplingInterval;

	for (size_t i = 1; i < l_probesCountPerLine; i++)
	{
		for (size_t j = 1; j < l_probesCountPerLine; j++)
		{
			auto l_currentIndex = i * probeMapSamplingInterval * m_probeMapResolution + j * probeMapSamplingInterval;
			auto l_textureResult = heightMap[l_currentIndex];

			Probe l_Probe;
			l_Probe.pos = l_textureResult;

			// Align the probe height over the surface
			auto l_adjustedHeight = std::ceil(l_textureResult.y / m_probeHeightOffset);

			// Edge case
			if (l_textureResult.y == 0.0f)
			{
				l_adjustedHeight = m_probeHeightOffset;
			}
			else
			{
				l_adjustedHeight = l_adjustedHeight * m_probeHeightOffset;
			}

			l_Probe.pos.y = l_adjustedHeight;

			probes.emplace_back(l_Probe);
		}
	}

	g_pModuleManager->getLogSystem()->Log(LogLevel::Success, "InnoBakerNS: ", probes.size(), " probe location generated over the surface.");

	return true;
}

uint32_t InnoBakerNS::generateProbesAlongTheWall(std::vector<Probe>& probes, const std::vector<Vec4>& heightMap, uint32_t probeMapSamplingInterval)
{
	std::vector<Probe> l_wallProbes;
	l_wallProbes.reserve(heightMap.size());

	uint32_t l_maxVerticalProbesCount = 1;

	auto l_probesCount = probes.size();
	auto l_probesCountPerLine = m_probeMapResolution / probeMapSamplingInterval;

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

			if (ddx > m_probeHeightOffset)
			{
				auto l_verticalProbesCount = std::floor(ddx / m_probeHeightOffset);

				l_maxVerticalProbesCount = std::max((uint32_t)l_verticalProbesCount + 1, l_maxVerticalProbesCount);

				for (size_t k = 0; k < l_verticalProbesCount; k++)
				{
					Probe l_verticalProbe;
					l_verticalProbe.pos = l_nextRowProbe.pos;
					l_verticalProbe.pos.y += m_probeHeightOffset * (k + 1);

					l_wallProbes.emplace_back(l_verticalProbe);
				}
			}
			if (ddx < -m_probeHeightOffset)
			{
				auto l_verticalProbesCount = std::floor(std::abs(ddx) / m_probeHeightOffset);

				l_maxVerticalProbesCount = std::max((uint32_t)l_verticalProbesCount + 1, l_maxVerticalProbesCount);

				for (size_t k = 0; k < l_verticalProbesCount; k++)
				{
					Probe l_verticalProbe;
					l_verticalProbe.pos = l_currentProbe.pos;
					l_verticalProbe.pos.y += m_probeHeightOffset * (k + 1);

					l_wallProbes.emplace_back(l_verticalProbe);
				}
			}
			if (ddy > m_probeHeightOffset)
			{
				auto l_verticalProbesCount = std::floor(ddy / m_probeHeightOffset);

				l_maxVerticalProbesCount = std::max((uint32_t)l_verticalProbesCount + 1, l_maxVerticalProbesCount);

				for (size_t k = 0; k < l_verticalProbesCount; k++)
				{
					Probe l_verticalProbe;
					l_verticalProbe.pos = l_nextColumnProbe.pos;
					l_verticalProbe.pos.y += m_probeHeightOffset * (k + 1);

					l_wallProbes.emplace_back(l_verticalProbe);
				}
			}
			if (ddy < -m_probeHeightOffset)
			{
				auto l_verticalProbesCount = std::floor(std::abs(ddy) / m_probeHeightOffset);

				l_maxVerticalProbesCount = std::max((uint32_t)l_verticalProbesCount + 1, l_maxVerticalProbesCount);

				for (size_t k = 0; k < l_verticalProbesCount; k++)
				{
					Probe l_verticalProbe;
					l_verticalProbe.pos = l_currentProbe.pos;
					l_verticalProbe.pos.y += m_probeHeightOffset * (k + 1);

					l_wallProbes.emplace_back(l_verticalProbe);
				}
			}
		}
	}

	l_wallProbes.shrink_to_fit();
	probes.insert(probes.end(), l_wallProbes.begin(), l_wallProbes.end());
	probes.shrink_to_fit();

	g_pModuleManager->getLogSystem()->Log(LogLevel::Success, "InnoBakerNS: ", probes.size() - l_probesCount, " probe location generated along the wall.");

	return l_maxVerticalProbesCount;
}

bool InnoBakerNS::serializeProbeInfos(const ProbeInfo& probeInfo)
{
	auto l_filePath = g_pModuleManager->getFileSystem()->getWorkingDirectory();

	std::ofstream l_file;
	l_file.open(l_filePath + "..//Res//Scenes//" + m_exportFileName + ".InnoProbeInfo", std::ios::out | std::ios::trunc | std::ios::binary);
	l_file.write((char*)&probeInfo, sizeof(probeInfo));
	l_file.close();

	return true;
}

bool InnoBakerNS::captureSurfels(std::vector<Probe>& probes)
{
	g_pModuleManager->getLogSystem()->Log(LogLevel::Success, "InnoBakerNS: Start to capture surfels...");

	auto l_perFrameConstantBuffer = g_pModuleManager->getRenderingFrontend()->getPerFrameConstantBuffer();

	auto l_p = InnoMath::generatePerspectiveMatrix((90.0f / 180.0f) * PI<float>, 1.0f, l_perFrameConstantBuffer.zNear, l_perFrameConstantBuffer.zFar);

	auto l_rPX = InnoMath::lookAt(Vec4(0.0f, 0.0f, 0.0f, 1.0f), Vec4(1.0f, 0.0f, 0.0f, 1.0f), Vec4(0.0f, -1.0f, 0.0f, 0.0f));
	auto l_rNX = InnoMath::lookAt(Vec4(0.0f, 0.0f, 0.0f, 1.0f), Vec4(-1.0f, 0.0f, 0.0f, 1.0f), Vec4(0.0f, -1.0f, 0.0f, 0.0f));
	auto l_rPY = InnoMath::lookAt(Vec4(0.0f, 0.0f, 0.0f, 1.0f), Vec4(0.0f, 1.0f, 0.0f, 1.0f), Vec4(0.0f, 0.0f, 1.0f, 0.0f));
	auto l_rNY = InnoMath::lookAt(Vec4(0.0f, 0.0f, 0.0f, 1.0f), Vec4(0.0f, -1.0f, 0.0f, 1.0f), Vec4(0.0f, 0.0f, 1.0f, 0.0f));
	auto l_rPZ = InnoMath::lookAt(Vec4(0.0f, 0.0f, 0.0f, 1.0f), Vec4(0.0f, 0.0f, 1.0f, 1.0f), Vec4(0.0f, -1.0f, 0.0f, 0.0f));
	auto l_rNZ = InnoMath::lookAt(Vec4(0.0f, 0.0f, 0.0f, 1.0f), Vec4(0.0f, 0.0f, -1.0f, 1.0f), Vec4(0.0f, -1.0f, 0.0f, 0.0f));

	std::vector<Mat4> l_v =
	{
		l_rPX, l_rNX, l_rPY, l_rNY, l_rPZ, l_rNZ
	};

	auto l_probeForSurfelCachesCount = probes.size();

	std::vector<Surfel> l_surfelCaches;
	l_surfelCaches.reserve(l_probeForSurfelCachesCount * m_surfelSampleCountPerFace * m_surfelSampleCountPerFace * 6);

	for (uint32_t i = 0; i < l_probeForSurfelCachesCount; i++)
	{
		drawObjects(probes[i], l_p, l_v);

		readBackSurfelCaches(probes[i], l_surfelCaches);

		g_pModuleManager->getLogSystem()->Log(LogLevel::Verbose, "InnoBakerNS: Progress: ", (float)i * 100.0f / (float)l_probeForSurfelCachesCount, "%...");
	}

	g_pModuleManager->getLogSystem()->Log(LogLevel::Success, "InnoBakerNS: ", l_surfelCaches.size(), " surfel caches captured.");

	serializeProbes(probes);

	eliminateDuplicatedSurfels(l_surfelCaches);

	serializeSurfelCaches(l_surfelCaches);

	return true;
}

bool InnoBakerNS::drawObjects(Probe& probeCache, const Mat4& p, const std::vector<Mat4>& v)
{
	auto l_t = InnoMath::getInvertTranslationMatrix(probeCache.pos);

	std::vector<Mat4> l_GICameraConstantBuffer(8);
	l_GICameraConstantBuffer[0] = p;
	for (size_t i = 0; i < 6; i++)
	{
		l_GICameraConstantBuffer[i + 1] = v[i];
	}
	l_GICameraConstantBuffer[7] = l_t;

	g_pModuleManager->getRenderingServer()->UploadGPUBufferDataComponent(GetGPUBufferDataComponent(GPUBufferUsageType::GI), l_GICameraConstantBuffer);

	auto l_MeshGBDC = GetGPUBufferDataComponent(GPUBufferUsageType::Mesh);
	auto l_MaterialGBDC = GetGPUBufferDataComponent(GPUBufferUsageType::Material);

	g_pModuleManager->getRenderingServer()->CommandListBegin(m_RPDC_Surfel, 0);
	g_pModuleManager->getRenderingServer()->BindRenderPassDataComponent(m_RPDC_Surfel);
	g_pModuleManager->getRenderingServer()->CleanRenderTargets(m_RPDC_Surfel);
	g_pModuleManager->getRenderingServer()->ActivateResourceBinder(m_RPDC_Surfel, ShaderStage::Pixel, m_SDC_Surfel->m_ResourceBinder, 8, 0);
	g_pModuleManager->getRenderingServer()->ActivateResourceBinder(m_RPDC_Surfel, ShaderStage::Geometry, GetGPUBufferDataComponent(GPUBufferUsageType::GI)->m_ResourceBinder, 0, 8, Accessibility::ReadOnly);

	uint32_t l_offset = 0;

	for (uint32_t i = 0; i < m_staticMeshDrawCallCount; i++)
	{
		auto l_staticPerObjectConstantBuffer = m_staticMeshDrawCallInfo[i];

		if (l_staticPerObjectConstantBuffer.mesh->m_ObjectStatus == ObjectStatus::Activated)
		{
			g_pModuleManager->getRenderingServer()->ActivateResourceBinder(m_RPDC_Surfel, ShaderStage::Vertex, l_MeshGBDC->m_ResourceBinder, 1, 1, Accessibility::ReadOnly, l_offset, 1);
			g_pModuleManager->getRenderingServer()->ActivateResourceBinder(m_RPDC_Surfel, ShaderStage::Pixel, l_MaterialGBDC->m_ResourceBinder, 2, 2, Accessibility::ReadOnly, l_offset, 1);

			if (l_staticPerObjectConstantBuffer.material->m_ObjectStatus == ObjectStatus::Activated)
			{
				g_pModuleManager->getRenderingServer()->ActivateResourceBinder(m_RPDC_Surfel, ShaderStage::Pixel, l_staticPerObjectConstantBuffer.material->m_TextureSlots[0].m_Texture->m_ResourceBinder, 3, 0);
				g_pModuleManager->getRenderingServer()->ActivateResourceBinder(m_RPDC_Surfel, ShaderStage::Pixel, l_staticPerObjectConstantBuffer.material->m_TextureSlots[1].m_Texture->m_ResourceBinder, 4, 1);
				g_pModuleManager->getRenderingServer()->ActivateResourceBinder(m_RPDC_Surfel, ShaderStage::Pixel, l_staticPerObjectConstantBuffer.material->m_TextureSlots[2].m_Texture->m_ResourceBinder, 5, 2);
				g_pModuleManager->getRenderingServer()->ActivateResourceBinder(m_RPDC_Surfel, ShaderStage::Pixel, l_staticPerObjectConstantBuffer.material->m_TextureSlots[3].m_Texture->m_ResourceBinder, 6, 3);
				g_pModuleManager->getRenderingServer()->ActivateResourceBinder(m_RPDC_Surfel, ShaderStage::Pixel, l_staticPerObjectConstantBuffer.material->m_TextureSlots[4].m_Texture->m_ResourceBinder, 7, 4);
			}

			g_pModuleManager->getRenderingServer()->DispatchDrawCall(m_RPDC_Surfel, l_staticPerObjectConstantBuffer.mesh);

			if (l_staticPerObjectConstantBuffer.material->m_ObjectStatus == ObjectStatus::Activated)
			{
				g_pModuleManager->getRenderingServer()->DeactivateResourceBinder(m_RPDC_Surfel, ShaderStage::Pixel, l_staticPerObjectConstantBuffer.material->m_TextureSlots[0].m_Texture->m_ResourceBinder, 3, 0);
				g_pModuleManager->getRenderingServer()->DeactivateResourceBinder(m_RPDC_Surfel, ShaderStage::Pixel, l_staticPerObjectConstantBuffer.material->m_TextureSlots[1].m_Texture->m_ResourceBinder, 4, 1);
				g_pModuleManager->getRenderingServer()->DeactivateResourceBinder(m_RPDC_Surfel, ShaderStage::Pixel, l_staticPerObjectConstantBuffer.material->m_TextureSlots[2].m_Texture->m_ResourceBinder, 5, 2);
				g_pModuleManager->getRenderingServer()->DeactivateResourceBinder(m_RPDC_Surfel, ShaderStage::Pixel, l_staticPerObjectConstantBuffer.material->m_TextureSlots[3].m_Texture->m_ResourceBinder, 6, 3);
				g_pModuleManager->getRenderingServer()->DeactivateResourceBinder(m_RPDC_Surfel, ShaderStage::Pixel, l_staticPerObjectConstantBuffer.material->m_TextureSlots[4].m_Texture->m_ResourceBinder, 7, 4);
			}
		}

		l_offset++;
	}

	g_pModuleManager->getRenderingServer()->CommandListEnd(m_RPDC_Surfel);

	g_pModuleManager->getRenderingServer()->ExecuteCommandList(m_RPDC_Surfel);

	g_pModuleManager->getRenderingServer()->WaitForFrame(m_RPDC_Surfel);

	return true;
}

bool InnoBakerNS::readBackSurfelCaches(Probe& probe, std::vector<Surfel>& surfelCaches)
{
	static uint32_t l_index = 0;

	auto l_posWSMetallic = g_pModuleManager->getRenderingServer()->ReadTextureBackToCPU(m_RPDC_Surfel, m_RPDC_Surfel->m_RenderTargets[0]);
	auto l_normalRoughness = g_pModuleManager->getRenderingServer()->ReadTextureBackToCPU(m_RPDC_Surfel, m_RPDC_Surfel->m_RenderTargets[1]);
	auto l_albedoAO = g_pModuleManager->getRenderingServer()->ReadTextureBackToCPU(m_RPDC_Surfel, m_RPDC_Surfel->m_RenderTargets[2]);
	auto l_depthStencilRT = g_pModuleManager->getRenderingServer()->ReadTextureBackToCPU(m_RPDC_Surfel, m_RPDC_Surfel->m_DepthStencilRenderTarget);

	auto l_TDC = g_pModuleManager->getRenderingServer()->AddTextureDataComponent();
	l_TDC->m_TextureDesc = m_RPDC_Surfel->m_RenderTargets[0]->m_TextureDesc;
	l_TDC->m_TextureData = l_albedoAO.data();
	g_pModuleManager->getAssetSystem()->saveTexture(("..//Res//Intermediate//SurfelTextureAlbedo_" + std::to_string(l_index)).c_str(), l_TDC);

	auto l_surfelsCount = m_surfelSampleCountPerFace * m_surfelSampleCountPerFace * 6;
	auto l_sampleStep = m_captureResolution / m_surfelSampleCountPerFace;

	std::vector<Surfel> l_surfels(l_surfelsCount);
	for (size_t i = 0; i < l_surfelsCount; i++)
	{
		l_surfels[i].pos = l_posWSMetallic[i * l_sampleStep];
		l_surfels[i].pos.w = 1.0f;
		l_surfels[i].normal = l_normalRoughness[i * l_sampleStep];
		l_surfels[i].normal.w = 0.0f;
		l_surfels[i].albedo = l_albedoAO[i * l_sampleStep];
		l_surfels[i].albedo.w = 1.0f;
		l_surfels[i].MRAT.x = l_posWSMetallic[i * l_sampleStep].w;
		l_surfels[i].MRAT.y = l_normalRoughness[i * l_sampleStep].w;
		l_surfels[i].MRAT.z = l_albedoAO[i * l_sampleStep].w;
		l_surfels[i].MRAT.w = 1.0f;
	}

	auto l_depthStencilRTSize = l_depthStencilRT.size();

	std::vector<Vec4> l_DSTDCData(l_depthStencilRTSize);

	l_depthStencilRTSize /= 6;

	for (size_t i = 0; i < 6; i++)
	{
		uint32_t l_stencil = 0;
		for (size_t j = 0; j < l_depthStencilRTSize; j++)
		{
			auto& l_depthStencil = l_depthStencilRT[i * l_depthStencilRTSize + j];

			if (l_depthStencil.y == 1.0f)
			{
				l_stencil++;
				l_DSTDCData[i * l_depthStencilRTSize + j] = Vec4(1.0f, 1.0f, 1.0f, 0.0f);
			}
		}

		probe.skyVisibility[i] = 1.0f - ((float)l_stencil / (float)l_depthStencilRTSize);
	}

	auto l_DSTDC = g_pModuleManager->getRenderingServer()->AddTextureDataComponent();
	l_DSTDC->m_TextureDesc = m_RPDC_Surfel->m_RenderTargets[0]->m_TextureDesc;
	l_DSTDC->m_TextureData = l_DSTDCData.data();
	g_pModuleManager->getAssetSystem()->saveTexture(("..//Res//Intermediate//SurfelTextureDS_" + std::to_string(l_index)).c_str(), l_DSTDC);

	surfelCaches.insert(surfelCaches.end(), l_surfels.begin(), l_surfels.end());

	l_index++;

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
	l_file.open(l_filePath + "..//Res//Intermediate//" + m_exportFileName + ".InnoSurfelCache", std::ios::out | std::ios::trunc | std::ios::binary);
	IOService::serializeVector(l_file, surfelCaches);
	l_file.close();

	g_pModuleManager->getLogSystem()->Log(LogLevel::Success, "InnoBakerNS: ", l_filePath.c_str(), "..//Res//Intermediate//", m_exportFileName.c_str(), ".InnoSurfelCache has been saved.");

	return true;
}

bool InnoBakerNS::generateBrickCaches(std::vector<Surfel>& surfelCaches)
{
	g_pModuleManager->getLogSystem()->Log(LogLevel::Success, "InnoBakerNS: Start to generate brick caches...");

	// Find bound corner position
	auto l_surfelsCount = surfelCaches.size();

	auto l_startPos = InnoMath::maxVec4<double>;
	l_startPos.w = 1.0;

	auto l_endPos = InnoMath::minVec4<double>;
	l_endPos.w = 1.0;

	for (size_t i = 0; i < l_surfelsCount; i++)
	{
		auto l_surfelPos = precisionConvert<float, double>(surfelCaches[i].pos);

		l_startPos = InnoMath::elementWiseMin(l_surfelPos, l_startPos);
		l_endPos = InnoMath::elementWiseMax(l_surfelPos, l_endPos);
	}

	// Fit the end corner to contain at least one brick in each axis
	auto l_extends = l_endPos - l_startPos;
	l_extends.w = 1.0;

	if (l_extends.x < m_brickSize.x)
	{
		l_endPos.x = l_startPos.x + m_brickSize.x;
	}
	if (l_extends.y < m_brickSize.y)
	{
		l_endPos.y = l_startPos.y + m_brickSize.y;
	}
	if (l_extends.z < m_brickSize.z)
	{
		l_endPos.z = l_startPos.z + m_brickSize.z;
	}

	// Adjusted extends
	auto l_adjustedExtends = l_endPos - l_startPos;
	l_adjustedExtends.w = 1.0f;

	auto l_bricksCountX = (size_t)std::ceil(l_adjustedExtends.x / m_brickSize.x);
	auto l_bricksCountY = (size_t)std::ceil(l_adjustedExtends.y / m_brickSize.y);
	auto l_bricksCountZ = (size_t)std::ceil(l_adjustedExtends.z / m_brickSize.z);
	auto l_brickCount = TVec4<size_t>(l_bricksCountX, l_bricksCountY, l_bricksCountZ, 1);

	// Adjusted end
	TVec4<double> l_adjustedEndPos = TVec4<double>();
	l_adjustedEndPos.x = l_startPos.x + l_bricksCountX * m_brickSize.x;
	l_adjustedEndPos.y = l_startPos.y + l_bricksCountY * m_brickSize.y;
	l_adjustedEndPos.z = l_startPos.z + l_bricksCountZ * m_brickSize.z;
	l_adjustedEndPos.w = 1.0;

	// generate all possible brick position
	auto l_totalBricksWorkCount = l_bricksCountX * l_bricksCountY * l_bricksCountZ;

	std::vector<BrickCache> l_brickCaches;
	l_brickCaches.reserve(l_totalBricksWorkCount);

	auto l_currentMaxPos = l_startPos + m_brickSize;
	auto l_currentMinPos = l_startPos;

	auto l_averangeSurfelInABrick = l_surfelsCount / l_totalBricksWorkCount;

	while (l_currentMaxPos.z <= l_adjustedEndPos.z)
	{
		l_currentMaxPos.y = l_startPos.y + m_brickSize.y;
		l_currentMinPos.y = l_startPos.y;

		while (l_currentMaxPos.y <= l_adjustedEndPos.y)
		{
			l_currentMaxPos.x = l_startPos.x + m_brickSize.x;
			l_currentMinPos.x = l_startPos.x;

			while (l_currentMaxPos.x <= l_adjustedEndPos.x)
			{
				BrickCache l_brickCache;
				l_brickCache.pos = precisionConvert<double, float>(l_currentMinPos + m_halfBrickSize);
				l_brickCache.surfelCaches.reserve(l_averangeSurfelInABrick);

				l_brickCaches.emplace_back(std::move(l_brickCache));

				l_currentMaxPos.x += m_brickSize.x;
				l_currentMinPos.x += m_brickSize.x;
			}

			l_currentMaxPos.y += m_brickSize.y;
			l_currentMinPos.y += m_brickSize.y;
		}

		l_currentMaxPos.z += m_brickSize.z;
		l_currentMinPos.z += m_brickSize.z;
	}

	// Assign surfels to brick cache
	for (size_t i = 0; i < l_surfelsCount; i++)
	{
		auto l_posVS = surfelCaches[i].pos - precisionConvert<double, float>(l_startPos);
		auto l_normalizedPos = l_posVS.scale(precisionConvert<double, float>(l_extends.reciprocal()));
		auto l_brickIndexX = (size_t)std::floor((float)(l_brickCount.x - 1) * l_normalizedPos.x);
		auto l_brickIndexY = (size_t)std::floor((float)(l_brickCount.y - 1) * l_normalizedPos.y);
		auto l_brickIndexZ = (size_t)std::floor((float)(l_brickCount.z - 1) * l_normalizedPos.z);
		auto l_brickIndex = l_brickIndexX + l_brickIndexY * l_brickCount.x + l_brickIndexZ * l_brickCount.x * l_brickCount.y;

		l_brickCaches[l_brickIndex].surfelCaches.emplace_back(surfelCaches[i]);

		g_pModuleManager->getLogSystem()->Log(LogLevel::Verbose, "InnoBakerNS: Progress: ", (float)i * 100.0f / (float)l_totalBricksWorkCount, "%...");
	}

	// Remove empty bricks
	l_brickCaches.erase(
		std::remove_if(l_brickCaches.begin(), l_brickCaches.end(),
			[&](auto val) {
				return val.surfelCaches.size() == 0;
			}), l_brickCaches.end());

	l_brickCaches.shrink_to_fit();

	auto l_finalBrickCount = l_brickCaches.size();

	for (size_t i = 0; i < l_finalBrickCount; i++)
	{
		l_brickCaches[i].surfelCaches.shrink_to_fit();
	}

	g_pModuleManager->getLogSystem()->Log(LogLevel::Success, "InnoBakerNS: ", l_brickCaches.size(), " brick caches have been generated.");

	serializeBrickCaches(l_brickCaches);

	return true;
}

bool InnoBakerNS::serializeBrickCaches(const std::vector<BrickCache>& brickCaches)
{
	auto l_brickCacheCount = brickCaches.size();

	// Serialize metadata
	auto l_filePath = g_pModuleManager->getFileSystem()->getWorkingDirectory();
	std::ofstream l_summaryFile;
	l_summaryFile.open(l_filePath + "..//Res//Intermediate//" + m_exportFileName + ".InnoBrickCacheSummary", std::ios::out | std::ios::trunc | std::ios::binary);

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
	l_surfelCacheFile.open(l_filePath + "..//Res//Intermediate//" + m_exportFileName + ".InnoBrickCache", std::ios::out | std::ios::trunc | std::ios::binary);

	for (size_t i = 0; i < l_brickCacheCount; i++)
	{
		IOService::serializeVector(l_surfelCacheFile, brickCaches[i].surfelCaches);
	}
	l_surfelCacheFile.close();

	g_pModuleManager->getLogSystem()->Log(LogLevel::Success, "InnoBakerNS: ", l_filePath.c_str(), "..//Res//Intermediate//", m_exportFileName.c_str(), ".InnoBrickCacheSummary has been saved.");

	return true;
}

bool InnoBakerNS::deserializeBrickCaches(const std::vector<BrickCacheSummary>& brickCacheSummaries, std::vector<BrickCache>& brickCaches)
{
	auto l_brickCount = brickCacheSummaries.size();
	brickCaches.reserve(l_brickCount);

	auto l_filePath = g_pModuleManager->getFileSystem()->getWorkingDirectory();

	std::ifstream l_file;
	l_file.open(l_filePath + "..//Res//Intermediate//" + m_exportFileName + ".InnoBrickCache", std::ios::binary);

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
		auto l_halfBrickSizeFloat = precisionConvert<double, float>(m_halfBrickSize);
		l_brick.boundBox = InnoMath::generateAABB(brickCaches[i].pos + l_halfBrickSizeFloat, brickCaches[i].pos - l_halfBrickSizeFloat);
		l_brick.surfelRangeBegin = (uint32_t)l_offset;
		l_brick.surfelRangeEnd = (uint32_t)(l_offset + brickCaches[i].surfelCaches.size() - 1);
		l_offset += brickCaches[i].surfelCaches.size();

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
	l_file.open(l_filePath + "..//Res//Scenes//" + m_exportFileName + ".InnoSurfel", std::ios::out | std::ios::trunc | std::ios::binary);
	IOService::serializeVector(l_file, surfels);
	l_file.close();

	g_pModuleManager->getLogSystem()->Log(LogLevel::Success, "InnoBakerNS: ", l_filePath.c_str(), "..//Res//Scenes//", m_exportFileName.c_str(), ".InnoSurfel has been saved.");

	return true;
}

bool InnoBakerNS::serializeBricks(const std::vector<Brick>& bricks)
{
	auto l_filePath = g_pModuleManager->getFileSystem()->getWorkingDirectory();

	std::ofstream l_file;
	l_file.open(l_filePath + "..//Res//Scenes//" + m_exportFileName + ".InnoBrick", std::ios::out | std::ios::trunc | std::ios::binary);
	IOService::serializeVector(l_file, bricks);
	l_file.close();

	g_pModuleManager->getLogSystem()->Log(LogLevel::Success, "InnoBakerNS: ", l_filePath.c_str(), "..//Res//Scenes//", m_exportFileName.c_str(), ".InnoBrick has been saved.");

	return true;
}

bool InnoBakerNS::assignBrickFactorToProbesByGPU(const std::vector<Brick>& bricks, std::vector<Probe>& probes)
{
	g_pModuleManager->getLogSystem()->Log(LogLevel::Success, "InnoBakerNS: Start to generate brick factor and assign to probes...");

	// Upload camera data and brick cubes data to GPU memory
	auto l_rPX = InnoMath::lookAt(Vec4(0.0f, 0.0f, 0.0f, 1.0f), Vec4(1.0f, 0.0f, 0.0f, 1.0f), Vec4(0.0f, -1.0f, 0.0f, 0.0f));
	auto l_rNX = InnoMath::lookAt(Vec4(0.0f, 0.0f, 0.0f, 1.0f), Vec4(-1.0f, 0.0f, 0.0f, 1.0f), Vec4(0.0f, -1.0f, 0.0f, 0.0f));
	auto l_rPY = InnoMath::lookAt(Vec4(0.0f, 0.0f, 0.0f, 1.0f), Vec4(0.0f, 1.0f, 0.0f, 1.0f), Vec4(0.0f, 0.0f, 1.0f, 0.0f));
	auto l_rNY = InnoMath::lookAt(Vec4(0.0f, 0.0f, 0.0f, 1.0f), Vec4(0.0f, -1.0f, 0.0f, 1.0f), Vec4(0.0f, 0.0f, 1.0f, 0.0f));
	auto l_rPZ = InnoMath::lookAt(Vec4(0.0f, 0.0f, 0.0f, 1.0f), Vec4(0.0f, 0.0f, 1.0f, 1.0f), Vec4(0.0f, -1.0f, 0.0f, 0.0f));
	auto l_rNZ = InnoMath::lookAt(Vec4(0.0f, 0.0f, 0.0f, 1.0f), Vec4(0.0f, 0.0f, -1.0f, 1.0f), Vec4(0.0f, -1.0f, 0.0f, 0.0f));

	std::vector<Mat4> l_v =
	{
		l_rPX, l_rNX, l_rPY, l_rNY, l_rPZ, l_rNZ
	};

	auto l_p = InnoMath::generatePerspectiveMatrix((90.0f / 180.0f) * PI<float>, 1.0f, 0.1f, 2000.0f);

	auto l_bricksCount = bricks.size();

	std::vector<PerObjectConstantBuffer> l_bricksCubePerObjectConstantBuffer;
	l_bricksCubePerObjectConstantBuffer.resize(l_bricksCount);

	for (size_t i = 0; i < l_bricksCount; i++)
	{
		auto l_t = InnoMath::toTranslationMatrix(bricks[i].boundBox.m_center);

		l_bricksCubePerObjectConstantBuffer[i].m = l_t;

		// @TODO: Find a better way to assign without error
		l_bricksCubePerObjectConstantBuffer[i].m.m00 *= (bricks[i].boundBox.m_extend.x / 2.0f) - 0.1f;
		l_bricksCubePerObjectConstantBuffer[i].m.m11 *= (bricks[i].boundBox.m_extend.y / 2.0f) - 0.1f;
		l_bricksCubePerObjectConstantBuffer[i].m.m22 *= (bricks[i].boundBox.m_extend.z / 2.0f) - 0.1f;

		// Index start from 1
		l_bricksCubePerObjectConstantBuffer[i].UUID = (float)i + 1.0f;
	}

	auto l_MeshGBDC = GetGPUBufferDataComponent(GPUBufferUsageType::Mesh);
	g_pModuleManager->getRenderingServer()->UploadGPUBufferDataComponent(l_MeshGBDC, l_bricksCubePerObjectConstantBuffer, 0, l_bricksCubePerObjectConstantBuffer.size());

	// assign bricks to probe by the depth test result
	auto l_probesCount = probes.size();

	std::vector<BrickFactor> l_brickFactors;
	l_brickFactors.reserve(l_probesCount * l_bricksCount);

	for (size_t i = 0; i < l_probesCount; i++)
	{
		drawBricks(probes[i].pos, (uint32_t)l_bricksCount, l_p, l_v);
		readBackBrickFactors(probes[i], l_brickFactors, bricks);

		g_pModuleManager->getLogSystem()->Log(LogLevel::Verbose, "InnoBakerNS: Progress: ", (float)i * 100.0f / (float)l_probesCount, "%...");
	}

	l_brickFactors.shrink_to_fit();

	g_pModuleManager->getLogSystem()->Log(LogLevel::Success, "InnoBakerNS: ", l_brickFactors.size(), " brick factors have been generated.");

	serializeBrickFactors(l_brickFactors);
	serializeProbes(probes);

	return true;
}

bool InnoBakerNS::drawBricks(Vec4 pos, uint32_t bricksCount, const Mat4& p, const std::vector<Mat4>& v)
{
	std::vector<Mat4> l_GICameraConstantBuffer(8);
	l_GICameraConstantBuffer[0] = p;
	for (size_t i = 0; i < 6; i++)
	{
		l_GICameraConstantBuffer[i + 1] = v[i];
	}
	l_GICameraConstantBuffer[7] = InnoMath::getInvertTranslationMatrix(pos);

	g_pModuleManager->getRenderingServer()->UploadGPUBufferDataComponent(GetGPUBufferDataComponent(GPUBufferUsageType::GI), l_GICameraConstantBuffer);

	auto l_MeshGBDC = GetGPUBufferDataComponent(GPUBufferUsageType::Mesh);

	auto l_mesh = g_pModuleManager->getRenderingFrontend()->getMeshDataComponent(ProceduralMeshShape::Cube);

	uint32_t l_offset = 0;

	g_pModuleManager->getRenderingServer()->CommandListBegin(m_RPDC_BrickFactor, 0);
	g_pModuleManager->getRenderingServer()->BindRenderPassDataComponent(m_RPDC_BrickFactor);
	g_pModuleManager->getRenderingServer()->CleanRenderTargets(m_RPDC_BrickFactor);
	g_pModuleManager->getRenderingServer()->ActivateResourceBinder(m_RPDC_BrickFactor, ShaderStage::Geometry, GetGPUBufferDataComponent(GPUBufferUsageType::GI)->m_ResourceBinder, 0, 8, Accessibility::ReadOnly);

	for (uint32_t i = 0; i < bricksCount; i++)
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

bool InnoBakerNS::readBackBrickFactors(Probe& probe, std::vector<BrickFactor>& brickFactors, const std::vector<Brick>& bricks)
{
	static int l_index = 0;

	auto l_brickIDResults = g_pModuleManager->getRenderingServer()->ReadTextureBackToCPU(m_RPDC_BrickFactor, m_RPDC_BrickFactor->m_RenderTargets[0]);

	auto l_TDC = g_pModuleManager->getRenderingServer()->AddTextureDataComponent();
	l_TDC->m_TextureDesc = m_RPDC_BrickFactor->m_RenderTargets[0]->m_TextureDesc;
	l_TDC->m_TextureData = l_brickIDResults.data();
	g_pModuleManager->getAssetSystem()->saveTexture(("..//Res//Intermediate//BrickTexture_" + std::to_string(l_index)).c_str(), l_TDC);
	l_index++;

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

				// Index start from 1
				l_BrickFactor.brickIndex = (uint32_t)(std::round(l_brickIDResult.y) - 1.0f);
				l_brickFactors.emplace_back(l_BrickFactor);
			}
		}

		// Calculate brick weight
		if (l_brickFactors.size() > 0)
		{
			std::sort(l_brickFactors.begin(), l_brickFactors.end(), [&](BrickFactor A, BrickFactor B)
				{
					return A.brickIndex < B.brickIndex;
				});

			l_brickFactors.erase(std::unique(l_brickFactors.begin(), l_brickFactors.end()), l_brickFactors.end());
			l_brickFactors.shrink_to_fit();

			if (l_brickFactors.size() == 1)
			{
				l_brickFactors[0].basisWeight = 1.0f;
			}
			else
			{
				// Weight
				auto l_brickFactorSize = l_brickFactors.size();

				// World space distance
				for (size_t j = 0; j < l_brickFactorSize; j++)
				{
					l_brickFactors[j].basisWeight = (bricks[l_brickFactors[j].brickIndex].boundBox.m_center - probe.pos).length();
				}

				auto l_min = std::numeric_limits<float>().max();
				auto l_max = std::numeric_limits<float>().min();

				for (size_t j = 0; j < l_brickFactorSize; j++)
				{
					l_min = l_brickFactors[j].basisWeight <= l_min ? l_brickFactors[j].basisWeight : l_min;
					l_max = l_brickFactors[j].basisWeight >= l_max ? l_brickFactors[j].basisWeight : l_max;
				}

				auto l_range = l_max + l_min;

				// Reverse along the view space Z axis
				for (size_t j = 0; j < l_brickFactorSize; j++)
				{
					l_brickFactors[j].basisWeight = (l_range - l_brickFactors[j].basisWeight);
				}

				// Normalize
				float denom = 0.0f;
				for (size_t j = 0; j < l_brickFactorSize; j++)
				{
					denom += l_brickFactors[j].basisWeight;
				}

				for (size_t j = 0; j < l_brickFactorSize; j++)
				{
					l_brickFactors[j].basisWeight /= denom;
				}
			}

			// Assign brick factor range to probes
			auto l_brickFactorRangeBegin = brickFactors.size();
			auto l_brickFactorRangeEnd = l_brickFactorRangeBegin + l_brickFactors.size() - 1;

			probe.brickFactorRange[i * 2] = (uint32_t)l_brickFactorRangeBegin;
			probe.brickFactorRange[i * 2 + 1] = (uint32_t)l_brickFactorRangeEnd;

			brickFactors.insert(brickFactors.end(), std::make_move_iterator(l_brickFactors.begin()), std::make_move_iterator(l_brickFactors.end()));
		}
		else
		{
			probe.brickFactorRange[i * 2] = -1;
			probe.brickFactorRange[i * 2 + 1] = -1;
		}
	}

	return true;
}

bool InnoBakerNS::serializeBrickFactors(const std::vector<BrickFactor>& brickFactors)
{
	auto l_filePath = g_pModuleManager->getFileSystem()->getWorkingDirectory();

	std::ofstream l_file;
	l_file.open(l_filePath + "..//Res//Scenes//" + m_exportFileName + ".InnoBrickFactor", std::ios::out | std::ios::trunc | std::ios::binary);
	IOService::serializeVector(l_file, brickFactors);
	l_file.close();

	g_pModuleManager->getLogSystem()->Log(LogLevel::Success, "InnoBakerNS: ", l_filePath.c_str(), "..//Res//Scenes//", m_exportFileName.c_str(), ".InnoBrickFactor has been saved.");

	return true;
}

bool InnoBakerNS::serializeProbes(const std::vector<Probe>& probes)
{
	auto l_filePath = g_pModuleManager->getFileSystem()->getWorkingDirectory();

	std::ofstream l_file;
	l_file.open(l_filePath + "..//Res//Scenes//" + m_exportFileName + ".InnoProbe", std::ios::out | std::ios::trunc | std::ios::binary);
	IOService::serializeVector(l_file, probes);
	l_file.close();

	g_pModuleManager->getLogSystem()->Log(LogLevel::Success, "InnoBakerNS: ", l_filePath.c_str(), "..//Res//Scenes//", m_exportFileName.c_str(), ".InnoProbe has been saved.");

	return true;
}

void InnoBaker::Setup()
{
	////
	auto l_RenderingCapability = g_pModuleManager->getRenderingFrontend()->getRenderingCapability();

	m_staticMeshDrawCallInfo.reserve(l_RenderingCapability.maxMeshes);
	m_staticMeshPerObjectConstantBuffer.reserve(l_RenderingCapability.maxMeshes);
	m_staticMeshMaterialConstantBuffer.reserve(l_RenderingCapability.maxMaterials);

	auto l_RenderPassDesc = g_pModuleManager->getRenderingFrontend()->getDefaultRenderPassDesc();
	l_RenderPassDesc.m_UseDepthBuffer = true;
	l_RenderPassDesc.m_UseStencilBuffer = true;

	////
	m_SPC_Probe = g_pModuleManager->getRenderingServer()->AddShaderProgramComponent("GIBakeProbePass/");

	m_SPC_Probe->m_ShaderFilePaths.m_VSPath = "GIBakeProbePass.vert/";
	m_SPC_Probe->m_ShaderFilePaths.m_PSPath = "GIBakeProbePass.frag/";

	g_pModuleManager->getRenderingServer()->InitializeShaderProgramComponent(m_SPC_Probe);

	m_RPDC_Probe = g_pModuleManager->getRenderingServer()->AddRenderPassDataComponent("GIBakeProbePass/");

	m_RPDC_Probe->m_RenderPassDesc = l_RenderPassDesc;
	m_RPDC_Probe->m_RenderPassDesc.m_RenderTargetDesc.Sampler = TextureSampler::Sampler2D;
	m_RPDC_Probe->m_RenderPassDesc.m_RenderTargetDesc.Width = m_probeMapResolution;
	m_RPDC_Probe->m_RenderPassDesc.m_RenderTargetDesc.Height = m_probeMapResolution;
	m_RPDC_Probe->m_RenderPassDesc.m_RenderTargetDesc.PixelDataType = TexturePixelDataType::Float32;

	m_RPDC_Probe->m_RenderPassDesc.m_GraphicsPipelineDesc.m_DepthStencilDesc.m_DepthEnable = true;
	m_RPDC_Probe->m_RenderPassDesc.m_GraphicsPipelineDesc.m_DepthStencilDesc.m_AllowDepthWrite = true;
	m_RPDC_Probe->m_RenderPassDesc.m_GraphicsPipelineDesc.m_DepthStencilDesc.m_DepthComparisionFunction = ComparisionFunction::LessEqual;
	m_RPDC_Probe->m_RenderPassDesc.m_GraphicsPipelineDesc.m_DepthStencilDesc.m_StencilEnable = true;
	m_RPDC_Probe->m_RenderPassDesc.m_GraphicsPipelineDesc.m_DepthStencilDesc.m_AllowStencilWrite = true;

	m_RPDC_Probe->m_RenderPassDesc.m_GraphicsPipelineDesc.m_RasterizerDesc.m_UseCulling = true;
	m_RPDC_Probe->m_RenderPassDesc.m_GraphicsPipelineDesc.m_ViewportDesc.m_Width = m_probeMapResolution;
	m_RPDC_Probe->m_RenderPassDesc.m_GraphicsPipelineDesc.m_ViewportDesc.m_Height = m_probeMapResolution;

	m_RPDC_Probe->m_ResourceBinderLayoutDescs.resize(2);
	m_RPDC_Probe->m_ResourceBinderLayoutDescs[0].m_ResourceBinderType = ResourceBinderType::Buffer;
	m_RPDC_Probe->m_ResourceBinderLayoutDescs[0].m_DescriptorSetIndex = 0;
	m_RPDC_Probe->m_ResourceBinderLayoutDescs[0].m_DescriptorIndex = 8;

	m_RPDC_Probe->m_ResourceBinderLayoutDescs[1].m_ResourceBinderType = ResourceBinderType::Buffer;
	m_RPDC_Probe->m_ResourceBinderLayoutDescs[1].m_DescriptorSetIndex = 1;
	m_RPDC_Probe->m_ResourceBinderLayoutDescs[1].m_DescriptorIndex = 1;

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

	m_RPDC_Surfel->m_RenderPassDesc.m_RenderTargetDesc.Sampler = TextureSampler::SamplerCubemap;
	m_RPDC_Surfel->m_RenderPassDesc.m_RenderTargetDesc.Width = m_captureResolution;
	m_RPDC_Surfel->m_RenderPassDesc.m_RenderTargetDesc.Height = m_captureResolution;
	m_RPDC_Surfel->m_RenderPassDesc.m_RenderTargetDesc.PixelDataType = TexturePixelDataType::Float32;

	m_RPDC_Surfel->m_RenderPassDesc.m_GraphicsPipelineDesc.m_DepthStencilDesc.m_DepthEnable = true;
	m_RPDC_Surfel->m_RenderPassDesc.m_GraphicsPipelineDesc.m_DepthStencilDesc.m_AllowDepthWrite = true;
	m_RPDC_Surfel->m_RenderPassDesc.m_GraphicsPipelineDesc.m_DepthStencilDesc.m_AllowDepthClamp = true;
	m_RPDC_Surfel->m_RenderPassDesc.m_GraphicsPipelineDesc.m_DepthStencilDesc.m_DepthComparisionFunction = ComparisionFunction::LessEqual;

	m_RPDC_Surfel->m_RenderPassDesc.m_GraphicsPipelineDesc.m_DepthStencilDesc.m_StencilEnable = true;
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
	m_RPDC_Surfel->m_ResourceBinderLayoutDescs[0].m_DescriptorSetIndex = 0;
	m_RPDC_Surfel->m_ResourceBinderLayoutDescs[0].m_DescriptorIndex = 8;

	m_RPDC_Surfel->m_ResourceBinderLayoutDescs[1].m_ResourceBinderType = ResourceBinderType::Buffer;
	m_RPDC_Surfel->m_ResourceBinderLayoutDescs[1].m_DescriptorSetIndex = 1;
	m_RPDC_Surfel->m_ResourceBinderLayoutDescs[1].m_DescriptorIndex = 1;

	m_RPDC_Surfel->m_ResourceBinderLayoutDescs[2].m_ResourceBinderType = ResourceBinderType::Buffer;
	m_RPDC_Surfel->m_ResourceBinderLayoutDescs[2].m_DescriptorSetIndex = 2;
	m_RPDC_Surfel->m_ResourceBinderLayoutDescs[2].m_DescriptorIndex = 2;

	m_RPDC_Surfel->m_ResourceBinderLayoutDescs[3].m_ResourceBinderType = ResourceBinderType::Image;
	m_RPDC_Surfel->m_ResourceBinderLayoutDescs[3].m_DescriptorSetIndex = 3;
	m_RPDC_Surfel->m_ResourceBinderLayoutDescs[3].m_DescriptorIndex = 0;
	m_RPDC_Surfel->m_ResourceBinderLayoutDescs[3].m_IndirectBinding = true;

	m_RPDC_Surfel->m_ResourceBinderLayoutDescs[4].m_ResourceBinderType = ResourceBinderType::Image;
	m_RPDC_Surfel->m_ResourceBinderLayoutDescs[4].m_DescriptorSetIndex = 4;
	m_RPDC_Surfel->m_ResourceBinderLayoutDescs[4].m_DescriptorIndex = 1;
	m_RPDC_Surfel->m_ResourceBinderLayoutDescs[4].m_IndirectBinding = true;

	m_RPDC_Surfel->m_ResourceBinderLayoutDescs[5].m_ResourceBinderType = ResourceBinderType::Image;
	m_RPDC_Surfel->m_ResourceBinderLayoutDescs[5].m_DescriptorSetIndex = 5;
	m_RPDC_Surfel->m_ResourceBinderLayoutDescs[5].m_DescriptorIndex = 2;
	m_RPDC_Surfel->m_ResourceBinderLayoutDescs[5].m_ResourceCount = 1;
	m_RPDC_Surfel->m_ResourceBinderLayoutDescs[5].m_IndirectBinding = true;

	m_RPDC_Surfel->m_ResourceBinderLayoutDescs[6].m_ResourceBinderType = ResourceBinderType::Image;
	m_RPDC_Surfel->m_ResourceBinderLayoutDescs[6].m_DescriptorSetIndex = 6;
	m_RPDC_Surfel->m_ResourceBinderLayoutDescs[6].m_DescriptorIndex = 3;
	m_RPDC_Surfel->m_ResourceBinderLayoutDescs[6].m_IndirectBinding = true;

	m_RPDC_Surfel->m_ResourceBinderLayoutDescs[7].m_ResourceBinderType = ResourceBinderType::Image;
	m_RPDC_Surfel->m_ResourceBinderLayoutDescs[7].m_DescriptorSetIndex = 7;
	m_RPDC_Surfel->m_ResourceBinderLayoutDescs[7].m_DescriptorIndex = 4;
	m_RPDC_Surfel->m_ResourceBinderLayoutDescs[7].m_IndirectBinding = true;

	m_RPDC_Surfel->m_ResourceBinderLayoutDescs[8].m_ResourceBinderType = ResourceBinderType::Sampler;
	m_RPDC_Surfel->m_ResourceBinderLayoutDescs[8].m_DescriptorSetIndex = 8;
	m_RPDC_Surfel->m_ResourceBinderLayoutDescs[8].m_DescriptorIndex = 0;
	m_RPDC_Surfel->m_ResourceBinderLayoutDescs[8].m_IndirectBinding = true;

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
	m_RPDC_BrickFactor->m_RenderPassDesc.m_RenderTargetDesc.Sampler = TextureSampler::SamplerCubemap;
	m_RPDC_BrickFactor->m_RenderPassDesc.m_RenderTargetDesc.Width = 64;
	m_RPDC_BrickFactor->m_RenderPassDesc.m_RenderTargetDesc.Height = 64;
	m_RPDC_BrickFactor->m_RenderPassDesc.m_RenderTargetDesc.PixelDataType = TexturePixelDataType::Float32;

	m_RPDC_BrickFactor->m_RenderPassDesc.m_GraphicsPipelineDesc.m_DepthStencilDesc.m_DepthEnable = true;
	m_RPDC_BrickFactor->m_RenderPassDesc.m_GraphicsPipelineDesc.m_DepthStencilDesc.m_AllowDepthWrite = true;
	m_RPDC_BrickFactor->m_RenderPassDesc.m_GraphicsPipelineDesc.m_DepthStencilDesc.m_AllowDepthClamp = true;
	m_RPDC_BrickFactor->m_RenderPassDesc.m_GraphicsPipelineDesc.m_DepthStencilDesc.m_DepthComparisionFunction = ComparisionFunction::LessEqual;
	m_RPDC_BrickFactor->m_RenderPassDesc.m_GraphicsPipelineDesc.m_DepthStencilDesc.m_StencilEnable = true;
	m_RPDC_BrickFactor->m_RenderPassDesc.m_GraphicsPipelineDesc.m_DepthStencilDesc.m_AllowStencilWrite = true;
	m_RPDC_BrickFactor->m_RenderPassDesc.m_GraphicsPipelineDesc.m_DepthStencilDesc.m_StencilReference = 0x01;
	m_RPDC_BrickFactor->m_RenderPassDesc.m_GraphicsPipelineDesc.m_DepthStencilDesc.m_FrontFaceStencilPassOperation = StencilOperation::Replace;
	m_RPDC_BrickFactor->m_RenderPassDesc.m_GraphicsPipelineDesc.m_DepthStencilDesc.m_FrontFaceStencilComparisionFunction = ComparisionFunction::Always;
	m_RPDC_BrickFactor->m_RenderPassDesc.m_GraphicsPipelineDesc.m_DepthStencilDesc.m_BackFaceStencilPassOperation = StencilOperation::Replace;
	m_RPDC_BrickFactor->m_RenderPassDesc.m_GraphicsPipelineDesc.m_DepthStencilDesc.m_BackFaceStencilComparisionFunction = ComparisionFunction::Always;

	m_RPDC_BrickFactor->m_RenderPassDesc.m_GraphicsPipelineDesc.m_RasterizerDesc.m_UseCulling = true;
	m_RPDC_BrickFactor->m_RenderPassDesc.m_GraphicsPipelineDesc.m_ViewportDesc.m_Width = 64;
	m_RPDC_BrickFactor->m_RenderPassDesc.m_GraphicsPipelineDesc.m_ViewportDesc.m_Height = 64;

	m_RPDC_BrickFactor->m_ResourceBinderLayoutDescs.resize(2);
	m_RPDC_BrickFactor->m_ResourceBinderLayoutDescs[0].m_ResourceBinderType = ResourceBinderType::Buffer;
	m_RPDC_BrickFactor->m_ResourceBinderLayoutDescs[0].m_DescriptorSetIndex = 0;
	m_RPDC_BrickFactor->m_ResourceBinderLayoutDescs[0].m_DescriptorIndex = 8;

	m_RPDC_BrickFactor->m_ResourceBinderLayoutDescs[1].m_ResourceBinderType = ResourceBinderType::Buffer;
	m_RPDC_BrickFactor->m_ResourceBinderLayoutDescs[1].m_DescriptorSetIndex = 1;
	m_RPDC_BrickFactor->m_ResourceBinderLayoutDescs[1].m_DescriptorIndex = 1;

	m_RPDC_BrickFactor->m_ShaderProgram = m_SPC_BrickFactor;

	g_pModuleManager->getRenderingServer()->InitializeRenderPassDataComponent(m_RPDC_BrickFactor);
}

void InnoBaker::BakeProbeCache(const char* sceneName)
{
	g_pModuleManager->getSceneSystem()->loadScene(sceneName, false);
	//GetComponentManager(LightComponent)->Simulate();

	//g_pModuleManager->getPhysicsSystem()->updateCulling();
	//g_pModuleManager->getRenderingFrontend()->update();
	m_exportFileName = g_pModuleManager->getSceneSystem()->getCurrentSceneName();

	std::vector<Probe> l_probes;

	auto l_InnoBakerProbeCacheTask = g_pModuleManager->getTaskSystem()->submit("InnoBakerProbeCacheTask", 2, nullptr,
		[&]() {
			gatherStaticMeshData();
			generateProbeCaches(l_probes);
			captureSurfels(l_probes);
		});

	l_InnoBakerProbeCacheTask->Wait();
}

void InnoBaker::BakeBrickCache(const char* surfelCacheFileName)
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

void InnoBaker::BakeBrick(const char* brickCacheFileName)
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

void InnoBaker::BakeBrickFactor(const char* brickFileName)
{
	auto l_filePath = g_pModuleManager->getFileSystem()->getWorkingDirectory();
	m_exportFileName = parseFileName(brickFileName);

	std::ifstream l_brickFile;

	l_brickFile.open(l_filePath + brickFileName, std::ios::binary);

	if (l_brickFile.is_open())
	{
		std::vector<Brick> l_bricks;

		IOService::deserializeVector(l_brickFile, l_bricks);

		l_brickFile.close();

		std::ifstream l_probeFile;

		l_probeFile.open(l_filePath + "..//Res//Scenes//" + m_exportFileName + ".InnoProbe", std::ios::binary);

		if (l_probeFile.is_open())
		{
			std::vector<Probe> l_probes;

			IOService::deserializeVector(l_probeFile, l_probes);

			l_probeFile.close();

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

bool InnoBakerRenderingClient::Setup(ISystemConfig* systemConfig)
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

ObjectStatus InnoBakerRenderingClient::GetStatus()
{
	return ObjectStatus();
}
