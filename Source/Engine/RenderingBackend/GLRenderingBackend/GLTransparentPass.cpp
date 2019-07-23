#include "GLTransparentPass.h"
#include "GLOpaquePass.h"

#include "GLRenderingBackendUtilities.h"
#include "../../Component/GLRenderingBackendComponent.h"

#include "../../ModuleManager/IModuleManager.h"

extern IModuleManager* g_pModuleManager;

using namespace GLRenderingBackendNS;

namespace GLTransparentPass
{
	void initializeShaders();

	EntityID m_entityID;

	GLShaderProgramComponent* m_GLSPC;

	ShaderFilePaths m_shaderFilePaths = { "transparentPass.vert/", "", "", "", "transparentPass.frag/" };
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
	glEnable(GL_DEPTH_TEST);
	glDepthFunc(GL_LESS);
	glDepthMask(GL_FALSE);

	glEnable(GL_DEPTH_CLAMP);

	glEnable(GL_BLEND);
	glBlendFuncSeparate(GL_SRC_ALPHA, GL_SRC1_COLOR, GL_ONE, GL_ZERO);

	bindRenderPass(prePassGLRPC);
	copyDepthBuffer(GLOpaquePass::getGLRPC(), prePassGLRPC);

	activateShaderProgram(m_GLSPC);

	unsigned int l_offset = 0;

	for (unsigned int i = 0; i < g_pModuleManager->getRenderingFrontend()->getTransparentPassDrawCallCount(); i++)
	{
		auto l_transparentPassGPUData = g_pModuleManager->getRenderingFrontend()->getTransparentPassGPUData()[i];

		bindUBO(GLRenderingBackendComponent::get().m_meshUBO, 1, l_offset * sizeof(MeshGPUData), sizeof(MeshGPUData));
		bindUBO(GLRenderingBackendComponent::get().m_materialUBO, 2, l_offset * sizeof(MaterialGPUData), sizeof(MaterialGPUData));

		drawMesh(reinterpret_cast<GLMeshDataComponent*>(l_transparentPassGPUData.mesh));

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