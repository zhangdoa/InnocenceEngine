#include "GLEnvironmentCapturePass.h"
#include "GLRenderingBackendUtilities.h"

#include "GLSkyPass.h"
#include "GLOpaquePass.h"

#include "../../Component/GLRenderingBackendComponent.h"

#include "../../ModuleManager/IModuleManager.h"

extern IModuleManager* g_pModuleManager;

using namespace GLRenderingBackendNS;

INNO_PRIVATE_SCOPE GLEnvironmentCapturePass
{
	bool render(vec4 pos, GLTextureDataComponent* RT);
	bool drawOpaquePass(vec4 capturePos, mat4 p, const std::vector<mat4>& v, unsigned int faceIndex);
	bool drawSkyPass(mat4 p, const std::vector<mat4>& v, GLTextureDataComponent* RT, unsigned int faceIndex);
	bool drawLightPass(mat4 p, const std::vector<mat4>& v, GLTextureDataComponent* RT, unsigned int faceIndex);

	EntityID m_entityID;

	GLRenderPassComponent* m_opaquePassGLRPC;
	GLRenderPassComponent* m_lightPassGLRPC;

	GLShaderProgramComponent* m_GLSPC;

	ShaderFilePaths m_shaderFilePaths = { "environmentCapturePass.vert/" , "", "", "", "environmentCapturePass.frag/" };

	const unsigned int m_captureResolution = 256;
	const unsigned int m_sampleCountPerFace = m_captureResolution * m_captureResolution;
	const unsigned int m_subDivideDimension = 2;
	const unsigned int m_totalCubemaps = m_subDivideDimension * m_subDivideDimension * m_subDivideDimension;
	std::vector<GLTextureDataComponent*> m_capturedCubemaps;
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

	m_lightPassGLRPC = addGLRenderPassComponent(m_entityID, "EnvironmentCaptureLightPassGLRPC/");
	l_renderPassDesc.RTNumber = 1;
	m_lightPassGLRPC->m_renderPassDesc = l_renderPassDesc;
	m_lightPassGLRPC->m_drawColorBuffers = true;

	initializeGLRenderPassComponent(m_lightPassGLRPC);

	m_GLSPC = addGLShaderProgramComponent(m_entityID);
	initializeGLShaderProgramComponent(m_GLSPC, m_shaderFilePaths);

	m_capturedCubemaps.reserve(m_totalCubemaps);
	for (size_t i = 0; i < m_totalCubemaps; i++)
	{
		auto l_capturedPassCubemap = addGLTextureDataComponent();
		l_capturedPassCubemap->m_textureDataDesc = l_renderPassDesc.RTDesc;
		l_capturedPassCubemap->m_textureData = nullptr;
		initializeGLTextureDataComponent(l_capturedPassCubemap);

		m_capturedCubemaps.emplace_back(l_capturedPassCubemap);
	}

	return true;
}

