#include "GLEnvironmentCapturePass.h"
#include "GLRenderingBackendUtilities.h"

#include "GLSkyPass.h"
#include "GLOpaquePass.h"
#include "GLSHPass.h"

#include "../../Component/GLRenderingBackendComponent.h"

#include "../../ModuleManager/IModuleManager.h"

extern IModuleManager* g_pModuleManager;

using namespace GLRenderingBackendNS;

#include "../../FileSystem/IOService.h"

namespace GLEnvironmentCapturePass
{
	bool loadGIData();
	bool generateProbes();
	bool capture();
	bool drawCubemaps(unsigned int probeIndex, const mat4& p, const std::vector<mat4>& v);
	bool drawOpaquePass(unsigned int probeIndex, const mat4& p, const std::vector<mat4>& v, unsigned int faceIndex);
	bool drawSkyVisibilityPass(const mat4& p, const std::vector<mat4>& v, unsigned int faceIndex);
	bool drawSkyPass(const mat4& p, const std::vector<mat4>& v, unsigned int faceIndex);
	bool drawLightPass(const mat4& p, const std::vector<mat4>& v, unsigned int faceIndex);

	bool isBrickEmpty(const std::vector<Surfel>& surfels, const Brick& brick);
	bool generateSurfel(unsigned int probeIndex);
	bool generateBricks(const std::vector<Surfel>& surfels, unsigned int probeIndex);
	bool eliminateDuplication();
	bool findSurfelRangeForBrick(Brick& brick);
	bool assignSurfelRangeToBricks();
	bool serializeProbes();
	bool serializeSurfels();
	bool serializeBricks();

	EntityID m_entityID;

	GLRenderPassComponent* m_opaquePassGLRPC;
	GLRenderPassComponent* m_capturePassGLRPC;
	GLRenderPassComponent* m_skyVisibilityPassGLRPC;

	GLShaderProgramComponent* m_capturePassGLSPC;
	ShaderFilePaths m_capturePassShaderFilePaths = { "environmentCapturePass.vert/" , "", "", "", "environmentCapturePass.frag/" };

	GLShaderProgramComponent* m_skyVisibilityGLSPC;
	ShaderFilePaths m_skyVisibilityShaderFilePaths = { "skyVisibilityPass.vert/" , "", "", "", "skyVisibilityPass.frag/" };

	const unsigned int m_captureResolution = 128;
	const unsigned int m_sampleCountPerFace = m_captureResolution * m_captureResolution;
	const unsigned int m_subDivideDimension = 2;
	const unsigned int m_totalCaptureProbes = m_subDivideDimension * m_subDivideDimension * m_subDivideDimension;

	std::vector<Probe> m_probes;
	std::vector<BrickFactor> m_brickFactors;
	std::vector<Brick> m_bricks;
	std::vector<Surfel> m_surfels;
}

