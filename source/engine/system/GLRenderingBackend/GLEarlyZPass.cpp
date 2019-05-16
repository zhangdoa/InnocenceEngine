#include "GLEarlyZPass.h"
#include "GLRenderingSystemUtilities.h"
#include "../../component/GLRenderingSystemComponent.h"
#include "../../component/RenderingFrontendSystemComponent.h"

using namespace GLRenderingSystemNS;

INNO_PRIVATE_SCOPE GLEarlyZPass
{
	void initializeShaders();

	EntityID m_entityID;

	GLRenderPassComponent* m_GLRPC;

	GLShaderProgramComponent* m_GLSPC;

	ShaderFilePaths m_shaderFilePaths = { "GL//earlyZPass.vert/" , "", "","", "GL//earlyZPass.frag/" };
}

bool GLEarlyZPass::initialize()
{
	m_entityID = InnoMath::createEntityID();

	auto l_renderPassDesc = GLRenderingSystemComponent::get().m_deferredRenderPassDesc;
	l_renderPassDesc.RTDesc.pixelDataFormat = TexturePixelDataFormat::R;
	l_renderPassDesc.RTDesc.pixelDataType = TexturePixelDataType::UINT32;
	l_renderPassDesc.useDepthAttachment = true;
	l_renderPassDesc.useStencilAttachment = true;

	m_GLRPC = addGLRenderPassComponent(m_entityID, "EarlyZPassGLRPC/");
	m_GLRPC->m_renderPassDesc = l_renderPassDesc;
	initializeGLRenderPassComponent(m_GLRPC);

	initializeShaders();

	return true;
}

void GLEarlyZPass::initializeShaders()
{
	// shader programs and shaders
	auto rhs = addGLShaderProgramComponent(m_entityID);

	initializeGLShaderProgramComponent(rhs, m_shaderFilePaths);

	m_GLSPC = rhs;
}

bool GLEarlyZPass::update()
{
	glEnable(GL_DEPTH_TEST);
	glDepthFunc(GL_LESS);
	glDepthMask(GL_TRUE);

	glEnable(GL_DEPTH_CLAMP);

	glEnable(GL_CULL_FACE);
	glCullFace(GL_BACK);

	activateRenderPass(m_GLRPC);

	activateShaderProgram(m_GLSPC);

	unsigned int l_offset = 0;

	for (unsigned int i = 0; i < RenderingFrontendSystemComponent::get().m_opaquePassDrawcallCount; i++)
	{
		auto l_opaquePassGPUData = RenderingFrontendSystemComponent::get().m_opaquePassGPUDatas[i];

		bindUBO(GLRenderingSystemComponent::get().m_meshUBO, 1, l_offset * sizeof(MeshGPUData), sizeof(MeshGPUData));

		drawMesh(reinterpret_cast<GLMeshDataComponent*>(l_opaquePassGPUData.MDC));

		l_offset++;
	}

	glDisable(GL_CULL_FACE);
	glDisable(GL_DEPTH_CLAMP);
	glDisable(GL_DEPTH_TEST);

	return true;
}

bool GLEarlyZPass::resize(unsigned int newSizeX, unsigned int newSizeY)
{
	resizeGLRenderPassComponent(m_GLRPC, newSizeX, newSizeY);

	return true;
}

bool GLEarlyZPass::reloadShader()
{
	deleteShaderProgram(m_GLSPC);

	initializeGLShaderProgramComponent(m_GLSPC, m_shaderFilePaths);

	return true;
}

GLRenderPassComponent * GLEarlyZPass::getGLRPC()
{
	return m_GLRPC;
}