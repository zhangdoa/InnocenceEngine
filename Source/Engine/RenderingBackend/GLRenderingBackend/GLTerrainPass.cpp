#include "GLTerrainPass.h"
#include "GLOpaquePass.h"

#include "GLRenderingBackendUtilities.h"
#include "../../Component/GLRenderingBackendComponent.h"

using namespace GLRenderingBackendNS;

#include "../../ModuleManager/IModuleManager.h"

extern IModuleManager* g_pModuleManager;

namespace GLTerrainPass
{
	void initializeShaders();

	EntityID m_entityID;

	GLShaderProgramComponent* m_GLSPC;

	ShaderFilePaths m_shaderFilePaths = { "terrainPass.vert/", "terrainPass.tesc/", "terrainPass.tese/", "", "terrainPass.frag/" };

	GLRenderPassComponent* m_h2nGLRPC;

	GLShaderProgramComponent* m_h2nGLSPC;

	ShaderFilePaths m_h2nShaderFilePaths = { "2DImageProcess.vert/", "", "", "", "heightToNormalPass.frag/" };

	bool generateNormal();

	GLTextureDataComponent* m_terrainGLTDC;
}

bool GLTerrainPass::initialize()
{
	m_entityID = InnoMath::createEntityID();

	initializeShaders();

	auto l_terrainTDC = g_pModuleManager->getAssetSystem()->loadTexture("Res//Textures//basic_terrain.png", TextureSamplerType::SAMPLER_2D, TextureUsageType::NORMAL);

	m_terrainGLTDC = reinterpret_cast<GLTextureDataComponent*>(l_terrainTDC);

	initializeGLTextureDataComponent(m_terrainGLTDC);

	m_h2nGLRPC = addGLRenderPassComponent(m_entityID, "HeightToNormalGLRPC/");
	m_h2nGLRPC->m_renderPassDesc = GLRenderingBackendComponent::get().m_deferredRenderPassDesc;
	m_h2nGLRPC->m_renderPassDesc.RTDesc.wrapMethod = TextureWrapMethod::REPEAT;
	m_h2nGLRPC->m_renderPassDesc.RTDesc.width = m_terrainGLTDC->m_textureDataDesc.width;
	m_h2nGLRPC->m_renderPassDesc.RTDesc.height = m_terrainGLTDC->m_textureDataDesc.height;
	initializeGLRenderPassComponent(m_h2nGLRPC);

	generateNormal();

	return true;
}

void GLTerrainPass::initializeShaders()
{
	// shader programs and shaders
	m_GLSPC = addGLShaderProgramComponent(m_entityID);

	initializeGLShaderProgramComponent(m_GLSPC, m_shaderFilePaths);

	m_h2nGLSPC = addGLShaderProgramComponent(m_entityID);

	initializeGLShaderProgramComponent(m_h2nGLSPC, m_h2nShaderFilePaths);
}

bool GLTerrainPass::generateNormal()
{
	bindRenderPass(m_h2nGLRPC);
	cleanRenderBuffers(m_h2nGLRPC);

	activateShaderProgram(m_h2nGLSPC);

	auto l_MDC = getGLMeshDataComponent(MeshShapeType::QUAD);

	activateTexture(m_terrainGLTDC, 0);

	drawMesh(l_MDC);

	return true;
}

bool GLTerrainPass::update()
{
	auto l_MDC = getGLMeshDataComponent(MeshShapeType::TERRAIN);

	glEnable(GL_DEPTH_TEST);
	glDepthFunc(GL_LEQUAL);
	glDepthMask(GL_TRUE);

	glEnable(GL_DEPTH_CLAMP);

	glEnable(GL_STENCIL_TEST);
	glStencilOp(GL_KEEP, GL_KEEP, GL_REPLACE);
	glStencilMask(0xFF);
	glStencilFunc(GL_ALWAYS, 0x01, 0xFF);

	glEnable(GL_CULL_FACE);
	glCullFace(GL_BACK);

	bindRenderPass(GLOpaquePass::getGLRPC());

	activateShaderProgram(m_GLSPC);

	activateTexture(m_terrainGLTDC, 0);
	activateTexture(m_h2nGLRPC->m_GLTDCs[0], 1);
	activateTexture(getGLTextureDataComponent(TextureUsageType::ALBEDO), 2);
	activateTexture(getGLTextureDataComponent(TextureUsageType::METALLIC), 3);
	activateTexture(getGLTextureDataComponent(TextureUsageType::ROUGHNESS), 4);
	activateTexture(getGLTextureDataComponent(TextureUsageType::AMBIENT_OCCLUSION), 5);

	//glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
	glBindVertexArray(l_MDC->m_VAO);
	glPatchParameteri(GL_PATCH_VERTICES, 4);
	glDrawArrays(GL_PATCHES, 0, (GLsizei)l_MDC->m_vertices.size());
	//glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);

	glDisable(GL_CULL_FACE);
	glDisable(GL_STENCIL_TEST);
	glDisable(GL_DEPTH_CLAMP);
	glDisable(GL_DEPTH_TEST);

	return true;
}

bool GLTerrainPass::resize(unsigned int newSizeX, unsigned int newSizeY)
{
	return true;
}

bool GLTerrainPass::reloadShader()
{
	deleteShaderProgram(m_GLSPC);

	initializeGLShaderProgramComponent(m_GLSPC, m_shaderFilePaths);

	return true;
}

GLTextureDataComponent* GLTerrainPass::getHeightMap(unsigned int index)
{
	if (index == 0)
	{
		return m_terrainGLTDC;
	}
	else
	{
		return m_h2nGLRPC->m_GLTDCs[0];
	}
}