bool GLEnvironmentCapturePass::initialize()
{
	m_entityID = InnoMath::createEntityID();

	auto l_renderPassDesc = GLRenderingBackendComponent::get().m_deferredRenderPassDesc;

	l_renderPassDesc.RTNumber = 4;
	l_renderPassDesc.RTDesc.samplerType = TextureSamplerType::SAMPLER_CUBEMAP;
	l_renderPassDesc.RTDesc.usageType = TextureUsageType::COLOR_ATTACHMENT;
	l_renderPassDesc.RTDesc.pixelDataFormat = TexturePixelDataFormat::RGBA;
	l_renderPassDesc.RTDesc.minFilterMethod = TextureFilterMethod::LINEAR;
	l_renderPassDesc.RTDesc.magFilterMethod = TextureFilterMethod::LINEAR;
	l_renderPassDesc.RTDesc.wrapMethod = TextureWrapMethod::REPEAT;
	l_renderPassDesc.RTDesc.width = m_captureResolution;
	l_renderPassDesc.RTDesc.height = m_captureResolution;
	l_renderPassDesc.RTDesc.pixelDataType = TexturePixelDataType::FLOAT32;
	l_renderPassDesc.useDepthAttachment = true;
	l_renderPassDesc.useStencilAttachment = true;

	m_opaquePassGLRPC = addGLRenderPassComponent(m_entityID, "EnvironmentCaptureOpaquePassGLRPC/");
	m_opaquePassGLRPC->m_renderPassDesc = l_renderPassDesc;
	m_opaquePassGLRPC->m_drawColorBuffers = true;

	initializeGLRenderPassComponent(m_opaquePassGLRPC);

	m_capturePassGLRPC = addGLRenderPassComponent(m_entityID, "EnvironmentCaptureLightPassGLRPC/");
	l_renderPassDesc.RTNumber = 1;
	m_capturePassGLRPC->m_renderPassDesc = l_renderPassDesc;
	m_capturePassGLRPC->m_drawColorBuffers = true;

	initializeGLRenderPassComponent(m_capturePassGLRPC);

	m_skyVisibilityPassGLRPC = addGLRenderPassComponent(m_entityID, "EnvironmentCaptureSkyVisibilityPassGLRPC/");
	l_renderPassDesc.RTNumber = 1;
	l_renderPassDesc.RTDesc.pixelDataFormat = TexturePixelDataFormat::RGBA;
	l_renderPassDesc.RTDesc.pixelDataType = TexturePixelDataType::FLOAT32;
	m_skyVisibilityPassGLRPC->m_renderPassDesc = l_renderPassDesc;
	m_skyVisibilityPassGLRPC->m_drawColorBuffers = true;

	initializeGLRenderPassComponent(m_skyVisibilityPassGLRPC);

	m_capturePassGLSPC = addGLShaderProgramComponent(m_entityID);
	initializeGLShaderProgramComponent(m_capturePassGLSPC, m_capturePassShaderFilePaths);

	m_skyVisibilityGLSPC = addGLShaderProgramComponent(m_entityID);
	initializeGLShaderProgramComponent(m_skyVisibilityGLSPC, m_skyVisibilityShaderFilePaths);

	return true;
}

