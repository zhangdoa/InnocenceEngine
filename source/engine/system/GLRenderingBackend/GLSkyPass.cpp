#include "GLSkyPass.h"
#include "GLRenderingSystemUtilities.h"
#include "../../component/GLRenderingSystemComponent.h"

using namespace GLRenderingSystemNS;

#include "../ICoreSystem.h"

extern ICoreSystem* g_pCoreSystem;

INNO_PRIVATE_SCOPE GLSkyPass
{
	void initializeShaders();
	void bindUniformLocations(GLShaderProgramComponent* rhs);

	EntityID m_entityID;

	GLRenderPassComponent* m_GLRPC;
	GLShaderProgramComponent* m_GLSPC;
	ShaderFilePaths m_shaderFilePaths = { "GL//skyPassVertex.sf", "", "GL//skyPassFragment.sf" };

	GLuint m_uni_p;
	GLuint m_uni_r;
	GLuint m_uni_viewportSize;
	GLuint m_uni_eyePos;
	GLuint m_uni_lightDir;
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

	bindUniformLocations(rhs);

	m_GLSPC = rhs;
}

void GLSkyPass::bindUniformLocations(GLShaderProgramComponent* rhs)
{
	m_uni_p = getUniformLocation(
		rhs->m_program,
		"uni_p");
	m_uni_r = getUniformLocation(
		rhs->m_program,
		"uni_r");
	m_uni_viewportSize = getUniformLocation(
		rhs->m_program,
		"uni_viewportSize");
	m_uni_eyePos = getUniformLocation(
		rhs->m_program,
		"uni_eyePos");
	m_uni_lightDir = getUniformLocation(
		rhs->m_program,
		"uni_lightDir");
}

bool GLSkyPass::update()
{
	auto l_renderingConfig = g_pCoreSystem->getVisionSystem()->getRenderingFrontend()->getRenderingConfig();
	auto l_cameraDataPack = g_pCoreSystem->getVisionSystem()->getRenderingFrontend()->getCameraDataPack();
	auto l_sunDataPack = g_pCoreSystem->getVisionSystem()->getRenderingFrontend()->getSunDataPack();

	if (l_renderingConfig.drawSky)
	{
		glEnable(GL_DEPTH_TEST);
		glDepthFunc(GL_LEQUAL);

		//glEnable(GL_CULL_FACE);
		//glFrontFace(GL_CW);
		//glCullFace(GL_FRONT);

		activateRenderPass(m_GLRPC);

		activateShaderProgram(m_GLSPC);

		updateUniform(
			m_uni_p,
			l_cameraDataPack.p_original);
		updateUniform(
			m_uni_r,
			l_cameraDataPack.r);
		updateUniform(
			m_uni_viewportSize,
			(float)GLRenderingSystemComponent::get().deferredPassFBDesc.sizeX, (float)GLRenderingSystemComponent::get().deferredPassFBDesc.sizeY);
		updateUniform(
			m_uni_eyePos,
			l_cameraDataPack.globalPos.x, l_cameraDataPack.globalPos.y, l_cameraDataPack.globalPos.z);
		updateUniform(
			m_uni_lightDir,
			l_sunDataPack.dir.x, l_sunDataPack.dir.y, l_sunDataPack.dir.z);

		auto l_MDC = g_pCoreSystem->getAssetSystem()->getMeshDataComponent(MeshShapeType::CUBE);
		drawMesh(l_MDC);

		glDisable(GL_CULL_FACE);
		glDisable(GL_DEPTH_TEST);
	}
	else
	{
		cleanFBC(m_GLRPC);
	}

	return true;
}

bool GLSkyPass::resize()
{
	resizeGLRenderPassComponent(m_GLRPC, GLRenderingSystemComponent::get().deferredPassFBDesc);

	return true;
}

bool GLSkyPass::reloadShader()
{
	deleteShaderProgram(m_GLSPC);

	initializeGLShaderProgramComponent(m_GLSPC, m_shaderFilePaths);

	bindUniformLocations(m_GLSPC);

	return true;
}

GLRenderPassComponent * GLSkyPass::getGLRPC()
{
	return m_GLRPC;
}