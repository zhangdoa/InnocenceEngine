#include "GLRenderingBackendUtilities.h"
#include "GLShadowPass.h"
#include "../../../Component/GLRenderingBackendComponent.h"
#include "../../../Component/RenderingFrontendComponent.h"

#include "../../ICoreSystem.h"

extern ICoreSystem* g_pCoreSystem;

using namespace GLRenderingBackendNS;

INNO_PRIVATE_SCOPE GLShadowPass
{
	void drawAllMeshDataComponents();

	EntityID m_entityID;

	GLRenderPassComponent* m_DirLight_GLRPC;
	GLRenderPassComponent* m_PointLight_GLRPC;

	GLShaderProgramComponent* m_GLSPC;

	ShaderFilePaths m_shaderFilePaths = { "GL//shadowPass.vert/" , "", "", "", "GL//shadowPass.frag/" };
}

void GLShadowPass::initialize()
{
	m_entityID = InnoMath::createEntityID();

	auto l_renderPassDesc = GLRenderingBackendComponent::get().m_deferredRenderPassDesc;
	l_renderPassDesc.RTDesc.samplerType = TextureSamplerType::SAMPLER_2D;
	l_renderPassDesc.RTDesc.usageType = TextureUsageType::COLOR_ATTACHMENT;
	l_renderPassDesc.RTDesc.pixelDataFormat = TexturePixelDataFormat::RG;
	l_renderPassDesc.RTDesc.minFilterMethod = TextureFilterMethod::LINEAR;
	l_renderPassDesc.RTDesc.magFilterMethod = TextureFilterMethod::LINEAR;
	l_renderPassDesc.RTDesc.wrapMethod = TextureWrapMethod::CLAMP_TO_BORDER;
	l_renderPassDesc.RTDesc.width = 4096;
	l_renderPassDesc.RTDesc.height = 4096;
	l_renderPassDesc.RTDesc.pixelDataType = TexturePixelDataType::FLOAT32;
	l_renderPassDesc.RTDesc.borderColor[0] = 1.0f;
	l_renderPassDesc.RTDesc.borderColor[1] = 1.0f;
	l_renderPassDesc.RTDesc.borderColor[2] = 1.0f;
	l_renderPassDesc.RTDesc.borderColor[3] = 1.0f;

	m_DirLight_GLRPC = addGLRenderPassComponent(m_entityID, "DirectionalLightShadowPassGLRPC/");
	m_DirLight_GLRPC->m_renderPassDesc = l_renderPassDesc;
	m_DirLight_GLRPC->m_renderPassDesc.useDepthAttachment = true;
	m_DirLight_GLRPC->m_drawColorBuffers = true;
	initializeGLRenderPassComponent(m_DirLight_GLRPC);

	l_renderPassDesc.RTDesc.samplerType = TextureSamplerType::CUBEMAP;
	l_renderPassDesc.RTDesc.usageType = TextureUsageType::DEPTH_ATTACHMENT;
	l_renderPassDesc.RTDesc.pixelDataFormat = TexturePixelDataFormat::DEPTH_COMPONENT;
	l_renderPassDesc.RTDesc.minFilterMethod = TextureFilterMethod::LINEAR;
	l_renderPassDesc.RTDesc.magFilterMethod = TextureFilterMethod::LINEAR;
	l_renderPassDesc.RTDesc.wrapMethod = TextureWrapMethod::CLAMP_TO_BORDER;
	l_renderPassDesc.RTDesc.width = 4096;
	l_renderPassDesc.RTDesc.height = 4096;
	l_renderPassDesc.RTDesc.pixelDataType = TexturePixelDataType::FLOAT16;
	l_renderPassDesc.RTDesc.borderColor[0] = 1.0f;
	l_renderPassDesc.RTDesc.borderColor[1] = 1.0f;
	l_renderPassDesc.RTDesc.borderColor[2] = 1.0f;
	l_renderPassDesc.RTDesc.borderColor[3] = 1.0f;

	m_PointLight_GLRPC = addGLRenderPassComponent(m_entityID, "PointLightShadowPassGLRPC/");
	m_PointLight_GLRPC->m_renderPassDesc = l_renderPassDesc;
	m_PointLight_GLRPC->m_renderPassDesc.useDepthAttachment = true;
	m_PointLight_GLRPC->m_drawColorBuffers = false;
	initializeGLRenderPassComponent(m_PointLight_GLRPC);

	m_GLSPC = addGLShaderProgramComponent(m_entityID);
	initializeGLShaderProgramComponent(m_GLSPC, m_shaderFilePaths);
}

