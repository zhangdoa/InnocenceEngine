#include "GLRenderingSystemUtilities.h"
#include "GLLightRenderingPassUtilities.h"
#include "../component/GLLightRenderPassComponent.h"
#include "../component/GLShadowRenderPassComponent.h"
#include "../component/GLEnvironmentRenderPassComponent.h"
#include "../component/GLGeometryRenderPassComponent.h"
#include "../component/GameSystemComponent.h"
#include "../component/GLRenderingSystemComponent.h"

#include "ICoreSystem.h"

extern ICoreSystem* g_pCoreSystem;

INNO_PRIVATE_SCOPE GLRenderingSystemNS
{
	void initializeLightPassShaders();
	void bindLightPassUniformLocations(GLShaderProgramComponent* rhs);
}

void GLRenderingSystemNS::initializeLightPass()
{
	GLLightRenderPassComponent::get().m_GLRPC = addGLRenderPassComponent(1, GLRenderingSystemComponent::get().deferredPassFBDesc, GLRenderingSystemComponent::get().deferredPassTextureDesc);

	initializeLightPassShaders();
}

void GLRenderingSystemNS::initializeLightPassShaders()
{
	// shader programs and shaders
	auto l_GLSPC = addGLShaderProgramComponent(0);

	initializeGLShaderProgramComponent(l_GLSPC, GLLightRenderPassComponent::get().m_shaderFilePaths);

	bindLightPassUniformLocations(l_GLSPC);

	GLLightRenderPassComponent::get().m_GLSPC = l_GLSPC;
}

void GLRenderingSystemNS::bindLightPassUniformLocations(GLShaderProgramComponent* rhs)
{
	updateTextureUniformLocations(rhs->m_program, GLLightRenderPassComponent::get().m_textureUniformNames);

	for (size_t i = 0; i < 4; i++)
	{
		GLLightRenderPassComponent::get().m_uni_shadowSplitAreas.emplace_back(
			getUniformLocation(rhs->m_program, "uni_shadowSplitAreas[" + std::to_string(i) + "]")
		);
	}
	for (size_t i = 0; i < 4; i++)
	{
		GLLightRenderPassComponent::get().m_uni_dirLightProjs.emplace_back(
			getUniformLocation(rhs->m_program, "uni_dirLightProjs[" + std::to_string(i) + "]")
		);
	}
	GLLightRenderPassComponent::get().m_uni_viewPos = getUniformLocation(
		rhs->m_program,
		"uni_viewPos");
	GLLightRenderPassComponent::get().m_uni_dirLight_direction = getUniformLocation(
		rhs->m_program,
		"uni_dirLight.direction");
	GLLightRenderPassComponent::get().m_uni_dirLight_color = getUniformLocation(
		rhs->m_program,
		"uni_dirLight.color");
	GLLightRenderPassComponent::get().m_uni_dirLight_rot = getUniformLocation(
		rhs->m_program,
		"uni_dirLight_rot");
	
	for (size_t i = 0; i < GameSystemComponent::get().m_PointLightComponents.size(); i++)
	{
		GLLightRenderPassComponent::get().m_uni_pointLights_position.emplace_back(
			getUniformLocation(rhs->m_program, "uni_pointLights[" + std::to_string(i) + "].position")
		);
		GLLightRenderPassComponent::get().m_uni_pointLights_attenuationRadius.emplace_back(
			getUniformLocation(rhs->m_program, "uni_pointLights[" + std::to_string(i) + "].attenuationRadius")
		);
		GLLightRenderPassComponent::get().m_uni_pointLights_luminance.emplace_back(
			getUniformLocation(rhs->m_program, "uni_pointLights[" + std::to_string(i) + "].luminance")
		);
	}

	for (size_t i = 0; i < GameSystemComponent::get().m_SphereLightComponents.size(); i++)
	{
		GLLightRenderPassComponent::get().m_uni_sphereLights_position.emplace_back(
			getUniformLocation(rhs->m_program, "uni_sphereLights[" + std::to_string(i) + "].position")
		);
		GLLightRenderPassComponent::get().m_uni_sphereLights_sphereRadius.emplace_back(
			getUniformLocation(rhs->m_program, "uni_sphereLights[" + std::to_string(i) + "].sphereRadius")
		);
		GLLightRenderPassComponent::get().m_uni_sphereLights_luminance.emplace_back(
			getUniformLocation(rhs->m_program, "uni_sphereLights[" + std::to_string(i) + "].luminance")
		);
	}

	GLLightRenderPassComponent::get().m_uni_isEmissive = getUniformLocation(
		rhs->m_program,
		"uni_isEmissive");
}

