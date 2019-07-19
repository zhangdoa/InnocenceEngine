#include "GLBillboardPass.h"

#include "GLOpaquePass.h"

#include "GLRenderingBackendUtilities.h"
#include "../../Component/GLRenderingBackendComponent.h"

using namespace GLRenderingBackendNS;

#include "../../ModuleManager/IModuleManager.h"

extern IModuleManager* g_pModuleManager;

INNO_PRIVATE_SCOPE GLBillboardPass
{
	void initializeShaders();

	EntityID m_entityID;

	GLRenderPassComponent* m_GLRPC;
	GLShaderProgramComponent* m_GLSPC;
	ShaderFilePaths m_shaderFilePaths = { "billboardPass.vert/", "", "", "", "billboardPass.frag/" };
}

bool GLBillboardPass::initialize()
{
	m_entityID = InnoMath::createEntityID();

	m_GLRPC = addGLRenderPassComponent(m_entityID, "BillboardPassGLRPC/");
	m_GLRPC->m_renderPassDesc = GLRenderingBackendComponent::get().m_deferredRenderPassDesc;
	m_GLRPC->m_renderPassDesc.useDepthAttachment = true;
	m_GLRPC->m_renderPassDesc.useStencilAttachment = true;
	initializeGLRenderPassComponent(m_GLRPC);

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

	bindRenderPass(m_GLRPC);
	cleanRenderBuffers(m_GLRPC);

	// copy depth buffer from G-Pass
	copyDepthBuffer(GLOpaquePass::getGLRPC(), m_GLRPC);

	activateShaderProgram(m_GLSPC);

	updateUniform(
		0,
		g_pModuleManager->getRenderingFrontend()->getCameraGPUData().p_original);
	updateUniform(
		1,
		g_pModuleManager->getRenderingFrontend()->getCameraGPUData().r);
	updateUniform(
		2,
		g_pModuleManager->getRenderingFrontend()->getCameraGPUData().t);

	for (unsigned int i = 0; i < g_pModuleManager->getRenderingFrontend()->getBillboardPassDrawCallCount(); i++)
	{
		auto l_billboardPassGPUData = g_pModuleManager->getRenderingFrontend()->getBillboardPassGPUData()[i];

		auto l_GlobalPos = l_billboardPassGPUData.globalPos;

		updateUniform(
			3,
			l_GlobalPos);

		auto l_distanceToCamera = l_billboardPassGPUData.distanceToCamera;

		vec2 l_shearingRatio;
		if (l_distanceToCamera > 1.0f)
		{
			l_shearingRatio = vec2(1.0f / (l_distanceToCamera * g_pModuleManager->getRenderingFrontend()->getCameraGPUData().WHRatio), (1.0f / l_distanceToCamera));
		}
		else
		{
			l_shearingRatio = vec2(1.0f / g_pModuleManager->getRenderingFrontend()->getCameraGPUData().WHRatio, 1.0f);
		}

		updateUniform(
			4,
			l_shearingRatio);

		auto l_iconTexture = getGLTextureDataComponent(l_billboardPassGPUData.iconType);

		activateTexture(l_iconTexture, 0);

		auto l_MDC = getGLMeshDataComponent(MeshShapeType::QUAD);
		drawMesh(l_MDC);
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