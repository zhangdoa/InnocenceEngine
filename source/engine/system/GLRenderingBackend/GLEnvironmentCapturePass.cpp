#include "GLEnvironmentCapturePass.h"
#include "GLRenderingSystemUtilities.h"
#include "../../component/GLRenderingSystemComponent.h"
#include "../../component/RenderingFrontendSystemComponent.h"

#include "../ICoreSystem.h"

extern ICoreSystem* g_pCoreSystem;

using namespace GLRenderingSystemNS;

INNO_PRIVATE_SCOPE GLEnvironmentCapturePass
{
	bool render(vec4 pos);

	EntityID m_entityID;

	GLRenderPassComponent* m_GLRPC;

	GLShaderProgramComponent* m_GLSPC;

	GLFrameBufferDesc m_frameBufferDesc = GLFrameBufferDesc();
	TextureDataDesc m_textureDesc = TextureDataDesc();

	ShaderFilePaths m_shaderFilePaths = { "GL//environmentCapturePass.vert" , "", "GL//environmentCapturePass.frag" };

	const unsigned int m_subDivideDimension = 16;
}

bool GLEnvironmentCapturePass::initialize()
{
	m_entityID = InnoMath::createEntityID();

	m_textureDesc.samplerType = TextureSamplerType::CUBEMAP;
	m_textureDesc.usageType = TextureUsageType::COLOR_ATTACHMENT;
	m_textureDesc.pixelDataFormat = TexturePixelDataFormat::RGB;
	m_textureDesc.minFilterMethod = TextureFilterMethod::LINEAR;
	m_textureDesc.magFilterMethod = TextureFilterMethod::LINEAR;
	m_textureDesc.wrapMethod = TextureWrapMethod::REPEAT;
	m_textureDesc.width = 2048;
	m_textureDesc.height = 2048;
	m_textureDesc.pixelDataType = TexturePixelDataType::FLOAT16;

	m_frameBufferDesc.renderBufferAttachmentType = GL_DEPTH_ATTACHMENT;
	m_frameBufferDesc.renderBufferInternalFormat = GL_DEPTH_COMPONENT24;
	m_frameBufferDesc.sizeX = m_textureDesc.width;
	m_frameBufferDesc.sizeY = m_textureDesc.height;
	m_frameBufferDesc.drawColorBuffers = true;

	m_GLRPC = addGLRenderPassComponent(1, m_frameBufferDesc, m_textureDesc);

	auto rhs = addGLShaderProgramComponent(m_entityID);
	initializeGLShaderProgramComponent(rhs, m_shaderFilePaths);

	m_GLSPC = rhs;

	return true;
}

