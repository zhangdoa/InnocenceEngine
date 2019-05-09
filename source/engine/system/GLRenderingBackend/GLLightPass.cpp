#include "GLLightPass.h"
#include "GLOpaquePass.h"
#include "GLSSAOBlurPass.h"
#include "GLBRDFLUTPass.h"
#include "GLEnvironmentConvolutionPass.h"
#include "GLEnvironmentPreFilterPass.h"
#include "GLShadowPass.h"

#include "../../component/GLRenderingSystemComponent.h"
#include "../../component/RenderingFrontendSystemComponent.h"

#include "GLRenderingSystemUtilities.h"

#include "../ICoreSystem.h"

extern ICoreSystem* g_pCoreSystem;

using namespace GLRenderingSystemNS;

INNO_PRIVATE_SCOPE GLLightPass
{
	void initializeLightPassShaders();

	EntityID m_entityID;

	GLRenderPassComponent* m_GLRPC;

	GLShaderProgramComponent* m_GLSPC;

	ShaderFilePaths m_shaderFilePaths = { "GL//lightPass.vert/" , "", "", "", "GL//lightPass.frag/" };
}

void GLLightPass::initialize()
{
	m_entityID = InnoMath::createEntityID();

	m_GLRPC = addGLRenderPassComponent(m_entityID, "LightPassGLRPC//");
	m_GLRPC->m_renderPassDesc = GLRenderingSystemComponent::get().m_deferredRenderPassDesc;
	m_GLRPC->m_renderPassDesc.useDepthAttachment = true;
	m_GLRPC->m_renderPassDesc.useStencilAttachment = true;

	initializeGLRenderPassComponent(m_GLRPC);

	initializeLightPassShaders();
}

void GLLightPass::initializeLightPassShaders()
{
	// shader programs and shaders
	auto l_GLSPC = addGLShaderProgramComponent(m_entityID);

	initializeGLShaderProgramComponent(l_GLSPC, m_shaderFilePaths);

	m_GLSPC = l_GLSPC;
}

void GLLightPass::update()
{
	glEnable(GL_STENCIL_TEST);
	glStencilOp(GL_KEEP, GL_KEEP, GL_KEEP);
	glStencilFunc(GL_EQUAL, 0x01, 0xFF);
	glStencilMask(0x00);

	// bind to framebuffer
	activateRenderPass(m_GLRPC);

	// 1. opaque objects
	// copy stencil buffer of opaque objects from G-Pass
	copyStencilBuffer(GLOpaquePass::getGLRPC(), m_GLRPC);

	activateShaderProgram(m_GLSPC);

	// Cook-Torrance
	// world space position + metallic
	activateTexture(
		GLOpaquePass::getGLRPC()->m_GLTDCs[0],
		0);
	// world space normal + roughness
	activateTexture(
		GLOpaquePass::getGLRPC()->m_GLTDCs[1],
		1);
	// albedo + ambient occlusion
	activateTexture(
		GLOpaquePass::getGLRPC()->m_GLTDCs[2],
		2);
	// SSAO
	activateTexture(
		GLSSAOBlurPass::getGLRPC()->m_GLTDCs[0],
		3);
	// shadow map
	activateTexture(
		GLShadowPass::getGLRPC(0)->m_GLTDCs[0],
		4);
	// BRDF look-up table 1
	activateTexture(
		GLBRDFLUTPass::getBRDFSplitSumLUT(),
		5);
	// BRDF look-up table 2
	activateTexture(
		GLBRDFLUTPass::getBRDFMSAverageLUT(),
		6);
	// Irradiance env cubemap
	activateTexture(
		GLEnvironmentConvolutionPass::getGLRPC()->m_GLTDCs[0],
		7);
	// pre-filtered specular env cubemap
	activateTexture(
		GLEnvironmentPreFilterPass::getGLRPC()->m_GLTDCs[0],
		8);

	// draw light pass rectangle
	auto l_MDC = getGLMeshDataComponent(MeshShapeType::QUAD);
	drawMesh(l_MDC);

	// 2. draw emissive objects
	glStencilOp(GL_KEEP, GL_KEEP, GL_KEEP);
	glStencilFunc(GL_EQUAL, 0x02, 0xFF);
	glStencilMask(0x00);

	glClear(GL_STENCIL_BUFFER_BIT);

	// copy stencil buffer of emmisive objects from G-Pass
	copyStencilBuffer(GLOpaquePass::getGLRPC(), m_GLRPC);

	// draw light pass rectangle
	drawMesh(l_MDC);

	glDisable(GL_STENCIL_TEST);
}

bool GLLightPass::resize(unsigned int newSizeX, unsigned int newSizeY)
{
	resizeGLRenderPassComponent(m_GLRPC, newSizeX, newSizeY);

	return true;
}

bool GLLightPass::reloadShader()
{
	deleteShaderProgram(m_GLSPC);

	initializeGLShaderProgramComponent(m_GLSPC, m_shaderFilePaths);

	return true;
}

GLRenderPassComponent * GLLightPass::getGLRPC()
{
	return m_GLRPC;
}