bool GLEnvironmentCapturePass::drawOpaquePass(unsigned int probeIndex, const mat4& p, const std::vector<mat4>& v, unsigned int faceIndex)
{
	bindRenderPass(m_opaquePassGLRPC);
	cleanRenderBuffers(m_opaquePassGLRPC);

	CameraGPUData l_cameraGPUData;
	l_cameraGPUData.p_original = p;
	l_cameraGPUData.p_jittered = p;
	l_cameraGPUData.globalPos = m_probes[probeIndex].pos;
	auto l_t = InnoMath::getInvertTranslationMatrix(m_probes[probeIndex].pos);
	l_cameraGPUData.t = l_t;
	l_cameraGPUData.t_prev = l_t;

	// draw opaque meshes
	glEnable(GL_DEPTH_TEST);
	glDepthFunc(GL_LEQUAL);
	glDepthMask(GL_FALSE);

	glEnable(GL_DEPTH_CLAMP);

	glEnable(GL_STENCIL_TEST);
	glStencilOp(GL_KEEP, GL_KEEP, GL_REPLACE);
	glStencilMask(0xFF);
	glStencilFunc(GL_ALWAYS, 0x01, 0xFF);

	glEnable(GL_CULL_FACE);
	glCullFace(GL_BACK);

	activateShaderProgram(GLOpaquePass::getGLSPC());

	l_cameraGPUData.r = v[faceIndex];
	l_cameraGPUData.r_prev = v[faceIndex];

	updateUBO(GLRenderingBackendComponent::get().m_cameraUBO, l_cameraGPUData);

	cleanRenderBuffers(m_opaquePassGLRPC);

	bindCubemapTextureForWrite(m_opaquePassGLRPC->m_GLTDCs[0], m_opaquePassGLRPC, 0, faceIndex, 0);
	bindCubemapTextureForWrite(m_opaquePassGLRPC->m_GLTDCs[1], m_opaquePassGLRPC, 1, faceIndex, 0);
	bindCubemapTextureForWrite(m_opaquePassGLRPC->m_GLTDCs[2], m_opaquePassGLRPC, 2, faceIndex, 0);
	bindCubemapTextureForWrite(m_opaquePassGLRPC->m_GLTDCs[3], m_opaquePassGLRPC, 3, faceIndex, 0);

	unsigned int l_offset = 0;

	for (unsigned int i = 0; i < g_pModuleManager->getRenderingFrontend()->getGIPassDrawCallCount(); i++)
	{
		auto l_GIPassGPUData = g_pModuleManager->getRenderingFrontend()->getGIPassGPUData()[i];

		if (l_GIPassGPUData.material->m_normalTexture)
		{
			activateTexture(reinterpret_cast<GLTextureDataComponent*>(l_GIPassGPUData.material->m_normalTexture), 0);
		}
		if (l_GIPassGPUData.material->m_albedoTexture)
		{
			activateTexture(reinterpret_cast<GLTextureDataComponent*>(l_GIPassGPUData.material->m_albedoTexture), 1);
		}
		if (l_GIPassGPUData.material->m_metallicTexture)
		{
			activateTexture(reinterpret_cast<GLTextureDataComponent*>(l_GIPassGPUData.material->m_metallicTexture), 2);
		}
		if (l_GIPassGPUData.material->m_roughnessTexture)
		{
			activateTexture(reinterpret_cast<GLTextureDataComponent*>(l_GIPassGPUData.material->m_roughnessTexture), 3);
		}
		if (l_GIPassGPUData.material->m_aoTexture)
		{
			activateTexture(reinterpret_cast<GLTextureDataComponent*>(l_GIPassGPUData.material->m_aoTexture), 4);
		}

		bindUBO(GLRenderingBackendComponent::get().m_meshUBO, 1, l_offset * sizeof(MeshGPUData), sizeof(MeshGPUData));
		bindUBO(GLRenderingBackendComponent::get().m_materialUBO, 2, l_offset * sizeof(MaterialGPUData), sizeof(MaterialGPUData));

		drawMesh(reinterpret_cast<GLMeshDataComponent*>(l_GIPassGPUData.mesh));

		l_offset++;
	}

	unbindCubemapTextureForWrite(m_opaquePassGLRPC, 0, faceIndex, 0);
	unbindCubemapTextureForWrite(m_opaquePassGLRPC, 1, faceIndex, 0);
	unbindCubemapTextureForWrite(m_opaquePassGLRPC, 2, faceIndex, 0);
	unbindCubemapTextureForWrite(m_opaquePassGLRPC, 3, faceIndex, 0);

	glDisable(GL_CULL_FACE);
	glDisable(GL_STENCIL_TEST);
	glDisable(GL_DEPTH_CLAMP);
	glDisable(GL_DEPTH_TEST);

	return true;
}

bool GLEnvironmentCapturePass::isBrickEmpty(const std::vector<Surfel>& surfels, const Brick& brick)
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

bool GLEnvironmentCapturePass::generateBricks(const std::vector<Surfel>& surfels, unsigned int probeIndex)
{
	auto l_sceneAABB = g_pModuleManager->getPhysicsSystem()->getTotalSceneAABB();

	auto l_sceneCenter = l_sceneAABB.m_center;
	auto l_extendedAxisSize = l_sceneAABB.m_extend;
	l_extendedAxisSize.w = 0.0f;
	auto l_startPos = l_sceneAABB.m_center - (l_extendedAxisSize / 2.0f);
	auto l_currentPos = l_startPos;

	auto l_brickSize = 32.0f;
	auto l_maxBrickCountX = (unsigned int)std::ceil(l_extendedAxisSize.x / l_brickSize);
	auto l_maxBrickCountY = (unsigned int)std::ceil(l_extendedAxisSize.y / l_brickSize);
	auto l_maxBrickCountZ = (unsigned int)std::ceil(l_extendedAxisSize.z / l_brickSize);

	auto l_totalBricks = l_maxBrickCountX * l_maxBrickCountY * l_maxBrickCountZ;

	std::vector<Brick> l_bricks;
	l_bricks.reserve(l_totalBricks);

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

				if (!isBrickEmpty(surfels, l_brick))
				{
					l_bricks.emplace_back(l_brick);
				}

				l_currentPos.z += l_brickSize;
				l_brickIndex++;
			}
			l_currentPos.y += l_brickSize;
		}
		l_currentPos.x += l_brickSize;
		g_pModuleManager->getLogSystem()->printLog(LogType::INNO_DEV_VERBOSE, "GLRenderingBackend: Generating brick: " + std::to_string((float)l_brickIndex * 100.0f / (float)l_totalBricks) + "%");
	}

	l_bricks.shrink_to_fit();

	m_bricks.insert(m_bricks.end(), l_bricks.begin(), l_bricks.end());

	m_probes[probeIndex].bricks = l_bricks;

	return true;
}

