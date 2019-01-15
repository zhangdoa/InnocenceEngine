#include "GLRenderingSystemUtilities.h"
#include "GLGeometryRenderingPassUtilities.h"

#include "../component/GLGeometryRenderPassComponent.h"
#include "../component/GLTerrainRenderPassComponent.h"
#include "../component/GameSystemComponent.h"
#include "../component/RenderingSystemComponent.h"
#include "../component/GLRenderingSystemComponent.h"

#include "ICoreSystem.h"

extern ICoreSystem* g_pCoreSystem;

using namespace GLRenderingSystemNS;

INNO_PRIVATE_SCOPE GLGeometryRenderingPassUtilities
{
	void initializeOpaquePass();
	void initializeOpaquePassShaders();
	void bindOpaquePassUniformLocations(GLShaderProgramComponent* rhs);

	void initializeSSAOPass();
	void initializeSSAOPassShaders();
	void bindSSAOPassUniformLocations(GLShaderProgramComponent* rhs);

	void generateRandomNoise();

	void initializeSSAOBlurPass();
	void initializeSSAOBlurPassShaders();
	void bindSSAOBlurPassUniformLocations(GLShaderProgramComponent* rhs);

	void initializeTransparentPass();
	void initializeTransparentPassShaders();
	void bindTransparentPassUniformLocations(GLShaderProgramComponent* rhs);

	void initializeTerrainPass();
	void initializeTerrainPassShaders();
	void bindTerrainPassUniformLocations(GLShaderProgramComponent* rhs);

	void updateGeometryPass();
	void updateOpaquePass();
	void updateSSAOPass();
	void updateSSAOBlurPass();
	void updateTransparentPass();

	void updateTerrainPass();

	EntityID m_entityID;
}

void GLGeometryRenderingPassUtilities::initialize()
{
	m_entityID = InnoMath::createEntityID();

	initializeOpaquePass();
	initializeSSAOPass();
	initializeSSAOBlurPass();
	initializeTransparentPass();
	initializeTerrainPass();
}

void GLGeometryRenderingPassUtilities::initializeOpaquePass()
{
	GLGeometryRenderPassComponent::get().m_opaquePass_GLRPC = addGLRenderPassComponent(4, GLRenderingSystemComponent::get().deferredPassFBDesc, GLRenderingSystemComponent::get().deferredPassTextureDesc);

	// UBO
	auto l_UBO = generateUBO(sizeof(GPassCameraUBOData));
	GLGeometryRenderPassComponent::get().m_cameraUBO = l_UBO;

	l_UBO = generateUBO(sizeof(GPassMeshUBOData));
	GLGeometryRenderPassComponent::get().m_meshUBO = l_UBO;

	l_UBO = generateUBO(sizeof(GPassTextureUBOData));
	GLGeometryRenderPassComponent::get().m_textureUBO = l_UBO;

	initializeOpaquePassShaders();
}

void GLGeometryRenderingPassUtilities::initializeOpaquePassShaders()
{
	// shader programs and shaders
	auto rhs = addGLShaderProgramComponent(m_entityID);

	initializeGLShaderProgramComponent(rhs, GLGeometryRenderPassComponent::get().m_opaquePass_shaderFilePaths);

	bindOpaquePassUniformLocations(rhs);

	GLGeometryRenderPassComponent::get().m_opaquePass_GLSPC = rhs;
}

void GLGeometryRenderingPassUtilities::bindOpaquePassUniformLocations(GLShaderProgramComponent* rhs)
{
	bindUniformBlock(GLGeometryRenderPassComponent::get().m_cameraUBO, sizeof(GPassCameraUBOData), rhs->m_program, "cameraUBO", 0);

	bindUniformBlock(GLGeometryRenderPassComponent::get().m_meshUBO, sizeof(GPassMeshUBOData), rhs->m_program, "meshUBO", 1);

	bindUniformBlock(GLGeometryRenderPassComponent::get().m_textureUBO, sizeof(GPassTextureUBOData), rhs->m_program, "textureUBO", 2);

#ifdef CookTorrance
	updateTextureUniformLocations(rhs->m_program, GLGeometryRenderPassComponent::get().m_opaquePassTextureUniformNames);
#elif BlinnPhong
	// @TODO: texture uniforms
#endif
}

