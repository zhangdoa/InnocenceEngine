#include "GLLightPass.h"
#include "GLOpaquePass.h"
#include "GLSSAOBlurPass.h"
#include "GLEnvironmentRenderPass.h"
#include "GLShadowRenderPass.h"

#include "../../component/GLRenderingSystemComponent.h"
#include "../../component/RenderingFrontendSystemComponent.h"

#include "GLRenderingSystemUtilities.h"

#include "../ICoreSystem.h"

extern ICoreSystem* g_pCoreSystem;

using namespace GLRenderingSystemNS;

INNO_PRIVATE_SCOPE GLLightPass
{
	void initializeLightPassShaders();
	void bindLightPassUniformLocations(GLShaderProgramComponent* rhs);

	EntityID m_entityID;

	GLRenderPassComponent* m_GLRPC;

	GLShaderProgramComponent* m_GLSPC;

	ShaderFilePaths m_shaderFilePaths = { "GL//lightPass.vert" , "", "GL//lightPass.frag" };

	std::vector<std::string> m_textureUniformNames =
	{
		"uni_opaquePassRT0",
		"uni_opaquePassRT1",
		"uni_opaquePassRT2",
		"uni_SSAOBlurPassRT0",
		"uni_directionalLightShadowMap",
		"uni_brdfLUT",
		"uni_brdfMSLUT",
		"uni_irradianceMap",
		"uni_preFiltedMap"
	};

	std::vector<GLuint> m_uni_shadowSplitAreas;
	std::vector<GLuint> m_uni_dirLightProjs;
	std::vector<GLuint> m_uni_dirLightViews;

	GLuint m_uni_viewPos;
	GLuint m_uni_dirLight_direction;
	GLuint m_uni_dirLight_luminance;
	GLuint m_uni_dirLight_rot;

	std::vector<GLuint> m_uni_pointLights_position;
	std::vector<GLuint> m_uni_pointLights_attenuationRadius;
	std::vector<GLuint> m_uni_pointLights_luminance;

	std::vector<GLuint> m_uni_sphereLights_position;
	std::vector<GLuint> m_uni_sphereLights_sphereRadius;
	std::vector<GLuint> m_uni_sphereLights_luminance;

	GLuint m_uni_isEmissive;
}

void GLLightPass::initialize()
{
	m_entityID = InnoMath::createEntityID();

	m_GLRPC = addGLRenderPassComponent(1, GLRenderingSystemComponent::get().deferredPassFBDesc, GLRenderingSystemComponent::get().deferredPassTextureDesc);

	initializeLightPassShaders();
}

void GLLightPass::initializeLightPassShaders()
{
	// shader programs and shaders
	auto l_GLSPC = addGLShaderProgramComponent(m_entityID);

	initializeGLShaderProgramComponent(l_GLSPC, m_shaderFilePaths);

	bindLightPassUniformLocations(l_GLSPC);

	m_GLSPC = l_GLSPC;
}

void GLLightPass::bindLightPassUniformLocations(GLShaderProgramComponent* rhs)
{
	updateTextureUniformLocations(rhs->m_program, m_textureUniformNames);

	m_uni_shadowSplitAreas.reserve(4);
	m_uni_dirLightProjs.reserve(4);
	m_uni_dirLightViews.reserve(4);

	for (size_t i = 0; i < 4; i++)
	{
		m_uni_shadowSplitAreas.emplace_back(
			getUniformLocation(rhs->m_program, "uni_shadowSplitAreas[" + std::to_string(i) + "]")
		);
		m_uni_dirLightProjs.emplace_back(
			getUniformLocation(rhs->m_program, "uni_dirLightProjs[" + std::to_string(i) + "]")
		);
		m_uni_dirLightViews.emplace_back(
			getUniformLocation(rhs->m_program, "uni_dirLightViews[" + std::to_string(i) + "]")
		);
	}

	m_uni_viewPos = getUniformLocation(
		rhs->m_program,
		"uni_viewPos");
	m_uni_dirLight_direction = getUniformLocation(
		rhs->m_program,
		"uni_dirLight.direction");
	m_uni_dirLight_luminance = getUniformLocation(
		rhs->m_program,
		"uni_dirLight.luminance");

	m_uni_pointLights_position.reserve(RenderingFrontendSystemComponent::get().m_maxPointLights);
	m_uni_pointLights_attenuationRadius.reserve(RenderingFrontendSystemComponent::get().m_maxPointLights);
	m_uni_pointLights_luminance.reserve(RenderingFrontendSystemComponent::get().m_maxPointLights);

	for (size_t i = 0; i < RenderingFrontendSystemComponent::get().m_maxPointLights; i++)
	{
		m_uni_pointLights_position.emplace_back(
			getUniformLocation(rhs->m_program, "uni_pointLights[" + std::to_string(i) + "].position")
		);
		m_uni_pointLights_attenuationRadius.emplace_back(
			getUniformLocation(rhs->m_program, "uni_pointLights[" + std::to_string(i) + "].attenuationRadius")
		);
		m_uni_pointLights_luminance.emplace_back(
			getUniformLocation(rhs->m_program, "uni_pointLights[" + std::to_string(i) + "].luminance")
		);
	}

	m_uni_sphereLights_position.reserve(RenderingFrontendSystemComponent::get().m_maxSphereLights);
	m_uni_sphereLights_sphereRadius.reserve(RenderingFrontendSystemComponent::get().m_maxSphereLights);
	m_uni_sphereLights_luminance.reserve(RenderingFrontendSystemComponent::get().m_maxSphereLights);

	for (size_t i = 0; i < RenderingFrontendSystemComponent::get().m_maxSphereLights; i++)
	{
		m_uni_sphereLights_position.emplace_back(
			getUniformLocation(rhs->m_program, "uni_sphereLights[" + std::to_string(i) + "].position")
		);
		m_uni_sphereLights_sphereRadius.emplace_back(
			getUniformLocation(rhs->m_program, "uni_sphereLights[" + std::to_string(i) + "].sphereRadius")
		);
		m_uni_sphereLights_luminance.emplace_back(
			getUniformLocation(rhs->m_program, "uni_sphereLights[" + std::to_string(i) + "].luminance")
		);
	}

	m_uni_isEmissive = getUniformLocation(
		rhs->m_program,
		"uni_isEmissive");
}

