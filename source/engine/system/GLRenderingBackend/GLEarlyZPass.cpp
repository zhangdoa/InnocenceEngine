#include "GLEarlyZPass.h"
#include "GLRenderingSystemUtilities.h"
#include "../../component/GLRenderingSystemComponent.h"
#include "../../component/RenderingFrontendSystemComponent.h"

using namespace GLRenderingSystemNS;

INNO_PRIVATE_SCOPE GLEarlyZPass
{
	void initializeShaders();
	void bindUniformLocations(GLShaderProgramComponent* rhs);

	EntityID m_entityID;

	GLRenderPassComponent* m_GLRPC;

	GLShaderProgramComponent* m_GLSPC;

	ShaderFilePaths m_shaderFilePaths = { "GL//earlyZPass.vert" , "", "GL//earlyZPass.frag" };
}

bool GLEarlyZPass::initialize()
{
	m_entityID = InnoMath::createEntityID();

	m_GLRPC = addGLRenderPassComponent(1, GLRenderingSystemComponent::get().deferredPassFBDesc, GLRenderingSystemComponent::get().deferredPassTextureDesc);

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
	bindUniformBlock(GLRenderingSystemComponent::get().m_cameraUBO, sizeof(CameraGPUData), rhs->m_program, "cameraUBO", 0);

	bindUniformBlock(GLRenderingSystemComponent::get().m_meshUBO, sizeof(MeshGPUData), rhs->m_program, "meshUBO", 1);
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

	auto l_queueCopy = RenderingFrontendSystemComponent::get().m_opaquePassGPUDataQueue.getRawData();

	while (l_queueCopy.size() > 0)
	{
		auto l_geometryPassGPUData = l_queueCopy.front();

		if (l_geometryPassGPUData.MDC->m_meshShapeType != MeshShapeType::CUSTOM)
		{
			glFrontFace(GL_CW);
		}
		else
		{
			glFrontFace(GL_CCW);
		}

		updateUBO(GLRenderingSystemComponent::get().m_meshUBO, l_geometryPassGPUData.meshGPUData);

		drawMesh(reinterpret_cast<GLMeshDataComponent*>(l_geometryPassGPUData.MDC));

		l_queueCopy.pop();
	}
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

	bindUniformLocations(m_GLSPC);

	return true;
}

GLRenderPassComponent * GLEarlyZPass::getGLRPC()
{
	return m_GLRPC;
}