void GLGeometryRenderingPassUtilities::initializeSSAOPass()
{
	GLGeometryRenderPassComponent::get().m_SSAOPass_GLRPC = addGLRenderPassComponent(1, GLRenderingSystemComponent::get().deferredPassFBDesc, GLRenderingSystemComponent::get().deferredPassTextureDesc);

	initializeSSAOPassShaders();

	generateRandomNoise();
}

void GLGeometryRenderingPassUtilities::initializeSSAOPassShaders()
{
	// shader programs and shaders
	auto rhs = addGLShaderProgramComponent(m_entityID);

	initializeGLShaderProgramComponent(rhs, GLGeometryRenderPassComponent::get().m_SSAOPass_shaderFilePaths);

	bindSSAOPassUniformLocations(rhs);

	GLGeometryRenderPassComponent::get().m_SSAOPass_GLSPC = rhs;
}

void GLGeometryRenderingPassUtilities::bindSSAOPassUniformLocations(GLShaderProgramComponent * rhs)
{
	updateTextureUniformLocations(rhs->m_program, GLGeometryRenderPassComponent::get().m_SSAOPassTextureUniformNames);

	GLGeometryRenderPassComponent::get().m_SSAOPass_uni_p = getUniformLocation(
		rhs->m_program,
		"uni_p");
	GLGeometryRenderPassComponent::get().m_SSAOPass_uni_r = getUniformLocation(
		rhs->m_program,
		"uni_r");
	GLGeometryRenderPassComponent::get().m_SSAOPass_uni_t = getUniformLocation(
		rhs->m_program,
		"uni_t");

	for (size_t i = 0; i < 64; i++)
	{
		GLGeometryRenderPassComponent::get().m_SSAOPass_uni_samples.emplace_back(
			getUniformLocation(rhs->m_program, "uni_samples[" + std::to_string(i) + "]")
		);
	}
}

void GLGeometryRenderingPassUtilities::generateRandomNoise()
{
	std::uniform_real_distribution<GLfloat> randomFloats(0.0f, 1.0f); // generates random floats between 0.0 and 1.0
	std::default_random_engine generator;
	for (unsigned int i = 0; i < 64; ++i)
	{
		auto sample = vec4(randomFloats(generator) * 2.0f - 1.0f, randomFloats(generator) * 2.0f - 1.0f, randomFloats(generator), 0.0f);
		sample = sample.normalize();
		sample = sample * randomFloats(generator);
		float scale = float(i) / 64.0f;

		// scale samples s.t. they're more aligned to center of kernel
		auto alpha = scale * scale;
		scale = 0.1f + 0.9f * alpha;
		sample = sample * scale;

		GLGeometryRenderPassComponent::get().ssaoKernel.push_back(sample);
	}

	for (unsigned int i = 0; i < 16; i++)
	{
		auto noise = vec4(randomFloats(generator) * 2.0f - 1.0f, randomFloats(generator) * 2.0f - 1.0f, 0.0f, 0.0f); // rotate around z-axis (in tangent space)
		noise = noise.normalize();
		GLGeometryRenderPassComponent::get().ssaoNoise.push_back(noise);
	}

	GLGeometryRenderPassComponent::get().m_noiseTDC = g_pCoreSystem->getMemorySystem()->spawn<TextureDataComponent>();

	GLGeometryRenderPassComponent::get().m_noiseTDC->m_textureDataDesc.textureUsageType = TextureUsageType::RENDER_TARGET;
	GLGeometryRenderPassComponent::get().m_noiseTDC->m_textureDataDesc.textureColorComponentsFormat = TextureColorComponentsFormat::RGB32F;
	GLGeometryRenderPassComponent::get().m_noiseTDC->m_textureDataDesc.texturePixelDataFormat = TexturePixelDataFormat::RGB;
	GLGeometryRenderPassComponent::get().m_noiseTDC->m_textureDataDesc.textureMinFilterMethod = TextureFilterMethod::NEAREST;
	GLGeometryRenderPassComponent::get().m_noiseTDC->m_textureDataDesc.textureMagFilterMethod = TextureFilterMethod::NEAREST;
	GLGeometryRenderPassComponent::get().m_noiseTDC->m_textureDataDesc.textureWrapMethod = TextureWrapMethod::REPEAT;
	GLGeometryRenderPassComponent::get().m_noiseTDC->m_textureDataDesc.textureWidth = 4;
	GLGeometryRenderPassComponent::get().m_noiseTDC->m_textureDataDesc.textureHeight = 4;
	GLGeometryRenderPassComponent::get().m_noiseTDC->m_textureDataDesc.texturePixelDataType = TexturePixelDataType::FLOAT;

	std::vector<float> l_pixelBuffer;
	auto l_containerSize = GLGeometryRenderPassComponent::get().ssaoNoise.size() * 4;
	l_pixelBuffer.reserve(l_containerSize);

	std::for_each(GLGeometryRenderPassComponent::get().ssaoNoise.begin(), GLGeometryRenderPassComponent::get().ssaoNoise.end(), [&](vec4 val)
	{
		l_pixelBuffer.emplace_back(val.x);
		l_pixelBuffer.emplace_back(val.y);
		l_pixelBuffer.emplace_back(val.z);
		l_pixelBuffer.emplace_back(val.w);
	});

	GLGeometryRenderPassComponent::get().m_noiseTDC->m_textureData.emplace_back(&l_pixelBuffer[0]);

	GLGeometryRenderPassComponent::get().m_noiseGLTDC = generateGLTextureDataComponent(GLGeometryRenderPassComponent::get().m_noiseTDC);
}

