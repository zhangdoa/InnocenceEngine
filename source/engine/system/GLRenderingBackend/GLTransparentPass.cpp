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

bool GLTransparentPass::update()
{
	auto l_GLRPC = GLPreTAAPass::getGLRPC();
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

	for (unsigned int i = 0; i < RenderingFrontendSystemComponent::get().m_transparentPassDrawcallCount; i++)
	{
		auto l_transparentPassGPUData = RenderingFrontendSystemComponent::get().m_transparentPassGPUDatas[i];
		auto l_meshGPUData = RenderingFrontendSystemComponent::get().m_transparentPassMeshGPUDatas[l_transparentPassGPUData.meshGPUDataIndex];
		auto l_materialGPUData = RenderingFrontendSystemComponent::get().m_transparentPassMaterialGPUDatas[l_transparentPassGPUData.materialGPUDataIndex];

		vec4 l_albedo = vec4(
			l_materialGPUData.customMaterial.albedo_r,
			l_materialGPUData.customMaterial.albedo_g,
			l_materialGPUData.customMaterial.albedo_b,
			l_materialGPUData.customMaterial.alpha
		);
		vec4 l_TR = vec4(
			l_materialGPUData.customMaterial.thickness,
			l_materialGPUData.customMaterial.roughness,
			0.0f, 0.0f
		);
		updateUniform(3, l_albedo);
		updateUniform(4, l_TR);

		updateUBO(GLRenderingSystemComponent::get().m_meshUBO, l_meshGPUData);
		updateUBO(GLRenderingSystemComponent::get().m_materialUBO, l_materialGPUData);

		drawMesh(reinterpret_cast<GLMeshDataComponent*>(l_transparentPassGPUData.MDC));
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