bool GLEnvironmentCapturePass::generateSurfel(unsigned int probeIndex)
{
	auto l_posWSMetallic = readCubemapSamples(m_opaquePassGLRPC, m_opaquePassGLRPC->m_GLTDCs[0]);
	auto l_normalRoughness = readCubemapSamples(m_opaquePassGLRPC, m_opaquePassGLRPC->m_GLTDCs[1]);
	auto l_albedoAO = readCubemapSamples(m_opaquePassGLRPC, m_opaquePassGLRPC->m_GLTDCs[2]);

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

	generateBricks(l_surfels, probeIndex);

	m_surfels.insert(m_surfels.end(), l_surfels.begin(), l_surfels.end());

	return true;
}

bool GLEnvironmentCapturePass::drawCubemaps(unsigned int probeIndex, const mat4& p, const std::vector<mat4>& v)
{
	auto l_renderingConfig = g_pModuleManager->getRenderingFrontend()->getRenderingConfig();

	for (unsigned int i = 0; i < 6; i++)
	{
		drawOpaquePass(probeIndex, p, v, i);

		bindRenderPass(m_skyVisibilityPassGLRPC);
		cleanRenderBuffers(m_skyVisibilityPassGLRPC);

		drawSkyVisibilityPass(p, v, i);

		bindRenderPass(m_capturePassGLRPC);
		cleanRenderBuffers(m_capturePassGLRPC);

		if (l_renderingConfig.drawSky)
		{
			drawSkyPass(p, v, i);
		}

		drawLightPass(p, v, i);
	}

	return true;
}

bool GLEnvironmentCapturePass::drawSkyPass(const mat4& p, const std::vector<mat4>& v, unsigned int faceIndex)
{
	auto l_MDC = getGLMeshDataComponent(MeshShapeType::CUBE);

	vec4 l_probePos = vec4(0.0f, 0.0f, 0.0f, 1.0f);
	CameraGPUData l_cameraGPUData;
	l_cameraGPUData.p_original = p;
	l_cameraGPUData.p_jittered = p;
	l_cameraGPUData.globalPos = l_probePos;
	auto l_t = InnoMath::getInvertTranslationMatrix(l_probePos);
	l_cameraGPUData.t = l_t;
	l_cameraGPUData.t_prev = l_t;

	SkyGPUData l_skyGPUData;
	l_skyGPUData.p_inv = p.inverse();
	l_skyGPUData.viewportSize = vec2((float)m_captureResolution, (float)m_captureResolution);

	activateShaderProgram(GLSkyPass::getGLSPC());

	l_cameraGPUData.r = v[faceIndex];
	l_cameraGPUData.r_prev = v[faceIndex];
	l_skyGPUData.r_inv = v[faceIndex].inverse();

	updateUBO(GLRenderingBackendComponent::get().m_skyUBO, l_skyGPUData);
	updateUBO(GLRenderingBackendComponent::get().m_cameraUBO, l_cameraGPUData);

	bindCubemapTextureForWrite(m_capturePassGLRPC->m_GLTDCs[0], m_capturePassGLRPC, 0, faceIndex, 0);

	drawMesh(l_MDC);

	unbindCubemapTextureForWrite(m_capturePassGLRPC, 0, faceIndex, 0);

	return true;
}