void GLGeometryRenderingPassUtilities::initializeSSAOBlurPass()
{
	GLGeometryRenderPassComponent::get().m_SSAOBlurPass_GLRPC = addGLRenderPassComponent(1, GLRenderingSystemComponent::get().deferredPassFBDesc, GLRenderingSystemComponent::get().deferredPassTextureDesc);

	initializeSSAOBlurPassShaders();
}

void GLGeometryRenderingPassUtilities::initializeSSAOBlurPassShaders()
{
	// shader programs and shaders
	auto rhs = addGLShaderProgramComponent(m_entityID);

	initializeGLShaderProgramComponent(rhs, GLGeometryRenderPassComponent::get().m_SSAOBlurPass_shaderFilePaths);

	bindSSAOBlurPassUniformLocations(rhs);

	GLGeometryRenderPassComponent::get().m_SSAOBlurPass_GLSPC = rhs;
}

void GLGeometryRenderingPassUtilities::bindSSAOBlurPassUniformLocations(GLShaderProgramComponent * rhs)
{
	updateTextureUniformLocations(rhs->m_program, GLGeometryRenderPassComponent::get().m_SSAOBlurPassTextureUniformNames);
}


void GLGeometryRenderingPassUtilities::initializeTransparentPass()
{
	GLGeometryRenderPassComponent::get().m_transparentPass_GLRPC = addGLRenderPassComponent(2, GLRenderingSystemComponent::get().deferredPassFBDesc, GLRenderingSystemComponent::get().deferredPassTextureDesc);

	initializeTransparentPassShaders();
}

void GLGeometryRenderingPassUtilities::initializeTransparentPassShaders()
{
	// shader programs and shaders
	auto rhs = addGLShaderProgramComponent(m_entityID);

	initializeGLShaderProgramComponent(rhs, GLGeometryRenderPassComponent::get().m_transparentPass_shaderFilePaths);

	bindTransparentPassUniformLocations(rhs);

	GLGeometryRenderPassComponent::get().m_transparentPass_GLSPC = rhs;
}

