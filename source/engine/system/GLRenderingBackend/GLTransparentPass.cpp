#include "GLTransparentPass.h"
#include "GLOpaquePass.h"
#include "GLLightPass.h"

#include "GLRenderingSystemUtilities.h"
#include "../../component/GLRenderingSystemComponent.h"

#include "../ICoreSystem.h"

extern ICoreSystem* g_pCoreSystem;

using namespace GLRenderingSystemNS;

INNO_PRIVATE_SCOPE GLTransparentPass
{
	void initializeShaders();
	void bindUniformLocations(GLShaderProgramComponent* rhs);

	EntityID m_entityID;

	GLRenderPassComponent* m_GLRPC;

	GLShaderProgramComponent* m_GLSPC;

	ShaderFilePaths m_shaderFilePaths = { "GL//transparentPass.vert" , "", "GL//transparentPass.frag" };

	GLuint m_uni_albedo;
	GLuint m_uni_TR;
	GLuint m_uni_viewPos;
	GLuint m_uni_dirLight_direction;
	GLuint m_uni_dirLight_color;

	CameraDataPack m_cameraDataPack;
	SunDataPack m_sunDataPack;
}

bool GLTransparentPass::initialize()
{
	m_entityID = InnoMath::createEntityID();

	m_GLRPC = addGLRenderPassComponent(1, GLRenderingSystemComponent::get().deferredPassFBDesc, GLRenderingSystemComponent::get().deferredPassTextureDesc);

	initializeShaders();

	return true;
}

void GLTransparentPass::initializeShaders()
{
	// shader programs and shaders
	auto rhs = addGLShaderProgramComponent(m_entityID);

	initializeGLShaderProgramComponent(rhs, m_shaderFilePaths);

	bindUniformLocations(rhs);

	m_GLSPC = rhs;
}

void GLTransparentPass::bindUniformLocations(GLShaderProgramComponent* rhs)
{
	bindUniformBlock(GLRenderingSystemComponent::get().m_cameraUBO, sizeof(GPassCameraUBOData), rhs->m_program, "cameraUBO", 0);

	bindUniformBlock(GLRenderingSystemComponent::get().m_meshUBO, sizeof(GPassMeshUBOData), rhs->m_program, "meshUBO", 1);

	m_uni_albedo = getUniformLocation(
		rhs->m_program,
		"uni_albedo");
	m_uni_TR = getUniformLocation(
		rhs->m_program,
		"uni_TR");
	m_uni_viewPos = getUniformLocation(
		rhs->m_program,
		"uni_viewPos");
	m_uni_dirLight_direction = getUniformLocation(
		rhs->m_program,
		"uni_dirLight.direction");
	m_uni_dirLight_color = getUniformLocation(
		rhs->m_program,
		"uni_dirLight.color");
}

bool GLTransparentPass::update()
{
	// copy camera data pack for local scope
	auto l_cameraDataPack = g_pCoreSystem->getVisionSystem()->getRenderingFrontend()->getCameraDataPack();
	if (l_cameraDataPack.has_value())
	{
		m_cameraDataPack = l_cameraDataPack.value();
	}

	// copy sun data pack for local scope
	auto l_sunDataPack = g_pCoreSystem->getVisionSystem()->getRenderingFrontend()->getSunDataPack();
	if (l_sunDataPack.has_value())
	{
		m_sunDataPack = l_sunDataPack.value();
	}

	glEnable(GL_DEPTH_TEST);
	glDepthFunc(GL_LEQUAL);
	glEnable(GL_DEPTH_CLAMP);

	glEnable(GL_BLEND);
	glBlendFuncSeparate(GL_SRC_ALPHA, GL_SRC1_COLOR, GL_ONE, GL_ZERO);

	glBindFramebuffer(GL_FRAMEBUFFER, GLLightPass::getGLRPC()->m_FBO);
	glBindRenderbuffer(GL_RENDERBUFFER, GLLightPass::getGLRPC()->m_RBO);
	glRenderbufferStorage(GL_RENDERBUFFER, GLLightPass::getGLRPC()->m_GLFrameBufferDesc.renderBufferInternalFormat, GLLightPass::getGLRPC()->m_GLFrameBufferDesc.sizeX, GLLightPass::getGLRPC()->m_GLFrameBufferDesc.sizeY);
	glViewport(0, 0, GLLightPass::getGLRPC()->m_GLFrameBufferDesc.sizeX, GLLightPass::getGLRPC()->m_GLFrameBufferDesc.sizeY);

	copyDepthBuffer(GLOpaquePass::getGLRPC(), GLLightPass::getGLRPC());

	//activateRenderPass(m_GLRPC);
	//copyDepthBuffer(GLOpaquePass::getGLRPC(), m_GLRPC);

	activateShaderProgram(m_GLSPC);

	updateUBO(GLRenderingSystemComponent::get().m_cameraUBO, GLRenderingSystemComponent::get().m_GPassCameraUBOData);

	updateUniform(
		m_uni_viewPos,
		m_cameraDataPack.globalPos.x, m_cameraDataPack.globalPos.y, m_cameraDataPack.globalPos.z);
	updateUniform(
		m_uni_dirLight_direction,
		m_sunDataPack.dir.x, m_sunDataPack.dir.y, m_sunDataPack.dir.z);
	updateUniform(
		m_uni_dirLight_color,
		m_sunDataPack.luminance.x, m_sunDataPack.luminance.y, m_sunDataPack.luminance.z);

	while (GLRenderingSystemComponent::get().m_transparentPassDataQueue.size() > 0)
	{
		auto l_renderPack = GLRenderingSystemComponent::get().m_transparentPassDataQueue.front();

		updateUBO(GLRenderingSystemComponent::get().m_meshUBO, l_renderPack.meshUBOData);

		updateUniform(m_uni_albedo, l_renderPack.meshCustomMaterial.albedo_r, l_renderPack.meshCustomMaterial.albedo_g, l_renderPack.meshCustomMaterial.albedo_b, l_renderPack.meshCustomMaterial.alpha);
		updateUniform(m_uni_TR, l_renderPack.meshCustomMaterial.thickness, l_renderPack.meshCustomMaterial.roughness, 0.0f, 0.0f);

		drawMesh(l_renderPack.indiceSize, l_renderPack.meshPrimitiveTopology, l_renderPack.GLMDC);

		GLRenderingSystemComponent::get().m_transparentPassDataQueue.pop();
	}

	glDisable(GL_BLEND);
	glDisable(GL_DEPTH_CLAMP);
	glDisable(GL_DEPTH_TEST);

	return true;
}

bool GLTransparentPass::resize(unsigned int newSizeX, unsigned int newSizeY)
{
	resizeGLRenderPassComponent(m_GLRPC, newSizeX, newSizeY);

	return true;
}

bool GLTransparentPass::reloadShader()
{
	deleteShaderProgram(m_GLSPC);

	initializeGLShaderProgramComponent(m_GLSPC, m_shaderFilePaths);

	bindUniformLocations(m_GLSPC);

	return true;
}

GLRenderPassComponent * GLTransparentPass::getGLRPC()
{
	return m_GLRPC;
}