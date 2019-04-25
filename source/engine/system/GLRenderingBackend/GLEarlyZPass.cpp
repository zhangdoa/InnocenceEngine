#include "GLEarlyZPass.h"
#include "GLRenderingSystemUtilities.h"
#include "../../component/GLRenderingSystemComponent.h"
#include "../../component/RenderingFrontendSystemComponent.h"

using namespace GLRenderingSystemNS;

INNO_PRIVATE_SCOPE GLEarlyZPass
{
	void initializeShaders();

	EntityID m_entityID;

	GLRenderPassComponent* m_GLRPC;

	GLShaderProgramComponent* m_GLSPC;

	ShaderFilePaths m_shaderFilePaths = { "GL//earlyZPass.vert" , "", "GL//earlyZPass.frag" };
}

bool GLEarlyZPass::initialize()
{
	m_entityID = InnoMath::createEntityID();

	m_GLRPC = addGLRenderPassComponent(1, GLRenderingSystemComponent::get().deferredPassFBDesc, GLRenderingSystemComponent::get().deferredPassTextureDesc);

	initializeShaders();

	return true;
}

void GLEarlyZPass::initializeShaders()
{
	// shader programs and shaders
	auto rhs = addGLShaderProgramComponent(m_entityID);

	initializeGLShaderProgramComponent(rhs, m_shaderFilePaths);

	m_GLSPC = rhs;
}

bool GLEarlyZPass::update()
{
	glEnable(GL_DEPTH_TEST);
	glDepthFunc(GL_LESS);
	glDepthMask(GL_TRUE);

	glEnable(GL_DEPTH_CLAMP);

	glEnable(GL_CULL_FACE);
	glCullFace(GL_BACK);

	activateRenderPass(m_GLRPC);

	activateShaderProgram(m_GLSPC);

	auto l_queueCopy = RenderingFrontendSystemComponent::get().m_opaquePassGPUDataQueue.getRawData();

	while (l_queueCopy.size() > 0)
	{
		auto l_geometryPassGPUData = l_queueCopy.front();

		updateUBO(GLRenderingSystemComponent::get().m_meshUBO, l_geometryPassGPUData.meshGPUData);

		drawMesh(reinterpret_cast<GLMeshDataComponent*>(l_geometryPassGPUData.MDC));

		l_queueCopy.pop();
	}

	glDisable(GL_CULL_FACE);
	glDisable(GL_DEPTH_CLAMP);
	glDisable(GL_DEPTH_TEST);

	return true;
}

bool GLEarlyZPass::resize(unsigned int newSizeX, unsigned int newSizeY)
{
	resizeGLRenderPassComponent(m_GLRPC, newSizeX, newSizeY);

	return true;
}

bool GLEarlyZPass::reloadShader()
{
	deleteShaderProgram(m_GLSPC);

	initializeGLShaderProgramComponent(m_GLSPC, m_shaderFilePaths);

	return true;
}

GLRenderPassComponent * GLEarlyZPass::getGLRPC()
{
	return m_GLRPC;
}