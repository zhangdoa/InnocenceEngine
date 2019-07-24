#include "GLBloomMergePass.h"
#include "GLBloomExtractPass.h"
#include "GLPreTAAPass.h"

#include "GLRenderingBackendUtilities.h"
#include "../../Component/GLRenderingBackendComponent.h"

using namespace GLRenderingBackendNS;

#include "../../ModuleManager/IModuleManager.h"

extern IModuleManager* g_pModuleManager;

namespace GLBloomMergePass
{
	void initializeShaders();

	EntityID m_entityID;

	GLRenderPassComponent* m_GLRPC;
	GLShaderProgramComponent* m_GLSPC;
	ShaderFilePaths m_shaderFilePaths = { "2DImageProcess.vert/", "", "", "", "bloomMergePass.frag/" };
}

bool GLBloomMergePass::initialize()
{
	m_entityID = InnoMath::createEntityID();

	auto l_renderPassDesc = GLRenderingBackendComponent::get().m_deferredRenderPassDesc;
	l_renderPassDesc.RTDesc.magFilterMethod = TextureFilterMethod::LINEAR;
	l_renderPassDesc.RTDesc.minFilterMethod = TextureFilterMethod::LINEAR_MIPMAP_LINEAR;

	m_GLRPC = addGLRenderPassComponent(m_entityID, "BloomMergePassGLRPC/");
	m_GLRPC->m_renderPassDesc = l_renderPassDesc;
	initializeGLRenderPassComponent(m_GLRPC);

	initializeShaders();

	return true;
}

void GLBloomMergePass::initializeShaders()
{
	// shader programs and shaders
	auto rhs = addGLShaderProgramComponent(m_entityID);

	initializeGLShaderProgramComponent(rhs, m_shaderFilePaths);

	m_GLSPC = rhs;
}

bool GLBloomMergePass::update()
{
	auto l_MDC = getGLMeshDataComponent(MeshShapeType::QUAD);

	bindRenderPass(m_GLRPC);
	cleanRenderBuffers(m_GLRPC);

	activateShaderProgram(m_GLSPC);

	glEnable(GL_BLEND);

	glBlendFuncSeparate(GL_SRC_ALPHA, GL_SRC_ALPHA, GL_ONE, GL_ZERO);

	for (unsigned int i = 0; i < 4; i++)
	{
		activateTexture(GLBloomExtractPass::getGLRPC(i)->m_GLTDCs[0], 0);
		drawMesh(l_MDC);
	}

	activateTexture(GLPreTAAPass::getGLRPC()->m_GLTDCs[0], 0);
	drawMesh(l_MDC);

	glDisable(GL_BLEND);

	return true;
}

bool GLBloomMergePass::resize(unsigned int newSizeX, unsigned int newSizeY)
{
	resizeGLRenderPassComponent(m_GLRPC, newSizeX, newSizeY);

	return true;
}

bool GLBloomMergePass::reloadShader()
{
	deleteShaderProgram(m_GLSPC);

	initializeGLShaderProgramComponent(m_GLSPC, m_shaderFilePaths);

	return true;
}

GLRenderPassComponent * GLBloomMergePass::getGLRPC()
{
	return m_GLRPC;
}