#include "GLLightPass.h"
#include "GLEarlyZPass.h"
#include "GLOpaquePass.h"
#include "GLSSAOBlurPass.h"
#include "GLBRDFLUTPass.h"
#include "GLEnvironmentConvolutionPass.h"
#include "GLEnvironmentPreFilterPass.h"
#include "GLShadowPass.h"
#include "GLLightCullingPass.h"

#include "../../Component/GLRenderingBackendComponent.h"

#include "GLRenderingBackendUtilities.h"

#include "../../ModuleManager/IModuleManager.h"

extern IModuleManager* g_pModuleManager;

using namespace GLRenderingBackendNS;

INNO_PRIVATE_SCOPE GLLightPass
{
	void initializeLightPassShaders();

	EntityID m_entityID;

	GLRenderPassComponent* m_GLRPC;

	GLShaderProgramComponent* m_GLSPC;

	ShaderFilePaths m_shaderFilePaths = { "lightPass.vert/" , "", "", "", "lightPass.frag/" };
}

void GLLightPass::initialize()
{
	m_entityID = InnoMath::createEntityID();

	m_GLRPC = addGLRenderPassComponent(m_entityID, "LightPassGLRPC/");
	m_GLRPC->m_renderPassDesc = GLRenderingBackendComponent::get().m_deferredRenderPassDesc;
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
	// motion vector + UUID + mesh usage type
	activateTexture(
		GLOpaquePass::getGLRPC()->m_GLTDCs[3],
		3);
	// SSAO
	activateTexture(
		GLSSAOBlurPass::getGLRPC()->m_GLTDCs[0],
		4);
	// CSM
	activateTexture(
		GLShadowPass::getGLRPC(0)->m_GLTDCs[0],
		5);
	// point light shadow map
	activateTexture(
		GLShadowPass::getGLRPC(1)->m_GLTDCs[0],
		6);
	// BRDF look-up table 1
	activateTexture(
		GLBRDFLUTPass::getBRDFSplitSumLUT(),
		7);
	// BRDF look-up table 2
	activateTexture(
		GLBRDFLUTPass::getBRDFMSAverageLUT(),
		8);
	// Irradiance env cubemap
	activateTexture(
		GLEnvironmentConvolutionPass::getGLRPC()->m_GLTDCs[0],
		9);
	// pre-filtered specular env cubemap
	activateTexture(
		GLEnvironmentPreFilterPass::getGLRPC()->m_GLTDCs[0],
		10);
	activateTexture(
		GLEarlyZPass::getGLRPC()->m_GLTDCs[0],
		11);

	// culled point light
	glBindImageTexture(0, GLLightCullingPass::getLightGrid()->m_TO, 0, GL_FALSE, 0, GL_READ_ONLY, GL_RGBA16F);

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