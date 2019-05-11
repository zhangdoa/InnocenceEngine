#include "GLBloomBlurPass.h"

#include "GLRenderingSystemUtilities.h"
#include "../../component/GLRenderingSystemComponent.h"

using namespace GLRenderingSystemNS;

#include "../ICoreSystem.h"

extern ICoreSystem* g_pCoreSystem;

INNO_PRIVATE_SCOPE GLBloomBlurPass
{
	void initializeShaders();

	EntityID m_entityID;

	GLRenderPassComponent* m_PingPassGLRPC;
	GLRenderPassComponent* m_PongPassGLRPC;
	GLShaderProgramComponent* m_GLSPC;
	ShaderFilePaths m_shaderFilePaths = { "GL//bloomBlurPass.vert/", "", "", "", "GL//bloomBlurPass.frag/" };
}

bool GLBloomBlurPass::initialize()
{
	m_entityID = InnoMath::createEntityID();

	//Ping pass
	m_PingPassGLRPC = addGLRenderPassComponent(m_entityID, "BloomBlurPingPassGLRPC/");
	m_PingPassGLRPC->m_renderPassDesc = GLRenderingSystemComponent::get().m_deferredRenderPassDesc;
	initializeGLRenderPassComponent(m_PingPassGLRPC);

	//Pong pass
	m_PongPassGLRPC = addGLRenderPassComponent(m_entityID, "BloomBlurPongPassGLRPC/");
	m_PongPassGLRPC->m_renderPassDesc = GLRenderingSystemComponent::get().m_deferredRenderPassDesc;
	initializeGLRenderPassComponent(m_PongPassGLRPC);

	initializeShaders();

	return true;
}

void GLBloomBlurPass::initializeShaders()
{
	// shader programs and shaders
	auto rhs = addGLShaderProgramComponent(m_entityID);

	initializeGLShaderProgramComponent(rhs, m_shaderFilePaths);

	m_GLSPC = rhs;
}

bool GLBloomBlurPass::update(GLRenderPassComponent* prePassGLRPC)
{
	GLTextureDataComponent* l_currentFrameBloomBlurGLTDC = m_PingPassGLRPC->m_GLTDCs[0];
	GLTextureDataComponent* l_lastFrameBloomBlurGLTDC = m_PongPassGLRPC->m_GLTDCs[0];

	activateShaderProgram(m_GLSPC);

	bool l_isPing = true;
	bool l_isFirstIteration = true;

	auto l_MDC = getGLMeshDataComponent(MeshShapeType::QUAD);

	for (size_t i = 0; i < 5; i++)
	{
		if (l_isPing)
		{
			l_currentFrameBloomBlurGLTDC = m_PingPassGLRPC->m_GLTDCs[0];
			l_lastFrameBloomBlurGLTDC = m_PongPassGLRPC->m_GLTDCs[0];

			activateRenderPass(m_PingPassGLRPC);

			updateUniform(
				0,
				true);

			if (l_isFirstIteration)
			{
				activateTexture(
					prePassGLRPC->m_GLTDCs[0],
					0);
				l_isFirstIteration = false;
			}
			else
			{
				activateTexture(
					l_lastFrameBloomBlurGLTDC,
					0);
			}

			drawMesh(l_MDC);

			l_isPing = false;
		}
		else
		{
			l_currentFrameBloomBlurGLTDC = m_PongPassGLRPC->m_GLTDCs[0];
			l_lastFrameBloomBlurGLTDC = m_PingPassGLRPC->m_GLTDCs[0];

			activateRenderPass(m_PongPassGLRPC);

			updateUniform(
				0,
				false);

			activateTexture(
				l_lastFrameBloomBlurGLTDC,
				0);

			drawMesh(l_MDC);

			l_isPing = true;
		}
	}

	// store result on another channel
	copyColorBuffer(m_PongPassGLRPC, 0, prePassGLRPC, 0);

	return true;
}

bool GLBloomBlurPass::resize(unsigned int newSizeX, unsigned int newSizeY)
{
	resizeGLRenderPassComponent(m_PingPassGLRPC, newSizeX, newSizeY);
	resizeGLRenderPassComponent(m_PongPassGLRPC, newSizeX, newSizeY);

	return true;
}

bool GLBloomBlurPass::reloadShader()
{
	deleteShaderProgram(m_GLSPC);

	initializeGLShaderProgramComponent(m_GLSPC, m_shaderFilePaths);

	return true;
}

GLRenderPassComponent * GLBloomBlurPass::getGLRPC(unsigned int index)
{
	if (index == 0)
	{
		return m_PingPassGLRPC;
	}
	else if (index == 1)
	{
		return m_PongPassGLRPC;
	}
	else
	{
		return nullptr;
	}
}