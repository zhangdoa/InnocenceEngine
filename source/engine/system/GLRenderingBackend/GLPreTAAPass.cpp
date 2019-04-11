#include "GLPreTAAPass.h"

#include "GLLightPass.h"
#include "GLTransparentPass.h"
#include "GLSkyPass.h"
#include "GLTerrainPass.h"

#include "GLRenderingSystemUtilities.h"
#include "../../component/GLRenderingSystemComponent.h"

using namespace GLRenderingSystemNS;

#include "../ICoreSystem.h"

extern ICoreSystem* g_pCoreSystem;

INNO_PRIVATE_SCOPE GLPreTAAPass
{
	void initializeShaders();
	void bindUniformLocations(GLShaderProgramComponent* rhs);

	EntityID m_entityID;

	GLRenderPassComponent* m_GLRPC;
	GLShaderProgramComponent* m_GLSPC;
	ShaderFilePaths m_shaderFilePaths = { "GL//preTAAPassVertex.sf", "", "GL//preTAAPassFragment.sf" };

	std::vector<std::string> m_uniformNames =
	{
		"uni_lightPassRT0",
		"uni_transparentPassRT0",
		"uni_skyPassRT0",
		"uni_terrainPassRT0",
	};
}

bool GLPreTAAPass::initialize()
{
	m_entityID = InnoMath::createEntityID();

	m_GLRPC = addGLRenderPassComponent(1, GLRenderingSystemComponent::get().deferredPassFBDesc, GLRenderingSystemComponent::get().deferredPassTextureDesc);

	initializeShaders();

	return true;
}

void GLPreTAAPass::initializeShaders()
{
	// shader programs and shaders
	auto rhs = addGLShaderProgramComponent(m_entityID);

	initializeGLShaderProgramComponent(rhs, m_shaderFilePaths);

	bindUniformLocations(rhs);

	m_GLSPC = rhs;
}

void GLPreTAAPass::bindUniformLocations(GLShaderProgramComponent* rhs)
{
	updateTextureUniformLocations(rhs->m_program, m_uniformNames);
}

bool GLPreTAAPass::update()
{
	activateRenderPass(m_GLRPC);

	activateShaderProgram(m_GLSPC);

	activateTexture(
		GLLightPass::getGLRPC()->m_GLTDCs[0],
		0);
	activateTexture(
		GLTransparentPass::getGLRPC()->m_GLTDCs[0],
		1);
	activateTexture(
		GLSkyPass::getGLRPC()->m_GLTDCs[0],
		2);
	activateTexture(
		GLTerrainPass::getGLRPC()->m_GLTDCs[0],
		3);

	auto l_MDC = g_pCoreSystem->getAssetSystem()->getMeshDataComponent(MeshShapeType::QUAD);
	drawMesh(l_MDC);

	return true;
}

bool GLPreTAAPass::resize()
{
	resizeGLRenderPassComponent(m_GLRPC, GLRenderingSystemComponent::get().deferredPassFBDesc);

	return true;
}

bool GLPreTAAPass::reloadShader()
{
	deleteShaderProgram(m_GLSPC);

	initializeGLShaderProgramComponent(m_GLSPC, m_shaderFilePaths);

	bindUniformLocations(m_GLSPC);

	return true;
}

GLRenderPassComponent * GLPreTAAPass::getGLRPC()
{
	return m_GLRPC;
}