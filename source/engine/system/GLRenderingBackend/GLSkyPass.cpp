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
	ShaderFilePaths m_shaderFilePaths = { "GL//skyPass.vert", "", "GL//skyPass.frag" };
}

bool GLSkyPass::initialize()
{
	m_entityID = InnoMath::createEntityID();

	m_GLRPC = addGLRenderPassComponent(1, GLRenderingSystemComponent::get().deferredPassFBDesc, GLRenderingSystemComponent::get().deferredPassTextureDesc);

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
	auto l_renderingConfig = g_pCoreSystem->getVisionSystem()->getRenderingFrontend()->getRenderingConfig();

	if (l_renderingConfig.drawSky)
	{
		glEnable(GL_DEPTH_TEST);
		glDepthFunc(GL_LEQUAL);

		//glEnable(GL_CULL_FACE);
		//glFrontFace(GL_CW);
		//glCullFace(GL_FRONT);

		activateRenderPass(m_GLRPC);

		activateShaderProgram(m_GLSPC);

		// uni_p
		updateUniform(
			0,
			RenderingFrontendSystemComponent::get().m_cameraGPUData.p_original);
		// uni_r
		updateUniform(
			1,
			RenderingFrontendSystemComponent::get().m_cameraGPUData.r);

		vec2 l_viewportSize = vec2((float)GLRenderingSystemComponent::get().deferredPassFBDesc.sizeX, (float)GLRenderingSystemComponent::get().deferredPassFBDesc.sizeY);

		// uni_viewportSize
		updateUniform(
			2,
			l_viewportSize);

		// uni_eyePos
		updateUniform(
			3,
			RenderingFrontendSystemComponent::get().m_cameraGPUData.globalPos);

		// uni_lightDir
		updateUniform(
			4,
			RenderingFrontendSystemComponent::get().m_sunGPUData.dir);

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