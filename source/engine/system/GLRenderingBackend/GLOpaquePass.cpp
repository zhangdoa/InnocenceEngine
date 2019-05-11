#include "GLOpaquePass.h"
#include "GLEarlyZPass.h"
#include "GLRenderingSystemUtilities.h"
#include "../../component/GLRenderingSystemComponent.h"
#include "../../component/RenderingFrontendSystemComponent.h"

using namespace GLRenderingSystemNS;

#include "../ICoreSystem.h"

extern ICoreSystem* g_pCoreSystem;

INNO_PRIVATE_SCOPE GLOpaquePass
{
	void initializeShaders();

	EntityID m_entityID;

	GLRenderPassComponent* m_GLRPC;

	GLShaderProgramComponent* m_GLSPC;

	ShaderFilePaths m_shaderFilePaths = { "GL//opaquePass.vert/" , "", "", "", "GL//opaquePass.frag/" };
}

bool GLOpaquePass::initialize()
{
	m_entityID = InnoMath::createEntityID();

	m_GLRPC = addGLRenderPassComponent(m_entityID, "OpaquePassGLRPC/");
	m_GLRPC->m_renderPassDesc = GLRenderingSystemComponent::get().m_deferredRenderPassDesc;
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
	glDepthMask(GL_FALSE);

	glEnable(GL_DEPTH_CLAMP);

	glEnable(GL_STENCIL_TEST);
	glStencilOp(GL_KEEP, GL_KEEP, GL_REPLACE);
	glStencilMask(0xFF);

	glEnable(GL_CULL_FACE);
	glCullFace(GL_BACK);

	activateRenderPass(m_GLRPC);

	copyDepthBuffer(GLEarlyZPass::getGLRPC(), m_GLRPC);

	activateShaderProgram(m_GLSPC);

	while (RenderingFrontendSystemComponent::get().m_opaquePassGPUDataQueue.size() > 0)
	{
		GeometryPassGPUData l_geometryPassGPUData = {};

		if (RenderingFrontendSystemComponent::get().m_opaquePassGPUDataQueue.tryPop(l_geometryPassGPUData))
		{
			glStencilFunc(GL_ALWAYS, 0x01, 0xFF);

			// any normal?
			if (l_geometryPassGPUData.materialGPUData.useNormalTexture)
			{
				activateTexture(reinterpret_cast<GLTextureDataComponent*>(l_geometryPassGPUData.normalTDC), 0);
			}
			// any albedo?
			if (l_geometryPassGPUData.materialGPUData.useAlbedoTexture)
			{
				activateTexture(reinterpret_cast<GLTextureDataComponent*>(l_geometryPassGPUData.albedoTDC), 1);
			}
			// any metallic?
			if (l_geometryPassGPUData.materialGPUData.useMetallicTexture)
			{
				activateTexture(reinterpret_cast<GLTextureDataComponent*>(l_geometryPassGPUData.metallicTDC), 2);
			}
			// any roughness?
			if (l_geometryPassGPUData.materialGPUData.useRoughnessTexture)
			{
				activateTexture(reinterpret_cast<GLTextureDataComponent*>(l_geometryPassGPUData.roughnessTDC), 3);
			}
			// any ao?
			if (l_geometryPassGPUData.materialGPUData.useAOTexture)
			{
				activateTexture(reinterpret_cast<GLTextureDataComponent*>(l_geometryPassGPUData.AOTDC), 4);
			}

			updateUBO(GLRenderingSystemComponent::get().m_meshUBO, l_geometryPassGPUData.meshGPUData);
			updateUBO(GLRenderingSystemComponent::get().m_materialUBO, l_geometryPassGPUData.materialGPUData);

			drawMesh(reinterpret_cast<GLMeshDataComponent*>(l_geometryPassGPUData.MDC));
		}
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