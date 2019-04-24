#include "GLRenderingSystemUtilities.h"
#include "GLShadowRenderPass.h"
#include "../../component/GLRenderingSystemComponent.h"
#include "../../component/RenderingFrontendSystemComponent.h"

#include "../ICoreSystem.h"

extern ICoreSystem* g_pCoreSystem;

using namespace GLRenderingSystemNS;

INNO_PRIVATE_SCOPE GLShadowRenderPass
{
	void drawAllMeshDataComponents();

	GLFrameBufferDesc DirLightShadowPassFBDesc = GLFrameBufferDesc();
	TextureDataDesc DirLightShadowPassTextureDesc = TextureDataDesc();

	GLFrameBufferDesc PointLightShadowPassFBDesc = GLFrameBufferDesc();
	TextureDataDesc PointLightShadowPassTextureDesc = TextureDataDesc();

	EntityID m_entityID;

	GLRenderPassComponent* m_DirLight_GLRPC;
	GLRenderPassComponent* m_PointLight_GLRPC;

	GLShaderProgramComponent* m_GLSPC;

	ShaderFilePaths m_shaderFilePaths = { "GL//shadowPass.vert" , "", "GL//shadowPass.frag" };
}

void GLShadowRenderPass::initialize()
{
	m_entityID = InnoMath::createEntityID();

	DirLightShadowPassFBDesc.renderBufferAttachmentType = GL_DEPTH_ATTACHMENT;
	DirLightShadowPassFBDesc.renderBufferInternalFormat = GL_DEPTH_COMPONENT32;
	DirLightShadowPassFBDesc.sizeX = 4096;
	DirLightShadowPassFBDesc.sizeY = 4096;
	DirLightShadowPassFBDesc.drawColorBuffers = false;

	DirLightShadowPassTextureDesc.samplerType = TextureSamplerType::SAMPLER_2D;
	DirLightShadowPassTextureDesc.usageType = TextureUsageType::DEPTH_ATTACHMENT;
	DirLightShadowPassTextureDesc.pixelDataFormat = TexturePixelDataFormat::DEPTH_COMPONENT;
	DirLightShadowPassTextureDesc.minFilterMethod = TextureFilterMethod::NEAREST;
	DirLightShadowPassTextureDesc.magFilterMethod = TextureFilterMethod::NEAREST;
	DirLightShadowPassTextureDesc.wrapMethod = TextureWrapMethod::CLAMP_TO_BORDER;
	DirLightShadowPassTextureDesc.width = DirLightShadowPassFBDesc.sizeX;
	DirLightShadowPassTextureDesc.height = DirLightShadowPassFBDesc.sizeY;
	DirLightShadowPassTextureDesc.pixelDataType = TexturePixelDataType::FLOAT16;
	DirLightShadowPassTextureDesc.borderColor[0] = 1.0f;
	DirLightShadowPassTextureDesc.borderColor[1] = 1.0f;
	DirLightShadowPassTextureDesc.borderColor[2] = 1.0f;
	DirLightShadowPassTextureDesc.borderColor[3] = 1.0f;

	m_DirLight_GLRPC = addGLRenderPassComponent(1, DirLightShadowPassFBDesc, DirLightShadowPassTextureDesc);

	PointLightShadowPassFBDesc.renderBufferAttachmentType = GL_DEPTH_ATTACHMENT;
	PointLightShadowPassFBDesc.renderBufferInternalFormat = GL_DEPTH_COMPONENT32;
	PointLightShadowPassFBDesc.sizeX = 4096;
	PointLightShadowPassFBDesc.sizeY = 4096;
	PointLightShadowPassFBDesc.drawColorBuffers = false;

	PointLightShadowPassTextureDesc.samplerType = TextureSamplerType::CUBEMAP;
	PointLightShadowPassTextureDesc.usageType = TextureUsageType::DEPTH_ATTACHMENT;
	PointLightShadowPassTextureDesc.pixelDataFormat = TexturePixelDataFormat::DEPTH_COMPONENT;
	PointLightShadowPassTextureDesc.minFilterMethod = TextureFilterMethod::NEAREST;
	PointLightShadowPassTextureDesc.magFilterMethod = TextureFilterMethod::NEAREST;
	PointLightShadowPassTextureDesc.wrapMethod = TextureWrapMethod::CLAMP_TO_BORDER;
	PointLightShadowPassTextureDesc.width = PointLightShadowPassFBDesc.sizeX;
	PointLightShadowPassTextureDesc.height = PointLightShadowPassFBDesc.sizeY;
	PointLightShadowPassTextureDesc.pixelDataType = TexturePixelDataType::FLOAT16;
	PointLightShadowPassTextureDesc.borderColor[0] = 1.0f;
	PointLightShadowPassTextureDesc.borderColor[1] = 1.0f;
	PointLightShadowPassTextureDesc.borderColor[2] = 1.0f;
	PointLightShadowPassTextureDesc.borderColor[3] = 1.0f;

	m_PointLight_GLRPC = addGLRenderPassComponent(1, PointLightShadowPassFBDesc, PointLightShadowPassTextureDesc);

	auto rhs = addGLShaderProgramComponent(m_entityID);
	initializeGLShaderProgramComponent(rhs, m_shaderFilePaths);

	m_GLSPC = rhs;
}

void GLShadowRenderPass::drawAllMeshDataComponents()
{
	auto l_queueCopy = RenderingFrontendSystemComponent::get().m_opaquePassGPUDataQueue.getRawData();

	while (l_queueCopy.size() > 0)
	{
		auto l_geometryPassGPUData = l_queueCopy.front();
		if (l_geometryPassGPUData.MDC->m_meshShapeType != MeshShapeType::CUSTOM)
		{
			glFrontFace(GL_CW);
		}
		else
		{
			glFrontFace(GL_CCW);
		}
		//uni_m
		updateUniform(
			2, l_geometryPassGPUData.meshGPUData.m);

		drawMesh(reinterpret_cast<GLMeshDataComponent*>(l_geometryPassGPUData.MDC));

		l_queueCopy.pop();
	}
}

void GLShadowRenderPass::update()
{
	// copy CSM data pack for local scope

	glEnable(GL_DEPTH_TEST);
	glDepthFunc(GL_LESS);

	glEnable(GL_CULL_FACE);
	glFrontFace(GL_CCW);
	glCullFace(GL_FRONT);

	activateShaderProgram(m_GLSPC);

	activateRenderPass(m_DirLight_GLRPC);

	auto sizeX = m_DirLight_GLRPC->m_GLFrameBufferDesc.sizeX;
	auto sizeY = m_DirLight_GLRPC->m_GLFrameBufferDesc.sizeY;

	unsigned int splitCount = 0;

	for (unsigned int i = 0; i < 2; i++)
	{
		for (unsigned int j = 0; j < 2; j++)
		{
			glViewport(i * sizeX / 2, j * sizeY / 2, sizeX / 2, sizeY / 2);
			// uni_p
			updateUniform(
				0,
				RenderingFrontendSystemComponent::get().m_CSMGPUDataVector[splitCount].p);
			//uni_v
			updateUniform(
				1,
				RenderingFrontendSystemComponent::get().m_CSMGPUDataVector[splitCount].v);

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

	//glDisable(GL_CULL_FACE);
	//glDisable(GL_DEPTH_TEST);
}

GLRenderPassComponent * GLShadowRenderPass::getGLRPC(unsigned int index)
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