void GLGeometryRenderingPassUtilities::bindTransparentPassUniformLocations(GLShaderProgramComponent* rhs)
{
	bindUniformBlock(GLGeometryRenderPassComponent::get().m_cameraUBO, sizeof(GPassCameraUBOData), rhs->m_program, "cameraUBO", 0);

	bindUniformBlock(GLGeometryRenderPassComponent::get().m_meshUBO, sizeof(GPassMeshUBOData), rhs->m_program, "meshUBO", 1);

	GLGeometryRenderPassComponent::get().m_transparentPass_uni_albedo = getUniformLocation(
		rhs->m_program,
		"uni_albedo");
	GLGeometryRenderPassComponent::get().m_transparentPass_uni_TR = getUniformLocation(
		rhs->m_program,
		"uni_TR");	
	GLGeometryRenderPassComponent::get().m_transparentPass_uni_viewPos = getUniformLocation(
		rhs->m_program,
		"uni_viewPos");
	GLGeometryRenderPassComponent::get().m_transparentPass_uni_dirLight_direction = getUniformLocation(
		rhs->m_program,
		"uni_dirLight.direction");
	GLGeometryRenderPassComponent::get().m_transparentPass_uni_dirLight_color = getUniformLocation(
		rhs->m_program,
		"uni_dirLight.color");
}

void GLGeometryRenderingPassUtilities::initializeTerrainPass()
{
	GLTerrainRenderPassComponent::get().m_GLRPC = addGLRenderPassComponent(1, GLRenderingSystemComponent::get().deferredPassFBDesc, GLRenderingSystemComponent::get().deferredPassTextureDesc);

	initializeTerrainPassShaders();
}

void GLGeometryRenderingPassUtilities::initializeTerrainPassShaders()
{
	// shader programs and shaders
	auto rhs = addGLShaderProgramComponent(m_entityID);

	initializeGLShaderProgramComponent(rhs, GLTerrainRenderPassComponent::get().m_shaderFilePaths);

	bindTerrainPassUniformLocations(rhs);

	GLTerrainRenderPassComponent::get().m_GLSPC = rhs;
}

void GLGeometryRenderingPassUtilities::bindTerrainPassUniformLocations(GLShaderProgramComponent* rhs)
{
	GLTerrainRenderPassComponent::get().m_terrainPass_uni_p_camera = getUniformLocation(
		rhs->m_program,
		"uni_p_camera");
	GLTerrainRenderPassComponent::get().m_terrainPass_uni_r_camera = getUniformLocation(
		rhs->m_program,
		"uni_r_camera");
	GLTerrainRenderPassComponent::get().m_terrainPass_uni_t_camera = getUniformLocation(
		rhs->m_program,
		"uni_t_camera");
	GLTerrainRenderPassComponent::get().m_terrainPass_uni_m = getUniformLocation(
		rhs->m_program,
		"uni_m");
	GLTerrainRenderPassComponent::get().m_terrainPass_uni_albedoTexture = getUniformLocation(
		rhs->m_program,
		"uni_albedoTexture");
	updateUniform(
		GLTerrainRenderPassComponent::get().m_terrainPass_uni_albedoTexture,
		0);
}

void GLGeometryRenderingPassUtilities::update()
{
	updateOpaquePass();
	updateSSAOPass();
	updateSSAOBlurPass();
	updateTransparentPass();
	updateTerrainPass();
}

