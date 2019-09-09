#include "GLSSAOBlurPass.h"
#include "GLSSAONoisePass.h"

#include "GLRenderingBackendUtilities.h"
#include "../../Component/GLRenderingBackendComponent.h"

using namespace GLRenderingBackendNS;

#include "../../ModuleManager/IModuleManager.h"

extern IModuleManager* g_pModuleManager;

namespace GLSSAOBlurPass
{
	void initializeShaders();

	EntityID m_entityID;

	GLRenderPassComponent* m_GLRPC;

	GLShaderProgramComponent* m_GLSPC;

	ShaderFilePaths m_shaderFilePaths = { "2DImageProcess.vert/", "", "", "", "SSAOBlurPass.frag/" };
}

bool GLSSAOBlurPass::initialize()
{
	m_entityID = InnoMath::createEntityID();

	m_GLRPC = addGLRenderPassComponent(m_entityID, "SSAOBlurPassGLRPC/");
	m_GLRPC->m_renderPassDesc = GLRenderingBackendComponent::get().m_deferredRenderPassDesc;
	initializeGLRenderPassComponent(m_GLRPC);

	initializeShaders();

	return true;
}

void GLSSAOBlurPass::initializeShaders()
{
	// shader programs and shaders
	auto rhs = addGLShaderProgramComponent(m_entityID);

	initializeGLShaderProgramComponent(rhs, m_shaderFilePaths);

	m_GLSPC = rhs;
}

bool GLSSAOBlurPass::update()
{
	bindRenderPass(m_GLRPC);
	cleanRenderBuffers(m_GLRPC);

	activateShaderProgram(m_GLSPC);

	activateTexture(GLSSAONoisePass::getGLRPC()->m_GLTDCs[0], 0);

	auto l_MDC = getGLMeshDataComponent(MeshShapeType::QUAD);
	drawMesh(l_MDC);

	return true;
}

bool GLSSAOBlurPass::resize(uint32_t newSizeX, uint32_t newSizeY)
{
	resizeGLRenderPassComponent(m_GLRPC, newSizeX, newSizeY);

	return true;
}

bool GLSSAOBlurPass::reloadShader()
{
	deleteShaderProgram(m_GLSPC);

	initializeGLShaderProgramComponent(m_GLSPC, m_shaderFilePaths);

	return true;
}

GLRenderPassComponent * GLSSAOBlurPass::getGLRPC()
{
	return m_GLRPC;
}