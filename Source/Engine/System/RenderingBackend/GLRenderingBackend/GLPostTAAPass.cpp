#include "GLPostTAAPass.h"

#include "GLTAAPass.h"

#include "GLRenderingBackendUtilities.h"
#include "../../../Component/GLRenderingBackendComponent.h"

using namespace GLRenderingBackendNS;

#include "../../ICoreSystem.h"

extern ICoreSystem* g_pCoreSystem;

INNO_PRIVATE_SCOPE GLPostTAAPass
{
	void initializeShaders();

	EntityID m_entityID;

	GLRenderPassComponent* m_GLRPC;
	GLShaderProgramComponent* m_GLSPC;
	ShaderFilePaths m_shaderFilePaths = { "GL//postTAAPass.vert/", "", "", "", "GL//postTAAPass.frag/" };
}

bool GLPostTAAPass::initialize()
{
	m_entityID = InnoMath::createEntityID();

	m_GLRPC = addGLRenderPassComponent(m_entityID, "PostTAAPassGLRPC/");
	m_GLRPC->m_renderPassDesc = GLRenderingBackendComponent::get().m_deferredRenderPassDesc;
	m_GLRPC->m_renderPassDesc.useDepthAttachment = true;
	m_GLRPC->m_renderPassDesc.useStencilAttachment = true;
	initializeGLRenderPassComponent(m_GLRPC);

	initializeShaders();

	return true;
}

void GLPostTAAPass::initializeShaders()
{
	// shader programs and shaders
	auto rhs = addGLShaderProgramComponent(m_entityID);

	initializeGLShaderProgramComponent(rhs, m_shaderFilePaths);

	m_GLSPC = rhs;
}

bool GLPostTAAPass::update()
{
	activateRenderPass(m_GLRPC);

	activateShaderProgram(m_GLSPC);

	activateTexture(
		GLTAAPass::getGLRPC()->m_GLTDCs[0],
		0);

	auto l_MDC = getGLMeshDataComponent(MeshShapeType::QUAD);
	drawMesh(l_MDC);

	return true;
}

bool GLPostTAAPass::resize(unsigned int newSizeX, unsigned int newSizeY)
{
	resizeGLRenderPassComponent(m_GLRPC, newSizeX, newSizeY);

	return true;
}

bool GLPostTAAPass::reloadShader()
{
	deleteShaderProgram(m_GLSPC);

	initializeGLShaderProgramComponent(m_GLSPC, m_shaderFilePaths);

	return true;
}

GLRenderPassComponent * GLPostTAAPass::getGLRPC()
{
	return m_GLRPC;
}