void GLGeometryRenderingPassUtilities::updateOpaquePass()
{
	glEnable(GL_DEPTH_TEST);
	glDepthFunc(GL_LESS);
	glEnable(GL_DEPTH_CLAMP);

	glEnable(GL_STENCIL_TEST);
	glStencilOp(GL_KEEP, GL_KEEP, GL_REPLACE);
	glStencilMask(0xFF);

	//glEnable(GL_CULL_FACE);
	//glFrontFace(GL_CCW);
	//glCullFace(GL_BACK);

	// bind to framebuffer
	auto l_FBC = GLGeometryRenderPassComponent::get().m_opaquePass_GLRPC->m_GLFBC;
	bindFBC(l_FBC);

	activateShaderProgram(GLGeometryRenderPassComponent::get().m_opaquePass_GLSPC);

	updateUBO(GLGeometryRenderPassComponent::get().m_cameraUBO, GLRenderingSystemComponent::get().m_GPassCameraUBOData);

#ifdef CookTorrance
	while (GLRenderingSystemComponent::get().m_opaquePassDataQueue.size() > 0)
	{
		auto l_renderPack = GLRenderingSystemComponent::get().m_opaquePassDataQueue.front();
		if (l_renderPack.visiblilityType == VisiblilityType::INNO_OPAQUE)
		{
			glStencilFunc(GL_ALWAYS, 0x01, 0xFF);

			// any normal?
			if (l_renderPack.textureUBOData.useNormalTexture)
			{
				activateTexture(l_renderPack.normalGLTDC, 0);
			}
			// any albedo?
			if (l_renderPack.textureUBOData.useAlbedoTexture)
			{
				activateTexture(l_renderPack.albedoGLTDC, 1);
			}
			// any metallic?
			if (l_renderPack.textureUBOData.useMetallicTexture)
			{
				activateTexture(l_renderPack.metallicGLTDC, 2);
			}
			// any roughness?
			if (l_renderPack.textureUBOData.useRoughnessTexture)
			{
				activateTexture(l_renderPack.roughnessGLTDC, 3);
			}
			// any ao?
			if (l_renderPack.textureUBOData.useAOTexture)
			{
				activateTexture(l_renderPack.AOGLTDC, 4);
			}

			updateUBO(GLGeometryRenderPassComponent::get().m_meshUBO, l_renderPack.meshUBOData);
			updateUBO(GLGeometryRenderPassComponent::get().m_textureUBO, l_renderPack.textureUBOData);
			drawMesh(l_renderPack.indiceSize, l_renderPack.meshPrimitiveTopology, l_renderPack.GLMDC);
		}
		else if (l_renderPack.visiblilityType == VisiblilityType::INNO_EMISSIVE)
		{
			glStencilFunc(GL_ALWAYS, 0x02, 0xFF);

			updateUBO(GLGeometryRenderPassComponent::get().m_meshUBO, l_renderPack.meshUBOData);
			updateUBO(GLGeometryRenderPassComponent::get().m_textureUBO, l_renderPack.textureUBOData);

			drawMesh(l_renderPack.indiceSize, l_renderPack.meshPrimitiveTopology, l_renderPack.GLMDC);
		}
		else
		{
			glStencilFunc(GL_ALWAYS, 0x00, 0xFF);
		}
		GLRenderingSystemComponent::get().m_opaquePassDataQueue.pop();
	}

	//glDisable(GL_CULL_FACE);
	glDisable(GL_STENCIL_TEST);
	glDisable(GL_DEPTH_CLAMP);
	glDisable(GL_DEPTH_TEST);

#elif BlinnPhong
#endif
}

void GLGeometryRenderingPassUtilities::updateSSAOPass()
{
	// bind to framebuffer
	auto l_FBC = GLGeometryRenderPassComponent::get().m_SSAOPass_GLRPC->m_GLFBC;
	bindFBC(l_FBC);

	activateShaderProgram(GLGeometryRenderPassComponent::get().m_SSAOPass_GLSPC);

	activateTexture(GLGeometryRenderPassComponent::get().m_opaquePass_GLRPC->m_GLTDCs[0], 0);
	activateTexture(GLGeometryRenderPassComponent::get().m_opaquePass_GLRPC->m_GLTDCs[1], 1);
	activateTexture(GLGeometryRenderPassComponent::get().m_noiseGLTDC, 2);

	updateUniform(
		GLGeometryRenderPassComponent::get().m_SSAOPass_uni_p,
		GLRenderingSystemComponent::get().m_CamProjJittered);
	updateUniform(
		GLGeometryRenderPassComponent::get().m_SSAOPass_uni_r,
		GLRenderingSystemComponent::get().m_CamRot);
	updateUniform(
		GLGeometryRenderPassComponent::get().m_SSAOPass_uni_t,
		GLRenderingSystemComponent::get().m_CamTrans);

	for (size_t i = 0; i < GLGeometryRenderPassComponent::get().m_SSAOPass_uni_samples.size(); i++)
	{
		auto l_kernel = GLGeometryRenderPassComponent::get().ssaoKernel[i];
		updateUniform(
			GLGeometryRenderPassComponent::get().m_SSAOPass_uni_samples[i],
			l_kernel.x, l_kernel.y, l_kernel.z, l_kernel.w);
	}

	auto l_MDC = g_pCoreSystem->getAssetSystem()->getMeshDataComponent(MeshShapeType::QUAD);
	drawMesh(l_MDC);
}
void GLGeometryRenderingPassUtilities::updateSSAOBlurPass()
{
	// bind to framebuffer
	auto l_FBC = GLGeometryRenderPassComponent::get().m_SSAOBlurPass_GLRPC->m_GLFBC;
	bindFBC(l_FBC);

	activateShaderProgram(GLGeometryRenderPassComponent::get().m_SSAOBlurPass_GLSPC);

	activateTexture(GLGeometryRenderPassComponent::get().m_SSAOPass_GLRPC->m_GLTDCs[0], 0);

	auto l_MDC = g_pCoreSystem->getAssetSystem()->getMeshDataComponent(MeshShapeType::QUAD);
	drawMesh(l_MDC);
}

