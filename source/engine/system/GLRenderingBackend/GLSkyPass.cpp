#include "GLSkyPass.h"
#include "GLRenderingSystemUtilities.h"
#include "../../component/GLRenderingSystemComponent.h"
#include "../../component/RenderingFrontendSystemComponent.h"

using namespace GLRenderingSystemNS;

#include "../ICoreSystem.h"

extern ICoreSystem* g_pCoreSystem;

INNO_PRIVATE_SCOPE GLSkyPass
{
	void initializeShaders();

	EntityID m_entityID;

	GLRenderPassComponent* m_GLRPC;
	GLShaderProgramComponent* m_GLSPC;
	ShaderFilePaths m_shaderFilePaths = { "GL//skyPass.vert/", "", "", "", "GL//skyPass.frag/" };
}

bool GLSkyPass::initialize()
{
	m_entityID = InnoMath::createEntityID();

	m_GLRPC = addGLRenderPassComponent(m_entityID, "SkyGLRPC/");
	m_GLRPC->m_renderPassDesc = GLRenderingSystemComponent::get().m_deferredRenderPassDesc;
	initializeGLRenderPassComponent(m_GLRPC);

	initializeShaders();

	return true;
}

void GLSkyPass::initializeShaders()
{
	// shader programs and shaders
	auto rhs = addGLShaderProgramComponent(m_entityID);

	initializeGLShaderProgramComponent(rhs, m_shaderFilePaths);

	m_GLSPC = rhs;
}

bool GLSkyPass::update()
{
	auto l_renderingConfig = g_pCoreSystem->getRenderingFrontendSystem()->getRenderingConfig();

	if (l_renderingConfig.drawSky)
	{
		glEnable(GL_DEPTH_TEST);
		glDepthFunc(GL_LEQUAL);
		glDepthMask(GL_TRUE);

		glEnable(GL_CULL_FACE);
		glFrontFace(GL_CCW);
		glCullFace(GL_BACK);

		activateRenderPass(m_GLRPC);

		activateShaderProgram(m_GLSPC);

		auto l_MDC = getGLMeshDataComponent(MeshShapeType::CUBE);
		drawMesh(l_MDC);

		glDisable(GL_CULL_FACE);
		glDisable(GL_DEPTH_TEST);
	}
	else
	{
		cleanRenderBuffers(m_GLRPC);
	}

	return true;
}

bool GLSkyPass::resize(unsigned int newSizeX, unsigned int newSizeY)
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