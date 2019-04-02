#include "GLBloomBlurPass.h"

#include "GLRenderingSystemUtilities.h"
#include "../../component/GLRenderingSystemComponent.h"

using namespace GLRenderingSystemNS;

#include "../ICoreSystem.h"

extern ICoreSystem* g_pCoreSystem;

INNO_PRIVATE_SCOPE GLBloomBlurPass
{
	void initializeShaders();
	void bindUniformLocations(GLShaderProgramComponent* rhs);

	EntityID m_entityID;

	GLRenderPassComponent* m_PingPassGLRPC;
	GLRenderPassComponent* m_PongPassGLRPC;
	GLShaderProgramComponent* m_GLSPC;
	ShaderFilePaths m_shaderFilePaths = { "GL//bloomBlurPassVertex.sf", "", "GL//bloomBlurPassFragment.sf" };

	std::vector<std::string> m_uniformNames =
	{
		"uni_bloomExtractPassRT0",
	};

	GLuint m__uni_horizontal;
}

bool GLBloomBlurPass::initialize()
{
	m_entityID = InnoMath::createEntityID();

	//Ping pass
	m_PingPassGLRPC = addGLRenderPassComponent(1, GLRenderingSystemComponent::get().deferredPassFBDesc, GLRenderingSystemComponent::get().deferredPassTextureDesc);

	//Pong pass
	m_PongPassGLRPC = addGLRenderPassComponent(1, GLRenderingSystemComponent::get().deferredPassFBDesc, GLRenderingSystemComponent::get().deferredPassTextureDesc);

	initializeShaders();

	return true;
}

void GLBloomBlurPass::initializeShaders()
{
	// shader programs and shaders
	auto rhs = addGLShaderProgramComponent(m_entityID);

	initializeGLShaderProgramComponent(rhs, m_shaderFilePaths);

	bindUniformLocations(rhs);

	m_GLSPC = rhs;
}

void GLBloomBlurPass::bindUniformLocations(GLShaderProgramComponent* rhs)
{
	updateTextureUniformLocations(rhs->m_program, m_uniformNames);

	m__uni_horizontal = getUniformLocation(
		rhs->m_program,
		"uni_horizontal");
}

bool GLBloomBlurPass::update(GLRenderPassComponent* prePassGLRPC)
{
	GLTextureDataComponent* l_currentFrameBloomBlurGLTDC = m_PingPassGLRPC->m_GLTDCs[0];
	GLTextureDataComponent* l_lastFrameBloomBlurGLTDC = m_PongPassGLRPC->m_GLTDCs[0];

	activateShaderProgram(m_GLSPC);

	bool l_isPing = true;
	bool l_isFirstIteration = true;

	auto l_MDC = g_pCoreSystem->getAssetSystem()->getMeshDataComponent(MeshShapeType::QUAD);

	for (size_t i = 0; i < 5; i++)
	{
		if (l_isPing)
		{
			l_currentFrameBloomBlurGLTDC = m_PingPassGLRPC->m_GLTDCs[0];
			l_lastFrameBloomBlurGLTDC = m_PongPassGLRPC->m_GLTDCs[0];

			activateRenderPass(m_PingPassGLRPC);

			updateUniform(
				m__uni_horizontal,
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
				m__uni_horizontal,
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

bool GLBloomBlurPass::resize()
{
	resizeGLRenderPassComponent(m_PingPassGLRPC, GLRenderingSystemComponent::get().deferredPassFBDesc);
	resizeGLRenderPassComponent(m_PongPassGLRPC, GLRenderingSystemComponent::get().deferredPassFBDesc);

	return true;
}

bool GLBloomBlurPass::reloadShader()
{
	deleteShaderProgram(m_GLSPC);

	initializeGLShaderProgramComponent(m_GLSPC, m_shaderFilePaths);

	bindUniformLocations(m_GLSPC);

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