void GLLightPass::update()
{
	glDisable(GL_DEPTH_TEST);

	glEnable(GL_STENCIL_TEST);
	glStencilOp(GL_KEEP, GL_KEEP, GL_KEEP);
	glStencilFunc(GL_EQUAL, 0x01, 0xFF);
	glStencilMask(0x00);

	glDisable(GL_CULL_FACE);

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
		GLShadowRenderPass::getGLRPC(0)->m_GLTDCs[0],
		4);
	// BRDF look-up table 1
	activateTexture(
		GLEnvironmentRenderPass::getBRDFSplitSumLUT(),
		5);
	// BRDF look-up table 2
	activateTexture(
		GLEnvironmentRenderPass::getBRDFMSAverageLUT(),
		6);
	// Irradiance env cubemap
	activateTexture(
		GLEnvironmentRenderPass::getConvPassGLTDC(),
		7);
	// pre-filtered specular env cubemap
	activateTexture(
		GLEnvironmentRenderPass::getPreFilterPassGLTDC(),
		8);

	updateUniform(
		m_uni_isEmissive,
		false);

	updateUniform(
		m_uni_viewPos,
		RenderingFrontendSystemComponent::get().m_cameraGPUData.globalPos);

	updateUniform(
		m_uni_dirLight_direction,
		RenderingFrontendSystemComponent::get().m_sunGPUData.dir);
	updateUniform(
		m_uni_dirLight_luminance,
		RenderingFrontendSystemComponent::get().m_sunGPUData.luminance);

	if (RenderingFrontendSystemComponent::get().m_CSMGPUDataVector.size())
	{
		for (size_t j = 0; j < 4; j++)
		{
			updateUniform(
				m_uni_shadowSplitAreas[j],
				RenderingFrontendSystemComponent::get().m_CSMGPUDataVector[j].splitCorners);
			updateUniform(
				m_uni_dirLightProjs[j],
				RenderingFrontendSystemComponent::get().m_CSMGPUDataVector[j].p);
			updateUniform(
				m_uni_dirLightViews[j],
				RenderingFrontendSystemComponent::get().m_CSMGPUDataVector[j].v);
		}
	}

	for (size_t i = 0; i < RenderingFrontendSystemComponent::get().m_pointLightGPUDataVector.size(); i++)
	{
		auto l_pos = RenderingFrontendSystemComponent::get().m_pointLightGPUDataVector[i].pos;
		auto l_luminance = RenderingFrontendSystemComponent::get().m_pointLightGPUDataVector[i].luminance;
		auto l_attenuationRadius = RenderingFrontendSystemComponent::get().m_pointLightGPUDataVector[i].attenuationRadius;

		updateUniform(
			m_uni_pointLights_position[i],
			l_pos);
		updateUniform(
			m_uni_pointLights_attenuationRadius[i],
			l_attenuationRadius);
		updateUniform(
			m_uni_pointLights_luminance[i],
			l_luminance);
	}

	for (size_t i = 0; i < RenderingFrontendSystemComponent::get().m_sphereLightGPUDataVector.size(); i++)
	{
		auto l_pos = RenderingFrontendSystemComponent::get().m_sphereLightGPUDataVector[i].pos;
		auto l_luminance = RenderingFrontendSystemComponent::get().m_sphereLightGPUDataVector[i].luminance;
		auto l_sphereRadius = RenderingFrontendSystemComponent::get().m_sphereLightGPUDataVector[i].sphereRadius;

		updateUniform(
			m_uni_sphereLights_position[i],
			l_pos);
		updateUniform(
			m_uni_sphereLights_sphereRadius[i],
			l_sphereRadius);
		updateUniform(
			m_uni_sphereLights_luminance[i],
			l_luminance);
	}

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

	updateUniform(
		m_uni_isEmissive,
		true);

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

	bindLightPassUniformLocations(m_GLSPC);

	return true;
}

GLRenderPassComponent * GLLightPass::getGLRPC()
{
	return m_GLRPC;
}