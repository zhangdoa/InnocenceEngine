#include "GLBillboardPass.h"

#include "GLOpaquePass.h"

#include "GLRenderingSystemUtilities.h"
#include "../../component/GLRenderingSystemComponent.h"
#include "../../component/RenderingFrontendSystemComponent.h"

using namespace GLRenderingSystemNS;

#include "../ICoreSystem.h"

extern ICoreSystem* g_pCoreSystem;

INNO_PRIVATE_SCOPE GLBillboardPass
{
	void initializeShaders();

	EntityID m_entityID;

	GLRenderPassComponent* m_GLRPC;
	GLShaderProgramComponent* m_GLSPC;
	ShaderFilePaths m_shaderFilePaths = { "GL//billboardPass.vert", "", "GL//billboardPass.frag" };
}

bool GLBillboardPass::initialize()
{
	m_entityID = InnoMath::createEntityID();

	m_GLRPC = addGLRenderPassComponent(1, GLRenderingSystemComponent::get().deferredPassFBDesc, GLRenderingSystemComponent::get().deferredPassTextureDesc);

	initializeShaders();

	return true;
}

void GLBillboardPass::initializeShaders()
{
	// shader programs and shaders
	auto rhs = addGLShaderProgramComponent(m_entityID);

	initializeGLShaderProgramComponent(rhs, m_shaderFilePaths);

	m_GLSPC = rhs;
}

bool GLBillboardPass::update()
{
	glEnable(GL_DEPTH_TEST);
	glDepthFunc(GL_LESS);

	activateRenderPass(m_GLRPC);

	// copy depth buffer from G-Pass
	copyDepthBuffer(GLOpaquePass::getGLRPC(), m_GLRPC);

	activateShaderProgram(m_GLSPC);

	updateUniform(
		0,
		RenderingFrontendSystemComponent::get().m_cameraGPUData.p_original);
	updateUniform(
		1,
		RenderingFrontendSystemComponent::get().m_cameraGPUData.r);
	updateUniform(
		2,
		RenderingFrontendSystemComponent::get().m_cameraGPUData.t);

	while (RenderingFrontendSystemComponent::get().m_billboardPassGPUDataQueue.size() > 0)
	{
		BillboardPassGPUData l_billboardPassGPUData;

		if (RenderingFrontendSystemComponent::get().m_billboardPassGPUDataQueue.tryPop(l_billboardPassGPUData))
		{
			auto l_GlobalPos = l_billboardPassGPUData.globalPos;

			updateUniform(
				3,
				l_GlobalPos);

			auto l_distanceToCamera = l_billboardPassGPUData.distanceToCamera;

			vec2 l_shearingRatio;
			if (l_distanceToCamera > 1.0f)
			{
				l_shearingRatio = vec2(1.0f / (l_distanceToCamera * RenderingFrontendSystemComponent::get().m_cameraGPUData.WHRatio), (1.0f / l_distanceToCamera));
			}
			else
			{
				l_shearingRatio = vec2(1.0f / RenderingFrontendSystemComponent::get().m_cameraGPUData.WHRatio, 1.0f);
			}

			updateUniform(
				4,
				l_shearingRatio);

			auto l_iconTexture = getGLTextureDataComponent(l_billboardPassGPUData.iconType);

			activateTexture(l_iconTexture, 0);

			auto l_MDC = getGLMeshDataComponent(MeshShapeType::QUAD);
			drawMesh(l_MDC);
		}
	}

	glDisable(GL_DEPTH_TEST);

	return true;
}

bool GLBillboardPass::resize(unsigned int newSizeX, unsigned int newSizeY)
{
	resizeGLRenderPassComponent(m_GLRPC, newSizeX, newSizeY);

	return true;
}

bool GLBillboardPass::reloadShader()
{
	deleteShaderProgram(m_GLSPC);

	initializeGLShaderProgramComponent(m_GLSPC, m_shaderFilePaths);

	return true;
}

GLRenderPassComponent * GLBillboardPass::getGLRPC()
{
	return m_GLRPC;
}