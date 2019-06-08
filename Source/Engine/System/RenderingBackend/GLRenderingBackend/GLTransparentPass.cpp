#include "GLTransparentPass.h"
#include "GLOpaquePass.h"

#include "GLRenderingBackendUtilities.h"
#include "../../../Component/GLRenderingBackendComponent.h"
#include "../../../Component/RenderingFrontendComponent.h"

#include "../../ICoreSystem.h"

extern ICoreSystem* g_pCoreSystem;

using namespace GLRenderingBackendNS;

INNO_PRIVATE_SCOPE GLTransparentPass
{
	void initializeShaders();

	EntityID m_entityID;

	GLShaderProgramComponent* m_GLSPC;

	ShaderFilePaths m_shaderFilePaths = { "GL//transparentPass.vert/", "", "", "", "GL//transparentPass.frag/" };
}

bool GLTransparentPass::initialize()
{
	m_entityID = InnoMath::createEntityID();

	initializeShaders();

	return true;
}

void GLTransparentPass::initializeShaders()
{
	// shader programs and shaders
	auto rhs = addGLShaderProgramComponent(m_entityID);

	initializeGLShaderProgramComponent(rhs, m_shaderFilePaths);

	m_GLSPC = rhs;
}

bool GLTransparentPass::update(GLRenderPassComponent* prePassGLRPC)
{
	auto l_GLRPC = prePassGLRPC;
	glEnable(GL_DEPTH_TEST);
	glDepthFunc(GL_LESS);
	glDepthMask(GL_FALSE);

	glEnable(GL_DEPTH_CLAMP);

	glEnable(GL_BLEND);
	glBlendFuncSeparate(GL_SRC_ALPHA, GL_SRC1_COLOR, GL_ONE, GL_ZERO);

	glBindFramebuffer(GL_FRAMEBUFFER, l_GLRPC->m_FBO);
	glBindRenderbuffer(GL_RENDERBUFFER, l_GLRPC->m_RBO);
	glRenderbufferStorage(GL_RENDERBUFFER, l_GLRPC->m_renderBufferInternalFormat, l_GLRPC->m_renderPassDesc.RTDesc.width, l_GLRPC->m_renderPassDesc.RTDesc.height);
	glViewport(0, 0, l_GLRPC->m_renderPassDesc.RTDesc.width, l_GLRPC->m_renderPassDesc.RTDesc.height);

	copyDepthBuffer(GLOpaquePass::getGLRPC(), l_GLRPC);

	activateShaderProgram(m_GLSPC);

	unsigned int l_offset = 0;

	for (unsigned int i = 0; i < RenderingFrontendComponent::get().m_transparentPassDrawcallCount; i++)
	{
		auto l_transparentPassGPUData = RenderingFrontendComponent::get().m_transparentPassGPUDatas[i];

		bindUBO(GLRenderingBackendComponent::get().m_meshUBO, 1, l_offset * sizeof(MeshGPUData), sizeof(MeshGPUData));
		bindUBO(GLRenderingBackendComponent::get().m_materialUBO, 2, l_offset * sizeof(MaterialGPUData), sizeof(MaterialGPUData));

		drawMesh(reinterpret_cast<GLMeshDataComponent*>(l_transparentPassGPUData.MDC));

		l_offset++;
	}

	glDisable(GL_BLEND);
	glDisable(GL_DEPTH_CLAMP);
	glDisable(GL_DEPTH_TEST);

	return true;
}

bool GLTransparentPass::resize(unsigned int newSizeX, unsigned int newSizeY)
{
	return true;
}

bool GLTransparentPass::reloadShader()
{
	deleteShaderProgram(m_GLSPC);

	initializeGLShaderProgramComponent(m_GLSPC, m_shaderFilePaths);

	return true;
}