void GLGeometryRenderingPassUtilities::updateTransparentPass()
{
	glEnable(GL_DEPTH_TEST);
	glDepthFunc(GL_LESS);
	glEnable(GL_DEPTH_CLAMP);

	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

	// bind to framebuffer
	auto l_FBC = GLGeometryRenderPassComponent::get().m_transparentPass_GLRPC->m_GLFBC;
	bindFBC(l_FBC);

	copyDepthBuffer(GLGeometryRenderPassComponent::get().m_opaquePass_GLRPC->m_GLFBC, l_FBC);

	activateShaderProgram(GLGeometryRenderPassComponent::get().m_transparentPass_GLSPC);

	updateUBO(GLGeometryRenderPassComponent::get().m_cameraUBO, GLRenderingSystemComponent::get().m_GPassCameraUBOData);

	updateUniform(
		GLGeometryRenderPassComponent::get().m_transparentPass_uni_viewPos,
		GLRenderingSystemComponent::get().m_CamGlobalPos.x, GLRenderingSystemComponent::get().m_CamGlobalPos.y, GLRenderingSystemComponent::get().m_CamGlobalPos.z);
	updateUniform(
		GLGeometryRenderPassComponent::get().m_transparentPass_uni_dirLight_direction,
		GLRenderingSystemComponent::get().m_sunDir.x, GLRenderingSystemComponent::get().m_sunDir.y, GLRenderingSystemComponent::get().m_sunDir.z);
	updateUniform(
		GLGeometryRenderPassComponent::get().m_transparentPass_uni_dirLight_color,
		GLRenderingSystemComponent::get().m_sunColor.x, GLRenderingSystemComponent::get().m_sunColor.y, GLRenderingSystemComponent::get().m_sunColor.z);

	while (GLRenderingSystemComponent::get().m_transparentPassDataQueue.size() > 0)
	{
		auto l_renderPack = GLRenderingSystemComponent::get().m_transparentPassDataQueue.front();

		updateUBO(GLGeometryRenderPassComponent::get().m_meshUBO, l_renderPack.meshUBOData);

		updateUniform(GLGeometryRenderPassComponent::get().m_transparentPass_uni_albedo, l_renderPack.meshCustomMaterial.albedo_r, l_renderPack.meshCustomMaterial.albedo_g, l_renderPack.meshCustomMaterial.albedo_b, l_renderPack.meshCustomMaterial.alpha);
		updateUniform(GLGeometryRenderPassComponent::get().m_transparentPass_uni_TR, l_renderPack.meshCustomMaterial.thickness, l_renderPack.meshCustomMaterial.roughness, 0.0f, 0.0f);

		drawMesh(l_renderPack.indiceSize, l_renderPack.meshPrimitiveTopology, l_renderPack.GLMDC);

		GLRenderingSystemComponent::get().m_transparentPassDataQueue.pop();
	}

	glDisable(GL_BLEND);
	glDisable(GL_DEPTH_CLAMP);
	glDisable(GL_DEPTH_TEST);
}

