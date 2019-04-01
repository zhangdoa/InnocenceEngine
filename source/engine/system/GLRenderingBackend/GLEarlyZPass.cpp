#include "GLEarlyZPass.h"
#include "GLRenderingSystemUtilities.h"
#include "../../component/GLRenderingSystemComponent.h"

using namespace GLRenderingSystemNS;

INNO_PRIVATE_SCOPE GLEarlyZPass
{
	void initializeShaders();
	void bindUniformLocations(GLShaderProgramComponent* rhs);

	EntityID m_entityID;

	GLRenderPassComponent* m_GLRPC;

	GLShaderProgramComponent* m_GLSPC;

	ShaderFilePaths m_shaderFilePaths = { "GL//earlyZPassVertex.sf" , "", "GL//earlyZPassFragment.sf" };
}

bool GLEarlyZPass::initialize()
{
	m_entityID = InnoMath::createEntityID();

	m_GLRPC = addGLRenderPassComponent(2, GLRenderingSystemComponent::get().deferredPassFBDesc, GLRenderingSystemComponent::get().deferredPassTextureDesc);

	initializeShaders();

	return true;
}

void GLEarlyZPass::initializeShaders()
{
	// shader programs and shaders
	auto rhs = addGLShaderProgramComponent(m_entityID);

	initializeGLShaderProgramComponent(rhs, m_shaderFilePaths);

	bindUniformLocations(rhs);

	m_GLSPC = rhs;
}

void GLEarlyZPass::bindUniformLocations(GLShaderProgramComponent* rhs)
{
	bindUniformBlock(GLRenderingSystemComponent::get().m_cameraUBO, sizeof(GPassCameraUBOData), rhs->m_program, "cameraUBO", 0);

	bindUniformBlock(GLRenderingSystemComponent::get().m_meshUBO, sizeof(GPassMeshUBOData), rhs->m_program, "meshUBO", 1);
}

bool GLEarlyZPass::update()
{
	glEnable(GL_DEPTH_TEST);
	glDepthFunc(GL_LESS);
	glEnable(GL_DEPTH_CLAMP);
	glDepthMask(GL_TRUE);

	glEnable(GL_CULL_FACE);
	glCullFace(GL_BACK);

	activateRenderPass(m_GLRPC);

	activateShaderProgram(m_GLSPC);

	updateUBO(GLRenderingSystemComponent::get().m_cameraUBO, GLRenderingSystemComponent::get().m_GPassCameraUBOData);

	auto l_queueCopy = GLRenderingSystemComponent::get().m_opaquePassDataQueue;

	while (l_queueCopy.size() > 0)
	{
		auto l_renderPack = l_queueCopy.front();
		if (l_renderPack.meshShapeType != MeshShapeType::CUSTOM)
		{
			glFrontFace(GL_CW);
		}
		else
		{
			glFrontFace(GL_CCW);
		}
		if (l_renderPack.visiblilityType == VisiblilityType::INNO_OPAQUE)
		{
			updateUBO(GLRenderingSystemComponent::get().m_meshUBO, l_renderPack.meshUBOData);

			drawMesh(l_renderPack.indiceSize, l_renderPack.meshPrimitiveTopology, l_renderPack.GLMDC);
		}
		else if (l_renderPack.visiblilityType == VisiblilityType::INNO_EMISSIVE)
		{
			updateUBO(GLRenderingSystemComponent::get().m_meshUBO, l_renderPack.meshUBOData);

			drawMesh(l_renderPack.indiceSize, l_renderPack.meshPrimitiveTopology, l_renderPack.GLMDC);
		}
		else
		{
		}
		l_queueCopy.pop();
	}
	return true;
}

bool GLEarlyZPass::resize()
{
	resizeGLRenderPassComponent(m_GLRPC, GLRenderingSystemComponent::get().deferredPassFBDesc);

	return true;
}

bool GLEarlyZPass::reloadShader()
{
	deleteShaderProgram(m_GLSPC);

	initializeGLShaderProgramComponent(m_GLSPC, m_shaderFilePaths);

	bindUniformLocations(m_GLSPC);

	return true;
}

GLRenderPassComponent * GLEarlyZPass::getGLRPC()
{
	return m_GLRPC;
}