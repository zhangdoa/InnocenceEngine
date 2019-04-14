#include "GLSkyPass.h"
#include "GLRenderingSystemUtilities.h"
#include "../../component/GLRenderingSystemComponent.h"

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

	CameraDataPack m_cameraDataPack;
	SunDataPack m_sunDataPack;
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

	// copy camera data pack for local scope
	auto l_cameraDataPack = g_pCoreSystem->getVisionSystem()->getRenderingFrontend()->getCameraDataPack();
	if (l_cameraDataPack.has_value())
	{
		m_cameraDataPack = l_cameraDataPack.value();
	}

	// copy sun data pack for local scope
	auto l_sunDataPack = g_pCoreSystem->getVisionSystem()->getRenderingFrontend()->getSunDataPack();
	if (l_sunDataPack.has_value())
	{
		m_sunDataPack = l_sunDataPack.value();
	}

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
			m_cameraDataPack.p_original);
		// uni_r
		updateUniform(
			1,
			m_cameraDataPack.r);
		// uni_viewportSize
		updateUniform(
			2,
			(float)GLRenderingSystemComponent::get().deferredPassFBDesc.sizeX, (float)GLRenderingSystemComponent::get().deferredPassFBDesc.sizeY);
		// uni_eyePos
		updateUniform(
			3,
			m_cameraDataPack.globalPos.x, m_cameraDataPack.globalPos.y, m_cameraDataPack.globalPos.z);
		// uni_lightDir
		updateUniform(
			4,
			m_sunDataPack.dir.x, m_sunDataPack.dir.y, m_sunDataPack.dir.z);

		auto l_MDC = g_pCoreSystem->getAssetSystem()->getMeshDataComponent(MeshShapeType::CUBE);
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