void GLGeometryRenderingPassUtilities::updateTerrainPass()
{
	if (RenderingSystemComponent::get().m_drawTerrain)
	{
		glEnable(GL_DEPTH_TEST);

		// bind to framebuffer
		auto l_FBC = GLTerrainRenderPassComponent::get().m_GLRPC->m_GLFBC;
		bindFBC(l_FBC);

		copyDepthBuffer(GLGeometryRenderPassComponent::get().m_opaquePass_GLRPC->m_GLFBC, l_FBC);

		activateShaderProgram(GLTerrainRenderPassComponent::get().m_GLSPC);

		mat4 m = InnoMath::generateIdentityMatrix<float>();

		updateUniform(
			GLTerrainRenderPassComponent::get().m_terrainPass_uni_p_camera,
			GLRenderingSystemComponent::get().m_CamProjOriginal);
		updateUniform(
			GLTerrainRenderPassComponent::get().m_terrainPass_uni_r_camera,
			GLRenderingSystemComponent::get().m_CamRot);
		updateUniform(
			GLTerrainRenderPassComponent::get().m_terrainPass_uni_t_camera,
			GLRenderingSystemComponent::get().m_CamTrans);
		updateUniform(
			GLTerrainRenderPassComponent::get().m_terrainPass_uni_m,
			m);

		auto l_MDC = g_pCoreSystem->getAssetSystem()->getMeshDataComponent(MeshShapeType::TERRAIN);
		activateTexture(GLRenderingSystemComponent::get().m_basicAlbedoGLTDC, 0);

		drawMesh(l_MDC);

		glDisable(GL_DEPTH_TEST);
	}
	else
	{
		cleanFBC(GLTerrainRenderPassComponent::get().m_GLRPC->m_GLFBC);
	}
}

bool GLGeometryRenderingPassUtilities::resize()
{
	resizeGLRenderPassComponent(GLGeometryRenderPassComponent::get().m_opaquePass_GLRPC, GLRenderingSystemComponent::get().deferredPassFBDesc);
	resizeGLRenderPassComponent(GLGeometryRenderPassComponent::get().m_SSAOPass_GLRPC, GLRenderingSystemComponent::get().deferredPassFBDesc);
	resizeGLRenderPassComponent(GLGeometryRenderPassComponent::get().m_SSAOBlurPass_GLRPC, GLRenderingSystemComponent::get().deferredPassFBDesc);
	resizeGLRenderPassComponent(GLGeometryRenderPassComponent::get().m_transparentPass_GLRPC, GLRenderingSystemComponent::get().deferredPassFBDesc);
	resizeGLRenderPassComponent(GLTerrainRenderPassComponent::get().m_GLRPC, GLRenderingSystemComponent::get().deferredPassFBDesc);

	return true;
}

bool GLGeometryRenderingPassUtilities::reloadOpaquePassShaders()
{
	deleteShaderProgram(GLGeometryRenderPassComponent::get().m_opaquePass_GLSPC);

	initializeGLShaderProgramComponent(GLGeometryRenderPassComponent::get().m_opaquePass_GLSPC, GLGeometryRenderPassComponent::get().m_opaquePass_shaderFilePaths);

	bindOpaquePassUniformLocations(GLGeometryRenderPassComponent::get().m_opaquePass_GLSPC);

	return true;
}

bool GLGeometryRenderingPassUtilities::reloadTransparentPassShaders()
{
	deleteShaderProgram(GLGeometryRenderPassComponent::get().m_transparentPass_GLSPC);

	initializeGLShaderProgramComponent(GLGeometryRenderPassComponent::get().m_transparentPass_GLSPC, GLGeometryRenderPassComponent::get().m_transparentPass_shaderFilePaths);

	bindTransparentPassUniformLocations(GLGeometryRenderPassComponent::get().m_transparentPass_GLSPC);

	return true;
}

bool GLGeometryRenderingPassUtilities::reloadTerrainPassShaders()
{
	deleteShaderProgram(GLTerrainRenderPassComponent::get().m_GLSPC);

	initializeGLShaderProgramComponent(GLTerrainRenderPassComponent::get().m_GLSPC, GLTerrainRenderPassComponent::get().m_shaderFilePaths);

	bindTransparentPassUniformLocations(GLTerrainRenderPassComponent::get().m_GLSPC);

	return true;
}