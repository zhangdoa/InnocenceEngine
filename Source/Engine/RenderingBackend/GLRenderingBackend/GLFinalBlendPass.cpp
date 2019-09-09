#include "GLFinalBlendPass.h"
#include "GLBillboardPass.h"
#include "GLDebuggerPass.h"
#include "GLLightCullingPass.h"

#include "GLRenderingBackendUtilities.h"
#include "../../Component/GLRenderingBackendComponent.h"

using namespace GLRenderingBackendNS;

#include "../../ModuleManager/IModuleManager.h"

extern IModuleManager* g_pModuleManager;

namespace GLFinalBlendPass
{
	void initializeShaders();

	EntityID m_EntityID;

	GLRenderPassComponent* m_GLRPC;
	GLShaderProgramComponent* m_GLSPC;
	ShaderFilePaths m_shaderFilePaths = { "2DImageProcess.vert/", "", "", "", "finalBlendPass.frag/" };

	bool m_visualizeLightCulling = false;
	std::function<void()> f_toggleVisualizeLightCulling;
}

bool GLFinalBlendPass::initialize()
{
	m_EntityID = InnoMath::createEntityID();

	m_GLRPC = addGLRenderPassComponent(m_EntityID, "FinalBlendPassGLRPC/");
	m_GLRPC->m_renderPassDesc = GLRenderingBackendComponent::get().m_deferredRenderPassDesc;
	initializeGLRenderPassComponent(m_GLRPC);

	initializeShaders();

	f_toggleVisualizeLightCulling = [&]() {
		m_visualizeLightCulling = !m_visualizeLightCulling;
	};
	g_pModuleManager->getEventSystem()->addButtonStateCallback(ButtonData{ INNO_KEY_T, ButtonStatus::PRESSED }, &f_toggleVisualizeLightCulling);

	return true;
}

void GLFinalBlendPass::initializeShaders()
{
	// shader programs and shaders
	auto rhs = addGLShaderProgramComponent(m_EntityID);

	initializeGLShaderProgramComponent(rhs, m_shaderFilePaths);

	m_GLSPC = rhs;
}

bool GLFinalBlendPass::update(GLRenderPassComponent* prePassGLRPC)
{
	bindRenderPass(m_GLRPC);
	cleanRenderBuffers(m_GLRPC);

	activateShaderProgram(m_GLSPC);

	if (m_visualizeLightCulling)
	{
		activateTexture(GLLightCullingPass::getHeatMap(), 0);
	}
	else
	{	// last pass rendering target as the mixing background
		activateTexture(prePassGLRPC->m_GLTDCs[0], 0);
	}

	// billboard pass rendering target
	activateTexture(GLBillboardPass::getGLRPC()->m_GLTDCs[0], 1);
	// debugger pass rendering target
	activateTexture(GLDebuggerPass::getGLRPC(0)->m_GLTDCs[0], 2);

	// draw final pass rectangle
	auto l_MDC = getGLMeshDataComponent(MeshShapeType::QUAD);
	drawMesh(l_MDC);

	// draw again for game build
	glBindFramebuffer(GL_FRAMEBUFFER, 0);
	glBindRenderbuffer(GL_RENDERBUFFER, 0);

	drawMesh(l_MDC);

	return true;
}

bool GLFinalBlendPass::resize(uint32_t newSizeX, uint32_t newSizeY)
{
	resizeGLRenderPassComponent(m_GLRPC, newSizeX, newSizeY);

	return true;
}

bool GLFinalBlendPass::reloadShader()
{
	deleteShaderProgram(m_GLSPC);

	initializeGLShaderProgramComponent(m_GLSPC, m_shaderFilePaths);

	return true;
}

GLRenderPassComponent * GLFinalBlendPass::getGLRPC()
{
	return m_GLRPC;
}