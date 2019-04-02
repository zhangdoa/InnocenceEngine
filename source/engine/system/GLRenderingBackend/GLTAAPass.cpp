#include "GLTAAPass.h"

#include "GLOpaquePass.h"

#include "GLRenderingSystemUtilities.h"
#include "../../component/GLRenderingSystemComponent.h"

using namespace GLRenderingSystemNS;

#include "../ICoreSystem.h"

extern ICoreSystem* g_pCoreSystem;

INNO_PRIVATE_SCOPE GLTAAPass
{
	void initializeShaders();
	void bindUniformLocations(GLShaderProgramComponent* rhs);

	EntityID m_entityID;

	bool m_isTAAPingPass = true;

	GLRenderPassComponent* m_History0PassGLRPC;
	GLRenderPassComponent* m_History1PassGLRPC;
	GLRenderPassComponent* m_History2PassGLRPC;

	GLRenderPassComponent* m_PingPassGLRPC;
	GLRenderPassComponent* m_PongPassGLRPC;
	GLShaderProgramComponent* m_GLSPC;
	ShaderFilePaths m_shaderFilePaths = { "GL//TAAPassVertex.sf", "", "GL//TAAPassFragment.sf" };

	std::vector<std::string> m_uniformNames =
	{
		"uni_preTAAPassRT0",
		"uni_history0",
		"uni_history1",
		"uni_history2",
		"uni_history3",
		"uni_motionVectorTexture",
	};
}

bool GLTAAPass::initialize()
{
	m_entityID = InnoMath::createEntityID();

	// history buffer pass
	m_History0PassGLRPC = addGLRenderPassComponent(1, GLRenderingSystemComponent::get().deferredPassFBDesc, GLRenderingSystemComponent::get().deferredPassTextureDesc);
	m_History1PassGLRPC = addGLRenderPassComponent(1, GLRenderingSystemComponent::get().deferredPassFBDesc, GLRenderingSystemComponent::get().deferredPassTextureDesc);
	m_History2PassGLRPC = addGLRenderPassComponent(1, GLRenderingSystemComponent::get().deferredPassFBDesc, GLRenderingSystemComponent::get().deferredPassTextureDesc);

	// Ping pass
	m_PingPassGLRPC = addGLRenderPassComponent(1, GLRenderingSystemComponent::get().deferredPassFBDesc, GLRenderingSystemComponent::get().deferredPassTextureDesc);

	// Pong pass
	m_PongPassGLRPC = addGLRenderPassComponent(1, GLRenderingSystemComponent::get().deferredPassFBDesc, GLRenderingSystemComponent::get().deferredPassTextureDesc);

	initializeShaders();

	return true;
}

void GLTAAPass::initializeShaders()
{
	// shader programs and shaders
	auto rhs = addGLShaderProgramComponent(m_entityID);

	initializeGLShaderProgramComponent(rhs, m_shaderFilePaths);

	bindUniformLocations(rhs);

	m_GLSPC = rhs;
}

void GLTAAPass::bindUniformLocations(GLShaderProgramComponent* rhs)
{
	updateTextureUniformLocations(rhs->m_program, m_uniformNames);
}

bool GLTAAPass::update(GLRenderPassComponent* prePassGLRPC)
{
	GLTextureDataComponent* l_currentFrameGLTDC;
	GLTextureDataComponent* l_lastFrameGLTDC;
	GLRenderPassComponent* l_currentFrameGLRPC;

	if (m_isTAAPingPass)
	{
		l_currentFrameGLTDC = m_PingPassGLRPC->m_GLTDCs[0];
		l_lastFrameGLTDC = m_PongPassGLRPC->m_GLTDCs[0];

		l_currentFrameGLRPC = m_PingPassGLRPC;

		m_isTAAPingPass = false;
	}
	else
	{
		l_currentFrameGLTDC = m_PongPassGLRPC->m_GLTDCs[0];
		l_lastFrameGLTDC = m_PingPassGLRPC->m_GLTDCs[0];

		l_currentFrameGLRPC = m_PongPassGLRPC;

		m_isTAAPingPass = true;
	}

	copyColorBuffer(m_History1PassGLRPC, 0, m_History0PassGLRPC, 0);
	copyColorBuffer(m_History2PassGLRPC, 0, m_History1PassGLRPC, 0);
	copyColorBuffer(l_currentFrameGLRPC, 0, m_History2PassGLRPC, 0);

	activateShaderProgram(m_GLSPC);

	activateRenderPass(l_currentFrameGLRPC);

	activateTexture(prePassGLRPC->m_GLTDCs[0], 0);
	activateTexture(m_History0PassGLRPC->m_GLTDCs[0], 1);
	activateTexture(m_History1PassGLRPC->m_GLTDCs[0], 2);
	activateTexture(m_History2PassGLRPC->m_GLTDCs[0], 3);
	activateTexture(l_lastFrameGLTDC, 4);
	activateTexture(GLOpaquePass::getGLRPC()->m_GLTDCs[3], 5);

	auto l_MDC = g_pCoreSystem->getAssetSystem()->getMeshDataComponent(MeshShapeType::QUAD);
	drawMesh(l_MDC);

	return true;
}

bool GLTAAPass::resize()
{
	resizeGLRenderPassComponent(m_History0PassGLRPC, GLRenderingSystemComponent::get().deferredPassFBDesc);
	resizeGLRenderPassComponent(m_History1PassGLRPC, GLRenderingSystemComponent::get().deferredPassFBDesc);
	resizeGLRenderPassComponent(m_History2PassGLRPC, GLRenderingSystemComponent::get().deferredPassFBDesc);
	resizeGLRenderPassComponent(m_PingPassGLRPC, GLRenderingSystemComponent::get().deferredPassFBDesc);
	resizeGLRenderPassComponent(m_PongPassGLRPC, GLRenderingSystemComponent::get().deferredPassFBDesc);

	return true;
}

bool GLTAAPass::reloadShader()
{
	deleteShaderProgram(m_GLSPC);

	initializeGLShaderProgramComponent(m_GLSPC, m_shaderFilePaths);

	bindUniformLocations(m_GLSPC);

	return true;
}

GLRenderPassComponent * GLTAAPass::getGLRPC()
{
	if (m_isTAAPingPass)
	{
		return m_PingPassGLRPC;
	}
	else
	{
		return m_PongPassGLRPC;
	}
}