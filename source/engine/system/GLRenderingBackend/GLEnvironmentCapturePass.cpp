#include "GLEnvironmentCapturePass.h"
#include "GLRenderingSystemUtilities.h"

#include "GLSkyPass.h"
#include "GLOpaquePass.h"

#include "../../component/GLRenderingSystemComponent.h"
#include "../../component/RenderingFrontendSystemComponent.h"

#include "../ICoreSystem.h"

extern ICoreSystem* g_pCoreSystem;

using namespace GLRenderingSystemNS;

INNO_PRIVATE_SCOPE GLEnvironmentCapturePass
{
	bool render(vec4 pos);
	bool drawOpaquePass(vec4 capturePos, mat4 p, const std::vector<mat4>& v);
	bool drawSkyPass(vec4 capturePos, mat4 p, const std::vector<mat4>& v);
	bool drawLightPass(vec4 capturePos, mat4 p, const std::vector<mat4>& v);

	EntityID m_entityID;

	GLRenderPassComponent* m_opaquePassGLRPC;
	GLRenderPassComponent* m_lightPassGLRPC;

	GLShaderProgramComponent* m_GLSPC;

	ShaderFilePaths m_shaderFilePaths = { "GL//environmentCapturePass.vert/" , "", "", "", "GL//environmentCapturePass.frag/" };

	const unsigned int m_subDivideDimension = 16;
}

bool GLEnvironmentCapturePass::initialize()
{
	m_entityID = InnoMath::createEntityID();

	auto l_renderPassDesc = GLRenderingSystemComponent::get().m_deferredRenderPassDesc;

	l_renderPassDesc.RTNumber = 4;
	l_renderPassDesc.RTDesc.samplerType = TextureSamplerType::CUBEMAP;
	l_renderPassDesc.RTDesc.usageType = TextureUsageType::COLOR_ATTACHMENT;
	l_renderPassDesc.RTDesc.pixelDataFormat = TexturePixelDataFormat::RGBA;
	l_renderPassDesc.RTDesc.minFilterMethod = TextureFilterMethod::LINEAR;
	l_renderPassDesc.RTDesc.magFilterMethod = TextureFilterMethod::LINEAR;
	l_renderPassDesc.RTDesc.wrapMethod = TextureWrapMethod::REPEAT;
	l_renderPassDesc.RTDesc.width = 2048;
	l_renderPassDesc.RTDesc.height = 2048;
	l_renderPassDesc.RTDesc.pixelDataType = TexturePixelDataType::FLOAT16;
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

	return true;
}

bool GLEnvironmentCapturePass::drawOpaquePass(vec4 capturePos, mat4 p, const std::vector<mat4>& v)
{
	CameraGPUData l_cameraGPUData = RenderingFrontendSystemComponent::get().m_cameraGPUData;
	l_cameraGPUData.p_original = p;
	l_cameraGPUData.globalPos = capturePos;
	auto l_t = InnoMath::getInvertTranslationMatrix(capturePos);

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

	for (unsigned int i = 0; i < 6; ++i)
	{
		l_cameraGPUData.r = v[i] * l_t;

		updateUBO(GLRenderingSystemComponent::get().m_cameraUBO, l_cameraGPUData);

		attachCubemapColorRT(m_opaquePassGLRPC->m_GLTDCs[0], m_opaquePassGLRPC, 0, i, 0);
		attachCubemapColorRT(m_opaquePassGLRPC->m_GLTDCs[1], m_opaquePassGLRPC, 1, i, 0);
		attachCubemapColorRT(m_opaquePassGLRPC->m_GLTDCs[2], m_opaquePassGLRPC, 2, i, 0);
		attachCubemapColorRT(m_opaquePassGLRPC->m_GLTDCs[3], m_opaquePassGLRPC, 3, i, 0);

		for (unsigned int i = 0; i < RenderingFrontendSystemComponent::get().m_GIPassDrawcallCount; i++)
		{
			auto l_opaquePassGPUData = RenderingFrontendSystemComponent::get().m_GIPassGPUDatas[i];
			auto l_meshGPUData = RenderingFrontendSystemComponent::get().m_GIPassMeshGPUDatas[i];
			auto l_materialGPUData = RenderingFrontendSystemComponent::get().m_GIPassMaterialGPUDatas[i];

			if (l_materialGPUData.useNormalTexture)
			{
				activateTexture(reinterpret_cast<GLTextureDataComponent*>(l_opaquePassGPUData.normalTDC), 0);
			}
			if (l_materialGPUData.useAlbedoTexture)
			{
				activateTexture(reinterpret_cast<GLTextureDataComponent*>(l_opaquePassGPUData.albedoTDC), 1);
			}
			if (l_materialGPUData.useMetallicTexture)
			{
				activateTexture(reinterpret_cast<GLTextureDataComponent*>(l_opaquePassGPUData.metallicTDC), 2);
			}
			if (l_materialGPUData.useRoughnessTexture)
			{
				activateTexture(reinterpret_cast<GLTextureDataComponent*>(l_opaquePassGPUData.roughnessTDC), 3);
			}
			if (l_materialGPUData.useAOTexture)
			{
				activateTexture(reinterpret_cast<GLTextureDataComponent*>(l_opaquePassGPUData.AOTDC), 4);
			}

			updateUBO(GLRenderingSystemComponent::get().m_meshUBO, l_meshGPUData);
			updateUBO(GLRenderingSystemComponent::get().m_materialUBO, l_materialGPUData);

			drawMesh(reinterpret_cast<GLMeshDataComponent*>(l_opaquePassGPUData.MDC));
		}
	}

	glDisable(GL_CULL_FACE);
	glDisable(GL_STENCIL_TEST);
	glDisable(GL_DEPTH_CLAMP);
	glDisable(GL_DEPTH_TEST);

	return true;
}