bool GLEnvironmentCapturePass::drawOpaquePass(vec4 capturePos, mat4 p, const std::vector<mat4>& v, unsigned int faceIndex)
{
	CameraGPUData l_cameraGPUData;
	l_cameraGPUData.p_original = p;
	l_cameraGPUData.p_jittered = p;
	l_cameraGPUData.globalPos = capturePos;
	auto l_t = InnoMath::getInvertTranslationMatrix(capturePos);
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

bool GLEnvironmentCapturePass::drawSkyPass(mat4 p, const std::vector<mat4>& v, GLTextureDataComponent* RT, unsigned int faceIndex)
{
	auto l_MDC = getGLMeshDataComponent(MeshShapeType::CUBE);

	vec4 l_capturePos = vec4(0.0f, 0.0f, 0.0f, 1.0f);
	CameraGPUData l_cameraGPUData;
	l_cameraGPUData.p_original = p;
	l_cameraGPUData.p_jittered = p;
	l_cameraGPUData.globalPos = l_capturePos;
	auto l_t = InnoMath::getInvertTranslationMatrix(l_capturePos);
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

	bindCubemapTextureForWrite(RT, m_lightPassGLRPC, 0, faceIndex, 0);

	drawMesh(l_MDC);

	unbindCubemapTextureForWrite(m_lightPassGLRPC, 0, faceIndex, 0);

	return true;
}

bool GLEnvironmentCapturePass::drawLightPass(mat4 p, const std::vector<mat4>& v, GLTextureDataComponent* RT, unsigned int faceIndex)
{
	auto l_MDC = getGLMeshDataComponent(MeshShapeType::CUBE);

	vec4 capturePos = vec4(0.0f, 0.0f, 0.0f, 1.0f);
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

	copyStencilBuffer(m_opaquePassGLRPC, m_lightPassGLRPC);

	activateShaderProgram(m_GLSPC);

	activateTexture(m_opaquePassGLRPC->m_GLTDCs[0], 0);
	activateTexture(m_opaquePassGLRPC->m_GLTDCs[1], 1);
	activateTexture(m_opaquePassGLRPC->m_GLTDCs[2], 2);

	l_cameraGPUData.r = v[faceIndex];

	updateUBO(GLRenderingBackendComponent::get().m_cameraUBO, l_cameraGPUData);

	bindCubemapTextureForWrite(RT, m_lightPassGLRPC, 0, faceIndex, 0);

	drawMesh(l_MDC);

	unbindCubemapTextureForWrite(m_lightPassGLRPC, 0, faceIndex, 0);

	glDisable(GL_STENCIL_TEST);

	return true;
}

bool GLEnvironmentCapturePass::render(vec4 pos, GLTextureDataComponent* RT)
{
	auto l_capturePos = pos;
	auto l_p = InnoMath::generatePerspectiveMatrix((90.0f / 180.0f) * PI<float>, 1.0f, 0.1f, 1000.0f);

	auto l_rPX = InnoMath::toRotationMatrix(InnoMath::getQuatRotator(vec4(0.0f, 1.0f, 0.0f, 0.0f), -90.0f)).inverse();
	auto l_rNX = InnoMath::toRotationMatrix(InnoMath::getQuatRotator(vec4(0.0f, 1.0f, 0.0f, 0.0f), 90.0f)).inverse();
	auto l_rPY = InnoMath::toRotationMatrix(InnoMath::getQuatRotator(vec4(1.0f, 0.0f, 0.0f, 0.0f), -90.0f)).inverse();
	auto l_rNY = InnoMath::toRotationMatrix(InnoMath::getQuatRotator(vec4(1.0f, 0.0f, 0.0f, 0.0f), 90.0f)).inverse();
	auto l_rPZ = InnoMath::toRotationMatrix(InnoMath::getQuatRotator(vec4(0.0f, 1.0f, 0.0f, 0.0f), 0.0f)).inverse();
	auto l_rNZ = InnoMath::toRotationMatrix(InnoMath::getQuatRotator(vec4(0.0f, 1.0f, 0.0f, 0.0f), 180.0f)).inverse();

	std::vector<mat4> l_v =
	{
		l_rPX, l_rNX, l_rPY, l_rNY, l_rPZ, l_rNZ
	};

	auto l_renderingConfig = g_pModuleManager->getRenderingFrontend()->getRenderingConfig();

	for (unsigned int i = 0; i < 6; i++)
	{
		activateRenderPass(m_opaquePassGLRPC);

		drawOpaquePass(l_capturePos, l_p, l_v, i);

		activateRenderPass(m_lightPassGLRPC);

		// @TODO: optimize
		if (l_renderingConfig.drawSky)
		{
			drawSkyPass(l_p, l_v, RT, i);
		}

		drawLightPass(l_p, l_v, RT, i);
	}

	return true;
}

bool GLEnvironmentCapturePass::update()
{
	updateUBO(GLRenderingBackendComponent::get().m_meshUBO, g_pModuleManager->getRenderingFrontend()->getGIPassMeshGPUData());
	updateUBO(GLRenderingBackendComponent::get().m_materialUBO, g_pModuleManager->getRenderingFrontend()->getGIPassMaterialGPUData());

	auto l_sceneAABB = g_pModuleManager->getPhysicsSystem()->getTotalSceneAABB();

	auto l_sceneCenter = l_sceneAABB.m_center;
	auto l_extendedAxisSize = l_sceneAABB.m_extend + vec4(1.0f, 1.0f, 1.0f, 0.0f);
	l_extendedAxisSize.w = 0.0f;
	auto l_voxelSize = l_extendedAxisSize / (float)m_subDivideDimension;
	auto l_startPos = l_sceneAABB.m_center - (l_extendedAxisSize / 2.0f);
	auto l_currentPos = l_startPos;

	unsigned int l_index = 0;

	for (size_t i = 0; i < m_subDivideDimension; i++)
	{
		l_currentPos.y = l_startPos.y;
		for (size_t j = 0; j < m_subDivideDimension; j++)
		{
			l_currentPos.z = l_startPos.z;
			for (size_t k = 0; k < m_subDivideDimension; k++)
			{
				render(l_currentPos, m_capturedCubemaps[l_index]);
				l_index++;
				l_currentPos.z += l_voxelSize.z;
			}
			l_currentPos.y += l_voxelSize.y;
		}
		l_currentPos.x += l_voxelSize.x;
	}

	return true;
}

bool GLEnvironmentCapturePass::reloadShader()
{
	return true;
}

GLRenderPassComponent * GLEnvironmentCapturePass::getGLRPC()
{
	return m_lightPassGLRPC;
}

const std::vector<GLTextureDataComponent*>& GLEnvironmentCapturePass::getCapturedCubemaps()
{
	return m_capturedCubemaps;
}