#include "GLSSAOBlurPass.h"
#include "GLSSAONoisePass.h"

#include "GLRenderingSystemUtilities.h"
#include "../../component/GLRenderingSystemComponent.h"

using namespace GLRenderingSystemNS;

#include "../ICoreSystem.h"

extern ICoreSystem* g_pCoreSystem;

INNO_PRIVATE_SCOPE GLSSAOBlurPass
{
	void initializeShaders();
	void bindUniformLocations(GLShaderProgramComponent* rhs);

	EntityID m_entityID;

	GLRenderPassComponent* m_GLRPC;

	GLShaderProgramComponent* m_GLSPC;

	ShaderFilePaths m_shaderFilePaths = { "GL//SSAOBlurPassVertex.sf" , "", "GL//SSAOBlurPassFragment.sf" };

	std::vector<std::string> m_SSAOBlurPassTextureUniformNames =
	{
		"uni_SSAOPassRT0",
	};
}

bool GLSSAOBlurPass::initialize()
{
	m_entityID = InnoMath::createEntityID();

	m_GLRPC = addGLRenderPassComponent(1, GLRenderingSystemComponent::get().deferredPassFBDesc, GLRenderingSystemComponent::get().deferredPassTextureDesc);

	initializeShaders();

	return true;
}

void GLSSAOBlurPass::initializeShaders()
{
	// shader programs and shaders
	auto rhs = addGLShaderProgramComponent(m_entityID);

	initializeGLShaderProgramComponent(rhs, m_shaderFilePaths);

	bindUniformLocations(rhs);

	m_GLSPC = rhs;
}

void GLSSAOBlurPass::bindUniformLocations(GLShaderProgramComponent * rhs)
{
	updateTextureUniformLocations(rhs->m_program, m_SSAOBlurPassTextureUniformNames);
}

bool GLSSAOBlurPass::update()
{
	activateRenderPass(m_GLRPC);

	activateShaderProgram(m_GLSPC);

	activateTexture(GLSSAONoisePass::getGLRPC()->m_GLTDCs[0], 0);

	auto l_MDC = g_pCoreSystem->getAssetSystem()->getMeshDataComponent(MeshShapeType::QUAD);
	drawMesh(l_MDC);

	return true;
}

bool GLSSAOBlurPass::resize()
{
	resizeGLRenderPassComponent(m_GLRPC, GLRenderingSystemComponent::get().deferredPassFBDesc);

	return true;
}

bool GLSSAOBlurPass::reloadShader()
{
	deleteShaderProgram(m_GLSPC);

	initializeGLShaderProgramComponent(m_GLSPC, m_shaderFilePaths);

	bindUniformLocations(m_GLSPC);

	return true;
}

GLRenderPassComponent * GLSSAOBlurPass::getGLRPC()
{
	return m_GLRPC;
}