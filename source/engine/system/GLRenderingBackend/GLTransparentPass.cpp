#include "GLTransparentPass.h"
#include "GLOpaquePass.h"
#include "GLPreTAAPass.h"

#include "GLRenderingSystemUtilities.h"
#include "../../component/GLRenderingSystemComponent.h"
#include "../../component/RenderingFrontendSystemComponent.h"

#include "../ICoreSystem.h"

extern ICoreSystem* g_pCoreSystem;

using namespace GLRenderingSystemNS;

INNO_PRIVATE_SCOPE GLTransparentPass
{
	void initializeShaders();

	EntityID m_entityID;

	GLShaderProgramComponent* m_GLSPC;

	ShaderFilePaths m_shaderFilePaths = { "GL//transparentPass.vert" , "", "GL//transparentPass.frag" };
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

bool GLTransparentPass::update()
{
	glEnable(GL_DEPTH_TEST);
	glDepthFunc(GL_LEQUAL);
	glEnable(GL_DEPTH_CLAMP);

	glEnable(GL_BLEND);
	glBlendFuncSeparate(GL_SRC_ALPHA, GL_SRC1_COLOR, GL_ONE, GL_ZERO);

	glBindFramebuffer(GL_FRAMEBUFFER, GLPreTAAPass::getGLRPC()->m_FBO);
	glBindRenderbuffer(GL_RENDERBUFFER, GLPreTAAPass::getGLRPC()->m_RBO);
	glRenderbufferStorage(GL_RENDERBUFFER, GLPreTAAPass::getGLRPC()->m_GLFrameBufferDesc.renderBufferInternalFormat, GLPreTAAPass::getGLRPC()->m_GLFrameBufferDesc.sizeX, GLPreTAAPass::getGLRPC()->m_GLFrameBufferDesc.sizeY);
	glViewport(0, 0, GLPreTAAPass::getGLRPC()->m_GLFrameBufferDesc.sizeX, GLPreTAAPass::getGLRPC()->m_GLFrameBufferDesc.sizeY);

	copyDepthBuffer(GLOpaquePass::getGLRPC(), GLPreTAAPass::getGLRPC());

	activateShaderProgram(m_GLSPC);

	updateUniform(
		0,
		RenderingFrontendSystemComponent::get().m_cameraGPUData.globalPos);

	updateUniform(
		1,
		RenderingFrontendSystemComponent::get().m_sunGPUData.dir);
	updateUniform(
		2,
		RenderingFrontendSystemComponent::get().m_sunGPUData.luminance);

	while (RenderingFrontendSystemComponent::get().m_transparentPassGPUDataQueue.size() > 0)
	{
		GeometryPassGPUData l_geometryPassGPUData = {};

		if (RenderingFrontendSystemComponent::get().m_transparentPassGPUDataQueue.tryPop(l_geometryPassGPUData))
		{
			if (l_geometryPassGPUData.MDC->m_meshShapeType != MeshShapeType::CUSTOM)
			{
				glFrontFace(GL_CW);
			}
			else
			{
				glFrontFace(GL_CCW);
			}
			updateUBO(GLRenderingSystemComponent::get().m_meshUBO, l_geometryPassGPUData.meshGPUData);

			vec4 l_albedo = vec4(
				l_geometryPassGPUData.materialGPUData.customMaterial.albedo_r,
				l_geometryPassGPUData.materialGPUData.customMaterial.albedo_g,
				l_geometryPassGPUData.materialGPUData.customMaterial.albedo_b,
				l_geometryPassGPUData.materialGPUData.customMaterial.alpha
			);
			vec4 l_TR = vec4(
				l_geometryPassGPUData.materialGPUData.customMaterial.thickness,
				l_geometryPassGPUData.materialGPUData.customMaterial.roughness,
				0.0f, 0.0f
			);
			updateUniform(3, l_albedo);
			updateUniform(4, l_TR);

			drawMesh(reinterpret_cast<GLMeshDataComponent*>(l_geometryPassGPUData.MDC));
		}
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