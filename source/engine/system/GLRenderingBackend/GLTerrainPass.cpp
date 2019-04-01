#include "GLTerrainPass.h"
#include "GLOpaquePass.h"

#include "GLRenderingSystemUtilities.h"
#include "../../component/GLRenderingSystemComponent.h"

using namespace GLRenderingSystemNS;

#include "../ICoreSystem.h"

extern ICoreSystem* g_pCoreSystem;

INNO_PRIVATE_SCOPE GLTerrainPass
{
	void initializeShaders();
	void bindUniformLocations(GLShaderProgramComponent* rhs);

	EntityID m_entityID;

	GLRenderPassComponent* m_GLRPC;

	GLShaderProgramComponent* m_GLSPC;

	ShaderFilePaths m_shaderFilePaths = { "GL//terrainPassVertex.sf" , "", "GL//terrainPassFragment.sf" };

	GLuint m_uni_p_camera;
	GLuint m_uni_r_camera;
	GLuint m_uni_t_camera;
	GLuint m_uni_m;
	GLuint m_uni_chunkNumber;
	GLuint m_uni_albedoTexture;
}

bool GLTerrainPass::initialize()
{
	m_entityID = InnoMath::createEntityID();

	m_GLRPC = addGLRenderPassComponent(1, GLRenderingSystemComponent::get().deferredPassFBDesc, GLRenderingSystemComponent::get().deferredPassTextureDesc);

	initializeShaders();

	return true;
}

void GLTerrainPass::initializeShaders()
{
	// shader programs and shaders
	auto rhs = addGLShaderProgramComponent(m_entityID);

	initializeGLShaderProgramComponent(rhs, m_shaderFilePaths);

	bindUniformLocations(rhs);

	m_GLSPC = rhs;
}

void GLTerrainPass::bindUniformLocations(GLShaderProgramComponent* rhs)
{
	m_uni_p_camera = getUniformLocation(
		rhs->m_program,
		"uni_p_camera");
	m_uni_r_camera = getUniformLocation(
		rhs->m_program,
		"uni_r_camera");
	m_uni_t_camera = getUniformLocation(
		rhs->m_program,
		"uni_t_camera");
	m_uni_m = getUniformLocation(
		rhs->m_program,
		"uni_m");

	m_uni_albedoTexture = getUniformLocation(
		rhs->m_program,
		"uni_albedoTexture");
	updateUniform(
		m_uni_albedoTexture,
		0);

	m_uni_chunkNumber = getUniformLocation(
		rhs->m_program,
		"uni_heightTexture");
	updateUniform(
		m_uni_albedoTexture,
		1);
}

bool GLTerrainPass::update()
{
	auto l_renderingConfig = g_pCoreSystem->getVisionSystem()->getRenderingFrontend()->getRenderingConfig();
	auto l_cameraDataPack = g_pCoreSystem->getVisionSystem()->getRenderingFrontend()->getCameraDataPack();

	if (l_renderingConfig.drawTerrain)
	{
		glEnable(GL_DEPTH_TEST);

		activateRenderPass(m_GLRPC);

		copyDepthBuffer(GLOpaquePass::getGLRPC(), m_GLRPC);

		activateShaderProgram(m_GLSPC);

		mat4 m = InnoMath::generateIdentityMatrix<float>();

		updateUniform(
			m_uni_p_camera,
			l_cameraDataPack.p_Original);
		updateUniform(
			m_uni_r_camera,
			l_cameraDataPack.r);
		updateUniform(
			m_uni_t_camera,
			l_cameraDataPack.t);
		updateUniform(
			m_uni_m,
			m);

		auto l_MDC = g_pCoreSystem->getAssetSystem()->getMeshDataComponent(MeshShapeType::TERRAIN);
		activateTexture(GLRenderingSystemComponent::get().m_basicAlbedoGLTDC, 0);

		drawMesh(l_MDC);

		glDisable(GL_DEPTH_TEST);
	}
	else
	{
		cleanFBC(m_GLRPC);
	}
	return true;
}

bool GLTerrainPass::resize()
{
	return true;
}

bool GLTerrainPass::reloadShader()
{
	return true;
}

GLRenderPassComponent * GLTerrainPass::getGLRPC()
{
	return m_GLRPC;
}