void GLRenderingSystemNS::updateLightPass()
{
	glDisable(GL_DEPTH_TEST);

	glEnable(GL_STENCIL_TEST);
	glStencilOp(GL_KEEP, GL_KEEP, GL_KEEP);
	glStencilFunc(GL_EQUAL, 0x01, 0xFF);
	glStencilMask(0x00);

	glDisable(GL_CULL_FACE);

	// bind to framebuffer
	auto l_FBC = GLLightRenderPassComponent::get().m_GLRPC->m_GLFBC;
	bindFBC(l_FBC);

	// 1. opaque objects
	// copy stencil buffer of opaque objects from G-Pass
	copyStencilBuffer(GLGeometryRenderPassComponent::get().m_opaquePass_GLRPC->m_GLFBC, l_FBC);

	activateShaderProgram(GLLightRenderPassComponent::get().m_GLSPC);

#ifdef CookTorrance
	// Cook-Torrance
	// world space position + metallic
	activateTexture(
		GLGeometryRenderPassComponent::get().m_opaquePass_GLRPC->m_GLTDCs[0],
		0);
	// world space normal + roughness
	activateTexture(
		GLGeometryRenderPassComponent::get().m_opaquePass_GLRPC->m_GLTDCs[1],
		1);
	// albedo + ambient occlusion
	activateTexture(
		GLGeometryRenderPassComponent::get().m_opaquePass_GLRPC->m_GLTDCs[2],
		2);
	// motion vector + transparency
	activateTexture(
		GLGeometryRenderPassComponent::get().m_opaquePass_GLRPC->m_GLTDCs[3],
		3);
	// SSAO
	activateTexture(
		GLGeometryRenderPassComponent::get().m_SSAOPass_GLRPC->m_GLTDCs[0],
		4);
	// shadow map 0
	activateTexture(
		GLShadowRenderPassComponent::get().m_GLRPCs[0]->m_GLTDCs[0],
		5);
	// shadow map 1
	activateTexture(
		GLShadowRenderPassComponent::get().m_GLRPCs[1]->m_GLTDCs[0],
		6);
	// shadow map 2
	activateTexture(
		GLShadowRenderPassComponent::get().m_GLRPCs[2]->m_GLTDCs[0],
		7);
	// shadow map 3
	activateTexture(
		GLShadowRenderPassComponent::get().m_GLRPCs[3]->m_GLTDCs[0],
		8);
	// BRDF look-up table 1
	activateTexture(
		GLEnvironmentRenderPassComponent::get().m_BRDFSplitSumLUTPassGLTDC,
		9);
	// BRDF look-up table 2
	activateTexture(
		GLEnvironmentRenderPassComponent::get().m_BRDFMSAverageLUTPassGLTDC,
		10);
	// Irradiance env cubemap
	activateTexture(
		GLEnvironmentRenderPassComponent::get().m_convPassGLTDC,
		11);
	// pre-filtered specular env cubemap
	activateTexture(
		GLEnvironmentRenderPassComponent::get().m_preFilterPassGLTDC,
		12);
#endif

	updateUniform(
		GLLightRenderPassComponent::get().m_uni_isEmissive,
		false);

	updateUniform(
		GLLightRenderPassComponent::get().m_uni_viewPos,
		GLRenderingSystemComponent::get().m_CamGlobalPos.x, GLRenderingSystemComponent::get().m_CamGlobalPos.y, GLRenderingSystemComponent::get().m_CamGlobalPos.z);

	updateUniform(
		GLLightRenderPassComponent::get().m_uni_dirLight_direction,
		GLRenderingSystemComponent::get().m_sunDir.x, GLRenderingSystemComponent::get().m_sunDir.y, GLRenderingSystemComponent::get().m_sunDir.z);
	updateUniform(
		GLLightRenderPassComponent::get().m_uni_dirLight_color,
		GLRenderingSystemComponent::get().m_sunColor.x, GLRenderingSystemComponent::get().m_sunColor.y, GLRenderingSystemComponent::get().m_sunColor.z);
	updateUniform(
		GLLightRenderPassComponent::get().m_uni_dirLight_rot,
		GLRenderingSystemComponent::get().m_sunRot);

	for (size_t j = 0; j < 4; j++)
	{
		updateUniform(
			GLLightRenderPassComponent::get().m_uni_dirLightProjs[j],
			GLRenderingSystemComponent::get().m_CSMProjs[j]);
	}
	for (size_t j = 0; j < 4; j++)
	{
		updateUniform(
			GLLightRenderPassComponent::get().m_uni_shadowSplitAreas[j],
			GLRenderingSystemComponent::get().m_CSMSplitCorners[j].x, GLRenderingSystemComponent::get().m_CSMSplitCorners[j].y, GLRenderingSystemComponent::get().m_CSMSplitCorners[j].z, GLRenderingSystemComponent::get().m_CSMSplitCorners[j].w);
	}
	for (size_t i = 0; i < GLRenderingSystemComponent::get().m_PointLightDatas.size(); i++)
	{
		auto l_pos = GLRenderingSystemComponent::get().m_PointLightDatas[i].pos;
		auto l_luminance = GLRenderingSystemComponent::get().m_PointLightDatas[i].luminance;
		auto l_attenuationRadius = GLRenderingSystemComponent::get().m_PointLightDatas[i].attenuationRadius;

		updateUniform(
			GLLightRenderPassComponent::get().m_uni_pointLights_position[i],
			l_pos.x, l_pos.y, l_pos.z);
		updateUniform(
			GLLightRenderPassComponent::get().m_uni_pointLights_attenuationRadius[i],
			l_attenuationRadius);
		updateUniform(
			GLLightRenderPassComponent::get().m_uni_pointLights_luminance[i],
			l_luminance.x, l_luminance.y, l_luminance.z);
	}

	for (size_t i = 0; i < GLRenderingSystemComponent::get().m_SphereLightDatas.size(); i++)
	{
		auto l_pos = GLRenderingSystemComponent::get().m_SphereLightDatas[i].pos;
		auto l_luminance = GLRenderingSystemComponent::get().m_SphereLightDatas[i].luminance;
		auto l_sphereRadius = GLRenderingSystemComponent::get().m_SphereLightDatas[i].sphereRadius;

		updateUniform(
			GLLightRenderPassComponent::get().m_uni_sphereLights_position[i],
			l_pos.x, l_pos.y, l_pos.z);
		updateUniform(
			GLLightRenderPassComponent::get().m_uni_sphereLights_sphereRadius[i],
			l_sphereRadius);
		updateUniform(
			GLLightRenderPassComponent::get().m_uni_sphereLights_luminance[i],
			l_luminance.x, l_luminance.y, l_luminance.z);
	}

	// draw light pass rectangle
	auto l_MDC = g_pCoreSystem->getAssetSystem()->getMeshDataComponent(MeshShapeType::QUAD);
	drawMesh(l_MDC);

	// 2. draw emissive objects
	glStencilOp(GL_KEEP, GL_KEEP, GL_KEEP);
	glStencilFunc(GL_EQUAL, 0x02, 0xFF);
	glStencilMask(0x00);

	glClear(GL_STENCIL_BUFFER_BIT);

	// copy stencil buffer of emmisive objects from G-Pass
	copyStencilBuffer(GLGeometryRenderPassComponent::get().m_opaquePass_GLRPC->m_GLFBC, l_FBC);

	updateUniform(
		GLLightRenderPassComponent::get().m_uni_isEmissive,
		true);

	// draw light pass rectangle
	l_MDC = g_pCoreSystem->getAssetSystem()->getMeshDataComponent(MeshShapeType::QUAD);
	drawMesh(l_MDC);

	glDisable(GL_STENCIL_TEST);
}

bool GLRenderingSystemNS::resizeLightPass()
{
	resizeGLRenderPassComponent(GLLightRenderPassComponent::get().m_GLRPC, GLRenderingSystemComponent::get().deferredPassFBDesc);

	return true;
}

bool GLRenderingSystemNS::reloadLightPassShaders()
{
	deleteShaderProgram(GLLightRenderPassComponent::get().m_GLSPC);

	initializeGLShaderProgramComponent(GLLightRenderPassComponent::get().m_GLSPC, GLLightRenderPassComponent::get().m_shaderFilePaths);

	bindLightPassUniformLocations(GLLightRenderPassComponent::get().m_GLSPC);

	return true;
}