bool GLEnvironmentCapturePass::render(vec4 pos)
{
	auto l_capturePos = vec4(0.0f, 0.0f, 0.0f, 1.0f);
	mat4 l_p = InnoMath::generatePerspectiveMatrix((90.0f / 180.0f) * PI<float>, 1.0f, 0.1f, 10.0f);
	std::vector<mat4> l_v =
	{
		InnoMath::lookAt(l_capturePos, l_capturePos + vec4(1.0f,  0.0f,  0.0f, 0.0f), vec4(0.0f, -1.0f,  0.0f, 0.0f)),
		InnoMath::lookAt(l_capturePos, l_capturePos + vec4(-1.0f,  0.0f,  0.0f, 0.0f), vec4(0.0f, -1.0f,  0.0f, 0.0f)),
		InnoMath::lookAt(l_capturePos, l_capturePos + vec4(0.0f,  1.0f,  0.0f, 0.0f), vec4(0.0f,  0.0f,  1.0f, 0.0f)),
		InnoMath::lookAt(l_capturePos, l_capturePos + vec4(0.0f, -1.0f,  0.0f, 0.0f), vec4(0.0f,  0.0f, -1.0f, 0.0f)),
		InnoMath::lookAt(l_capturePos, l_capturePos + vec4(0.0f,  0.0f,  1.0f, 0.0f), vec4(0.0f, -1.0f,  0.0f, 0.0f)),
		InnoMath::lookAt(l_capturePos, l_capturePos + vec4(0.0f,  0.0f, -1.0f, 0.0f), vec4(0.0f, -1.0f,  0.0f, 0.0f))
	};

	auto l_renderingConfig = g_pCoreSystem->getVisionSystem()->getRenderingFrontend()->getRenderingConfig();

	auto l_MDC = getGLMeshDataComponent(MeshShapeType::CUBE);

	activateRenderPass(m_GLRPC);

	auto l_GLTDC = m_GLRPC->m_GLTDCs[0];

	// draw sky
	// @TODO: optimize
	if (l_renderingConfig.drawSky)
	{
		//glEnable(GL_DEPTH_TEST);
		//glDepthFunc(GL_LEQUAL);

		//activateShaderProgram(GLFinalRenderPassComponent::get().m_skyPassGLSPC);

		//updateUniform(
		//	GLFinalRenderPassComponent::get().m_skyPass_uni_p,
		//	l_p);

		//updateUniform(
		//	GLFinalRenderPassComponent::get().m_skyPass_uni_viewportSize,
		//	2048.0f, 2048.0f);

		//updateUniform(
		//	GLFinalRenderPassComponent::get().m_skyPass_uni_eyePos,
		//	l_cameraDataPack.globalPos.x, l_cameraDataPack.globalPos.y, l_cameraDataPack.globalPos.z);
		//updateUniform(
		//	GLFinalRenderPassComponent::get().m_skyPass_uni_lightDir,
		//	l_sunDataPack.dir.x, l_sunDataPack.dir.y, l_sunDataPack.dir.z);

		//for (unsigned int i = 0; i < 6; ++i)
		//{
		//	updateUniform(GLFinalRenderPassComponent::get().m_skyPass_uni_r, l_v[i]);
		//	attachCubemapColorRT(l_GLTDC, m_GLRPC, 0, i, 0);
		//	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
		//	drawMesh(l_MDC);
		//}
		//glDisable(GL_CULL_FACE);
		//glDisable(GL_DEPTH_TEST);
	}

	// draw opaque meshes
	activateShaderProgram(m_GLSPC);

	// uni_p
	updateUniform(0, l_p);

	for (unsigned int i = 0; i < 6; ++i)
	{
		// uni_v
		updateUniform(1, l_v[i]);
		attachCubemapColorRT(l_GLTDC, m_GLRPC, 0, i, 0);

		auto l_copy = RenderingFrontendSystemComponent::get().m_GIPassGPUDataQueue.getRawData();

		while (l_copy.size() > 0)
		{
			GeometryPassGPUData l_geometryPassGPUData = l_copy.front();

			if (l_geometryPassGPUData.materialGPUData.useAlbedoTexture)
			{
				activateTexture(reinterpret_cast<GLTextureDataComponent*>(l_geometryPassGPUData.albedoTDC), 1);
			}

			// uni_m
			updateUniform(2, l_geometryPassGPUData.meshGPUData.m);
			// uni_useAlbedoTexture
			updateUniform(4, l_geometryPassGPUData.materialGPUData.useAlbedoTexture);

			vec4 l_albedo = vec4(
				l_geometryPassGPUData.materialGPUData.customMaterial.albedo_r,
				l_geometryPassGPUData.materialGPUData.customMaterial.albedo_g,
				l_geometryPassGPUData.materialGPUData.customMaterial.albedo_b,
				1.0f
			);

			// uni_albedo
			updateUniform(5, l_albedo);

			drawMesh(reinterpret_cast<GLMeshDataComponent*>(l_geometryPassGPUData.MDC));

			l_copy.pop();
		}
	}

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

	RenderingFrontendSystemComponent::get().m_GIPassGPUDataQueue.clear();

	return true;
}

bool GLEnvironmentCapturePass::reloadShader()
{
	deleteShaderProgram(m_GLSPC);

	initializeGLShaderProgramComponent(m_GLSPC, m_shaderFilePaths);

	return true;
}

GLRenderPassComponent * GLEnvironmentCapturePass::getGLRPC()
{
	return m_GLRPC;
}