#include "GLRenderingSystemUtilities.h"
#include "GLLightRenderingPassUtilities.h"
#include "GLEnvironmentRenderingPassUtilities.h"
#include "../../component/GLLightRenderPassComponent.h"
#include "../../component/GLShadowRenderPassComponent.h"
#include "../../component/GLGeometryRenderPassComponent.h"
#include "../../component/GLRenderingSystemComponent.h"

#include "../ICoreSystem.h"

extern ICoreSystem* g_pCoreSystem;

using namespace GLRenderingSystemNS;

INNO_PRIVATE_SCOPE GLLightRenderingPassUtilities
{
	void initializeLightPassShaders();
	void bindLightPassUniformLocations(GLShaderProgramComponent* rhs);

	EntityID m_entityID;
}

void GLLightRenderingPassUtilities::initialize()
{
	m_entityID = InnoMath::createEntityID();

	GLLightRenderPassComponent::get().m_GLRPC = addGLRenderPassComponent(1, GLRenderingSystemComponent::get().deferredPassFBDesc, GLRenderingSystemComponent::get().deferredPassTextureDesc);

	initializeLightPassShaders();
}

void GLLightRenderingPassUtilities::initializeLightPassShaders()
{
	// shader programs and shaders
	auto l_GLSPC = addGLShaderProgramComponent(m_entityID);

	initializeGLShaderProgramComponent(l_GLSPC, GLLightRenderPassComponent::get().m_shaderFilePaths);

	bindLightPassUniformLocations(l_GLSPC);

	GLLightRenderPassComponent::get().m_GLSPC = l_GLSPC;
}

