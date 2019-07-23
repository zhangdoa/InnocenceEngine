#include "GLOpaquePass.h"
#include "GLEarlyZPass.h"
#include "GLRenderingBackendUtilities.h"
#include "../../Component/GLRenderingBackendComponent.h"

using namespace GLRenderingBackendNS;

#include "../../ModuleManager/IModuleManager.h"

extern IModuleManager* g_pModuleManager;

namespace GLOpaquePass
{
	void initializeShaders();

	EntityID m_entityID;

	GLRenderPassComponent* m_GLRPC;

	GLShaderProgramComponent* m_GLSPC;

	ShaderFilePaths m_shaderFilePaths = { "opaquePass.vert/" , "", "", "", "opaquePass.frag/" };
}

bool GLOpaquePass::initialize()
{
	m_entityID = InnoMath::createEntityID();

	m_GLRPC = addGLRenderPassComponent(m_entityID, "OpaquePassGLRPC/");
	m_GLRPC->m_renderPassDesc = GLRenderingBackendComponent::get().m_deferredRenderPassDesc;
	m_GLRPC->m_renderPassDesc.RTNumber = 4;
	m_GLRPC->m_renderPassDesc.useDepthAttachment = true;
	m_GLRPC->m_renderPassDesc.useStencilAttachment = true;

	initializeGLRenderPassComponent(m_GLRPC);

	initializeShaders();

	return true;
}

void GLOpaquePass::initializeShaders()
{
	// shader programs and shaders
	auto rhs = addGLShaderProgramComponent(m_entityID);

	initializeGLShaderProgramComponent(rhs, m_shaderFilePaths);

	m_GLSPC = rhs;
}

bool GLOpaquePass::update()
{
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

	bindRenderPass(m_GLRPC);
	cleanRenderBuffers(m_GLRPC);

	// @TODO: Eliminate transparency conflict
	//copyDepthBuffer(GLEarlyZPass::getGLRPC(), m_GLRPC);

	activateShaderProgram(m_GLSPC);

	unsigned int l_offset = 0;

	auto l_totalDrawCallCount = g_pModuleManager->getRenderingFrontend()->getOpaquePassDrawCallCount();
	for (unsigned int i = 0; i < l_totalDrawCallCount; i++)
	{
		auto l_opaquePassGPUData = g_pModuleManager->getRenderingFrontend()->getOpaquePassGPUData()[i];

		if (l_opaquePassGPUData.material->m_normalTexture)
		{
			activateTexture(reinterpret_cast<GLTextureDataComponent*>(l_opaquePassGPUData.material->m_normalTexture), 0);
		}
		if (l_opaquePassGPUData.material->m_albedoTexture)
		{
			activateTexture(reinterpret_cast<GLTextureDataComponent*>(l_opaquePassGPUData.material->m_albedoTexture), 1);
		}
		if (l_opaquePassGPUData.material->m_metallicTexture)
		{
			activateTexture(reinterpret_cast<GLTextureDataComponent*>(l_opaquePassGPUData.material->m_metallicTexture), 2);
		}
		if (l_opaquePassGPUData.material->m_roughnessTexture)
		{
			activateTexture(reinterpret_cast<GLTextureDataComponent*>(l_opaquePassGPUData.material->m_roughnessTexture), 3);
		}
		if (l_opaquePassGPUData.material->m_aoTexture)
		{
			activateTexture(reinterpret_cast<GLTextureDataComponent*>(l_opaquePassGPUData.material->m_aoTexture), 4);
		}

		bindUBO(GLRenderingBackendComponent::get().m_meshUBO, 1, l_offset * sizeof(MeshGPUData), sizeof(MeshGPUData));
		bindUBO(GLRenderingBackendComponent::get().m_materialUBO, 2, l_offset * sizeof(MaterialGPUData), sizeof(MaterialGPUData));

		drawMesh(reinterpret_cast<GLMeshDataComponent*>(l_opaquePassGPUData.mesh));

		l_offset++;
	}

	glDisable(GL_CULL_FACE);
	glDisable(GL_STENCIL_TEST);
	glDisable(GL_DEPTH_CLAMP);
	glDisable(GL_DEPTH_TEST);

	return true;
}

bool GLOpaquePass::resize(unsigned int newSizeX, unsigned int newSizeY)
{
	resizeGLRenderPassComponent(m_GLRPC, newSizeX, newSizeY);

	return true;
}

bool GLOpaquePass::reloadShader()
{
	deleteShaderProgram(m_GLSPC);

	initializeGLShaderProgramComponent(m_GLSPC, m_shaderFilePaths);

	return true;
}

GLRenderPassComponent * GLOpaquePass::getGLRPC()
{
	return m_GLRPC;
}

GLShaderProgramComponent * GLOpaquePass::getGLSPC()
{
	return m_GLSPC;
}