void GLShadowPass::drawAllMeshDataComponents()
{
	for (unsigned int i = 0; i < RenderingFrontendComponent::get().m_opaquePassDrawcallCount; i++)
	{
		auto l_opaquePassGPUData = RenderingFrontendComponent::get().m_opaquePassGPUDatas[i];
		auto l_meshGPUData = RenderingFrontendComponent::get().m_opaquePassMeshGPUDatas[i];

		//uni_m
		updateUniform(
			2, l_meshGPUData.m);

		drawMesh(reinterpret_cast<GLMeshDataComponent*>(l_opaquePassGPUData.MDC));
	}
}

void GLShadowPass::update()
{
	glEnable(GL_DEPTH_TEST);
	glDepthFunc(GL_LESS);
	glDepthMask(GL_TRUE);

	glEnable(GL_CULL_FACE);
	glCullFace(GL_FRONT);

	activateShaderProgram(m_GLSPC);

	activateRenderPass(m_DirLight_GLRPC);

	auto sizeX = m_DirLight_GLRPC->m_renderPassDesc.RTDesc.width;
	auto sizeY = m_DirLight_GLRPC->m_renderPassDesc.RTDesc.height;

	unsigned int splitCount = 0;

	for (unsigned int i = 0; i < 2; i++)
	{
		for (unsigned int j = 0; j < 2; j++)
		{
			glViewport(i * sizeX / 2, j * sizeY / 2, sizeX / 2, sizeY / 2);
			// uni_p
			updateUniform(
				0,
				RenderingFrontendComponent::get().m_CSMGPUDataVector[splitCount].p);
			//uni_v
			updateUniform(
				1,
				RenderingFrontendComponent::get().m_CSMGPUDataVector[splitCount].v);

			splitCount++;

			drawAllMeshDataComponents();
		}
	}

	//mat4 l_p = InnoMath::generatePerspectiveMatrix((90.0f / 180.0f) * PI<float>, 1.0f, 0.1f, 10.0f);
	//auto l_capturePos = vec4(0.0f, 0.0f, 0.0f, 1.0f);
	//std::vector<mat4> l_v =
	//{
	//	InnoMath::lookAt(l_capturePos, l_capturePos + vec4(1.0f,  0.0f,  0.0f, 0.0f), vec4(0.0f, -1.0f,  0.0f, 0.0f)),
	//	InnoMath::lookAt(l_capturePos, l_capturePos + vec4(-1.0f,  0.0f,  0.0f, 0.0f), vec4(0.0f, -1.0f,  0.0f, 0.0f)),
	//	InnoMath::lookAt(l_capturePos, l_capturePos + vec4(0.0f,  1.0f,  0.0f, 0.0f), vec4(0.0f,  0.0f,  1.0f, 0.0f)),
	//	InnoMath::lookAt(l_capturePos, l_capturePos + vec4(0.0f, -1.0f,  0.0f, 0.0f), vec4(0.0f,  0.0f, -1.0f, 0.0f)),
	//	InnoMath::lookAt(l_capturePos, l_capturePos + vec4(0.0f,  0.0f,  1.0f, 0.0f), vec4(0.0f, -1.0f,  0.0f, 0.0f)),
	//	InnoMath::lookAt(l_capturePos, l_capturePos + vec4(0.0f,  0.0f, -1.0f, 0.0f), vec4(0.0f, -1.0f,  0.0f, 0.0f))
	//};

	//l_GLFBC = m_PointLight_GLRPC->m_GLFBC;

	//bindFBC(l_GLFBC);

	//updateUniform(
	//	m_shadowPass_uni_p,
	//	l_p);

	//for (unsigned int i = 0; i < 6; ++i)
	//{
	//	updateUniform(m_shadowPass_uni_v, l_v[i]);
	//	attachCubemapDepthRT(m_PointLight_GLRPC->m_GLTDCs[0], l_GLFBC, i, 0);

	//	glClear(GL_DEPTH_BUFFER_BIT);

	//	drawAllMeshDataComponents();
	//}

	glDisable(GL_CULL_FACE);
	glDisable(GL_DEPTH_TEST);
}

bool GLShadowPass::reloadShader()
{
	deleteShaderProgram(m_GLSPC);

	initializeGLShaderProgramComponent(m_GLSPC, m_shaderFilePaths);

	return true;
}

GLRenderPassComponent * GLShadowPass::getGLRPC(unsigned int index)
{
	if (index == 0)
	{
		return m_DirLight_GLRPC;
	}
	else if (index == 1)
	{
		return m_PointLight_GLRPC;
	}
	else
	{
		return nullptr;
	}
}