bool GLEnvironmentCapturePass::drawSkyPass(vec4 capturePos, mat4 p, const std::vector<mat4>& v)
{
	auto l_MDC = getGLMeshDataComponent(MeshShapeType::CUBE);

	CameraGPUData l_cameraGPUData = RenderingFrontendSystemComponent::get().m_cameraGPUData;
	l_cameraGPUData.p_original = p;
	l_cameraGPUData.globalPos = capturePos;

	SkyGPUData l_skyGPUData;
	l_skyGPUData.p_inv = p.inverse();
	l_skyGPUData.viewportSize = vec2(2048.0f, 2048.0f);

	activateShaderProgram(GLSkyPass::getGLSPC());

	for (unsigned int i = 0; i < 6; ++i)
	{
		l_cameraGPUData.r = v[i];
		l_skyGPUData.r_inv = v[i].inverse();

		updateUBO(GLRenderingSystemComponent::get().m_skyUBO, l_skyGPUData);
		updateUBO(GLRenderingSystemComponent::get().m_cameraUBO, l_cameraGPUData);

		attachCubemapColorRT(m_lightPassGLRPC->m_GLTDCs[0], m_lightPassGLRPC, 0, i, 0);
		drawMesh(l_MDC);
	}

	return true;
}

bool GLEnvironmentCapturePass::drawLightPass(vec4 capturePos, mat4 p, const std::vector<mat4>& v)
{
	auto l_MDC = getGLMeshDataComponent(MeshShapeType::CUBE);

	CameraGPUData l_cameraGPUData = RenderingFrontendSystemComponent::get().m_cameraGPUData;
	l_cameraGPUData.p_original = p;
	l_cameraGPUData.globalPos = capturePos;

	glEnable(GL_STENCIL_TEST);
	glStencilOp(GL_KEEP, GL_KEEP, GL_KEEP);
	glStencilFunc(GL_EQUAL, 0x01, 0xFF);
	glStencilMask(0x00);

	copyStencilBuffer(m_opaquePassGLRPC, m_lightPassGLRPC);

	activateShaderProgram(m_GLSPC);

	activateTexture(m_opaquePassGLRPC->m_GLTDCs[0], 0);
	activateTexture(m_opaquePassGLRPC->m_GLTDCs[1], 1);
	activateTexture(m_opaquePassGLRPC->m_GLTDCs[2], 2);

	for (unsigned int i = 0; i < 6; ++i)
	{
		l_cameraGPUData.r = v[i];

		updateUBO(GLRenderingSystemComponent::get().m_cameraUBO, l_cameraGPUData);

		attachCubemapColorRT(m_lightPassGLRPC->m_GLTDCs[0], m_lightPassGLRPC, 0, i, 0);

		drawMesh(l_MDC);
	}

	glDisable(GL_STENCIL_TEST);

	return true;
}

bool GLEnvironmentCapturePass::render(vec4 pos)
{
	auto l_capturePos = vec4(0.0f, 2.0f, 0.0f, 1.0f);
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

	auto l_renderingConfig = g_pCoreSystem->getVisionSystem()->getRenderingFrontend()->getRenderingConfig();

	activateRenderPass(m_opaquePassGLRPC);

	drawOpaquePass(l_capturePos, l_p, l_v);

	activateRenderPass(m_lightPassGLRPC);

	// @TODO: optimize
	if (l_renderingConfig.drawSky)
	{
		drawSkyPass(l_capturePos, l_p, l_v);
	}

	drawLightPass(l_capturePos, l_p, l_v);

	return true;
}

bool GLEnvironmentCapturePass::update()
{
	auto l_sceneAABB = g_pCoreSystem->getPhysicsSystem()->getSceneAABB();

	auto axisSize = l_sceneAABB.m_extend;
	auto l_voxelSize = axisSize / m_subDivideDimension;
	auto l_startPos = l_sceneAABB.m_boundMin;
	auto l_currentPos = l_startPos;

	RenderingFrontendSystemComponent::get().m_debuggerPassGPUDataQueue.clear();

	for (size_t i = 0; i < m_subDivideDimension; i++)
	{
		l_currentPos.y = l_startPos.y;
		for (size_t j = 0; j < m_subDivideDimension; j++)
		{
			l_currentPos.z = l_startPos.z;
			for (size_t k = 0; k < m_subDivideDimension; k++)
			{
				DebuggerPassGPUData l_debuggerPassGPUData = {};
				l_debuggerPassGPUData.m = InnoMath::toTranslationMatrix(l_currentPos);
				l_debuggerPassGPUData.MDC = getGLMeshDataComponent(MeshShapeType::SPHERE);

				RenderingFrontendSystemComponent::get().m_debuggerPassGPUDataQueue.push(l_debuggerPassGPUData);

				l_currentPos.z += l_voxelSize.z;
			}
			l_currentPos.y += l_voxelSize.y;
		}
		l_currentPos.x += l_voxelSize.x;
	}

	render(l_sceneAABB.m_center);

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