void GLLightRenderingPassUtilities::bindLightPassUniformLocations(GLShaderProgramComponent* rhs)
{
	updateTextureUniformLocations(rhs->m_program, GLLightRenderPassComponent::get().m_textureUniformNames);

	GLLightRenderPassComponent::get().m_uni_shadowSplitAreas.reserve(4);
	GLLightRenderPassComponent::get().m_uni_dirLightProjs.reserve(4);
	GLLightRenderPassComponent::get().m_uni_dirLightViews.reserve(4);

	for (size_t i = 0; i < 4; i++)
	{
		GLLightRenderPassComponent::get().m_uni_shadowSplitAreas.emplace_back(
			getUniformLocation(rhs->m_program, "uni_shadowSplitAreas[" + std::to_string(i) + "]")
		);
		GLLightRenderPassComponent::get().m_uni_dirLightProjs.emplace_back(
			getUniformLocation(rhs->m_program, "uni_dirLightProjs[" + std::to_string(i) + "]")
		);
		GLLightRenderPassComponent::get().m_uni_dirLightViews.emplace_back(
			getUniformLocation(rhs->m_program, "uni_dirLightViews[" + std::to_string(i) + "]")
		);
	}

	GLLightRenderPassComponent::get().m_uni_viewPos = getUniformLocation(
		rhs->m_program,
		"uni_viewPos");
	GLLightRenderPassComponent::get().m_uni_dirLight_direction = getUniformLocation(
		rhs->m_program,
		"uni_dirLight.direction");
	GLLightRenderPassComponent::get().m_uni_dirLight_luminance = getUniformLocation(
		rhs->m_program,
		"uni_dirLight.luminance");

	GLLightRenderPassComponent::get().m_uni_pointLights_position.reserve(GLRenderingSystemComponent::get().m_maxPointLights);
	GLLightRenderPassComponent::get().m_uni_pointLights_attenuationRadius.reserve(GLRenderingSystemComponent::get().m_maxPointLights);
	GLLightRenderPassComponent::get().m_uni_pointLights_luminance.reserve(GLRenderingSystemComponent::get().m_maxPointLights);

	for (size_t i = 0; i < GLRenderingSystemComponent::get().m_maxPointLights; i++)
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

	GLLightRenderPassComponent::get().m_uni_sphereLights_position.reserve(GLRenderingSystemComponent::get().m_maxSphereLights);
	GLLightRenderPassComponent::get().m_uni_sphereLights_sphereRadius.reserve(GLRenderingSystemComponent::get().m_maxSphereLights);
	GLLightRenderPassComponent::get().m_uni_sphereLights_luminance.reserve(GLRenderingSystemComponent::get().m_maxSphereLights);

	for (size_t i = 0; i < GLRenderingSystemComponent::get().m_maxSphereLights; i++)
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

void GLLightRenderingPassUtilities::update()
{
	glDisable(GL_DEPTH_TEST);

	glEnable(GL_STENCIL_TEST);
	glStencilOp(GL_KEEP, GL_KEEP, GL_KEEP);
	glStencilFunc(GL_EQUAL, 0x01, 0xFF);
	glStencilMask(0x00);

	glDisable(GL_CULL_FACE);

	// bind to framebuffer
	activateRenderPass(GLLightRenderPassComponent::get().m_GLRPC);

	// 1. opaque objects
	// copy stencil buffer of opaque objects from G-Pass
	copyStencilBuffer(GLGeometryRenderPassComponent::get().m_opaquePass_GLRPC, GLLightRenderPassComponent::get().m_GLRPC);

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
	// SSAO
	activateTexture(
		GLGeometryRenderPassComponent::get().m_SSAOBlurPass_GLRPC->m_GLTDCs[0],
		3);
	// shadow map
	activateTexture(
		GLShadowRenderPassComponent::get().m_DirLight_GLRPC->m_GLTDCs[0],
		4);
	// BRDF look-up table 1
	activateTexture(
		GLEnvironmentRenderingPassUtilities::getBRDFSplitSumLUT(),
		5);
	// BRDF look-up table 2
	activateTexture(
		GLEnvironmentRenderingPassUtilities::getBRDFMSAverageLUT(),
		6);
	// Irradiance env cubemap
	activateTexture(
		GLEnvironmentRenderingPassUtilities::getConvPassGLTDC(),
		7);
	// pre-filtered specular env cubemap
	activateTexture(
		GLEnvironmentRenderingPassUtilities::getPreFilterPassGLTDC(),
		8);
#endif

	updateUniform(
		GLLightRenderPassComponent::get().m_uni_isEmissive,
		false);

	auto l_cameraDataPack = g_pCoreSystem->getVisionSystem()->getRenderingFrontend()->getCameraDataPack();
	auto l_sunDataPack = g_pCoreSystem->getVisionSystem()->getRenderingFrontend()->getSunDataPack();
	auto l_CSMDataPack = g_pCoreSystem->getVisionSystem()->getRenderingFrontend()->getCSMDataPack();

	updateUniform(
		GLLightRenderPassComponent::get().m_uni_viewPos,
		l_cameraDataPack.globalPos.x, l_cameraDataPack.globalPos.y, l_cameraDataPack.globalPos.z);

	updateUniform(
		GLLightRenderPassComponent::get().m_uni_dirLight_direction,
		l_sunDataPack.dir.x, l_sunDataPack.dir.y, l_sunDataPack.dir.z);
	updateUniform(
		GLLightRenderPassComponent::get().m_uni_dirLight_luminance,
		l_sunDataPack.luminance.x, l_sunDataPack.luminance.y, l_sunDataPack.luminance.z);

	for (size_t j = 0; j < 4; j++)
	{
		updateUniform(
			GLLightRenderPassComponent::get().m_uni_shadowSplitAreas[j],
			l_CSMDataPack[j].splitCorners.x, l_CSMDataPack[j].splitCorners.y, l_CSMDataPack[j].splitCorners.z, l_CSMDataPack[j].splitCorners.w);

		updateUniform(
			GLLightRenderPassComponent::get().m_uni_dirLightProjs[j],
			l_CSMDataPack[j].p);
		updateUniform(
			GLLightRenderPassComponent::get().m_uni_dirLightViews[j],
			l_CSMDataPack[j].v);
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
	copyStencilBuffer(GLGeometryRenderPassComponent::get().m_opaquePass_GLRPC, GLLightRenderPassComponent::get().m_GLRPC);

	updateUniform(
		GLLightRenderPassComponent::get().m_uni_isEmissive,
		true);

	// draw light pass rectangle
	l_MDC = g_pCoreSystem->getAssetSystem()->getMeshDataComponent(MeshShapeType::QUAD);
	drawMesh(l_MDC);

	glDisable(GL_STENCIL_TEST);
}

bool GLLightRenderingPassUtilities::resize()
{
	resizeGLRenderPassComponent(GLLightRenderPassComponent::get().m_GLRPC, GLRenderingSystemComponent::get().deferredPassFBDesc);

	return true;
}

bool GLLightRenderingPassUtilities::reloadLightPassShaders()
{
	deleteShaderProgram(GLLightRenderPassComponent::get().m_GLSPC);

	initializeGLShaderProgramComponent(GLLightRenderPassComponent::get().m_GLSPC, GLLightRenderPassComponent::get().m_shaderFilePaths);

	bindLightPassUniformLocations(GLLightRenderPassComponent::get().m_GLSPC);

	return true;
}