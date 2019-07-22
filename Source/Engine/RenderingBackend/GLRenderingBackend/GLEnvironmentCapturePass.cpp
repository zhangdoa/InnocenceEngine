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

INNO_PRIVATE_SCOPE GLEnvironmentCapturePass
{
	bool capture(vec4 probePos, unsigned int probeIndex);
	bool gatherGeometryData(vec4 probePos, mat4 p, const std::vector<mat4>& v);
	bool drawOpaquePass(vec4 probePos, mat4 p, const std::vector<mat4>& v, unsigned int faceIndex);
	bool generateSurfel(unsigned int probeIndex);
	bool generateBrick();
	void findSurfelRangeForBrick(Brick& brick);
	bool injectIrradiance(vec4 probePos, mat4 p, const std::vector<mat4>& v);
	bool drawSkyPass(mat4 p, const std::vector<mat4>& v, unsigned int faceIndex);
	bool drawLightPass(mat4 p, const std::vector<mat4>& v, unsigned int faceIndex);
	bool getSkyShadowMask(vec4 probePos, mat4 p, const std::vector<mat4>& v);
	bool drawSkyVisibilityPass(mat4 p, const std::vector<mat4>& v, unsigned int faceIndex);

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
	std::pair<std::vector<vec4>, std::vector<SH9>> m_radianceSH9s;
	std::pair<std::vector<vec4>, std::vector<SH9>> m_skyVisibilitySH9s;
}

bool GLEnvironmentCapturePass::initialize()
{
	m_entityID = InnoMath::createEntityID();

	m_radianceSH9s = std::make_pair(std::vector<vec4>(m_totalCaptureProbes), std::vector<SH9>(m_totalCaptureProbes));
	m_skyVisibilitySH9s = std::make_pair(std::vector<vec4>(m_totalCaptureProbes), std::vector<SH9>(m_totalCaptureProbes));

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

bool GLEnvironmentCapturePass::drawOpaquePass(vec4 probePos, mat4 p, const std::vector<mat4>& v, unsigned int faceIndex)
{
	bindRenderPass(m_opaquePassGLRPC);
	cleanRenderBuffers(m_opaquePassGLRPC);

	CameraGPUData l_cameraGPUData;
	l_cameraGPUData.p_original = p;
	l_cameraGPUData.p_jittered = p;
	l_cameraGPUData.globalPos = probePos;
	auto l_t = InnoMath::getInvertTranslationMatrix(probePos);
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

	std::vector<Surfel> l_surfelSet(l_surfels.begin(), l_surfels.end());
	m_surfels.assign(l_surfelSet.begin(), l_surfelSet.end());

	//auto l_filePath = g_pModuleManager->getFileSystem()->getWorkingDirectory();

	//std::ofstream l_file;
	//l_file.open(l_filePath + "//Res//Scenes//GITest_" + std::to_string(probeIndex) + ".InnoSurfel", std::ios::binary);
	//IOService::serializeVector(l_file, l_surfels);

	return true;
}

void GLEnvironmentCapturePass::findSurfelRangeForBrick(Brick& brick)
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
}

bool GLEnvironmentCapturePass::generateBrick()
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
				findSurfelRangeForBrick(l_brick);

				// Eliminate empty brick
				if (l_brick.surfelRangeEnd - l_brick.surfelRangeBegin != 0)
				{
					m_bricks.emplace_back(l_brick);
				}

				l_currentPos.z += l_brickSize;
				l_brickIndex++;
			}
			l_currentPos.y += l_brickSize;
		}
		l_currentPos.x += l_brickSize;
	}

	return true;
}

bool GLEnvironmentCapturePass::gatherGeometryData(vec4 probePos, mat4 p, const std::vector<mat4>& v)
{
	for (unsigned int i = 0; i < 6; i++)
	{
		drawOpaquePass(probePos, p, v, i);
	}

	return true;
}

bool GLEnvironmentCapturePass::drawSkyPass(mat4 p, const std::vector<mat4>& v, unsigned int faceIndex)
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

