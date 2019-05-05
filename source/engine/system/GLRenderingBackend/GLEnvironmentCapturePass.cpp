#include "GLEnvironmentCapturePass.h"
#include "GLRenderingSystemUtilities.h"

#include "GLSkyPass.h"

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
	auto l_capturePos = vec4(0.0f, 2.0f, 0.0f, 1.0f);
	auto l_p = InnoMath::generatePerspectiveMatrix((90.0f / 180.0f) * PI<float>, 1.0f, 0.1f, 10.0f);

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

	auto l_MDC = getGLMeshDataComponent(MeshShapeType::CUBE);

	activateRenderPass(m_GLRPC);

	auto l_GLTDC = m_GLRPC->m_GLTDCs[0];

	// draw sky
	// @TODO: optimize
	if (l_renderingConfig.drawSky)
	{
		activateShaderProgram(GLSkyPass::getGLSPC());

		SkyGPUData l_skyGPUData;
		l_skyGPUData.p_inv = l_p.inverse();
		l_skyGPUData.viewportSize = vec2(2048.0f, 2048.0f);

		CameraGPUData l_cameraGPUData = RenderingFrontendSystemComponent::get().m_cameraGPUData;
		l_cameraGPUData.p_original = l_p;
		l_cameraGPUData.globalPos = l_capturePos;

		for (unsigned int i = 0; i < 6; ++i)
		{
			l_cameraGPUData.r = l_v[i];
			l_skyGPUData.r_inv = l_v[i].inverse();

			updateUBO(GLRenderingSystemComponent::get().m_skyUBO, l_skyGPUData);
			updateUBO(GLRenderingSystemComponent::get().m_cameraUBO, l_cameraGPUData);

			attachCubemapColorRT(l_GLTDC, m_GLRPC, 0, i, 0);
			glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
			drawMesh(l_MDC);
		}
	}

	// draw opaque meshes
	activateShaderProgram(m_GLSPC);

	auto l_t = InnoMath::getInvertTranslationMatrix(l_capturePos);

	// uni_p
	updateUniform(0, l_p);

	for (unsigned int i = 0; i < 6; ++i)
	{
		// uni_v
		updateUniform(1, l_v[i] * l_t);
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