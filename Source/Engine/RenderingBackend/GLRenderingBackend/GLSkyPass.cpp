#include "GLSkyPass.h"
#include "GLRenderingBackendUtilities.h"
#include "../../Component/GLRenderingBackendComponent.h"

using namespace GLRenderingBackendNS;

#include "../../ModuleManager/IModuleManager.h"

extern IModuleManager* g_pModuleManager;

namespace GLSkyPass
{
	void initializeShaders();

	EntityID m_EntityID;

	GLRenderPassComponent* m_GLRPC;
	GLShaderProgramComponent* m_GLSPC;
	ShaderFilePaths m_shaderFilePaths = { "skyPass.vert/", "", "", "", "skyPass.frag/" };
}

bool GLSkyPass::initialize()
{
	m_EntityID = InnoMath::createEntityID();

	m_GLRPC = addGLRenderPassComponent(m_EntityID, "SkyGLRPC/");
	m_GLRPC->m_renderPassDesc = GLRenderingBackendComponent::get().m_deferredRenderPassDesc;
	initializeGLRenderPassComponent(m_GLRPC);

	initializeShaders();

	return true;
}

void GLSkyPass::initializeShaders()
{
	// shader programs and shaders
	auto rhs = addGLShaderProgramComponent(m_EntityID);

	initializeGLShaderProgramComponent(rhs, m_shaderFilePaths);

	m_GLSPC = rhs;
}

bool GLSkyPass::update()
{
	glEnable(GL_DEPTH_TEST);
	glDepthFunc(GL_LEQUAL);
	glDepthMask(GL_TRUE);

	glEnable(GL_CULL_FACE);
	glFrontFace(GL_CCW);
	glCullFace(GL_BACK);

	bindRenderPass(m_GLRPC);
	cleanRenderBuffers(m_GLRPC);

	activateShaderProgram(m_GLSPC);

	auto l_MDC = getGLMeshDataComponent(MeshShapeType::CUBE);
	drawMesh(l_MDC);

	glDisable(GL_CULL_FACE);
	glDisable(GL_DEPTH_TEST);

	return true;
}

bool GLSkyPass::resize(uint32_t newSizeX, uint32_t newSizeY)
{
	resizeGLRenderPassComponent(m_GLRPC, newSizeX, newSizeY);

	return true;
}

bool GLSkyPass::reloadShader()
{
	deleteShaderProgram(m_GLSPC);

	initializeGLShaderProgramComponent(m_GLSPC, m_shaderFilePaths);

	return true;
}

GLRenderPassComponent * GLSkyPass::getGLRPC()
{
	return m_GLRPC;
}

GLShaderProgramComponent * GLSkyPass::getGLSPC()
{
	return m_GLSPC;
}