bool GLEnvironmentCapturePass::drawLightPass(mat4 p, const std::vector<mat4>& v, unsigned int faceIndex)
{
	auto l_MDC = getGLMeshDataComponent(MeshShapeType::CUBE);

	vec4 probePos = vec4(0.0f, 0.0f, 0.0f, 1.0f);
	CameraGPUData l_cameraGPUData;
	l_cameraGPUData.p_original = p;
	l_cameraGPUData.p_jittered = p;
	l_cameraGPUData.globalPos = probePos;
	auto l_t = InnoMath::getInvertTranslationMatrix(probePos);
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

bool GLEnvironmentCapturePass::injectIrradiance(vec4 probePos, mat4 p, const std::vector<mat4>& v)
{
	bindRenderPass(m_capturePassGLRPC);
	cleanRenderBuffers(m_capturePassGLRPC);

	for (unsigned int i = 0; i < 6; i++)
	{
		auto l_renderingConfig = g_pModuleManager->getRenderingFrontend()->getRenderingConfig();

		if (l_renderingConfig.drawSky)
		{
			drawSkyPass(p, v, i);
		}

		drawLightPass(p, v, i);
	}

	return true;
}

bool GLEnvironmentCapturePass::drawSkyVisibilityPass(mat4 p, const std::vector<mat4>& v, unsigned int faceIndex)
{
	bindRenderPass(m_skyVisibilityPassGLRPC);
	cleanRenderBuffers(m_skyVisibilityPassGLRPC);

	auto l_MDC = getGLMeshDataComponent(MeshShapeType::CUBE);

	vec4 probePos = vec4(0.0f, 0.0f, 0.0f, 1.0f);
	CameraGPUData l_cameraGPUData;
	l_cameraGPUData.p_original = p;
	l_cameraGPUData.p_jittered = p;
	l_cameraGPUData.globalPos = probePos;
	auto l_t = InnoMath::getInvertTranslationMatrix(probePos);
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

bool GLEnvironmentCapturePass::getSkyShadowMask(vec4 probePos, mat4 p, const std::vector<mat4>& v)
{
	for (unsigned int i = 0; i < 6; i++)
	{
		drawSkyVisibilityPass(p, v, i);
	}

	return true;
}

bool GLEnvironmentCapturePass::capture(vec4 probePos, unsigned int probeIndex)
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

	gatherGeometryData(probePos, l_p, l_v);
	injectIrradiance(probePos, l_p, l_v);
	getSkyShadowMask(probePos, l_p, l_v);

	return true;
}

bool GLEnvironmentCapturePass::update()
{
	m_radianceSH9s.first.clear();
	m_radianceSH9s.second.clear();
	m_skyVisibilitySH9s.first.clear();
	m_skyVisibilitySH9s.second.clear();
	m_surfels.clear();
	m_bricks.clear();
	m_brickFactors.clear();

	updateUBO(GLRenderingBackendComponent::get().m_meshUBO, g_pModuleManager->getRenderingFrontend()->getGIPassMeshGPUData());
	updateUBO(GLRenderingBackendComponent::get().m_materialUBO, g_pModuleManager->getRenderingFrontend()->getGIPassMaterialGPUData());

	auto l_sceneAABB = g_pModuleManager->getPhysicsSystem()->getTotalSceneAABB();

	auto l_sceneCenter = l_sceneAABB.m_center;
	auto l_extendedAxisSize = l_sceneAABB.m_extend;
	l_extendedAxisSize = l_extendedAxisSize - vec4(2.0f, 2.0f, 2.0f, 0.0f);
	l_extendedAxisSize.w = 0.0f;
	auto l_probeDistance = l_extendedAxisSize / (float)m_subDivideDimension;
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
				capture(l_currentPos, l_probeIndex);
				generateSurfel(l_probeIndex);

				auto l_SH9 = GLSHPass::getSH9(m_capturePassGLRPC->m_GLTDCs[0]);
				m_radianceSH9s.first.emplace_back(l_currentPos);
				m_radianceSH9s.second.emplace_back(l_SH9);

				l_SH9 = GLSHPass::getSH9(m_skyVisibilityPassGLRPC->m_GLTDCs[0]);
				m_skyVisibilitySH9s.first.emplace_back(l_currentPos);
				m_skyVisibilitySH9s.second.emplace_back(l_SH9);

				l_currentPos.z += l_probeDistance.z;
				l_probeIndex++;
			}
			l_currentPos.y += l_probeDistance.y;
		}
		l_currentPos.x += l_probeDistance.x;
	}

	generateBrick();

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

const std::pair<std::vector<vec4>, std::vector<SH9>>& GLEnvironmentCapturePass::getRadianceSH9()
{
	return m_radianceSH9s;
}

const std::pair<std::vector<vec4>, std::vector<SH9>>& GLEnvironmentCapturePass::getSkyVisibilitySH9()
{
	return m_skyVisibilitySH9s;
}

const std::vector<Brick>& GLEnvironmentCapturePass::getBricks()
{
	return m_bricks;
}