bool GLEnvironmentCapturePass::drawLightPass(const mat4& p, const std::vector<mat4>& v, unsigned int faceIndex)
{
	auto l_MDC = getGLMeshDataComponent(MeshShapeType::CUBE);

	auto capturePos = vec4(0.0f, 0.0f, 0.0f, 1.0f);
	CameraGPUData l_cameraGPUData;
	l_cameraGPUData.p_original = p;
	l_cameraGPUData.p_jittered = p;
	l_cameraGPUData.globalPos = capturePos;
	auto l_t = InnoMath::getInvertTranslationMatrix(capturePos);
	l_cameraGPUData.t = l_t;
	l_cameraGPUData.t_prev = l_t;

	glEnable(GL_STENCIL_TEST);
	glStencilOp(GL_KEEP, GL_KEEP, GL_KEEP);
	glStencilFunc(GL_EQUAL, 0x01, 0xFF);
	glStencilMask(0x00);

	copyStencilBuffer(m_opaquePassGLRPC, m_capturePassGLRPC);

	activateShaderProgram(m_capturePassGLSPC);

	activateTexture(m_opaquePassGLRPC->m_GLTDCs[0], 0);
	activateTexture(m_opaquePassGLRPC->m_GLTDCs[1], 1);
	activateTexture(m_opaquePassGLRPC->m_GLTDCs[2], 2);

	l_cameraGPUData.r = v[faceIndex];

	updateUBO(GLRenderingBackendComponent::get().m_cameraUBO, l_cameraGPUData);

	bindCubemapTextureForWrite(m_capturePassGLRPC->m_GLTDCs[0], m_capturePassGLRPC, 0, faceIndex, 0);

	drawMesh(l_MDC);

	unbindCubemapTextureForWrite(m_capturePassGLRPC, 0, faceIndex, 0);

	glDisable(GL_STENCIL_TEST);

	return true;
}

bool GLEnvironmentCapturePass::drawSkyVisibilityPass(const mat4& p, const std::vector<mat4>& v, unsigned int faceIndex)
{
	auto l_MDC = getGLMeshDataComponent(MeshShapeType::CUBE);

	auto l_capturePos = vec4(0.0f, 0.0f, 0.0f, 1.0f);
	CameraGPUData l_cameraGPUData;
	l_cameraGPUData.p_original = p;
	l_cameraGPUData.p_jittered = p;
	l_cameraGPUData.globalPos = l_capturePos;
	auto l_t = InnoMath::getInvertTranslationMatrix(l_capturePos);
	l_cameraGPUData.t = l_t;
	l_cameraGPUData.t_prev = l_t;

	glEnable(GL_STENCIL_TEST);
	glStencilOp(GL_KEEP, GL_KEEP, GL_KEEP);
	glStencilFunc(GL_EQUAL, 0x01, 0xFF);
	glStencilMask(0x00);

	copyStencilBuffer(m_opaquePassGLRPC, m_skyVisibilityPassGLRPC);

	activateShaderProgram(m_skyVisibilityGLSPC);

	l_cameraGPUData.r = v[faceIndex];

	updateUBO(GLRenderingBackendComponent::get().m_cameraUBO, l_cameraGPUData);

	bindCubemapTextureForWrite(m_skyVisibilityPassGLRPC->m_GLTDCs[0], m_skyVisibilityPassGLRPC, 0, faceIndex, 0);

	drawMesh(l_MDC);

	unbindCubemapTextureForWrite(m_skyVisibilityPassGLRPC, 0, faceIndex, 0);

	glDisable(GL_STENCIL_TEST);

	return true;
}

bool GLEnvironmentCapturePass::eliminateDuplication()
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

	m_bricks.erase(std::unique(m_bricks.begin(), m_bricks.end()), m_bricks.end());
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

bool GLEnvironmentCapturePass::findSurfelRangeForBrick(Brick& brick)
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

bool GLEnvironmentCapturePass::assignSurfelRangeToBricks()
{
	for (size_t i = 0; i < m_bricks.size(); i++)
	{
		findSurfelRangeForBrick(m_bricks[i]);
	}

	return true;
}

bool GLEnvironmentCapturePass::capture()
{
	auto l_p = InnoMath::generatePerspectiveMatrix((90.0f / 180.0f) * PI<float>, 1.0f, 0.1f, 1000.0f);

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
		generateSurfel(i);

		auto l_SH9 = GLSHPass::getSH9(m_capturePassGLRPC->m_GLTDCs[0]);
		m_probes[i].radiance = l_SH9;

		l_SH9 = GLSHPass::getSH9(m_skyVisibilityPassGLRPC->m_GLTDCs[0]);
		m_probes[i].skyVisibility = l_SH9;
	}

	return true;
}

