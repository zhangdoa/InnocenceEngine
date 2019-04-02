#include "GLDebuggerPass.h"

#include "GLOpaquePass.h"

#include "GLRenderingSystemUtilities.h"
#include "../../component/GLRenderingSystemComponent.h"

using namespace GLRenderingSystemNS;

#include "../ICoreSystem.h"

extern ICoreSystem* g_pCoreSystem;

INNO_PRIVATE_SCOPE GLDebuggerPass
{
	void initializeShaders();
	void bindUniformLocations(GLShaderProgramComponent* rhs);

	EntityID m_entityID;

	GLRenderPassComponent* m_GLRPC;
	GLShaderProgramComponent* m_GLSPC;
	ShaderFilePaths m_shaderFilePaths = { "GL//wireframeOverlayPassVertex.sf", "", "GL//wireframeOverlayPassFragment.sf" };
	//ShaderFilePaths m_shaderFilePaths = { "GL//debuggerPassVertex.sf", "GL//debuggerPassGeometry.sf", "GL//debuggerPassFragment.sf" };

	std::vector<std::string> m_uniformNames =
	{
		"uni_normalTexture",
	};

	GLuint m_uni_p;
	GLuint m_uni_r;
	GLuint m_uni_t;
	GLuint m_uni_m;
}

bool GLDebuggerPass::initialize()
{
	m_entityID = InnoMath::createEntityID();

	m_GLRPC = addGLRenderPassComponent(1, GLRenderingSystemComponent::get().deferredPassFBDesc, GLRenderingSystemComponent::get().deferredPassTextureDesc);

	initializeShaders();

	return true;
}

void GLDebuggerPass::initializeShaders()
{
	// shader programs and shaders
	auto rhs = addGLShaderProgramComponent(m_entityID);

	initializeGLShaderProgramComponent(rhs, m_shaderFilePaths);

	bindUniformLocations(rhs);

	m_GLSPC = rhs;
}

void GLDebuggerPass::bindUniformLocations(GLShaderProgramComponent* rhs)
{
	updateTextureUniformLocations(rhs->m_program, m_uniformNames);

	m_uni_p = getUniformLocation(
		rhs->m_program,
		"uni_p");
	m_uni_r = getUniformLocation(
		rhs->m_program,
		"uni_r");
	m_uni_t = getUniformLocation(
		rhs->m_program,
		"uni_t");
	m_uni_m = getUniformLocation(
		rhs->m_program,
		"uni_m");
}

bool GLDebuggerPass::update()
{
	auto l_cameraDataPack = g_pCoreSystem->getVisionSystem()->getRenderingFrontend()->getCameraDataPack();

	glEnable(GL_DEPTH_TEST);
	glDepthFunc(GL_LESS);

	activateRenderPass(m_GLRPC);

	// copy depth buffer from G-Pass
	copyDepthBuffer(GLOpaquePass::getGLRPC(), m_GLRPC);

	activateShaderProgram(m_GLSPC);

	updateUniform(
		m_uni_p,
		l_cameraDataPack.p_Original);
	updateUniform(
		m_uni_r,
		l_cameraDataPack.r);
	updateUniform(
		m_uni_t,
		l_cameraDataPack.t);

	glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
	while (GLRenderingSystemComponent::get().m_debuggerPassDataQueue.size() > 0)
	{
		auto l_renderPack = GLRenderingSystemComponent::get().m_debuggerPassDataQueue.front();

		auto l_m = l_renderPack.m;

		updateUniform(
			m_uni_m,
			l_m);

		drawMesh(l_renderPack.indiceSize, l_renderPack.meshPrimitiveTopology, l_renderPack.GLMDC);

		GLRenderingSystemComponent::get().m_debuggerPassDataQueue.pop();
	}

	glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
	glDisable(GL_DEPTH_TEST);

	return true;
}

bool GLDebuggerPass::resize()
{
	resizeGLRenderPassComponent(m_GLRPC, GLRenderingSystemComponent::get().deferredPassFBDesc);

	return true;
}

bool GLDebuggerPass::reloadShader()
{
	deleteShaderProgram(m_GLSPC);

	initializeGLShaderProgramComponent(m_GLSPC, m_shaderFilePaths);

	bindUniformLocations(m_GLSPC);

	return true;
}

GLRenderPassComponent * GLDebuggerPass::getGLRPC()
{
	return m_GLRPC;
}