bool GLEnvironmentCapturePass::generateProbes()
{
	auto l_sceneAABB = g_pModuleManager->getPhysicsSystem()->getTotalSceneAABB();

	auto l_sceneCenter = l_sceneAABB.m_center;
	auto l_extendedAxisSize = l_sceneAABB.m_extend;
	l_extendedAxisSize = l_extendedAxisSize - vec4(32.0f, 0.0f, 32.0f, 0.0f);
	l_extendedAxisSize.w = 0.0f;
	auto l_probeDistance = l_extendedAxisSize / (float)(m_subDivideDimension - 1);
	auto l_startPos = l_sceneAABB.m_center - (l_extendedAxisSize / 2.0f);
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

				l_currentPos.z += l_probeDistance.z;
				l_probeIndex++;
			}
			l_currentPos.y += l_probeDistance.y;
		}
		l_currentPos.x += l_probeDistance.x;
		g_pModuleManager->getLogSystem()->printLog(LogType::INNO_DEV_VERBOSE, "GLRenderingBackend: Generating probes: " + std::to_string((float)l_probeIndex * 100.0f / (float)m_totalCaptureProbes) + "%");
	}

	return true;
}

bool GLEnvironmentCapturePass::serializeProbes()
{
	auto l_filePath = g_pModuleManager->getFileSystem()->getWorkingDirectory();
	auto l_currentSceneName = g_pModuleManager->getFileSystem()->getCurrentSceneName();

	std::ofstream l_file;
	l_file.open(l_filePath + "//Res//Scenes//" + l_currentSceneName + ".InnoProbe", std::ios::binary);
	IOService::serializeVector(l_file, m_probes);

	return true;
}

bool GLEnvironmentCapturePass::serializeSurfels()
{
	auto l_filePath = g_pModuleManager->getFileSystem()->getWorkingDirectory();
	auto l_currentSceneName = g_pModuleManager->getFileSystem()->getCurrentSceneName();

	std::ofstream l_file;
	l_file.open(l_filePath + "//Res//Scenes//" + l_currentSceneName + ".InnoSurfel", std::ios::binary);
	IOService::serializeVector(l_file, m_surfels);

	return true;
}

bool GLEnvironmentCapturePass::serializeBricks()
{
	auto l_filePath = g_pModuleManager->getFileSystem()->getWorkingDirectory();
	auto l_currentSceneName = g_pModuleManager->getFileSystem()->getCurrentSceneName();

	std::ofstream l_file;
	l_file.open(l_filePath + "//Res//Scenes//" + l_currentSceneName + ".InnoBrick", std::ios::binary);
	IOService::serializeVector(l_file, m_bricks);

	return true;
}

bool GLEnvironmentCapturePass::loadGIData()
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

		return true;
	}
	else
	{
		return false;
	}
}
bool GLEnvironmentCapturePass::update()
{
	if (!loadGIData())
	{
		updateUBO(GLRenderingBackendComponent::get().m_meshUBO, g_pModuleManager->getRenderingFrontend()->getGIPassMeshGPUData());
		updateUBO(GLRenderingBackendComponent::get().m_materialUBO, g_pModuleManager->getRenderingFrontend()->getGIPassMaterialGPUData());

		m_probes.clear();
		m_surfels.clear();
		m_bricks.clear();
		m_brickFactors.clear();

		generateProbes();
		capture();

		eliminateDuplication();
		assignSurfelRangeToBricks();

		serializeProbes();
		serializeSurfels();
		serializeBricks();
	}

	return true;
}

bool GLEnvironmentCapturePass::reloadShader()
{
	return true;
}

GLRenderPassComponent * GLEnvironmentCapturePass::getGLRPC()
{
	return m_capturePassGLRPC;
}

const std::vector<Probe>& GLEnvironmentCapturePass::getProbes()
{
	return m_probes;
}

const std::vector<Brick>& GLEnvironmentCapturePass::getBricks()
{
	return m_bricks;
}